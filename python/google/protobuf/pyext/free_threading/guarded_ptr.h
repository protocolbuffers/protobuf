// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// -----------------------------------------------------------------------------
// File: guarded_ptr.h
// -----------------------------------------------------------------------------
//
// This header file defines `GuardedPtr<T>`, which wraps a raw C++ pointer T*
// (such as a `google::protobuf::Message*` in CMessage) that is protected by an
// `AccessGuard`.
//
// The relationship between `AccessGuard` and `GuardedPtr<T>` can be 1:1 or
// 1:many. A single `AccessGuard` (for example, on a `MessageTree`) can protect
// arbitrarily many `GuardedPtr<T>` instances across a message hierarchy (or
// multiple `GuardedPtr` fields within a single struct).
//
// Unlike a raw pointer, `GuardedPtr<T>` cannot be dereferenced directly;
// callers must either:
//
//   1. Acquire a scoped `LockedPtr<const T>` or `LockedPtr<T>` via
//      `LockRead()` / `LockWrite()` (or dual-lock helpers `LockWriteRead()` /
//      `LockReadRead()` in multi_lock.h), or
//   2. Prove they already hold a `LockedPtr<U>` for the same `AccessGuard`
//      (acquired from any `GuardedPtr<U>` protected by that `AccessGuard`) and
//      unpack the raw pointer at zero atomic cost via `Get()` or
//      `GetMutable()`.
//
// In release builds (NDEBUG), sizeof(GuardedPtr<T>) == sizeof(T*). In debug
// builds (!NDEBUG), `GuardedPtr<T>` stores an additional pointer to the
// `AccessGuard` that protects it and asserts via `ABSL_DCHECK_EQ` on every
// `LockRead()`, `LockWrite()`, `Get()`, `GetMutable()`, and `Set()` call that
// the `AccessGuard` or `LockedPtr` passed by the caller matches the
// `AccessGuard` associated with this `GuardedPtr`.

#ifndef GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_GUARDED_PTR_H__
#define GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_GUARDED_PTR_H__

#include <atomic>
#include <optional>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"
#include "google/protobuf/pyext/free_threading/locked_ptr.h"

namespace google {
namespace protobuf {
namespace python {

namespace free_threading_internal {

// Tracks which `AccessGuard` protects a `GuardedPtr` in debug builds (!NDEBUG),
// compiling away to zero bytes under NDEBUG via
// ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS.
#ifdef NDEBUG
class DebugGuardTracker {
 public:
  DebugGuardTracker() = default;
  explicit DebugGuardTracker(const AccessGuard* /*guard*/) {}
  DebugGuardTracker(const DebugGuardTracker&) = default;
  DebugGuardTracker& operator=(const DebugGuardTracker&) = default;

  void Set(const AccessGuard* /*guard*/) {}
  void CheckMatches(const AccessGuard* /*expected*/) const {}
};
#else
class DebugGuardTracker {
 public:
  DebugGuardTracker() = default;
  explicit DebugGuardTracker(const AccessGuard* guard) : guard_(guard) {}

  DebugGuardTracker(const DebugGuardTracker& other)
      : guard_(other.guard_.load(std::memory_order_relaxed)) {}

  DebugGuardTracker& operator=(const DebugGuardTracker& other) {
    if (this != &other) {
      guard_.store(other.guard_.load(std::memory_order_relaxed),
                   std::memory_order_release);
    }
    return *this;
  }

  void Set(const AccessGuard* guard) {
    guard_.store(guard, std::memory_order_release);
  }

  void CheckMatches(const AccessGuard* expected) const {
    ABSL_DCHECK_EQ(guard_.load(std::memory_order_acquire), expected);
  }

