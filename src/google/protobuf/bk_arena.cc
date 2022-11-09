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

#include "google/protobuf/bk_arena.h"

#include <algorithm>
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
#include "google/protobuf/thread_cache.h"
#include "util/symbolize/symbolized_stacktrace.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

ABSL_CONST_INIT PROTOBUF_THREAD_LOCAL
    BKArena::ThreadCache BKArena::thread_cache_;

namespace {

struct Allocator {
  explicit Allocator(const AllocationPolicy* policy)
      : alloc(policy ? policy->block_alloc : nullptr) {}
  void* (*const alloc)(size_t);

  SizedPtr operator()(size_t n) const {
    if (alloc) {
      return {alloc(n), n};
    } else {
      return tcmalloc_size_returning_operator_new(n);
    }
  }
};

struct Deallocator {
  explicit Deallocator(const AllocationPolicy* policy)
      : dealloc(policy ? policy->block_dealloc : nullptr) {}
  void (*const dealloc)(void*, size_t);

  void operator()(void* p, size_t n) const {
    if (dealloc) {
      dealloc(p, n);
    } else {
      internal::SizedDelete(p, n);
    }
  }
};

size_t NextSize(const AllocationPolicy* policy, size_t last_size,
                size_t min_size) {
  AllocationPolicy local;  // default policy
  if (policy) local = *policy;
  size_t size;
  if (last_size != 0) {
    auto max_size = local.max_block_size;
    size = std::min(2 * last_size, max_size);
  } else {
    size = local.start_block_size;
  }
  return std::max(size, min_size);
}

template <typename Chunk, typename Key, typename Value>
inline Chunk* AllocateMoreChunks(Chunk* chunks, Key key, Value value) {
  size_t capacity = chunks->capacity() ? (chunks->capacity() + 1) * 2 - 1 : 3;
  void* mem = operator new(Chunk::AllocSize(capacity));
  return new (mem) Chunk(capacity, key, value, chunks);
}

}  // namespace

void BKArena::InitDonated(void* mem, size_t size,
                          const AllocationPolicy* policy) {
  auto* memory = new (mem) MemoryBlock(mem, size);
  Ptr head = memory->head();
  if (policy) {
    policy_ = new (head) AllocationPolicy(*policy);
    head += sizeof(AllocationPolicy);
  }
  head_.SetMemory(memory, head, memory->tail());
  first_memory_block_donated_ = true;
}

BKArena::BKArena() noexcept { thread_cache_.Set(thread_id_, &head_); }

BKArena::BKArena(void* mem, size_t size) {
  size_t min_size = sizeof(MemoryBlock);
  if (size >= min_size) {
    InitDonated(mem, size, nullptr);
  }
  thread_cache_.Set(thread_id_, &head_);
}

BKArena::BKArena(void* mem, size_t size, const AllocationPolicy& policy) {
  if (size >= sizeof(MemoryBlock) + sizeof(AllocationPolicy)) {
    InitDonated(mem, size, &policy);
  } else {
    policy_ = &policy;
    MemoryBlock* memory = NewBlock(sizeof(AllocationPolicy));
    Ptr head = memory->head();
    policy_ = new (head) AllocationPolicy(policy);
    head_.SetMemory(memory, head + sizeof(AllocationPolicy), memory->tail());
  }
  thread_cache_.Set(thread_id_, &head_);
}

void BKArena::RunCleanups(MemoryBlock* memory, Ptr limit) {
  Ptr epos = MemoryBlock::sentinel()->tail();
  while (limit != epos) {
    Ptr tail = memory->tail();
    Ptr prefetch = limit;
    for (int dist = 0; dist < 8; ++dist) {
      if (prefetch >= tail) break;
      prefetch += cleanupx::PrefetchNodeAt(prefetch);
    }
    while (prefetch < tail) {
      prefetch += cleanupx::PrefetchNodeAt(prefetch);
      limit += cleanupx::DestroyNodeAt(limit);
    }
    GOOGLE_DCHECK_EQ(prefetch, tail);
    ::compiler::PrefetchNta(memory->next());
    while (limit < tail) {
      limit += cleanupx::DestroyNodeAt(limit);
    }
    GOOGLE_DCHECK_EQ(limit, tail);

    memory = memory->next();
    limit = memory->limit();
  }
}

void BKArena::RunCleanups() {
  Chunk* chunk = chunks();
  while (Chunk* next = chunk->next()) {
    for (BKSerialArena& arena : *chunk) {
      RunCleanups(arena.memory(), arena.limit());
    }
    chunk = next;
  }
  RunCleanups(head_.memory(), head_.limit());
}

