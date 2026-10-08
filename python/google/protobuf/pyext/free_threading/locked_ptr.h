// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// -----------------------------------------------------------------------------
// File: locked_ptr.h
// -----------------------------------------------------------------------------
//
// This header file defines `LockedPtr<T>`, a movable, non-copyable RAII smart
// pointer that couples a raw pointer T* with an active lock token on a
// message tree's `AccessGuard`.
//
// `LockedPtr<T>` is obtained by locking a `GuardedPtr<T>` (see guarded_ptr.h).
// Its constness encodes the kind of lock held on the underlying `AccessGuard`:
//
//   * `LockedPtr<const T>` holds a shared read lock on the `AccessGuard`.
//   * `LockedPtr<T>` (for non-const T) holds an exclusive write lock on the
//     `AccessGuard`.
//
// In addition to providing direct pointer access (`operator->`, `operator*`,
// `get()`), a live `LockedPtr` serves as a capability token that can be passed
// to `GuardedPtr<ChildT>::Get()` or `GuardedPtr<ChildT>::GetMutable()` to
// unpack other `GuardedPtr` fields protected by the same `AccessGuard` with
// zero additional atomic operations.

#ifndef GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_LOCKED_PTR_H__
#define GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_LOCKED_PTR_H__

#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"

namespace google {
namespace protobuf {
namespace python {

template <typename T>
class GuardedPtr;

// -----------------------------------------------------------------------------
// PYPROTO_ASSIGN_OR_RETURN(lhs, rexpr, err_ret)
// -----------------------------------------------------------------------------
//
// Evaluates rexpr, which returns a `std::optional<T>` (such as
// `std::optional<LockedPtr<Message>>` or a `std::optional<std::pair<...>>` from
// `LockWriteRead()`). If the optional is empty (`!has_value()`), immediately
// returns err_ret from the current function (a Python RuntimeError is already
// set on the thread). Otherwise, moves the unwrapped value into lhs.
//
// If lhs contains commas (such as a structured binding declaration), wrap lhs
// in parentheses:
//
//   PYPROTO_ASSIGN_OR_RETURN(LockedPtr<const Message> msg,
//                            self->message.LockRead(self->tree->guard),
//                            nullptr);
//   PYPROTO_ASSIGN_OR_RETURN(
//       (auto [dst, src]),
//       LockWriteRead(self->message, self->tree->guard,
//                     other->message, other->tree->guard),
//       nullptr);
#define PYPROTO_INTERNAL_PROBE(...) ~, 1
#define PYPROTO_INTERNAL_SECOND_IMPL(a, b, ...) b
#define PYPROTO_INTERNAL_SECOND(...) PYPROTO_INTERNAL_SECOND_IMPL(__VA_ARGS__)
#define PYPROTO_INTERNAL_IS_PAREN(x) \
  PYPROTO_INTERNAL_SECOND(PYPROTO_INTERNAL_PROBE x, 0)
#define PYPROTO_INTERNAL_REM(...) __VA_ARGS__
#define PYPROTO_INTERNAL_UNPAREN_0(x) x
#define PYPROTO_INTERNAL_UNPAREN_1(x) PYPROTO_INTERNAL_REM x
#define PYPROTO_INTERNAL_CONCAT_INNER(x, y) x##y
#define PYPROTO_INTERNAL_CONCAT(x, y) PYPROTO_INTERNAL_CONCAT_INNER(x, y)
#define PYPROTO_INTERNAL_UNPAREN(x)                  \
  PYPROTO_INTERNAL_CONCAT(PYPROTO_INTERNAL_UNPAREN_, \
                          PYPROTO_INTERNAL_IS_PAREN(x))(x)

#define PYPROTO_INTERNAL_ASSIGN_OR_RETURN_IMPL(tmp, lhs, rexpr, err_ret) \
  auto tmp = (rexpr);                                                    \
  if (ABSL_PREDICT_FALSE(!tmp.has_value())) {                            \
    return err_ret;                                                      \
  }                                                                      \
  PYPROTO_INTERNAL_UNPAREN(lhs) = *std::move(tmp)

#define PYPROTO_ASSIGN_OR_RETURN(lhs, rexpr, err_ret)                       \
  PYPROTO_INTERNAL_ASSIGN_OR_RETURN_IMPL(                                   \
      PYPROTO_INTERNAL_CONCAT(_pyproto_assign_or_return_tmp_, __COUNTER__), \
      lhs, rexpr, err_ret)

// -----------------------------------------------------------------------------
// LockedPtr<T>
// -----------------------------------------------------------------------------
//
// Non-nullable scoped RAII smart pointer holding either a shared read lock
// (`LockedPtr<const T>`) or an exclusive write lock (`LockedPtr<T>`) on the
// `AccessGuard` protecting T*.
//
// Lock acquisition methods (`LockRead()`, `LockWrite()`) return
// `std::optional<LockedPtr<T>>`, which is empty (`std::nullopt`) if contention
// was detected (with a Python RuntimeError already set on the thread). Once
// unwrapped via `PYPROTO_ASSIGN_OR_RETURN`, a `LockedPtr<T>` is guaranteed to
// be non-null for its entire lifetime (unless moved from).
//
// Example (read-only access):
//
//   PyObject* GetByteSize(CMessage* self) {
//     PYPROTO_ASSIGN_OR_RETURN(LockedPtr<const Message> msg,
//                              self->message.LockRead(self->tree->guard),
//                              nullptr);
//     return PyLong_FromSize_t(msg->ByteSizeLong());
//   }  // msg destructor releases the shared read lock.
//
// Example (mutable access and scoped release before calling back into Python):
//
//   PyObject* MutateAndFormat(CMessage* self) {
//     {
//       PYPROTO_ASSIGN_OR_RETURN(LockedPtr<Message> msg,
//                                self->message.LockWrite(self->tree->guard),
//                                nullptr);
//       msg->Clear();
//       // msg goes out of scope here, releasing the write lock before
//       // invoking arbitrary Python code that might re-enter CMessage methods
//       // on the same tree.
//     }
//     return PyObject_Str(reinterpret_cast<PyObject*>(self));
//   }
//
// Example (using a `LockedPtr` to unpack another `GuardedPtr` protected by the
// same `AccessGuard`):
//
//   PYPROTO_ASSIGN_OR_RETURN(LockedPtr<Message> parent_msg,
//                            parent->message.LockWrite(parent->tree->guard),
//                            nullptr);
//   Message* child_msg = child->message.GetMutable(parent_msg);
//   child_msg->Clear();
template <typename T>
class ABSL_MUST_USE_RESULT LockedPtr {
 public:
  // Releases the held read or write lock via lock_'s destructor.
  ~LockedPtr() = default;

