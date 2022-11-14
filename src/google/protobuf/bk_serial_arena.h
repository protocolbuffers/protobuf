// Protocol Buffers - Google's data interchange format
// Copyright 2021 Google Inc.  All rights reserved.
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

#ifndef GOOGLE_PROTOBUF_BK_SERIAL_ARENA_H__
#define GOOGLE_PROTOBUF_BK_SERIAL_ARENA_H__

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "absl/base/optimization.h"
#include "google/protobuf/arena_allocation_policy.h"
#include "google/protobuf/arena_cleanupx.h"
#include "google/protobuf/array_cache.h"
#include "google/protobuf/memory_block.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

template <typename Cleanup, typename Align>
inline PROTOBUF_NDEBUG_INLINE Ptr WriteCleanup(Ptr& pos, Cleanup cleanup,
                                               Align align) {
  if (size_t skip = align.ModDefaultAligned(pos)) {
    cleanupx::WriteSkip(pos -= skip, skip);
  }
  pos -= cleanup.meta_size + cleanup.allocation_size();
  auto node = cleanup.CreateMeta();
  memcpy(pos, &node, sizeof(node));
  return pos + cleanup.meta_size;
}

class alignas(ArenaAlignDefault::align) BKSerialArena {
 public:
  struct AllocationInfo {
    size_t used;
    size_t allocated;
  };

  static constexpr size_t kBlockHeaderSize = sizeof(MemoryBlock);

  constexpr BKSerialArena();
  constexpr BKSerialArena(MemoryBlock* memory);

  MemoryBlock* memory() const;
  Ptr ptr() const;
  Ptr limit() const;

  void SetMemory(MemoryBlock* memory, Ptr ptr, Ptr limit);
  MemoryBlock* FinalizeMemory();

  template <bool use_array_cache = false, typename Align>
  void* AllocateAligned(size_t n, Align align);

  template <bool use_array_cache = false>
  void* AllocateAligned(size_t n);

  template <typename Align>
  void* BlindlyAllocate(size_t n, Align align);

  template <typename Type, typename Align = ArenaAlignDefault>
  void* AllocateCleanup(Type type, Align align = {});

  template <typename Type, typename Align>
  void* BlindlyAllocateCleanup(Type type, Align align = {});

  // Arena API
  bool MaybeAllocateAligned(size_t n, void** out);

  template <typename T>
  void* MaybeAllocateWithCleanup();

  void DonateArray(void* p, size_t n) { array_cache_.DonateArray(p, n); }

  AllocationInfo GetAllocationInfo() const;

 private:
  void set_ptr(Ptr ptr);
  void set_limit(Ptr ptr);

  void add_space_used(size_t used);
  void add_space_allocated(size_t allocated);

  std::atomic<MemoryBlock*> memory_;
  std::atomic<Ptr> ptr_;
  std::atomic<Ptr> limit_;

  ArrayCache array_cache_;
  std::atomic<size_t> space_allocated_{0};
  std::atomic<size_t> space_used_{0};
};

inline constexpr BKSerialArena::BKSerialArena()
    : BKSerialArena(MemoryBlock::sentinel()) {}

inline constexpr BKSerialArena::BKSerialArena(MemoryBlock* memory)
    : memory_(memory), ptr_(memory->head()), limit_(memory->tail()) {}

inline MemoryBlock* BKSerialArena::FinalizeMemory() {
  Ptr ptr = this->ptr();
  Ptr limit = this->limit();
  MemoryBlock* memory = this->memory();

  memory->set_limit(limit);

  add_space_allocated(memory->allocated_size());
  add_space_used(ptr - memory->head() + memory->tail() - limit);

  return memory;
}

inline MemoryBlock* BKSerialArena::memory() const {
  return memory_.load(std::memory_order_acquire);
}

inline void BKSerialArena::SetMemory(MemoryBlock* memory, Ptr ptr, Ptr limit) {
  GOOGLE_DCHECK_GE(limit, memory->head());
  GOOGLE_DCHECK_LE(limit, memory->tail());
  GOOGLE_DCHECK_GE(ptr, memory->head());
  GOOGLE_DCHECK_LE(ptr, limit);
  memory_.store(memory, std::memory_order_release);
  set_ptr(ptr);
  set_limit(limit);
}

