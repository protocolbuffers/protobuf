// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
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

#include <google/protobuf/arena.h>

#include <algorithm>
#include <limits>


#ifdef ADDRESS_SANITIZER
#include <sanitizer/asan_interface.h>
#endif  // ADDRESS_SANITIZER

namespace google {
static const size_t kMinCleanupListElements = 8;
static const size_t kMaxCleanupListElements = 64;  // 1kB on 64-bit.

namespace protobuf {
namespace internal {


google::protobuf::internal::SequenceNumber ArenaImpl::lifecycle_id_generator_;
#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
ArenaImpl::ThreadCache& ArenaImpl::thread_cache() {
  static internal::ThreadLocalStorage<ThreadCache>* thread_cache_ =
      new internal::ThreadLocalStorage<ThreadCache>();
  return *thread_cache_->Get();
}
#elif defined(PROTOBUF_USE_DLLS)
ArenaImpl::ThreadCache& ArenaImpl::thread_cache() {
  static GOOGLE_THREAD_LOCAL ThreadCache thread_cache_ = { -1, NULL };
  return thread_cache_;
}
#else
GOOGLE_THREAD_LOCAL ArenaImpl::ThreadCache ArenaImpl::thread_cache_ = {-1, NULL};
#endif

void ArenaImpl::Init() {
  lifecycle_id_ = lifecycle_id_generator_.GetNext();
  blocks_ = 0;
  hint_ = 0;
  space_allocated_ = 0;
  owns_first_block_ = true;

  if (options_.initial_block != NULL && options_.initial_block_size > 0) {
    GOOGLE_CHECK_GE(options_.initial_block_size, sizeof(Block))
        << ": Initial block size too small for header.";

    // Add first unowned block to list.
    Block* first_block = reinterpret_cast<Block*>(options_.initial_block);
    first_block->size = options_.initial_block_size;
    first_block->pos = kHeaderSize;
    first_block->next = NULL;
    first_block->cleanup = NULL;
    // Thread which calls Init() owns the first block. This allows the
    // single-threaded case to allocate on the first block without taking any
    // locks.
    first_block->owner = &thread_cache();
    AddBlockInternal(first_block);
    CacheBlock(first_block);
    owns_first_block_ = false;
  }
}

ArenaImpl::~ArenaImpl() { ResetInternal(); }

uint64 ArenaImpl::Reset() {
  // Invalidate any ThreadCaches pointing to any blocks we just destroyed.
  lifecycle_id_ = lifecycle_id_generator_.GetNext();
  return ResetInternal();
}

uint64 ArenaImpl::ResetInternal() {
  Block* head =
      reinterpret_cast<Block*>(google::protobuf::internal::NoBarrier_Load(&blocks_));
  CleanupList(head);
  uint64 space_allocated = FreeBlocks(head);

  return space_allocated;
}

ArenaImpl::Block* ArenaImpl::NewBlock(void* me, Block* my_last_block,
                                      size_t min_bytes, size_t start_block_size,
                                      size_t max_block_size) {
  size_t size;
  if (my_last_block != NULL) {
    // Double the current block size, up to a limit.
    size = std::min(2 * my_last_block->size, max_block_size);
  } else {
    size = start_block_size;
  }
  // Verify that min_bytes + kHeaderSize won't overflow.
  GOOGLE_CHECK_LE(min_bytes, std::numeric_limits<size_t>::max() - kHeaderSize);
  size = std::max(size, kHeaderSize + min_bytes);

  Block* b = reinterpret_cast<Block*>(options_.block_alloc(size));
  b->pos = kHeaderSize;
  b->size = size;
  b->owner = me;
  b->cleanup = NULL;
#ifdef ADDRESS_SANITIZER
  // Poison the rest of the block for ASAN. It was unpoisoned by the underlying
  // malloc but it's not yet usable until we return it as part of an allocation.
  ASAN_POISON_MEMORY_REGION(
      reinterpret_cast<char*>(b) + b->pos, b->size - b->pos);
#endif  // ADDRESS_SANITIZER
  AddBlock(b);
  return b;
}

void ArenaImpl::AddBlock(Block* b) {
  MutexLock l(&blocks_lock_);
  AddBlockInternal(b);
}

void ArenaImpl::AddBlockInternal(Block* b) {
  b->next = reinterpret_cast<Block*>(google::protobuf::internal::NoBarrier_Load(&blocks_));
  google::protobuf::internal::Release_Store(&blocks_, reinterpret_cast<google::protobuf::internal::AtomicWord>(b));
  space_allocated_ += b->size;
}

ArenaImpl::Block* ArenaImpl::ExpandCleanupList(Block* b) {
  size_t size = b->cleanup ? b->cleanup->size * 2 : kMinCleanupListElements;
  size = std::min(size, kMaxCleanupListElements);
  size_t bytes = internal::AlignUpTo8(CleanupChunk::SizeOf(size));
  if (b->avail() < bytes) {
    b = GetBlock(bytes);
  }
  CleanupChunk* list =
      reinterpret_cast<CleanupChunk*>(AllocFromBlock(b, bytes));
  list->next = b->cleanup;
  list->size = size;
  list->len = 0;
  b->cleanup = list;
  return b;
}

inline GOOGLE_ATTRIBUTE_ALWAYS_INLINE void ArenaImpl::AddCleanupInBlock(
    Block* b, void* elem, void (*cleanup)(void*)) {
  if (b->cleanup == NULL || b->cleanup->len == b->cleanup->size) {
    b = ExpandCleanupList(b);
  }

  CleanupNode* node = &b->cleanup->nodes[b->cleanup->len++];

  node->elem = elem;
  node->cleanup = cleanup;
}

void ArenaImpl::AddCleanup(void* elem, void (*cleanup)(void*)) {
  return AddCleanupInBlock(GetBlock(0), elem, cleanup);
}

void* ArenaImpl::AllocateAligned(size_t n) {
  GOOGLE_DCHECK_EQ(internal::AlignUpTo8(n), n);  // Must be already aligned.

  return AllocFromBlock(GetBlock(n), n);
}

void* ArenaImpl::AllocateAlignedAndAddCleanup(size_t n,
                                              void (*cleanup)(void*)) {
  GOOGLE_DCHECK_EQ(internal::AlignUpTo8(n), n);  // Must be already aligned.

  Block* b = GetBlock(n);
  void* mem = AllocFromBlock(b, n);
  AddCleanupInBlock(b, mem, cleanup);
  return mem;
}

inline GOOGLE_ATTRIBUTE_ALWAYS_INLINE ArenaImpl::Block* ArenaImpl::GetBlock(size_t n) {
  Block* my_block = NULL;

  // If this thread already owns a block in this arena then try to use that.
  // This fast path optimizes the case where multiple threads allocate from the
  // same arena.
  ThreadCache* tc = &thread_cache();
  if (tc->last_lifecycle_id_seen == lifecycle_id_) {
    my_block = tc->last_block_used_;
    if (my_block->avail() >= n) {
      return my_block;
    }
  }

  // Check whether we own the last accessed block on this arena.
  // This fast path optimizes the case where a single thread uses multiple
  // arenas.
  Block* b = reinterpret_cast<Block*>(google::protobuf::internal::Acquire_Load(&hint_));
  if (b != NULL && b->owner == tc) {
    my_block = b;
    if (my_block->avail() >= n) {
      return my_block;
    }
  }
  return GetBlockSlow(tc, my_block, n);
}

inline GOOGLE_ATTRIBUTE_ALWAYS_INLINE void* ArenaImpl::AllocFromBlock(Block* b,
                                                               size_t n) {
  GOOGLE_DCHECK_EQ(internal::AlignUpTo8(b->pos), b->pos);  // Must be already aligned.
  GOOGLE_DCHECK_EQ(internal::AlignUpTo8(n), n);  // Must be already aligned.
  GOOGLE_DCHECK_GE(b->avail(), n);
  size_t p = b->pos;
  b->pos = p + n;
#ifdef ADDRESS_SANITIZER
  ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<char*>(b) + p, n);
#endif  // ADDRESS_SANITIZER
  return reinterpret_cast<char*>(b) + p;
}

ArenaImpl::Block* ArenaImpl::GetBlockSlow(void* me, Block* my_full_block,
                                          size_t n) {
  Block* b = FindBlock(me);  // Find block owned by me.
  if (b == NULL || b->avail() < n) {
    b = NewBlock(me, b, n, options_.start_block_size, options_.max_block_size);

    // Try to steal the cleanup list from my_full_block.  It's too full for this
    // allocation, but it might have space left in its cleanup list and there's
    // no reason to waste that memory.
    if (my_full_block) {
      GOOGLE_DCHECK_EQ(my_full_block->owner, me);
      GOOGLE_DCHECK(b->cleanup == NULL);
      b->cleanup = my_full_block->cleanup;
      my_full_block->cleanup = NULL;
    }
  }
  CacheBlock(b);
  return b;
}

uint64 ArenaImpl::SpaceAllocated() const {
  MutexLock l(&blocks_lock_);
  return space_allocated_;
}

uint64 ArenaImpl::SpaceUsed() const {
  uint64 space_used = 0;
  Block* b = reinterpret_cast<Block*>(google::protobuf::internal::NoBarrier_Load(&blocks_));
  while (b != NULL) {
    space_used += (b->pos - kHeaderSize);
    b = b->next;
  }
  return space_used;
}

uint64 ArenaImpl::FreeBlocks(Block* head) {
  uint64 space_allocated = 0;
  Block* first_block = NULL;
  Block* b = head;

  while (b != NULL) {
    space_allocated += (b->size);
    Block* next = b->next;
    if (next != NULL) {
#ifdef ADDRESS_SANITIZER
      // This memory was provided by the underlying allocator as unpoisoned, so
      // return it in an unpoisoned state.
      ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<char*>(b), b->size);
#endif  // ADDRESS_SANITIZER
      options_.block_dealloc(b, b->size);
    } else {
      if (owns_first_block_) {
#ifdef ADDRESS_SANITIZER
        // This memory was provided by the underlying allocator as unpoisoned,
        // so return it in an unpoisoned state.
        ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<char*>(b), b->size);
#endif  // ADDRESS_SANITIZER
        options_.block_dealloc(b, b->size);
      } else {
        // User passed in the first block, skip free'ing the memory.
        first_block = b;
      }
    }
    b = next;
  }
  blocks_ = 0;
  hint_ = 0;
  space_allocated_ = 0;
  if (!owns_first_block_) {
    // Make the first block that was passed in through ArenaOptions
    // available for reuse.
    first_block->pos = kHeaderSize;
    first_block->cleanup = NULL;
    // Thread which calls Reset() owns the first block. This allows the
    // single-threaded case to allocate on the first block without taking any
    // locks.
    first_block->owner = &thread_cache();
    AddBlockInternal(first_block);
    CacheBlock(first_block);
  }
  return space_allocated;
}

void ArenaImpl::CleanupList(Block* head) {
  // Have to do this in a first pass, because some of the destructors might
  // refer to memory in other blocks.
  for (Block* b = head; b; b = b->next) {
    CleanupChunk* list = b->cleanup;
    while (list) {
      size_t n = list->len;
      CleanupNode* node = &list->nodes[list->len - 1];
      for (size_t i = 0; i < n; i++, node--) {
        node->cleanup(node->elem);
      }
      list = list->next;
    }
    b->cleanup = NULL;
  }
}

ArenaImpl::Block* ArenaImpl::FindBlock(void* me) {
  // TODO(sanjay): We might want to keep a separate list with one
  // entry per thread.
  Block* b = reinterpret_cast<Block*>(google::protobuf::internal::Acquire_Load(&blocks_));
  while (b != NULL && b->owner != me) {
    b = b->next;
  }
  return b;
}

}  // namespace internal

void Arena::OnArenaAllocation(const std::type_info* allocated_type,
                              size_t n) const {
  if (on_arena_allocation_ != NULL) {
    on_arena_allocation_(allocated_type, n, hooks_cookie_);
  }
}

}  // namespace protobuf
}  // namespace google