 private:
  std::atomic<const AccessGuard*> guard_{nullptr};
};
#endif

}  // namespace free_threading_internal

// -----------------------------------------------------------------------------
// GuardedPtr<T>
// -----------------------------------------------------------------------------
//
// Wraps a pointer T* protected by an `AccessGuard`. Many `GuardedPtr`
// instances (potentially of different pointee types T and U) may share a
// single `AccessGuard` (1:many).
//
// Example 1 (defining, initializing, and deallocating a `GuardedPtr<T>` member
// on a Python wrapper struct):
//
//   struct CMessage {
//     PyObject_HEAD
//     MessageTree* tree;
//     GuardedPtr<Message> message;
//   };
//
//   // During single-threaded construction, associate message with the
//   // AccessGuard that protects it:
//   self->tree = new MessageTree();
//   self->message = GuardedPtr<Message>(prototype->New(), self->tree->guard);
//
//   // Locking in a C-API method:
//   PYPROTO_ASSIGN_OR_RETURN(
//       LockedPtr<const Message> msg,
//       self->message.LockRead(self->tree->guard), nullptr);
//
//   // During tp_dealloc (when Py_REFCNT(self) == 0 and no other thread can
//   // reach self), access the pointer without locking via GetQuiescent():
//   delete self->message.GetQuiescent();
//
// Example 2 (1:many `AccessGuard` to `GuardedPtr<T>` - sharing one
// `AccessGuard` across multiple `GuardedPtr` instances in a tree and unpacking
// them without extra atomic operations once a single `LockedPtr` is held):
//
//   // Both parent->message and child->message are initialized with the
//   // same AccessGuard (tree->guard):
//   parent->message = GuardedPtr<Message>(raw_parent, tree->guard);
//   child->message = GuardedPtr<Message>(raw_child, tree->guard);
//
//   // Acquire the lock once via parent->message:
//   PYPROTO_ASSIGN_OR_RETURN(
//       LockedPtr<Message> parent_lock,
//       parent->message.LockWrite(tree->guard), nullptr);
//
//   // Because child->message is protected by the same AccessGuard, we can
//   // unpack child->message as const Message* or mutable Message* using
//   // parent_lock as compile-time + debug-time proof that the guard is held:
//   const Message* const_child = child->message.Get(parent_lock);
//   Message* mutable_child = child->message.GetMutable(parent_lock);
//
//   // Or replace the wrapped pointer in child->message under parent_lock
//   // (e.g. when promoting a submessage from a default prototype to a mutable
//   // instance):
//   Message* promoted = reflection->MutableMessage(parent_lock.get(), field);
//   child->message.Set(promoted, parent_lock);
//
// Example 3 (migrating a subtree of `GuardedPtr`s from old_tree to a newly
// created new_tree while holding a write lock on old_tree):
//
//   PYPROTO_ASSIGN_OR_RETURN(
//       LockedPtr<Message> new_tree_lock,
//       child->message.UpdateGuardAndLockWrite(old_lock, new_tree->guard),
//       nullptr);
//   grandchild->message.UpdateGuard(old_lock, new_tree_lock);
template <typename T>
class GuardedPtr {
 public:
  GuardedPtr() = default;

  GuardedPtr(T* ptr, const AccessGuard& guard)
      : ptr_(ptr), debug_guard_(&guard) {
    ABSL_DCHECK(ptr != nullptr);
  }

  GuardedPtr(const GuardedPtr& other) = default;
  GuardedPtr& operator=(const GuardedPtr& other) = default;

  // GuardedPtr::LockRead()
  //
  // Acquires a shared read lock on guard and returns a `LockedPtr<const T>`.
  // On contention, sets a Python RuntimeError and returns `std::nullopt`.
  ABSL_MUST_USE_RESULT std::optional<LockedPtr<const T>> LockRead(
      AccessGuard& guard) const {
    AccessGuard::ReadLock lock(guard);
    if (ABSL_PREDICT_FALSE(!lock)) {
      return std::nullopt;
    }
    return AdoptReadLock(std::move(lock));
  }

  // GuardedPtr::LockWrite()
  //
  // Acquires an exclusive write lock on guard and returns a `LockedPtr<T>`.
  // On contention, sets a Python RuntimeError and returns `std::nullopt`.
  ABSL_MUST_USE_RESULT std::optional<LockedPtr<T>> LockWrite(
      AccessGuard& guard) const {
    AccessGuard::WriteLock lock(guard);
    if (ABSL_PREDICT_FALSE(!lock)) {
      return std::nullopt;
    }
    return AdoptWriteLock(std::move(lock));
  }

  // GuardedPtr::Get()
  //
  // Unpacks const T* using an already-held read or write `LockedPtr<U>` for
  // the same tree without performing any atomic operations. In !NDEBUG builds,
  // `ABSL_DCHECK`s that lock is valid and guards the same tree.
  template <typename U>
  const T* Get(const LockedPtr<U>& lock ABSL_ATTRIBUTE_LIFETIME_BOUND) const {
    ABSL_DCHECK(lock.get() != nullptr);
    debug_guard_.CheckMatches(lock.lock_.guard());
    return ptr_;
  }

  // GuardedPtr::GetMutable()
  //
  // Unpacks T* using an already-held exclusive write `LockedPtr<U>` (where U is
  // non-const) for the same tree without performing any atomic operations.
  // Fails to compile if U is const, and in !NDEBUG builds, `ABSL_DCHECK`s that
  // lock is valid and guards the same tree.
  template <typename U>
  T* GetMutable(const LockedPtr<U>& lock ABSL_ATTRIBUTE_LIFETIME_BOUND) const {
    static_assert(!std::is_const_v<U>,
                  "GetMutable requires an exclusive write lock (LockedPtr<U> "
                  "with non-const U).");
    ABSL_DCHECK(lock.get() != nullptr);
    debug_guard_.CheckMatches(lock.lock_.guard());
    return ptr_;
  }

