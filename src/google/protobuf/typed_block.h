// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
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

#include <limits>
#include <memory>
#include <string>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/port.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

template <typename T>
class TypedBlock {
 public:
  static constexpr size_t max_count() {
    return std::numeric_limits<uint16_t>::max();
  }
  static constexpr size_t max_size() {
    return sizeof(TypedBlock) + sizeof(T) * max_count();
  }

  static constexpr size_t AllocSize(size_t count) {
    return sizeof(TypedBlock) + sizeof(T) * count;
  }

  static constexpr size_t SizeCount(size_t size) {
    return (size - sizeof(TypedBlock)) / sizeof(T);
  }

  static TypedBlock* Emplace(void* mem, size_t size, TypedBlock* next) {
    GOOGLE_ABSL_DCHECK_GE(size, sizeof(TypedBlock));
    GOOGLE_ABSL_DCHECK_LE(size, max_size());
    return new (mem) TypedBlock(SizeCount(size), false, next);
  }

  static TypedBlock* Create(size_t count, TypedBlock* next) {
    GOOGLE_ABSL_DCHECK_LE(count, max_count());
    SizedPtr res = internal::SizedAllocate(AllocSize(count));
    return new (res.p) TypedBlock(SizeCount(res.n), true, next);
  }

  static void Delete(TypedBlock* block) {
    internal::SizedDelete(block, AllocSize(block->capacity()));
  }

  void DestroyAll() {
    for (T& t : *this) t.~T();
  }

  static TypedBlock* sentinel();

  TypedBlock* next() const { return next_; }
  size_t capacity() const { return capacity_; }
  size_t space_used() const { return count_ * sizeof(T); }
  size_t space_allocated() const { return AllocSize(capacity_); }
  bool heap_allocated() const { return heap_allocated_; }

  inline std::string* TryAllocate() {
    if (ABSL_PREDICT_TRUE(count_ < capacity_)) {
      return reinterpret_cast<T*>(this + 1) + count_++;
    }
    return nullptr;
  }

  inline std::string* Allocate() {
    GOOGLE_ABSL_DCHECK_LT(count_, capacity_);
    return reinterpret_cast<T*>(this + 1) + count_++;
  }

 private:
  ~TypedBlock() = default;
  constexpr TypedBlock() = default;
  TypedBlock(size_t capacity, bool heap_allocated, TypedBlock* next)
      : capacity_(static_cast<uint16_t>(capacity)),
        heap_allocated_(heap_allocated),
        next_(next) {}

  T* begin() { return reinterpret_cast<T*>(this + 1); }
  T* end() { return reinterpret_cast<T*>(this + 1) + count_; }

  struct Sentinel;

  struct alignas(T) {
    const uint16_t capacity_ = 0;
    uint16_t count_ = 0;
    bool heap_allocated_ = false;
    TypedBlock* const next_ = nullptr;
  };
};

template <typename T>
struct TypedBlock<T>::Sentinel {
  static constexpr TypedBlock<T> kSentinel;
};

template <typename T>
inline TypedBlock<T>* TypedBlock<T>::sentinel() {
  return const_cast<TypedBlock<T>*>(&Sentinel::kSentinel);
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_TYPED_BLOCK_H__