inline Ptr BKSerialArena::ptr() const {
  return ptr_.load(std::memory_order_relaxed);
}

inline void BKSerialArena::set_ptr(Ptr ptr) {
  ptr_.store(ptr, std::memory_order_relaxed);
}

inline Ptr BKSerialArena::limit() const {
  return limit_.load(std::memory_order_relaxed);
}

inline void BKSerialArena::set_limit(Ptr ptr) {
  limit_.store(ptr, std::memory_order_relaxed);
}

inline void BKSerialArena::add_space_used(size_t used) {
  used += space_used_.load(std::memory_order_relaxed);
  space_used_.store(used, std::memory_order_relaxed);
}

inline void BKSerialArena::add_space_allocated(size_t allocated) {
  allocated += space_allocated_.load(std::memory_order_relaxed);
  space_allocated_.store(allocated, std::memory_order_relaxed);
}

template <typename T>
PROTOBUF_NDEBUG_INLINE void* BKSerialArena::MaybeAllocateWithCleanup() {
  static_assert(alignof(T) <= ArenaAlignDefault::align,
                "This function is only for standard aligned types");
  static_assert(!std::is_trivially_destructible<T>::value,
                "This function is only for non-trivial types.");
  return AllocateCleanup(cleanupx::CleanupArgFor<T>());
}

template <typename Type, typename Align>
PROTOBUF_NDEBUG_INLINE void* BKSerialArena::BlindlyAllocateCleanup(
    Type type, Align align) {
  Ptr limit = this->limit();
  const size_t n = type.meta_size + type.allocation_size();
  GOOGLE_DCHECK_LE(n + align.extra(), static_cast<size_t>(limit - ptr()));
  Ptr ptr = WriteCleanup(limit, type, align);
  set_limit(limit);
  return ptr;
}

template <typename Type, typename Align>
PROTOBUF_NDEBUG_INLINE void* BKSerialArena::AllocateCleanup(Type type,
                                                            Align align) {
  Ptr limit = this->limit();
  const size_t available = limit - ptr();
  const size_t n = type.meta_size + type.allocation_size();
  if (ABSL_PREDICT_TRUE(n + align.extra() <= available)) {
    Ptr ptr = WriteCleanup(limit, type, align);
    set_limit(limit);
    ABSL_ASSUME(ptr);
    return ptr;
  }
  return nullptr;
}

template <bool use_array_cache, typename Align>
inline PROTOBUF_NDEBUG_INLINE void* BKSerialArena::AllocateAligned(
    size_t n, Align align) {
  if (use_array_cache) {
    if (void* ptr = array_cache_.AllocateArray(n)) {
      return ptr;
    }
  }
  Ptr ptr = this->ptr();
  const size_t available = limit() - ptr;
  if (ABSL_PREDICT_TRUE(n + align.extra() <= available)) {
    ptr = align.CeilDefaultAligned(ptr);
    set_ptr(ptr + n);
    ABSL_ASSUME(ptr != nullptr);
    return ptr;
  }
  return nullptr;
}

template <bool use_array_cache>
inline PROTOBUF_NDEBUG_INLINE void* BKSerialArena::AllocateAligned(size_t n) {
  return AllocateAligned<use_array_cache>(n, ArenaAlignDefault{});
}

template <typename Align>
inline PROTOBUF_NDEBUG_INLINE void* BKSerialArena::BlindlyAllocate(
    size_t n, Align align) {
  GOOGLE_DCHECK_LE(n + align.extra(), static_cast<size_t>(limit() - this->ptr()));
  Ptr ptr = align.CeilDefaultAligned(this->ptr());
  set_ptr(ptr + n);
  return ptr;
}

inline PROTOBUF_NDEBUG_INLINE bool BKSerialArena::MaybeAllocateAligned(
    size_t n, void** out) {
  *out = AllocateAligned(n, ArenaAlignDefault{});
  return *out != nullptr;
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_BK_SERIAL_ARENA_H__
