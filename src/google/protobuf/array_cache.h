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

#ifndef GOOGLE_PROTOBUF_ARRAY_CACHE_H__
#define GOOGLE_PROTOBUF_ARRAY_CACHE_H__

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "absl/base/optimization.h"
#include "absl/numeric/bits.h"
#include "google/protobuf/arena_align.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

class ArrayCache {
 public:
  PROTOBUF_NDEBUG_INLINE void* AllocateArray(size_t n);
  PROTOBUF_NDEBUG_INLINE void DonateArray(void* p, size_t n);

  void* GetArrayCacheForTesting() { return array_cache_list_; }

 private:
  struct ArrayCacheBlock {
    static constexpr int kMaxCount = 36;

    union {
      ArrayCacheBlock* next;
      intptr_t count;
    };
    ArrayCacheBlock* blocks[kMaxCount];
  };

  ArrayCacheBlock* array_cache_list_{nullptr};
};

inline PROTOBUF_NDEBUG_INLINE void* ArrayCache::AllocateArray(size_t n) {
  GOOGLE_DCHECK(ArenaAlignDefault::IsAligned(n));

  if (/* sizeof(void*) >= 8 || */ ABSL_PREDICT_TRUE(n >= 16)) {
    intptr_t index = absl::bit_width(n - 1) - 4;
    ArrayCacheBlock* list = array_cache_list_;
    if (list != nullptr && index < list->count) {
      if (ArrayCacheBlock* head = list->blocks[index]) {
        PROTOBUF_UNPOISON_MEMORY_REGION(head, n);
        list->blocks[index] = head->next;
        ABSL_ASSUME(head != nullptr);
        return head;
      }
    }
  }
  return nullptr;
}

inline PROTOBUF_NDEBUG_INLINE void ArrayCache::DonateArray(void* p, size_t n) {
  if (sizeof(void*) < 8 && ABSL_PREDICT_FALSE(n < 16)) return;
  GOOGLE_DCHECK_GE(n, 16);

  intptr_t index = absl::bit_width(n) - 5;
  ArrayCacheBlock* block = static_cast<ArrayCacheBlock*>(p);

  if (ABSL_PREDICT_FALSE(array_cache_list_ == nullptr)) {
    block->count = index + 1;
    std::fill(block->blocks, block->blocks + block->count, nullptr);
    array_cache_list_ = block;
    PROTOBUF_POISON_MEMORY_REGION(block, n);
    PROTOBUF_UNPOISON_MEMORY_REGION(block, (index + 2) * sizeof(intptr_t));
    return;
  }

  ArrayCacheBlock* list = array_cache_list_;
  const intptr_t count = list->count;
  if (ABSL_PREDICT_TRUE(index < count)) {
    block->next = list->blocks[index];
    list->blocks[index] = block;
    PROTOBUF_POISON_MEMORY_REGION(p, n);
    return;
  }

  block->count = index + 1;
  std::copy(list->blocks, list->blocks + count, block->blocks);
  std::fill(block->blocks + list->count, block->blocks + block->count, nullptr);

  list->next = block->blocks[count - 1];
  block->blocks[count - 1] = list;
  PROTOBUF_POISON_MEMORY_REGION(list, size_t{1} << (count + 3));

  array_cache_list_ = block;
  PROTOBUF_POISON_MEMORY_REGION(block, n);
  PROTOBUF_UNPOISON_MEMORY_REGION(block, (index + 2) * sizeof(intptr_t));
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARRAY_CACHE_H__
