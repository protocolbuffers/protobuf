// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This file defines the internal templated class TypedBlock<T>

#ifndef GOOGLE_PROTOBUF_TYPED_BLOCK_H__
#define GOOGLE_PROTOBUF_TYPED_BLOCK_H__

#include <algorithm>
#include <limits>
#include <memory>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/log/absl_check.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/port.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

template <typename T>
class TypedBlock {
 public:
  static_assert(alignof(T) <= ArenaAlignDefault::align, "");

  static constexpr size_t kMinItemCount = size_t{4};
  static constexpr size_t kMinSize = size_t{128};
  static constexpr size_t kMaxSize = size_t{4 << 10};
  static constexpr size_t kInitialSize = size_t{256};

  static constexpr size_t min_size();
  static constexpr size_t max_size();
  static constexpr size_t initial_size();

  TypedBlock* Emplace(void* mem, size_t size, size_t count = 1);
  TypedBlock* Create(size_t size, size_t count = 1);
  static void Delete(TypedBlock* block);

  static TypedBlock* sentinel() ABSL_ATTRIBUTE_RETURNS_NONNULL;

  TypedBlock* next() const { return next_; }
  size_t next_size() const { return next_size_; }

  size_t space_used() const { return count_ * sizeof(T); }
  size_t space_allocated() const { return allocated_size_; }

  bool is_heap_allocated() const { return allocated_size_ > 0; }

  void* TryAllocate();
  void* Allocate() ABSL_ATTRIBUTE_RETURNS_NONNULL;

  T* begin() ABSL_ATTRIBUTE_RETURNS_NONNULL {
    return reinterpret_cast<T*>(this + 1);
  }

  T* end() ABSL_ATTRIBUTE_RETURNS_NONNULL {
    return reinterpret_cast<T*>(this + 1) + count_;
  }

 private:
  static constexpr size_t SizeToCount(size_t size) {
    return (size - sizeof(TypedBlock)) / sizeof(T);
  }

  static constexpr size_t CountToSize(size_t count) {
    return sizeof(TypedBlock) + count * sizeof(T);
  }

  constexpr TypedBlock() = default;
  constexpr TypedBlock(size_t allocated_size, size_t size, size_t count,
                       size_t next_size, TypedBlock* next)
      : count_(count),
        capacity_(static_cast<uint32_t>(SizeToCount(size))),
        allocated_size_(static_cast<uint32_t>(allocated_size)),
        next_size_(next_size),
        next_(next) {}

  static inline void PrefetchW(void* p, size_t dist) {
  }

  struct Sentinel;

  struct {
    uint32_t count_ = 0;
    const uint32_t capacity_ = 0;
    const uint32_t allocated_size_ = 0;
    const uint32_t next_size_ = static_cast<uint32_t>(min_size());
    TypedBlock* const next_ = nullptr;
  };
};

template <typename T>
constexpr size_t TypedBlock<T>::kMinItemCount;
template <typename T>
constexpr size_t TypedBlock<T>::kMinSize;
template <typename T>
constexpr size_t TypedBlock<T>::kMaxSize;
template <typename T>
constexpr size_t TypedBlock<T>::kInitialSize;

template <typename T>
inline constexpr size_t TypedBlock<T>::min_size() {
  return std::max(kMinSize, CountToSize(kMinItemCount));
}

template <typename T>
constexpr size_t TypedBlock<T>::max_size() {
  return std::max(kMaxSize, min_size());
}

template <typename T>
constexpr size_t TypedBlock<T>::initial_size() {
  return std::max(kInitialSize, min_size());
}

template <typename T>
inline PROTOBUF_ALWAYS_INLINE TypedBlock<T>* TypedBlock<T>::Emplace(
    void* mem, size_t size, size_t count) {
  PrefetchW(mem, sizeof(TypedBlock));
  ABSL_DCHECK_GE(size, CountToSize(count));
  ABSL_DCHECK_LE(size, max_size());
  size_t next_sz = std::min(next_size() + size, max_size());
  return new (mem) TypedBlock(0, size, count, next_sz, this);
}

template <typename T>
inline TypedBlock<T>* TypedBlock<T>::Create(size_t size, size_t count) {
  ABSL_DCHECK_GE(size, CountToSize(count));
  ABSL_DCHECK_LE(size, max_size());
  SizedPtr res = internal::AllocateAtLeast(size);
  PrefetchW(res.p, sizeof(TypedBlock));
  size_t next_sz = std::min(next_size() + res.n, max_size());
  return new (res.p) TypedBlock(res.n, res.n, count, next_sz, this);
}

template <typename T>
inline void TypedBlock<T>::Delete(TypedBlock* block) {
  ABSL_DCHECK(block != nullptr);
  ABSL_DCHECK(block->allocated_size_ != size_t{0});
  internal::SizedDelete(block, block->allocated_size_);
}

template <typename T>
struct TypedBlock<T>::Sentinel {
  static constexpr TypedBlock<T> kSentinel = {};
};

template <typename T>
constexpr TypedBlock<T> TypedBlock<T>::Sentinel::kSentinel;

template <typename T>
inline TypedBlock<T>* TypedBlock<T>::sentinel() {
  return const_cast<TypedBlock<T>*>(&Sentinel::kSentinel);
}

template <typename T>
inline PROTOBUF_ALWAYS_INLINE void* TypedBlock<T>::TryAllocate() {
  if (ABSL_PREDICT_FALSE(count_ >= capacity_)) return nullptr;
  void* ptr = reinterpret_cast<T*>(this + 1) + count_++;
  PrefetchW(ptr, 0);
  PROTOBUF_ASSUME(ptr != nullptr);
  return ptr;
}

template <typename T>
inline PROTOBUF_ALWAYS_INLINE void* TypedBlock<T>::Allocate() {
  ABSL_DCHECK_LT(count_, capacity_);
  return reinterpret_cast<T*>(this + 1) + count_++;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_TYPED_BLOCK_H__