  // GuardedPtr::Set()
  //
  // Updates the wrapped pointer to ptr while holding an exclusive write
  // `LockedPtr<U>` (where U is non-const) on the same tree.
  template <typename U>
  void Set(T* ptr, const LockedPtr<U>& lock) {
    static_assert(!std::is_const_v<U>,
                  "Set requires an exclusive write lock (LockedPtr<U> with "
                  "non-const U).");
    ABSL_DCHECK(ptr != nullptr);
    ABSL_DCHECK(lock.get() != nullptr);
    debug_guard_.CheckMatches(lock.lock_.guard());
    ptr_ = ptr;
  }

  // GuardedPtr::UpdateGuard()
  //
  // Hands off this `GuardedPtr` from old_lock's `AccessGuard` to new_lock's
  // `AccessGuard` while holding exclusive write locks on both guards. In
  // !NDEBUG builds, verifies that this `GuardedPtr` was previously guarded by
  // old_lock and that old_lock and new_lock refer to distinct guards.
  template <typename U, typename V>
  void UpdateGuard(const LockedPtr<U>& old_lock, const LockedPtr<V>& new_lock) {
    static_assert(!std::is_const_v<U>,
                  "UpdateGuard requires an exclusive write lock on the old "
                  "guard (LockedPtr<U> with non-const U).");
    static_assert(!std::is_const_v<V>,
                  "UpdateGuard requires an exclusive write lock on the new "
                  "guard (LockedPtr<V> with non-const V).");
    ABSL_DCHECK(old_lock.get() != nullptr);
    ABSL_DCHECK(new_lock.get() != nullptr);
    ABSL_DCHECK_NE(old_lock.lock_.guard(), new_lock.lock_.guard());
    debug_guard_.CheckMatches(old_lock.lock_.guard());
    debug_guard_.Set(new_lock.lock_.guard());
  }

  // GuardedPtr::UpdateGuardAndLockWrite()
  //
  // Hands off this `GuardedPtr` from old_lock's `AccessGuard` to a newly
  // created new_guard while holding an exclusive write lock on old_lock,
  // acquiring an exclusive write lock on new_guard and returning a
  // `LockedPtr<T>` for new_guard (which can then be passed as new_lock to
  // `UpdateGuard()` for any remaining nodes in the detached subtree).
  template <typename U>
  ABSL_MUST_USE_RESULT std::optional<LockedPtr<T>> UpdateGuardAndLockWrite(
      const LockedPtr<U>& old_lock, AccessGuard& new_guard) {
    static_assert(!std::is_const_v<U>,
                  "UpdateGuardAndLockWrite requires an exclusive write lock on "
                  "the old guard (LockedPtr<U> with non-const U).");
    ABSL_DCHECK(old_lock.get() != nullptr);
    ABSL_DCHECK_NE(old_lock.lock_.guard(), &new_guard);
    debug_guard_.CheckMatches(old_lock.lock_.guard());
    AccessGuard::WriteLock lock(new_guard);
    if (ABSL_PREDICT_FALSE(!lock)) {
      return std::nullopt;
    }
    debug_guard_.Set(&new_guard);
    return AdoptWriteLock(std::move(lock));
  }

  // GuardedPtr::GetQuiescent()
  //
  // Returns the underlying pointer without locking. Must ONLY be called when
  // concurrent access is otherwise ruled out:
  //   * During single-threaded object initialization or when the owning Python
  //     wrapper is verified to be uniquely referenced by the calling thread,
  //   * Inside tp_dealloc when Py_REFCNT(self) == 0, or
  //   * At legacy C-API boundaries that return an unguarded pointer after a
  //     point-in-time lock check.
  T* GetQuiescent() const { return ptr_; }

 private:
  friend struct CMessage;
  friend struct ContainerBase;
  template <typename DstT, typename SrcT>
  friend std::optional<std::pair<LockedPtr<DstT>, LockedPtr<const SrcT>>>
  LockWriteRead(const GuardedPtr<DstT>& dst, AccessGuard& dst_guard,
                const GuardedPtr<SrcT>& src, AccessGuard& src_guard);

  ABSL_MUST_USE_RESULT LockedPtr<const T> AdoptReadLock(
      AccessGuard::ReadLock&& lock) const {
    ABSL_DCHECK(ptr_ != nullptr);
    ABSL_DCHECK(lock);
    debug_guard_.CheckMatches(lock.guard());
    return LockedPtr<const T>(ptr_, std::move(lock));
  }

  ABSL_MUST_USE_RESULT LockedPtr<T> AdoptWriteLock(
      AccessGuard::WriteLock&& lock) const {
    ABSL_DCHECK(ptr_ != nullptr);
    ABSL_DCHECK(lock);
    debug_guard_.CheckMatches(lock.guard());
    return LockedPtr<T>(ptr_, std::move(lock));
  }

  T* ptr_ = nullptr;
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS
  free_threading_internal::DebugGuardTracker debug_guard_;
};

#ifdef NDEBUG
static_assert(sizeof(GuardedPtr<int>) == sizeof(int*),
              "GuardedPtr must have zero space overhead in NDEBUG builds.");
#endif

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_GUARDED_PTR_H__
