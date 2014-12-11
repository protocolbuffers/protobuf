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

#ifdef ADDRESS_SANITIZER
#include <sanitizer/asan_interface.h>
#endif

namespace google {
namespace protobuf {

google::protobuf::internal::SequenceNumber Arena::lifecycle_id_generator_;
#ifdef PROTOBUF_USE_DLLS
Arena::ThreadCache& Arena::thread_cache() {
  static GOOGLE_THREAD_LOCAL ThreadCache thread_cache_ = { -1, NULL };
  return thread_cache_;
}
#else
GOOGLE_THREAD_LOCAL Arena::ThreadCache Arena::thread_cache_ = { -1, NULL };
#endif

void Arena::Init(const ArenaOptions& options) {
  lifecycle_id_ = lifecycle_id_generator_.GetNext();
  start_block_size_ = options.start_block_size;
  max_block_size_ = options.max_block_size;
  block_alloc = options.block_alloc;
  block_dealloc = options.block_dealloc;
  blocks_ = 0;
  hint_ = 0;
  owns_first_block_ = true;
  cleanup_list_ = 0;

  if (options.initial_block != NULL && options.initial_block_size > 0) {
    // Add first unowned block to list.
    Block* first_block = reinterpret_cast<Block*>(options.initial_block);
    first_block->size = options.initial_block_size;
    first_block->pos = kHeaderSize;
    first_block->next = NULL;
    first_block->owner = &first_block->owner;
    AddBlock(first_block);
    owns_first_block_ = false;
  }
}

uint64 Arena::Reset() {
  CleanupList();
  uint64 space_used = FreeBlocks();
  // Invalidate any ThreadCaches pointing to any blocks we just destroyed.
  lifecycle_id_ = lifecycle_id_generator_.GetNext();
  return space_used;
}

Arena::Block* Arena::NewBlock(void* me, Block* my_last_block, size_t n,
                              size_t start_block_size, size_t max_block_size) {
  size_t size;
  if (my_last_block != NULL) {
    // Double the current block size, up to a limit.
    size = 2 * (my_last_block->size);
    if (size > max_block_size) size = max_block_size;
  } else {
    size = start_block_size;
  }
  if (n > size - kHeaderSize) {
    // TODO(sanjay): Check if n + kHeaderSize would overflow
    size = kHeaderSize + n;
  }

  Block* b = reinterpret_cast<Block*>(block_alloc(size));
  b->pos = kHeaderSize + n;
  b->size = size;
  if (b->avail() == 0) {
    // Do not attempt to reuse this block.
    b->owner = NULL;
  } else {
    b->owner = me;
  }
#ifdef ADDRESS_SANITIZER
  // Poison the rest of the block for ASAN. It was unpoisoned by the underlying
  // malloc but it's not yet usable until we return it as part of an allocation.
  ASAN_POISON_MEMORY_REGION(
      reinterpret_cast<char*>(b) + b->pos, b->size - b->pos);
#endif
  return b;
}

void Arena::AddBlock(Block* b) {
  MutexLock l(&blocks_lock_);
  b->next = reinterpret_cast<Block*>(google::protobuf::internal::NoBarrier_Load(&blocks_));
  google::protobuf::internal::Release_Store(&blocks_, reinterpret_cast<google::protobuf::internal::AtomicWord>(b));
  if (b->avail() != 0) {
    // Direct future allocations to this block.
    google::protobuf::internal::Release_Store(&hint_, reinterpret_cast<google::protobuf::internal::AtomicWord>(b));
  }
}

void Arena::AddListNode(void* elem, void (*cleanup)(void*)) {
  Node* node = reinterpret_cast<Node*>(AllocateAligned(sizeof(Node)));
  node->elem = elem;
  node->cleanup = cleanup;
  node->next = reinterpret_cast<Node*>(
      google::protobuf::internal::NoBarrier_AtomicExchange(&cleanup_list_,
            reinterpret_cast<google::protobuf::internal::AtomicWord>(node)));
}

void* Arena::AllocateAligned(size_t n) {
  // Align n to next multiple of 8 (from Hacker's Delight, Chapter 3.)
  n = (n + 7) & -8;

  // If this thread already owns a block in this arena then try to use that.
  // This fast path optimizes the case where multiple threads allocate from the
  // same arena.
  if (thread_cache().last_lifecycle_id_seen == lifecycle_id_ &&
      thread_cache().last_block_used_ != NULL) {
    if (thread_cache().last_block_used_->avail() < n) {
      return SlowAlloc(n);
    }
    return AllocFromBlock(thread_cache().last_block_used_, n);
  }

  // Check whether we own the last accessed block on this arena.
  // This fast path optimizes the case where a single thread uses multiple
  // arenas.
  void* me = &thread_cache();
  Block* b = reinterpret_cast<Block*>(google::protobuf::internal::Acquire_Load(&hint_));
  if (!b || b->owner != me || b->avail() < n) {
    // If the next block to allocate from is the first block, try to claim it
    // for this thread.
    if (!owns_first_block_ && b->next == NULL) {
      MutexLock l(&blocks_lock_);
      if (b->owner == &b->owner && b->avail() >= n) {
        b->owner = me;
        SetThreadCacheBlock(b);
        return AllocFromBlock(b, n);
      }
    }
    return SlowAlloc(n);
  }
  return AllocFromBlock(b, n);
}

void* Arena::AllocFromBlock(Block* b, size_t n) {
  size_t p = b->pos;
  b->pos = p + n;
#ifdef ADDRESS_SANITIZER
  ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<char*>(b) + p, n);
#endif
  return reinterpret_cast<char*>(b) + p;
}

void* Arena::SlowAlloc(size_t n) {
  void* me = &thread_cache();
  Block* b = FindBlock(me);  // Find block owned by me.
  // See if allocation fits in my latest block.
  if (b != NULL && b->avail() >= n) {
    SetThreadCacheBlock(b);
    google::protobuf::internal::NoBarrier_Store(&hint_, reinterpret_cast<google::protobuf::internal::AtomicWord>(b));
    return AllocFromBlock(b, n);
  }
  b = NewBlock(me, b, n, start_block_size_, max_block_size_);
  AddBlock(b);
  if (b->owner == me) {  // If this block can be reused (see NewBlock()).
    SetThreadCacheBlock(b);
  }
  return reinterpret_cast<char*>(b) + kHeaderSize;
}

uint64 Arena::SpaceUsed() const {
  uint64 space_used = 0;
  Block* b = reinterpret_cast<Block*>(google::protobuf::internal::NoBarrier_Load(&blocks_));
  while (b != NULL) {
    space_used += (b->size);
    b = b->next;
  }
  return space_used;
}


uint64 Arena::FreeBlocks() {
  uint64 space_used = 0;
  Block* b = reinterpret_cast<Block*>(google::protobuf::internal::NoBarrier_Load(&blocks_));
  Block* first_block = NULL;
  while (b != NULL) {
    space_used += (b->size);
    Block* next = b->next;
    if (next != NULL) {
      block_dealloc(b, b->size);
    } else {
      if (owns_first_block_) {
        block_dealloc(b, b->size);
      } else {
        // User passed in the first block, skip free'ing the memory.
        first_block = b;
      }
    }
    b = next;
  }
  blocks_ = 0;
  hint_ = 0;
  if (!owns_first_block_) {
    // Make the first block that was passed in through ArenaOptions
    // available for reuse.
    first_block->pos = kHeaderSize;
    first_block->owner = &first_block->owner;
    AddBlock(first_block);
  }
  return space_used;
}

void Arena::CleanupList() {
  Node* head =
      reinterpret_cast<Node*>(google::protobuf::internal::NoBarrier_Load(&cleanup_list_));
  while (head != NULL) {
    head->cleanup(head->elem);
    head = head->next;
  }
  cleanup_list_ = 0;
}

Arena::Block* Arena::FindBlock(void* me) {
  // TODO(sanjay): We might want to keep a separate list with one
  // entry per thread.
  Block* b = reinterpret_cast<Block*>(google::protobuf::internal::Acquire_Load(&blocks_));
  while (b != NULL && b->owner != me) {
    b = b->next;
  }
  return b;
}

}  // namespace protobuf
}  // namespace google