  // Move constructor and move assignment transfer ownership of both the raw
  // pointer and the active `AccessGuard` lock.
  LockedPtr(LockedPtr&& other) noexcept
      : ptr_(other.ptr_), lock_(std::move(other.lock_)) {
    other.ptr_ = nullptr;
  }

  LockedPtr& operator=(LockedPtr&& other) noexcept {
    if (this != &other) {
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      lock_ = std::move(other.lock_);
    }
    return *this;
  }

  LockedPtr(const LockedPtr&) = delete;
  LockedPtr& operator=(const LockedPtr&) = delete;

  // LockedPtr::UpdatePtr()
  //
  // Replaces the underlying pointer while keeping the existing lock held. Used
  // when an operation under a write lock replaces the underlying C++ object
  // in-place (for example, promoting a default prototype Message* to a newly
  // allocated mutable Message* via AssureWritable).
  void UpdatePtr(T* ptr) {
    ABSL_DCHECK(ptr != nullptr);
    ABSL_DCHECK(ptr_ != nullptr);
    ptr_ = ptr;
  }

  // Standard smart-pointer accessors.
  T* get() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    ABSL_DCHECK(ptr_ != nullptr);
    return ptr_;
  }

  T* operator->() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    ABSL_DCHECK(ptr_ != nullptr);
    return ptr_;
  }

  T& operator*() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    ABSL_DCHECK(ptr_ != nullptr);
    return *ptr_;
  }

 private:
  using LockType = std::conditional_t<std::is_const_v<T>, AccessGuard::ReadLock,
                                      AccessGuard::WriteLock>;

  template <typename U>
  friend class GuardedPtr;

  LockedPtr(T* ptr, LockType&& lock) : ptr_(ptr), lock_(std::move(lock)) {
    ABSL_DCHECK(ptr_ != nullptr);
    ABSL_DCHECK(lock_);
  }

  T* ptr_;
  LockType lock_;
};

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_LOCKED_PTR_H__
