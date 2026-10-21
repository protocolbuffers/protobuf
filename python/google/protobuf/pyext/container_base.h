// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// ContainerBase is the common base of all Python wrapper objects (CMessage and
// the containers) that are views into a protobuf message tree, and
// TreeLockable provides their scoped access to the tree's AccessGuard.

#ifndef GOOGLE_PROTOBUF_PYEXT_CONTAINER_BASE_H__
#define GOOGLE_PROTOBUF_PYEXT_CONTAINER_BASE_H__

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <atomic>
#include <type_traits>

#include "absl/base/optimization.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"
#include "google/protobuf/pyext/free_threading/guarded.h"
#include "google/protobuf/pyext/message_tree.h"

namespace google {
namespace protobuf {

class FieldDescriptor;
class Message;

namespace python {

struct CMessage;

namespace cmessage {
// Defined in message.cc. Promotes self (and its ancestors) to mutable.
Message* AssureWritable(CMessage* self, const WriteAccess& lock);
}  // namespace cmessage

struct ContainerBase {
  // clang-format off
  PyObject_HEAD

  // Shared pointer to the message tree containing the AccessGuard for the
  // root of this hierarchy. Never null for an initialized CMessage or
  // container. Atomic because a sub-message's or container's tree pointer is
  // updated when it is detached or reparented from its parent.
  std::atomic<MessageTree*> tree;

  // Strong reference to a parent message object, protected by the tree guard.
  // For a CMessage there are three cases:
  // - For a top-level message, this pointer is NULL.
  // - For a sub-message, this points to the parent message.
  // - For a message managed externally, this is a owned reference to Py_None.
  //
  // For all other types: repeated containers, maps, it always point to a
  // valid parent CMessage.
  Guarded<CMessage*> parent;
  // clang-format on

  MessageTree* GetTree() const { return tree.load(std::memory_order_acquire); }

  // Sets this object's tree and binds all of its Guarded<T> fields to the
  // tree's guard. Only for objects not reachable from other threads (newly
  // created, or being attached while quiescent).
  void InitTreeQuiescent(MessageTree* t);

  // Moves this object (not its children) from the tree guarded by old_access
  // to new_tree, whose guard is held by new_access, rebinding all Guarded<T>
  // fields. Used when detaching or reparenting a subtree.
  void MoveToTree(MessageTree* new_tree, const WriteAccess& old_access,
                  const WriteAccess& new_access);

  // If this object belongs to a parent message, describes which field it comes
  // from.
  // The pointer is owned by the DescriptorPool (which is kept alive
  // through the message's Python class)
  const FieldDescriptor* parent_field_descriptor;

  PyObject* AsPyObject() { return reinterpret_cast<PyObject*>(this); }

 protected:
  // Acquires this object's tree guard in the mode given by Access (ReadAccess
  // or WriteAccess) and re-validates that the object did not move to a
  // different tree while acquiring.
  template <typename Access, typename OnFail, typename F>
  auto WithTreeAccess(OnFail&& on_fail, F&& f) const;
};

// Scoped access to the MessageTree that an object belongs to. Messages and
// containers work the same way: both lock the MessageTree they belong to.
//
// Derived (CMessage or FieldContainer) must provide
// `CMessage* Owner(const ReadAccess&) const`: the message whose contents the
// object is a view of. It is resolved statically (CRTP).
template <typename Derived>
struct TreeLockable : public ContainerBase {
  // Acquires a read (or write) lock on this object's MessageTree guard and
  // invokes f(const ReadAccess&) (or f(const WriteAccess&)). Fails with
  // on_fail if the lock is contended or if this object is concurrently moved
  // to a different tree while acquiring.
  //
  // WithWrite() additionally promotes Owner() to mutable (AssureWritable)
  // before invoking f.
  //
  // As with AccessGuard::WithRead(), callbacks that return a value must pass an
  // explicit on_fail; the (f) overloads are only for void callbacks and return
  // whether f was invoked.
  template <typename OnFail, typename F>
  auto WithRead(OnFail&& on_fail, F&& f) const;
  template <typename F>
  bool WithRead(F&& f) const;

  template <typename OnFail, typename F>
  auto WithWrite(OnFail&& on_fail, F&& f) const;
  template <typename F>
  bool WithWrite(F&& f) const;

  // Acquires write access to dst's tree and read access to src's tree (a
  // single write lock if they share a tree), promotes dst->Owner() to mutable,
  // and invokes f(const WriteAccess&, const ReadAccess&).
  template <typename OnFail, typename F>
  static auto WithWriteRead(const Derived* dst, const ContainerBase* src,
                            OnFail&& on_fail, F&& f);
  template <typename F>
  static bool WithWriteRead(const Derived* dst, const ContainerBase* src,
                            F&& f);

  // Acquires read access to both objects' trees (a single read lock if they
  // share a tree) and invokes f(const ReadAccess&, const ReadAccess&).
  template <typename OnFail, typename F>
  static auto WithReadRead(const ContainerBase* a, const ContainerBase* b,
                           OnFail&& on_fail, F&& f);
  template <typename F>
  static bool WithReadRead(const ContainerBase* a, const ContainerBase* b,
                           F&& f);

  // Owner()'s C++ message. Read access yields const Message*, write access
  // yields Message*.
  const Message* TargetMessage(const ReadAccess& access) const;
  Message* TargetMessage(const WriteAccess& access) const;

