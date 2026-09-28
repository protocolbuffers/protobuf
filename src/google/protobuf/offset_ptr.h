// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_OFFSET_PTR_H__
#define GOOGLE_PROTOBUF_OFFSET_PTR_H__

#include <cstddef>
#include <cstdint>

#include "absl/base/optimization.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

[[noreturn]] PROTOBUF_PRESERVE_ALL PROTOBUF_EXPORT void BasePointerOverflow(
    const void* ptr, const void* base) noexcept;
[[noreturn]] PROTOBUF_PRESERVE_ALL PROTOBUF_EXPORT void
BasePointerInvalidSelfReference() noexcept;
[[noreturn]] PROTOBUF_PRESERVE_ALL PROTOBUF_EXPORT void
BasePointerNonnullFailure() noexcept;

// Offset based pointer-like class.
// It encodes its data relative to a `base` pointer.
// The caller must provide this pointer and it must be the same base pointer
// pass to `Resolve`.
// The offset is encoded in 32-bit and the caller must guarantee that.
//
// Is kAllowNull is false, then nullptr is not a valid input and will terminate
// the program. However, such mode is faster. The caller should choose the
// appropriate setting for the pointer in question.
template <typename T, bool kAllowNull>
class BasePointer {
  // We must use 0 as the null pointer because some of these are initialized via
  // memset.
  static constexpr int32_t kNullOffset = 0;

 public:
  // Uninitialized.
  BasePointer() = default;

  // Trivial copy/assign.
  BasePointer(const BasePointer&) = default;
  BasePointer& operator=(const BasePointer&) = default;

  BasePointer(T* ptr, const void* base) {
    if constexpr (kAllowNull) {
      if (ptr == nullptr) {
        offset_ = kNullOffset;
        return;
      }
      if (ABSL_PREDICT_FALSE(ptr == base)) {
        BasePointerInvalidSelfReference();
      }
    } else {
      if (ABSL_PREDICT_FALSE(ptr == nullptr)) {
        BasePointerNonnullFailure();
      }
    }

    ptrdiff_t diff = reinterpret_cast<const char*>(ptr) -
                     reinterpret_cast<const char*>(base);
    if (ABSL_PREDICT_FALSE(static_cast<int32_t>(diff) != diff)) {
      BasePointerOverflow(ptr, base);
    }

    offset_ = static_cast<int32_t>(diff);
  }

  // `base` must be the same `base` pointer as the one passed to the
  // constructor.
  T* Resolve(const void* base) const {
    if constexpr (kAllowNull) {
      if (offset_ == kNullOffset) return nullptr;
    }
    T* out = const_cast<T*>(reinterpret_cast<const T*>(
        reinterpret_cast<const char*>(base) + offset_));
    PROTOBUF_ASSUME(out != nullptr);
    return out;
  }

 private:
  int32_t offset_;
};

// Offset based pointer class.
// It uses its own address as the base pointer, which simplifies its use but
// restricts the input pointer to be in the same slab of memory as the
// `OffsetPtr` instance itself.
// kAllowNull follows the semantics of BasePointer.
template <typename T, bool kAllowNull>
class OffsetPtr {
 public:
  using value_type = T;

  OffsetPtr() = default;

  // Bit copy is wrong because we are relative to `this`.
  OffsetPtr(const OffsetPtr&) = delete;
  OffsetPtr& operator=(const OffsetPtr&) = delete;

  // We can't have a conversion constructor because it would allow for
  // temporaries to be made, which breaks the invariant of maximum distance.

  T* get() const { return ptr_.Resolve(this); }
  operator T*() const { return get(); }  // NOLINT
  T* operator->() const { return get(); }

  OffsetPtr& operator=(T* value) {
    ptr_ = BasePointer<T, kAllowNull>(value, this);
    return *this;
  }

  OffsetPtr& operator=(std::nullptr_t) {
    static_assert(kAllowNull, "Can't accept null.");
    return *this = static_cast<T*>(nullptr);
  }

 private:
  BasePointer<T, kAllowNull> ptr_;
};

template <typename T>
using NullableOffsetPtr = OffsetPtr<T, true>;

template <typename T>
using NonnullOffsetPtr = OffsetPtr<T, false>;

// Same as OffsetPtr, with a special case &T::default_instance.
// The pointer can be set to `&T::default_instance()` even though it is outside
// the range. It is handled specially.
// Null inputs are equivalent to the default instance.
template <typename T>
class OffsetProtoPtr {
 public:
  using value_type = T;

  OffsetProtoPtr() = default;

  // Bit copy is wrong because we are relative to `this`.
  OffsetProtoPtr(const OffsetProtoPtr&) = delete;
  OffsetProtoPtr& operator=(const OffsetProtoPtr&) = delete;

  // We can't have a conversion constructor because it would allow for
  // temporaries to be made, which breaks the invariant of maximum distance.

  T* get() const {
    T* value = ptr_.Resolve(this);
    return value == nullptr ? &T::default_instance() : value;
  }
  operator T*() const { return get(); }  // NOLINT
  T* operator->() const { return get(); }

  OffsetProtoPtr& operator=(T* value) {
    ptr_ = BasePointer<T, true>(
        value == &T::default_instance() ? nullptr : value, this);
    return *this;
  }

 private:
  BasePointer<T, true> ptr_;
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_OFFSET_PTR_H__