MemoryBlock* BKArena::NewBlock(size_t n, MemoryBlock* next) {
  size_t last_size = next->allocated_size();
  size_t size = NextSize(policy_, last_size, n + sizeof(MemoryBlock));
  SizedPtr ptr = Allocator(policy_)(size);
  return new (ptr.p) MemoryBlock(ptr.p, ptr.n, next);
}

template <typename Block>
inline void BKArena::DeleteBlocks(Block* block) {
  Deallocator deallocator(policy_);
  while (block != Block::sentinel()) {
    Block* next = block->next();
    deallocator(block, block->size());
    block = next;
  }
}

template <typename Block>
inline Block* BKArena::DeleteBlocksPopFirst(Block* block) {
  Deallocator deallocator(policy_);
  Block* next;
  while ((next = block->next()) != Block::sentinel()) {
    deallocator(block, block->size());
    block = next;
  }
  PROTOBUF_UNPOISON_MEMORY_REGION(memory, memory->size());
  return block;
}

void BKArena::DeleteChunks() {
  Chunk* chunk = chunks();
  while (Chunk* next = chunk->next()) {
    // Arenas are iterated in reverse order: chunks are allocated on the
    // memory blocks of the first (last iterated) arena: delete those last.
    for (BKSerialArena& arena : *chunk) {
      DeleteBlocks(arena.memory());
    }
    ::operator delete(chunk, Chunk::AllocSize(chunk->capacity()));
    chunk = next;
  }
}

PROTOBUF_NDEBUG_INLINE BKArena::~BKArena() {
  Deallocator deallocator(policy_);

  RunCleanups();
  DeleteChunks();

  MemoryBlock* memory = head_.memory();
  if (memory != MemoryBlock::sentinel()) {
    memory = DeleteBlocksPopFirst(memory);
    if (!first_memory_block_donated_) {
      deallocator(memory, memory->size());
    }
  }
}

BKSerialArena* BKArena::FindArena() {
  ThreadCache& thread_cache = thread_cache_;
  if (head_owner() == &thread_cache) {
    thread_cache_.Set(thread_id_, &head_);
    return &head_;
  }
  Chunk* chunk = chunks();
  while (Chunk* next = chunk->next()) {
    if (BKSerialArena* arena = chunk->Find(&thread_cache)) {
      thread_cache.Set(thread_id_, arena);
      return arena;
    }
    chunk = next;
  }
  return nullptr;
}

template <typename Align>
BKSerialArena* BKArena::NewArena(size_t n, Align align) {
  MemoryBlock* memory = NewBlock(sizeof(BKArena) + n + align.extra());
  Ptr head = memory->head();
  BKSerialArena* arena = new (head) BKSerialArena;
  arena->SetMemory(memory, head + sizeof(BKSerialArena), memory->tail());

  // Add to chunks and update per thread cache information
  ThreadCache* thread_cache = &thread_cache_;
  thread_cache->Set(thread_id_, arena);

#ifdef IS_THIS_WORTH_THE_HASSLE_QUESTION_MARK
  // Make a guess if we need to allocate more chunks. The idea is to allocate
  // the chunks outside of a mutex lock as to minimize any possible contention.
  Chunk* chunks = this->chunks();
  Chunk* new_chunks = chunks->size() >= chunks->capacity()
                          ? AllocateMoreChunks(chunks, thread_cache, arena)
                          : nullptr;

  {
    absl::MutexLock Lock(&mutex_);
    chunks = this->chunks();
    if (!chunks->Add(thread_cache, arena)) {
      if (new_chunks) {
        // We guessed right
        new_chunks->set_next(chunks);
        set_chunks(new_chunks);
        new_chunks = nullptr;
      } else {
        // We guessed wrong
        set_chunks(AllocateMoreChunks(chunks, thread_cache, arena));
      }
    }
  }
  if (new_chunks) {
    // We guessed wrong
    ::operator delete(new_chunks, Chunk::AllocSize(new_chunks->capacity()));
  }
#else
  absl::MutexLock Lock(&mutex_);
  Chunk* chunks = this->chunks();
  if (!chunks->Add(thread_cache, arena)) {
    set_chunks(AllocateMoreChunks(chunks, thread_cache, arena));
  }
#endif

  return arena;
}

template <typename Align>
PROTOBUF_NOINLINE void* BKArena::FallbackNew(size_t n, Align align) {
  BKSerialArena* arena = NewArena(n, align);
  return arena->BlindlyAllocate(n, align);
}

