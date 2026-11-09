// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_PYEXT_CHILD_WRAPPER_MAP_H__
#define GOOGLE_PROTOBUF_PYEXT_CHILD_WRAPPER_MAP_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"
#include "google/protobuf/pyext/lazy_unique_ptr.h"
#include "google/protobuf/pyext/weak_value_map.h"

namespace google {
namespace protobuf {
namespace python {

// A weak-value cache from a key to the live Python wrappers of a message's
// children (see CMessage::composite_fields and CMessage::child_submessages).
//
// The underlying PyWeakValueMap is internally synchronized, so every operation
// below is memory-safe on its own. The locking rules below exist instead to
// uphold the map's *invariants* relative to the rest of the message tree,
// which is protected by the tree guard. Each operation states the access it
// requires; methods that require a token take it as a parameter, and (in
// sanitizer builds) CHECK that it is for the tree guard this map is bound to.
//
// Invariants, for every entry (key -> w) in the maps of message P:
//
//   (1) Consistency: w->parent == P, w->GetTree() == P->GetTree(), and key
//       identifies w: w->parent_field_descriptor for composite_fields, or
//       w's C++ message for child_submessages.
//   (2) Completeness: every live wrapper whose parent is P is registered in
//       P's map. Only a write-lock holder may rely on this (a reader may be
//       creating a wrapper that is not registered yet).
//   (3) Whoever changes a wrapper's parent, tree, or map membership holds the
//       write lock AND a strong reference to the wrapper. So a wrapper that is
//       being deallocated (refcount 0) cannot be moved concurrently, and its
//       tp_dealloc may read its own parent quiescently.
//   (4) Wrapper creation has no side effects other than the registration
//       itself: if two readers race to create a wrapper for the same key, the
//       loser of TrySet() discards its wrapper and uses the winner's.
//
// Access required by each kind of operation:
//
//   | Operation                                           | Access         |
//   |-----------------------------------------------------|----------------|
//   | Iterate relying on completeness; move/erase entries | write          |
//   | Insert a wrapper created in the same lock section   | read           |
//   | Look up an entry, returning a strong reference      | none           |
//   | A dying wrapper erases its own entry                | none (dealloc) |
//
// Why lookups need no lock: Get() returns a strong reference to a live entry
// (dying entries are skipped), which is safe to use on its own. Holding no lock
// means the entry may be detached before the caller uses it (e.g. a concurrent
// Clear() may reparent the wrapper), which is the same as fetching the wrapper
// just before that Clear() and using it after. Callers then lock the wrapper's
// own tree.
//
// Why insertion needs (only) a read lock: a wrapper is created from P's tree
// state and starts out on P's tree, so it must be registered before that read
// section ends. A detach of P (which needs the write lock) then either happens
// before the wrapper exists, or finds it in the map by (2) and moves it along
// with the rest of P's subtree. Concurrent inserts by readers are made safe by
// (4).
//
// Why a dying wrapper needs no lock: by (3), nobody can be moving it, so its
// parent is still P. EraseIfEqual() only removes the entry if it still refers
// to this wrapper (another thread may already have replaced or dropped it),
// and never dereferences other entries.
class ChildWrapperMap {
 public:
  ChildWrapperMap() = default;
  ChildWrapperMap(const ChildWrapperMap&) = delete;
  ChildWrapperMap& operator=(const ChildWrapperMap&) = delete;

  // Returns a new reference to the live wrapper for `key`, or nullptr. Needs
  // no lock (see above); the entry may be detached before it is used.
  PyObject* Get(const void* key) const {
    PyWeakValueMap* map = map_.TryGet();
    return map == nullptr ? nullptr : map->Get(key, nullptr);
  }

  // Removes the entry for `key` if it still refers to `self`. Only for the
  // tp_dealloc of `self` (see above).
  void EraseIfEqualSelf(const void* key, PyObject* self) {
    if (PyWeakValueMap* map = map_.TryGet()) {
      map->EraseIfEqual(key, self);
    }
  }

  // Registers `value`, a new wrapper created in the same read (or write) lock
  // section as this call. If a live wrapper is already registered for `key`,
  // `value` is released and replaced with a new reference to the existing
  // wrapper.
  void TrySet(const void* key, PyObject*& value, const ReadAccess& access) {
    tracker_.CheckMatches(access.guard());
    map_.Get()->TrySet(key, value);
  }

  // Registers `value` for `key`, which must not have a live entry.
  void Set(const void* key, PyObject* value, const WriteAccess& access) {
    tracker_.CheckMatches(access.guard());
    map_.Get()->Set(key, value);
  }

  // Removes the entry for `key`, which must refer to `value`.
  void Erase(const void* key, PyObject* value, const WriteAccess& access) {
    tracker_.CheckMatches(access.guard());
    PyWeakValueMap* map = map_.TryGet();
    ABSL_CHECK(map != nullptr);
    map->Erase(key, value);
  }

  // Calls f(key, wrapper) for every live entry. By (2), these are all of the
  // live wrappers whose parent is the owning message. See
  // PyWeakValueMap::ForEach() for the lifetime of `wrapper`.
  template <typename F>
  void ForEach(F&& f, const WriteAccess& access) {
    tracker_.CheckMatches(access.guard());
    if (PyWeakValueMap* map = map_.TryGet()) {
      map->ForEach(std::forward<F>(f));
    }
  }

  // Returns true if there are no entries. Under a read lock, the result can be
  // invalidated by a concurrent reader's insertion right away.
  bool IsEmpty(const ReadAccess& access) const {
    tracker_.CheckMatches(access.guard());
    PyWeakValueMap* map = map_.TryGet();
    return map == nullptr || map->IsEmpty();
  }

  // Returns true if any wrapper was ever registered in this map.
  bool WasEverUsed(const ReadAccess& access) const {
    tracker_.CheckMatches(access.guard());
    return map_.TryGet() != nullptr;
  }

  // IsEmpty() for an owning message that is quiescent (e.g. in tp_dealloc).
  bool IsEmptyQuiescent() const {
    PyWeakValueMap* map = map_.TryGet();
    return map == nullptr || map->IsEmpty();
  }

  // Rebinds the tree guard that tokens are checked against. Called alongside
  // Guarded<T>::BindQuiescent() / UpdateGuard() for the owning message's other
  // fields (see TreeLockable::InitTreeQuiescent() and MoveToTree()).
  void BindQuiescent(const AccessGuard& guard) { tracker_.Set(&guard); }
  void UpdateGuard(const WriteAccess& old_access,
                   const WriteAccess& new_access) {
    ABSL_DCHECK_NE(old_access.guard(), new_access.guard());
    tracker_.CheckMatches(old_access.guard());
    tracker_.Set(new_access.guard());
  }

 private:
  mutable LazyUniquePtr<PyWeakValueMap> map_;
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS
  free_threading_internal::DebugGuardTracker tracker_;
};

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_CHILD_WRAPPER_MAP_H__
