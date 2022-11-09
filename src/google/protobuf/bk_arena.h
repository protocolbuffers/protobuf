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

#ifndef GOOGLE_PROTOBUF_BK_ARENA_H__
#define GOOGLE_PROTOBUF_BK_ARENA_H__

#include <atomic>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iosfwd>

#include "google/protobuf/stubs/logging.h"
#include "google/protobuf/stubs/common.h"
#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/arena_align.h"
#include "google/protobuf/arena_allocation_policy.h"
#include "google/protobuf/bk_serial_arena.h"
#include "google/protobuf/lookup_chunk.h"
#include "google/protobuf/memory_block.h"
#include "google/protobuf/port.h"
#include "google/protobuf/thread_cache.h"
#include "util/symbolize/symbolized_stacktrace.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

struct MessageOwned;

class BKArena {
 public:
  using AllocationInfo = BKSerialArena::AllocationInfo;
  using AlignDefault = ArenaAlignDefault;
  using Chunk = LookupChunk<const void, BKSerialArena>;

  // Constants used as part of Arena
  static constexpr size_t kBlockHeaderSize = sizeof(MemoryBlock);
  static constexpr size_t kSerialArenaSize = sizeof(BKSerialArena);
  static constexpr size_t kFirstChunkOverhead = Chunk::AllocSize(3);

  BKArena() noexcept;
  explicit BKArena(const internal::MessageOwned&) : message_owned_(true) {}
  BKArena(void* mem, size_t size);
  BKArena(void* mem, size_t size, const AllocationPolicy& policy);
  ~BKArena();

  bool GetSerialArenaFast(BKSerialArena** out);

  template <bool use_array_cache, typename Align = ArenaAlignDefault>
  void* AllocateAligned(size_t n, Align align = {});

  bool MaybeAllocateAligned(size_t n, void** out);

  template <typename Type, typename Align = ArenaAlignDefault>
  void* AllocateCleanup(Type type, Align align = {});

  void ReturnArrayMemory(void* p, size_t size);

  AllocationInfo GetAllocationInfo() const;
  uint64_t SpaceUsed() const { return GetAllocationInfo().used; }
  uint64_t SpaceAllocated() const { return GetAllocationInfo().allocated; }

  bool IsMessageOwned() const { return message_owned_; }
  size_t Reset();

  BKSerialArena* FindArenaForTesting() { return FindArena(); }

 private:
  using ThreadCache = ThreadCache<BKSerialArena>;
  using BKSerialArenaPtr = std::atomic<BKSerialArena*>;

  ThreadCache* head_owner() const;
  void set_head_owner(ThreadCache* cache);

  Chunk* chunks() const;
  void set_chunks(BKArena::Chunk* chunk);

  int64_t thread_id() const;
  void set_thread_id(int64_t thread_id);

  void InitDonated(void* mem, size_t size, const AllocationPolicy* policy);

  MemoryBlock* NewBlock(size_t n, MemoryBlock* next = MemoryBlock::sentinel());

  template <typename Align>
  BKSerialArena* NewArena(size_t n, Align align);

  template <typename Align>
  void* FallbackNew(size_t n, Align align);

  template <typename Align>
  void* Fallback(size_t n, Align align);

  template <typename Align>
  void* Fallback(size_t n, Align align, BKSerialArena* arena);

  template <typename Cleanup, typename Align>
  void* CleanupFallback(Cleanup cleanup, Align align);

  template <typename Cleanup, typename Align>
  void* CleanupFallbackNew(Cleanup cleanup, Align align);

  template <typename Cleanup, typename Align>
  void* CleanupFallback(Cleanup cleanup, Align align, BKSerialArena* arena);

  BKSerialArena* FindArena();

  void RunCleanups(MemoryBlock* memory, Ptr limit);
  void RunCleanups();

  template <typename Block>
  void DeleteBlocks(Block* block);
  template <typename Block>
  Block* DeleteBlocksPopFirst(Block* block);
  void DeleteChunks();

  ABSL_CONST_INIT static PROTOBUF_THREAD_LOCAL ThreadCache thread_cache_;

  BKSerialArena head_;
  std::atomic<ThreadCache*> head_owner_{&thread_cache_};
  std::atomic<Chunk*> chunks_{Chunk::sentinel()};

  const AllocationPolicy* policy_ = nullptr;
  int64_t thread_id_ = thread_cache_.GetUniqueId();

  absl::Mutex mutex_;
  bool first_memory_block_donated_ = false;
  bool message_owned_ = false;
};

inline BKArena::ThreadCache* BKArena::head_owner() const {
  return head_owner_.load(std::memory_order_relaxed);
}

inline void BKArena::set_head_owner(ThreadCache* cache) {
  head_owner_.store(cache, std::memory_order_release);
}

inline BKArena::Chunk* BKArena::chunks() const {
  return chunks_.load(std::memory_order_acquire);
}

inline void BKArena::set_chunks(BKArena::Chunk* chunks) {
  chunks_.store(chunks, std::memory_order_release);
}

inline int64_t BKArena::thread_id() const { return thread_id_; }

inline void BKArena::set_thread_id(int64_t thread_id) {
  thread_id_ = thread_id;
}

inline PROTOBUF_NDEBUG_INLINE bool BKArena::GetSerialArenaFast(
    BKSerialArena** out) {
  ThreadCache* thread_cache = &thread_cache_;
  if (ABSL_PREDICT_TRUE(thread_id() == thread_cache->thread_id())) {
    GOOGLE_DCHECK(thread_cache->value() != nullptr);
    *out = thread_cache->value();
    return true;
  }
  return false;
}

template <bool use_array_cache = false, typename Align>
inline PROTOBUF_NDEBUG_INLINE void* BKArena::AllocateAligned(size_t n,
                                                             Align align) {
  BKSerialArena* arena;
  if (ABSL_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
    void* ptr = arena->AllocateAligned<use_array_cache>(n, align);
    if (ABSL_PREDICT_TRUE(ptr != nullptr)) {
      return ptr;
    }
    return Fallback(n, align, arena);
  }
  return Fallback(n, align);
}

inline PROTOBUF_NDEBUG_INLINE bool BKArena::MaybeAllocateAligned(size_t n,
                                                                 void** out) {
  GOOGLE_DCHECK(ArenaAlignDefault::IsAligned(n));
  BKSerialArena* arena = nullptr;
  if (ABSL_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
    *out = arena->AllocateAligned(n, ArenaAlignDefault{});
    GOOGLE_DCHECK(ArenaAlignDefault::IsAligned(*out));
    return *out != nullptr;
  }
  return false;
}

template <typename Type, typename Align>
PROTOBUF_NDEBUG_INLINE void* BKArena::AllocateCleanup(Type cleanup,
                                                      Align align) {
  BKSerialArena* arena = nullptr;
  if (ABSL_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
    void* ptr = arena->AllocateCleanup(cleanup, align);
    if (ABSL_PREDICT_TRUE(ptr)) return ptr;
    return CleanupFallback(cleanup, align, arena);
  }
  return CleanupFallback(cleanup, align);
}

inline PROTOBUF_NDEBUG_INLINE void BKArena::ReturnArrayMemory(void* p,
                                                              size_t n) {
  BKSerialArena* arena = nullptr;
  if (ABSL_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
    arena->DonateArray(p, n);
  }
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_BK_ARENA_H__