template <typename Align>
PROTOBUF_NOINLINE void* BKArena::Fallback(size_t n, Align align,
                                          BKSerialArena* arena) {
  MemoryBlock* memory = NewBlock(n + align.extra(), arena->FinalizeMemory());
  Ptr head = memory->head();
  Ptr ptr = align.CeilDefaultAligned(head);
  arena->SetMemory(memory, ptr + n, memory->tail());
  return ptr;
}
template void* BKArena::Fallback(size_t, ArenaAlign, BKSerialArena*);
template void* BKArena::Fallback(size_t, ArenaAlignDefault, BKSerialArena*);

template <typename Align>
PROTOBUF_NOINLINE void* BKArena::Fallback(size_t n, Align align) {
  BKSerialArena* arena = FindArena();
  if (arena == nullptr) return FallbackNew(n, align);
  if (void* ptr = arena->AllocateAligned(n, align)) return ptr;
  return Fallback(n, align, arena);
}

template void* BKArena::Fallback(size_t, ArenaAlign);
template void* BKArena::Fallback(size_t, ArenaAlignDefault);

template <typename Cleanup, typename Align>
void* BKArena::CleanupFallbackNew(Cleanup cleanup, Align align) {
  size_t n = cleanup.meta_size + cleanup.allocation_size();
  BKSerialArena* arena = NewArena(n, align);
  return arena->BlindlyAllocateCleanup(cleanup, align);
}

template <typename Cleanup, typename Align>
void* BKArena::CleanupFallback(Cleanup cleanup, Align align) {
  BKSerialArena* arena = FindArena();
  if (arena == nullptr) return CleanupFallbackNew(cleanup, align);
  if (void* ptr = arena->AllocateCleanup(cleanup, align)) return ptr;
  return CleanupFallback(cleanup, align, arena);
}

template <typename Cleanup, typename Align>
void* PROTOBUF_NOINLINE BKArena::CleanupFallback(Cleanup cleanup, Align align,
                                                 BKSerialArena* arena) {
  size_t n = cleanup.meta_size + cleanup.allocation_size();
  MemoryBlock* memory = NewBlock(n + align.extra(), arena->FinalizeMemory());
  Ptr head = memory->head();
  Ptr limit = memory->tail();
  Ptr ptr = WriteCleanup(limit, cleanup, align);
  arena->SetMemory(memory, head, limit);
  return ptr;
}

BKArena::AllocationInfo BKArena::GetAllocationInfo() const {
  AllocationInfo info = head_.GetAllocationInfo();
  if (policy_ != nullptr) {
    info.used -= sizeof(AllocationPolicy);
  }
  for (Chunk* chunk = chunks(); chunk; chunk = chunk->next()) {
    for (const BKSerialArena& arena : *chunk) {
      AllocationInfo next = arena.GetAllocationInfo();
      info.allocated += next.allocated;
      info.used += next.used - sizeof(BKSerialArena);
    }
  }
  return info;
}

PROTOBUF_NDEBUG_INLINE size_t BKArena::Reset() {
  AllocationInfo info = GetAllocationInfo();

  RunCleanups();
  DeleteChunks();

  MemoryBlock* memory = DeleteBlocksPopFirst(head_.memory());
  Ptr head = memory->head();

  if (policy_) {
    AllocationPolicy policy = *policy_;
    policy_ = new (head) AllocationPolicy(policy);
    head += sizeof(AllocationPolicy);
  }

  new (&head_) BKSerialArena();
  head_.SetMemory(memory, head, memory->tail());

  ThreadCache& thread_cache = thread_cache_;
  set_head_owner(&thread_cache);
  set_chunks(Chunk::sentinel());

  thread_id_ = thread_cache.GetUniqueId();
  thread_cache.Set(thread_id_, &head_);

  if (first_memory_block_donated_) {
    return memory->allocated_size();
  }
  return info.allocated;
}

template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::TaggedAllocation, ArenaAlignDefault);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::StringAllocation, ArenaAlignDefault);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::CordAllocation, ArenaAlignDefault);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::DynamicAllocation, ArenaAlignDefault);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::TaggedCleanup, ArenaAlignDefault);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::DynamicCleanup, ArenaAlignDefault);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::DynamicAllocation, ArenaAlign);

template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::TaggedAllocation, ArenaAlignDefault, BKSerialArena*);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::StringAllocation, ArenaAlignDefault, BKSerialArena*);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::CordAllocation, ArenaAlignDefault, BKSerialArena*);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::DynamicAllocation, ArenaAlignDefault, BKSerialArena*);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::TaggedCleanup, ArenaAlignDefault, BKSerialArena*);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::DynamicCleanup, ArenaAlignDefault, BKSerialArena*);
template PROTOBUF_NOINLINE void* BKArena::CleanupFallback(
    cleanupx::DynamicAllocation, ArenaAlign, BKSerialArena*);

}  // namespace internal
}  // namespace protobuf
}  // namespace google
