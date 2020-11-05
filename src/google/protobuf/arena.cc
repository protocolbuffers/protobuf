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
#include <atomic>
#include <limits>

#include <google/protobuf/stubs/mutex.h>

#ifdef ADDRESS_SANITIZER
#include <sanitizer/asan_interface.h>
#endif  // ADDRESS_SANITIZER

#include <google/protobuf/port_def.inc>

static const size_t kMinCleanupListElements = 8;
static const size_t kMaxCleanupListElements = 64;  // 1kB on 64-bit.

namespace google {
namespace protobuf {

PROTOBUF_EXPORT /*static*/ void* (*const ArenaOptions::kDefaultBlockAlloc)(
    size_t) = &::operator new;

namespace internal {


ArenaImpl::CacheAlignedLifecycleIdGenerator ArenaImpl::lifecycle_id_generator_;
#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
ArenaImpl::ThreadCache& ArenaImpl::thread_cache() {
  static internal::ThreadLocalStorage<ThreadCache>* thread_cache_ =
      new internal::ThreadLocalStorage<ThreadCache>();
  return *thread_cache_->Get();
}
#elif defined(PROTOBUF_USE_DLLS)
ArenaImpl::ThreadCache& ArenaImpl::thread_cache() {
  static PROTOBUF_THREAD_LOCAL ThreadCache thread_cache_ = {
      0, static_cast<LifecycleIdAtomic>(-1), nullptr};
  return thread_cache_;
}
#else
PROTOBUF_THREAD_LOCAL ArenaImpl::ThreadCache ArenaImpl::thread_cache_ = {
    0, static_cast<LifecycleIdAtomic>(-1), nullptr};
#endif

void ArenaFree(void* object, size_t size) {
#if defined(__GXX_DELETE_WITH_SIZE__) || defined(__cpp_sized_deallocation)
  ::operator delete(object, size);
#else
  (void)size;
  ::operator delete(object);
#endif
}

ArenaImpl::ArenaImpl(const ArenaOptions& options) {
  ArenaMetricsCollector* collector = nullptr;
  bool record_allocs = false;
  if (options.make_metrics_collector != nullptr) {
    collector = (*options.make_metrics_collector)();
    record_allocs = (collector && collector->RecordAllocs());
  }

  // Get memory where we can store non-default options if needed.
  // Use supplied initial_block if it is large enough.
  size_t min_block_size = kOptionsSize + kBlockHeaderSize + kSerialArenaSize;
  char* mem = options.initial_block;
  size_t mem_size = options.initial_block_size;
  GOOGLE_DCHECK_EQ(reinterpret_cast<uintptr_t>(mem) & 7, 0);
  if (mem == nullptr || mem_size < min_block_size) {
    // Supplied initial block is not big enough.
    mem_size = std::max(min_block_size, options.start_block_size);
    mem = reinterpret_cast<char*>((*options.block_alloc)(mem_size));
  }

  // Create the special block.
  const bool special = true;
  const bool user_owned = (mem == options.initial_block);
  auto block =
      new (mem) SerialArena::Block(mem_size, nullptr, special, user_owned);

  // Options occupy the beginning of the initial block.
  options_ = new (block->Pointer(block->pos())) Options;
#ifdef ADDRESS_SANITIZER
  ASAN_UNPOISON_MEMORY_REGION(options_, kOptionsSize);
#endif  // ADDRESS_SANITIZER
  options_->start_block_size = options.start_block_size;
  options_->max_block_size = options.max_block_size;
  options_->block_alloc = options.block_alloc;
  options_->block_dealloc = options.block_dealloc;
  options_->metrics_collector = collector;
  block->set_pos(block->pos() + kOptionsSize);

  Init(record_allocs);
  SetInitialBlock(block);
}

void ArenaImpl::Init(bool record_allocs) {
  ThreadCache& tc = thread_cache();
  auto id = tc.next_lifecycle_id;
  constexpr uint64 kInc = ThreadCache::kPerThreadIds * 2;
  if (PROTOBUF_PREDICT_FALSE((id & (kInc - 1)) == 0)) {
    if (sizeof(lifecycle_id_generator_.id) == 4) {
      // 2^32 is dangerous low to guarantee uniqueness. If we start dolling out
      // unique id's in ranges of kInc it's unacceptably low. In this case
      // we increment by 1. The additional range of kPerThreadIds that are used
      // per thread effectively pushes the overflow time from weeks to years
      // of continuous running.
      id = lifecycle_id_generator_.id.fetch_add(1, std::memory_order_relaxed) *
           kInc;
    } else {
      id =
          lifecycle_id_generator_.id.fetch_add(kInc, std::memory_order_relaxed);
    }
  }
  tc.next_lifecycle_id = id + 2;
  // We store "record_allocs" in the low bit of lifecycle_id_.
  lifecycle_id_ = id | (record_allocs ? 1 : 0);
  hint_.store(nullptr, std::memory_order_relaxed);
  threads_.store(nullptr, std::memory_order_relaxed);
  space_allocated_.store(0, std::memory_order_relaxed);
}

void ArenaImpl::SetInitialBlock(SerialArena::Block* block) {
  // Calling thread owns the first block. This allows the single-threaded case
  // to allocate on the first block without having to perform atomic operations.
  SerialArena* serial = SerialArena::New(block, &thread_cache(), this);
  serial->set_next(NULL);
  threads_.store(serial, std::memory_order_relaxed);
  space_allocated_.store(block->size(), std::memory_order_relaxed);
  CacheSerialArena(serial);
}

ArenaImpl::~ArenaImpl() {
  // Have to do this in a first pass, because some of the destructors might
  // refer to memory in other blocks.
  CleanupList();

  ArenaMetricsCollector* collector = nullptr;
  auto deallocator = &ArenaFree;
  if (options_) {
    collector = options_->metrics_collector;
    deallocator = options_->block_dealloc;
  }

  PerBlock([deallocator](SerialArena::Block* b) {
#ifdef ADDRESS_SANITIZER
    // This memory was provided by the underlying allocator as unpoisoned, so
    // return it in an unpoisoned state.
    ASAN_UNPOISON_MEMORY_REGION(b->Pointer(0), b->size());
#endif  // ADDRESS_SANITIZER
    if (!b->user_owned()) {
      (*deallocator)(b, b->size());
    }
  });

  if (collector) {
    collector->OnDestroy(SpaceAllocated());
  }
}

uint64 ArenaImpl::Reset() {
  if (options_ && options_->metrics_collector) {
    options_->metrics_collector->OnReset(SpaceAllocated());
  }

  // Have to do this in a first pass, because some of the destructors might
  // refer to memory in other blocks.
  CleanupList();

  // Discard all blocks except the special block (if present).
  uint64 space_allocated = 0;
  SerialArena::Block* special_block = nullptr;
  auto deallocator = (options_ ? options_->block_dealloc : &ArenaFree);
  PerBlock(
      [&space_allocated, &special_block, deallocator](SerialArena::Block* b) {
        space_allocated += b->size();
#ifdef ADDRESS_SANITIZER
        // This memory was provided by the underlying allocator as unpoisoned,
        // so return it in an unpoisoned state.
        ASAN_UNPOISON_MEMORY_REGION(b->Pointer(0), b->size());
#endif  // ADDRESS_SANITIZER
        if (!b->special()) {
          (*deallocator)(b, b->size());
        } else {
          // Prepare special block for reuse.
          // Note: if options_ is present, it occupies the beginning of the
          // block and therefore pos is advanced past it.
          GOOGLE_DCHECK(special_block == nullptr);
          special_block = b;
        }
      });

  Init(record_allocs());
  if (special_block != nullptr) {
    // next() should still be nullptr since we are using a stack discipline, but
    // clear it anyway to reduce fragility.
    GOOGLE_DCHECK_EQ(special_block->next(), nullptr);
    special_block->clear_next();
    special_block->set_pos(kBlockHeaderSize + (options_ ? kOptionsSize : 0));
    SetInitialBlock(special_block);
  }
  return space_allocated;
}

std::pair<void*, size_t> ArenaImpl::NewBuffer(size_t last_size,
                                              size_t min_bytes) {
  size_t size;
  if (last_size != -1) {
    // Double the current block size, up to a limit.
    auto max_size = options_ ? options_->max_block_size : kDefaultMaxBlockSize;
    size = std::min(2 * last_size, max_size);
  } else {
    size = options_ ? options_->start_block_size : kDefaultStartBlockSize;
  }
  // Verify that min_bytes + kBlockHeaderSize won't overflow.
  GOOGLE_CHECK_LE(min_bytes, std::numeric_limits<size_t>::max() - kBlockHeaderSize);
  size = std::max(size, kBlockHeaderSize + min_bytes);

  void* mem = options_ ? (*options_->block_alloc)(size) : ::operator new(size);
  space_allocated_.fetch_add(size, std::memory_order_relaxed);
  return {mem, size};
}

SerialArena::Block* SerialArena::NewBlock(SerialArena::Block* last_block,
                                          size_t min_bytes, ArenaImpl* arena) {
  void* mem;
  size_t size;
  std::tie(mem, size) =
      arena->NewBuffer(last_block ? last_block->size() : -1, min_bytes);
  Block* b = new (mem) Block(size, last_block, false, false);
  return b;
}

PROTOBUF_NOINLINE
void SerialArena::AddCleanupFallback(void* elem, void (*cleanup)(void*)) {
  size_t size = cleanup_ ? cleanup_->size * 2 : kMinCleanupListElements;
  size = std::min(size, kMaxCleanupListElements);
  size_t bytes = internal::AlignUpTo8(CleanupChunk::SizeOf(size));
  CleanupChunk* list = reinterpret_cast<CleanupChunk*>(AllocateAligned(bytes));
  list->next = cleanup_;
  list->size = size;

  cleanup_ = list;
  cleanup_ptr_ = &list->nodes[0];
  cleanup_limit_ = &list->nodes[size];

  AddCleanup(elem, cleanup);
}

void* ArenaImpl::AllocateAlignedAndAddCleanup(size_t n,
                                              void (*cleanup)(void*)) {
  SerialArena* arena;
  if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
    return arena->AllocateAlignedAndAddCleanup(n, cleanup);
  } else {
    return AllocateAlignedAndAddCleanupFallback(n, cleanup);
  }
}

void ArenaImpl::AddCleanup(void* elem, void (*cleanup)(void*)) {
  SerialArena* arena;
  if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
    arena->AddCleanup(elem, cleanup);
  } else {
    return AddCleanupFallback(elem, cleanup);
  }
}

PROTOBUF_NOINLINE
void* ArenaImpl::AllocateAlignedFallback(size_t n) {
  return GetSerialArenaFallback(&thread_cache())->AllocateAligned(n);
}

PROTOBUF_NOINLINE
void* ArenaImpl::AllocateAlignedAndAddCleanupFallback(size_t n,
                                                      void (*cleanup)(void*)) {
  return GetSerialArenaFallback(
      &thread_cache())->AllocateAlignedAndAddCleanup(n, cleanup);
}

PROTOBUF_NOINLINE
void ArenaImpl::AddCleanupFallback(void* elem, void (*cleanup)(void*)) {
  GetSerialArenaFallback(&thread_cache())->AddCleanup(elem, cleanup);
}

PROTOBUF_NOINLINE
void* SerialArena::AllocateAlignedFallback(size_t n) {
  // Sync back to current's pos.
  head_->set_pos(head_->size() - (limit_ - ptr_));

  head_ = NewBlock(head_, n, arena_);
  ptr_ = head_->Pointer(head_->pos());
  limit_ = head_->Pointer(head_->size());

#ifdef ADDRESS_SANITIZER
  ASAN_POISON_MEMORY_REGION(ptr_, limit_ - ptr_);
#endif  // ADDRESS_SANITIZER

  return AllocateAligned(n);
}

uint64 ArenaImpl::SpaceAllocated() const {
  return space_allocated_.load(std::memory_order_relaxed);
}

uint64 ArenaImpl::SpaceUsed() const {
  SerialArena* serial = threads_.load(std::memory_order_acquire);
  uint64 space_used = 0;
  for (; serial; serial = serial->next()) {
    space_used += serial->SpaceUsed();
  }
  // Remove the overhead of Options structure, if any.
  if (options_) {
    space_used -= kOptionsSize;
  }
  return space_used;
}

uint64 SerialArena::SpaceUsed() const {
  // Get current block's size from ptr_ (since we can't trust head_->pos().
  uint64 space_used = ptr_ - head_->Pointer(kBlockHeaderSize);
  // Get subsequent block size from b->pos().
  for (Block* b = head_->next(); b; b = b->next()) {
    space_used += (b->pos() - kBlockHeaderSize);
  }
  // Remove the overhead of the SerialArena itself.
  space_used -= ArenaImpl::kSerialArenaSize;
  return space_used;
}

void ArenaImpl::CleanupList() {
  // By omitting an Acquire barrier we ensure that any user code that doesn't
  // properly synchronize Reset() or the destructor will throw a TSAN warning.
  SerialArena* serial = threads_.load(std::memory_order_relaxed);

  for (; serial; serial = serial->next()) {
    serial->CleanupList();
  }
}

void SerialArena::CleanupList() {
  if (cleanup_ != NULL) {
    CleanupListFallback();
  }
}

void SerialArena::CleanupListFallback() {
  // The first chunk might be only partially full, so calculate its size
  // from cleanup_ptr_. Subsequent chunks are always full, so use list->size.
  size_t n = cleanup_ptr_ - &cleanup_->nodes[0];
  CleanupChunk* list = cleanup_;
  while (true) {
    CleanupNode* node = &list->nodes[0];
    // Cleanup newest elements first (allocated last).
    for (size_t i = n; i > 0; i--) {
      node[i - 1].cleanup(node[i - 1].elem);
    }
    list = list->next;
    if (list == nullptr) {
      break;
    }
    // All but the first chunk are always full.
    n = list->size;
  }
}

SerialArena* SerialArena::New(Block* b, void* owner, ArenaImpl* arena) {
  auto pos = b->pos();
  GOOGLE_DCHECK_LE(pos + ArenaImpl::kSerialArenaSize, b->size());
  SerialArena* serial = reinterpret_cast<SerialArena*>(b->Pointer(pos));
  b->set_pos(pos + ArenaImpl::kSerialArenaSize);
  serial->arena_ = arena;
  serial->owner_ = owner;
  serial->head_ = b;
  serial->ptr_ = b->Pointer(b->pos());
  serial->limit_ = b->Pointer(b->size());
  serial->cleanup_ = NULL;
  serial->cleanup_ptr_ = NULL;
  serial->cleanup_limit_ = NULL;
  return serial;
}

PROTOBUF_NOINLINE
SerialArena* ArenaImpl::GetSerialArenaFallback(void* me) {
  // Look for this SerialArena in our linked list.
  SerialArena* serial = threads_.load(std::memory_order_acquire);
  for (; serial; serial = serial->next()) {
    if (serial->owner() == me) {
      break;
    }
  }

  if (!serial) {
    // This thread doesn't have any SerialArena, which also means it doesn't
    // have any blocks yet.  So we'll allocate its first block now.
    SerialArena::Block* b = SerialArena::NewBlock(NULL, kSerialArenaSize, this);
    serial = SerialArena::New(b, me, this);

    SerialArena* head = threads_.load(std::memory_order_relaxed);
    do {
      serial->set_next(head);
    } while (!threads_.compare_exchange_weak(
        head, serial, std::memory_order_release, std::memory_order_relaxed));
  }

  CacheSerialArena(serial);
  return serial;
}

ArenaMetricsCollector::~ArenaMetricsCollector() {}

}  // namespace internal

PROTOBUF_FUNC_ALIGN(32)
void* Arena::AllocateAlignedNoHook(size_t n) {
  return impl_.AllocateAligned(n);
}

}  // namespace protobuf
}  // namespace google