 private:
  const Derived* derived() const { return static_cast<const Derived*>(this); }
};

// Scoped tree access implementations:

template <typename Access, typename OnFail, typename F>
auto ContainerBase::WithTreeAccess(OnFail&& on_fail, F&& f) const {
  static_assert(std::is_same_v<Access, ReadAccess> ||
                std::is_same_v<Access, WriteAccess>);
  using RetT = std::invoke_result_t<F, const Access&>;
  MessageTree* t = GetTree();
  auto checked = [&](const Access& access) -> RetT {
    // A concurrent detach may have moved this object to a different tree
    // between GetTree() and acquiring t->guard().
    if (ABSL_PREDICT_FALSE(GetTree() != t)) {
      AccessGuard::ReportConcurrentAccess();
      return free_threading_internal::Fail<RetT>(on_fail);
    }
    return f(access);
  };
  if constexpr (std::is_same_v<Access, WriteAccess>) {
    return t->guard().WithWrite(on_fail, checked);
  } else {
    return t->guard().WithRead(on_fail, checked);
  }
}

template <typename Derived>
template <typename OnFail, typename F>
auto TreeLockable<Derived>::WithRead(OnFail&& on_fail, F&& f) const {
  return WithTreeAccess<ReadAccess>(on_fail, f);
}

template <typename Derived>
template <typename F>
bool TreeLockable<Derived>::WithRead(F&& f) const {
  static_assert(std::is_void_v<std::invoke_result_t<F, const ReadAccess&>>,
                "Callbacks that return a value must pass an explicit on_fail.");
  return WithRead(false, [&](const ReadAccess& a) {
    f(a);
    return true;
  });
}

template <typename Derived>
template <typename OnFail, typename F>
auto TreeLockable<Derived>::WithWrite(OnFail&& on_fail, F&& f) const {
  using RetT = std::invoke_result_t<F, const WriteAccess&>;
  return WithTreeAccess<WriteAccess>(
      on_fail, [&](const WriteAccess& access) -> RetT {
        if (cmessage::AssureWritable(derived()->Owner(access), access) ==
            nullptr) {
          return free_threading_internal::Fail<RetT>(on_fail);
        }
        return f(access);
      });
}

template <typename Derived>
template <typename F>
bool TreeLockable<Derived>::WithWrite(F&& f) const {
  static_assert(std::is_void_v<std::invoke_result_t<F, const WriteAccess&>>,
                "Callbacks that return a value must pass an explicit on_fail.");
  return WithWrite(false, [&](const WriteAccess& a) {
    f(a);
    return true;
  });
}

template <typename Derived>
template <typename OnFail, typename F>
auto TreeLockable<Derived>::WithWriteRead(const Derived* dst,
                                          const ContainerBase* src,
                                          OnFail&& on_fail, F&& f) {
  using RetT = std::invoke_result_t<F, const WriteAccess&, const ReadAccess&>;
  MessageTree* dst_tree = dst->GetTree();
  MessageTree* src_tree = src->GetTree();
  return AccessGuard::WithWriteRead(
      dst_tree->guard(), src_tree->guard(), on_fail,
      [&](const WriteAccess& dst_access, const ReadAccess& src_access) -> RetT {
        if (ABSL_PREDICT_FALSE(dst->GetTree() != dst_tree ||
                               src->GetTree() != src_tree)) {
          AccessGuard::ReportConcurrentAccess();
          return free_threading_internal::Fail<RetT>(on_fail);
        }
        if (cmessage::AssureWritable(dst->Owner(dst_access), dst_access) ==
            nullptr) {
          return free_threading_internal::Fail<RetT>(on_fail);
        }
        return f(dst_access, src_access);
      });
}

template <typename Derived>
template <typename F>
bool TreeLockable<Derived>::WithWriteRead(const Derived* dst,
                                          const ContainerBase* src, F&& f) {
  static_assert(
      std::is_void_v<
          std::invoke_result_t<F, const WriteAccess&, const ReadAccess&>>,
      "Callbacks that return a value must pass an explicit on_fail.");
  return WithWriteRead(dst, src, false,
                       [&](const WriteAccess& w, const ReadAccess& r) {
                         f(w, r);
                         return true;
                       });
}

template <typename Derived>
template <typename OnFail, typename F>
auto TreeLockable<Derived>::WithReadRead(const ContainerBase* a,
                                         const ContainerBase* b,
                                         OnFail&& on_fail, F&& f) {
  using RetT = std::invoke_result_t<F, const ReadAccess&, const ReadAccess&>;
  MessageTree* a_tree = a->GetTree();
  MessageTree* b_tree = b->GetTree();
  return AccessGuard::WithReadRead(
      a_tree->guard(), b_tree->guard(), on_fail,
      [&](const ReadAccess& a_access, const ReadAccess& b_access) -> RetT {
        if (ABSL_PREDICT_FALSE(a->GetTree() != a_tree ||
                               b->GetTree() != b_tree)) {
          AccessGuard::ReportConcurrentAccess();
          return free_threading_internal::Fail<RetT>(on_fail);
        }
        return f(a_access, b_access);
      });
}

template <typename Derived>
template <typename F>
bool TreeLockable<Derived>::WithReadRead(const ContainerBase* a,
                                         const ContainerBase* b, F&& f) {
  static_assert(
      std::is_void_v<
          std::invoke_result_t<F, const ReadAccess&, const ReadAccess&>>,
      "Callbacks that return a value must pass an explicit on_fail.");
  return WithReadRead(a, b, false,
                      [&](const ReadAccess& ra, const ReadAccess& rb) {
                        f(ra, rb);
                        return true;
                      });
}

template <typename Derived>
const Message* TreeLockable<Derived>::TargetMessage(
    const ReadAccess& access) const {
  return derived()->Owner(access)->message.Get(access);
}

template <typename Derived>
Message* TreeLockable<Derived>::TargetMessage(const WriteAccess& access) const {
  return derived()->Owner(access)->message.Get(access);
}

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_CONTAINER_BASE_H__
