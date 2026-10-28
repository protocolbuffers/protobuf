// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// -----------------------------------------------------------------------------
// File: guarded.h
// -----------------------------------------------------------------------------
//
// This header file defines `Guarded<T>`, a value of type T that is protected by
// an `AccessGuard`. The value can only be read or written by presenting an
// access token (`ReadAccess` / `WriteAccess`, see access_guard.h) proving that
// the calling thread holds the corresponding lock.
//
// A single `AccessGuard` may protect any number of `Guarded<T>` values (of any
// types) across many objects, such as every CMessage and container in a message
// tree. Once a token has been obtained from `AccessGuard::WithRead()` or
// `WithWrite()`, it can be used to access all of them with zero additional
// atomic operations.
//
// For pointer-like types (`U*` or `std::unique_ptr<U>`), `Guarded<T>`
// propagates constness to the pointee:
//
//   * `Get(const ReadAccess&)`  returns `const U*`.
//   * `Get(const WriteAccess&)` returns `U*`.
//
// so that the referent of a guarded pointer is protected along with the
// pointer value itself. `Guarded<std::unique_ptr<U>>` owns its pointee.
//
// `Guarded<T>` is neither copyable nor movable, since copying would access the
// value without a token. Initialize it in place (e.g. with placement new in a
// Python object's constructor) or with SetQuiescent().
//
// Example:
//
//   struct MessageTree {
//     AccessGuard guard;
//   };
//
//   struct CMessage {
//     PyObject_HEAD
//     MessageTree* tree;
//     Guarded<CMessage*> parent;        // Nullable; protected by tree->guard.
//     Guarded<Message*> message;        // Protected by tree->guard.
//     Guarded<MessageMutabilityState> state;
//   };
//
//   PyObject* ByteSize(CMessage* self) {
//     return self->tree->guard.WithRead([&](const ReadAccess& r) {
//       return PyLong_FromSize_t(self->message.Get(r)->ByteSizeLong());
//     });
//   }
//
//   PyObject* Clear(CMessage* self) {
//     return self->tree->guard.WithWrite([&](const WriteAccess& w) {
//       self->message.Get(w)->Clear();  // Message*, since w is a WriteAccess.
//       Py_RETURN_NONE;
//     });
//   }
//
// Normally sizeof(Guarded<T>) == sizeof(T). In sanitizer builds (ASan, MSan,
// TSan), `Guarded<T>` additionally records which `AccessGuard` protects it and
// CHECKs on every access that the presented token is for that guard. (This is
// keyed on sanitizers rather than NDEBUG because it changes the layout of
// `Guarded<T>`, and NDEBUG is commonly mixed within a single binary.)

#ifndef GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_GUARDED_H__
#define GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_GUARDED_H__

#include <memory>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/pyext/free_threading/access_guard.h"

namespace google {
namespace protobuf {
namespace python {

namespace free_threading_internal {

// Raw pointers and std::unique_ptr are "pointer-like": Guarded<T> propagates
// constness through them to the pointee.
template <typename T>
inline constexpr bool kIsPointerLike = std::is_pointer_v<T>;
template <typename U, typename D>
inline constexpr bool kIsPointerLike<std::unique_ptr<U, D>> = true;

// The raw pointer held by a pointer-like value.
template <typename U>
U* RawPointer(U* ptr) {
  return ptr;
}
template <typename U, typename D>
U* RawPointer(const std::unique_ptr<U, D>& ptr) {
  return ptr.get();
}

template <typename T, bool = kIsPointerLike<T>>
struct GuardedTypes {
  using ReadType = const T&;
  using WriteType = void;  // Unused: Get(WriteAccess) requires a pointer-like.
};

template <typename T>
struct GuardedTypes<T, true> {
  using Element = typename std::pointer_traits<T>::element_type;
  using ReadType = const Element*;
  using WriteType = Element*;
};

}  // namespace free_threading_internal

template <typename T>
class Guarded {
  static constexpr bool kIsPointerLike =
      free_threading_internal::kIsPointerLike<T>;
  using Types = free_threading_internal::GuardedTypes<T>;

 public:
  // The type returned by read access: `const U*` if T is a pointer-like
  // (`U*` or `std::unique_ptr<U>`), else `const T&`.
  using ReadType = typename Types::ReadType;

  Guarded() = default;
  Guarded(T value, const AccessGuard& guard)
      : value_(std::move(value)), debug_guard_(&guard) {}

  // Guarded values are neither copyable nor movable: a copy would read or
  // write the value without a token.
  Guarded(const Guarded&) = delete;
  Guarded& operator=(const Guarded&) = delete;

  // Guarded::Get()
  //
  // Reads the value using read (or write) access to the protecting guard. For
  // pointer-like T, returns `const U*`.
  ReadType Get(const ReadAccess& access ABSL_ATTRIBUTE_LIFETIME_BOUND) const {
    debug_guard_.CheckMatches(access.guard());
    if constexpr (kIsPointerLike) {
      return Raw();
    } else {
      return value_;
    }
  }

  // For pointer-like T, write access yields the mutable pointee `U*`. (For
  // other T, a WriteAccess binds to the ReadAccess overload above; use Set() to
  // modify the value.)
  auto Get(const WriteAccess& access ABSL_ATTRIBUTE_LIFETIME_BOUND) const ->
      typename Types::WriteType
    requires kIsPointerLike
  {
    debug_guard_.CheckMatches(access.guard());
    return Raw();
  }

  // Guarded::Set()
  //
  // Replaces the value using write access to the protecting guard.
  void Set(T value, const WriteAccess& access) {
    debug_guard_.CheckMatches(access.guard());
    value_ = std::move(value);
  }

  // Guarded::UpdateGuard()
  //
  // Hands off this value from old_access's guard to new_access's guard while
  // holding write locks on both (for example, when a subtree is detached into a
  // new MessageTree).
  void UpdateGuard(const WriteAccess& old_access,
                   const WriteAccess& new_access) {
    ABSL_DCHECK_NE(old_access.guard(), new_access.guard());
    debug_guard_.CheckMatches(old_access.guard());
    debug_guard_.Set(new_access.guard());
  }

  // Guarded::GetQuiescent() / SetQuiescent() / TakeQuiescent()
  //
  // Read, replace, or take (return and reset to T{}) the value without a
  // token. Must ONLY be used when concurrent access is otherwise ruled out:
  // during single-threaded initialization, when the owning object is uniquely
  // referenced by the calling thread, or inside tp_dealloc.
  const T& GetQuiescent() const { return value_; }
  void SetQuiescent(T value) { value_ = std::move(value); }
  T TakeQuiescent() { return std::exchange(value_, T{}); }

  // Guarded::BindQuiescent()
  //
  // Rebinds the protecting guard (used for debug checks) without a token. Same
  // restrictions as GetQuiescent(): for objects that are not yet (or no longer)
  // reachable from other threads, such as a newly created object being
  // attached to an existing tree.
  void BindQuiescent(const AccessGuard& guard) { debug_guard_.Set(&guard); }

 private:
  auto Raw() const
    requires kIsPointerLike
  {
    return free_threading_internal::RawPointer(value_);
  }

  T value_{};
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS
  free_threading_internal::DebugGuardTracker debug_guard_;
};

static_assert(free_threading_internal::kTrackGuards ||
                  sizeof(Guarded<int*>) == sizeof(int*),
              "Guarded must have zero space overhead unless tracking guards.");

}  // namespace python
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_PYEXT_FREE_THREADING_GUARDED_H__
