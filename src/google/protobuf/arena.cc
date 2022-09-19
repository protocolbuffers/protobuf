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

#include "google/protobuf/arena.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <typeinfo>

#include "absl/synchronization/mutex.h"
#include "google/protobuf/arena_impl.h"
#include "google/protobuf/arenaz_sampler.h"
#include "google/protobuf/port.h"


#ifdef ADDRESS_SANITIZER
#include <sanitizer/asan_interface.h>
#endif  // ADDRESS_SANITIZER

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

static SerialArena::Memory AllocateMemory(const AllocationPolicy* policy_ptr,
                                          size_t last_size, size_t min_bytes) {
  AllocationPolicy policy;  // default policy
  if (policy_ptr) policy = *policy_ptr;
  size_t size;
  if (last_size != 0) {
    // Double the current block size, up to a limit.
    auto max_size = policy.max_block_size;
    size = std::min(2 * last_size, max_size);
  } else {
    size = policy.start_block_size;
  }
  // Verify that min_bytes + kBlockHeaderSize won't overflow.
  GOOGLE_CHECK_LE(min_bytes,
           std::numeric_limits<size_t>::max() - SerialArena::kBlockHeaderSize);
  size = std::max(size, SerialArena::kBlockHeaderSize + min_bytes);

  void* mem;
  if (policy.block_alloc == nullptr) {
    mem = ::operator new(size);
  } else {
    mem = policy.block_alloc(size);
  }
  return {mem, size};
}

class GetDeallocator {
 public:
  GetDeallocator(const AllocationPolicy* policy, size_t* space_allocated)
      : dealloc_(policy ? policy->block_dealloc : nullptr),
        space_allocated_(space_allocated) {}

  void operator()(SerialArena::Memory mem) const {
#ifdef ADDRESS_SANITIZER
    // This memory was provided by the underlying allocator as unpoisoned,
    // so return it in an unpoisoned state.
    ASAN_UNPOISON_MEMORY_REGION(mem.ptr, mem.size);
#endif  // ADDRESS_SANITIZER
    if (dealloc_) {
      dealloc_(mem.ptr, mem.size);
    } else {
      internal::SizedDelete(mem.ptr, mem.size);
    }
    *space_allocated_ += mem.size;
  }

 private:
  void (*dealloc_)(void*, size_t);
  size_t* space_allocated_;
};

SerialArena::SerialArena(ArenaBlock* b, ThreadSafeArena& parent)
    : parent_(parent), space_allocated_(b->size()) {
  set_head(b);
  set_ptr(b->Pointer(kBlockHeaderSize + ThreadSafeArena::kSerialArenaSize));
  limit_ = b->Pointer(b->size() & static_cast<size_t>(-8));
}

SerialArena* SerialArena::New(Memory mem, ThreadSafeArena& parent) {
  GOOGLE_DCHECK_LE(kBlockHeaderSize + ThreadSafeArena::kSerialArenaSize, mem.size);
  ThreadSafeArenaStats::RecordAllocateStats(parent.arena_stats_.MutableStats(),
                                            /*used=*/0, /*allocated=*/mem.size,
                                            /*wasted=*/0);
  auto b = new (mem.ptr) ArenaBlock{nullptr, mem.size};
  return new (b->Pointer(kBlockHeaderSize)) SerialArena(b, parent);
}

template <typename Deallocator>
SerialArena::Memory SerialArena::Free(Deallocator deallocator) {
  ArenaBlock* b = head();
  Memory mem = {b, b->size()};
  while (b->next) {
    b = b->next;  // We must first advance before deleting this block
    deallocator(mem);
    mem = {b, b->size()};
  }
  return mem;
}

PROTOBUF_NOINLINE
void* SerialArena::AllocateAlignedFallback(size_t n) {
  AllocateNewBlock(n);
  return AllocateFromExisting(n);
}

PROTOBUF_NOINLINE
void* SerialArena::AllocateAlignedWithCleanupFallback(
    size_t n, size_t align, void (*destructor)(void*)) {
  size_t required = AlignUpTo(n, align) + cleanup::Size(destructor);
  AllocateNewBlock(required);
  return AllocateFromExistingWithCleanupFallback(n, align, destructor);
}

PROTOBUF_NOINLINE
void SerialArena::AddCleanupFallback(void* elem, void (*destructor)(void*)) {
  size_t required = cleanup::Size(destructor);
  AllocateNewBlock(required);
  AddCleanupFromExisting(elem, destructor);
}

void SerialArena::AllocateNewBlock(size_t n) {
  // Sync limit to block
  head()->cleanup_nodes = limit_;

  // Record how much used in this block.
  size_t used = static_cast<size_t>(ptr() - head()->Pointer(kBlockHeaderSize));
  size_t wasted = head()->size() - used;
  space_used_.store(space_used_.load(std::memory_order_relaxed) + used,
                    std::memory_order_relaxed);

  // TODO(sbenza): Evaluate if pushing unused space into the cached blocks is a
  // win. In preliminary testing showed increased memory savings as expected,
  // but with a CPU regression. The regression might have been an artifact of
  // the microbenchmark.

  auto mem = AllocateMemory(parent_.AllocPolicy(), head()->size(), n);
  // We don't want to emit an expensive RMW instruction that requires
  // exclusive access to a cacheline. Hence we write it in terms of a
  // regular add.
  space_allocated_.store(
      space_allocated_.load(std::memory_order_relaxed) + mem.size,
      std::memory_order_relaxed);
  ThreadSafeArenaStats::RecordAllocateStats(parent_.arena_stats_.MutableStats(),
                                            /*used=*/used,
                                            /*allocated=*/mem.size, wasted);
  set_head(new (mem.ptr) ArenaBlock{head(), mem.size});
  set_ptr(head()->Pointer(kBlockHeaderSize));
  limit_ = head()->Pointer(head()->size());

#ifdef ADDRESS_SANITIZER
  ASAN_POISON_MEMORY_REGION(ptr(), limit_ - ptr());
#endif  // ADDRESS_SANITIZER
}

uint64_t SerialArena::SpaceUsed() const {
  // Note: the calculation below technically causes a race with
  // AllocateNewBlock when called from another thread (which happens in
  // ThreadSafeArena::SpaceUsed).  However, worst-case space_used_ will have
  // stale data and the calculation will incorrectly assume 100%
  // usage of the *current* block.
  // TODO(mkruskal) Consider eliminating this race in exchange for a possible
  // performance hit on ARM (see cl/455186837).
  const uint64_t current_block_size = head()->size();
  uint64_t space_used = std::min(
      static_cast<uint64_t>(
          ptr() - const_cast<ArenaBlock*>(head())->Pointer(kBlockHeaderSize)),
      current_block_size);
  space_used += space_used_.load(std::memory_order_relaxed);
  // Remove the overhead of the SerialArena itself.
  space_used -= ThreadSafeArena::kSerialArenaSize;
  return space_used;
}

void SerialArena::CleanupList() {
  ArenaBlock* b = head();
  b->cleanup_nodes = limit_;
  do {
    char* limit = reinterpret_cast<char*>(
        b->Pointer(b->size() & static_cast<size_t>(-8)));
    char* it = reinterpret_cast<char*>(b->cleanup_nodes);
    if (it < limit) {
      // A prefetch distance of 8 here was chosen arbitrarily.  It makes the
      // pending nodes fill a cacheline which seemed nice.
      constexpr int kPrefetchDist = 8;
      cleanup::Tag pending_type[kPrefetchDist];
      char* pending_node[kPrefetchDist];

      int pos = 0;
      for (; pos < kPrefetchDist && it < limit; ++pos) {
        pending_type[pos] = cleanup::Type(it);
        pending_node[pos] = it;
        it += cleanup::Size(pending_type[pos]);
      }

      if (pos < kPrefetchDist) {
        for (int i = 0; i < pos; ++i) {
          cleanup::DestroyNode(pending_type[i], pending_node[i]);
        }
      } else {
        pos = 0;
        while (it < limit) {
          cleanup::PrefetchNode(it);
          cleanup::DestroyNode(pending_type[pos], pending_node[pos]);
          pending_type[pos] = cleanup::Type(it);
          pending_node[pos] = it;
          it += cleanup::Size(pending_type[pos]);
          pos = (pos + 1) % kPrefetchDist;
        }
        for (int i = pos; i < pos + kPrefetchDist; ++i) {
          cleanup::DestroyNode(pending_type[i % kPrefetchDist],
                               pending_node[i % kPrefetchDist]);
        }
      }
    }
    b = b->next;
  } while (b);
}

// Stores arrays of void* and SerialArena* instead of linked list of
// SerialArena* to speed up traversing all SerialArena. The cost of walk is non
// trivial when there are many nodes. Separately storing "ids" minimizes cache
// footprints and more efficient when looking for matching arena.
//
// Uses absl::container_internal::Layout to emulate the following:
//
// struct SerialArenaChunk {
//   SerialArenaChunk* next_chunk;
//   const uint32_t capacity;
//   std::atomic<uint32_t> size;
//   std::atomic<void*> ids[];
//   std::atomic<SerialArena*> arenas[];
// };
//
// where the size of "ids" and "arenas" is determined at runtime; hence the use
// of Layout.
class ThreadSafeArena::SerialArenaChunk {
 public:
  explicit SerialArenaChunk(uint32_t capacity) {
    set_next(nullptr);
    set_capacity(capacity);
    new (&size()) std::atomic<uint32_t>{0};

    for (unsigned i = 0; i < capacity; ++i) {
      new (&id(i)) std::atomic<void*>{nullptr};
    }

    for (unsigned i = 0; i < capacity; ++i) {
      new (&arena(i)) std::atomic<void*>{nullptr};
    }
  }

  SerialArenaChunk(uint32_t capacity, void* me, SerialArena* serial) {
    set_next(nullptr);
    set_capacity(capacity);
    new (&size()) std::atomic<uint32_t>{1};

    new (&id(0)) std::atomic<void*>{me};
    for (unsigned i = 1; i < capacity; ++i) {
      new (&id(i)) std::atomic<void*>{nullptr};
    }

    new (&arena(0)) std::atomic<SerialArena*>{serial};
    for (unsigned i = 1; i < capacity; ++i) {
      new (&arena(i)) std::atomic<void*>{nullptr};
    }
  }

  // next_chunk
  const SerialArenaChunk* next_chunk() const {
    return *layout_type::Partial().Pointer<kNextChunk>(ptr());
  }
  SerialArenaChunk* next_chunk() {
    return *layout_type::Partial().Pointer<kNextChunk>(ptr());
  }
  void set_next(SerialArenaChunk* next_chunk) {
    *layout_type::Partial().Pointer<kNextChunk>(ptr()) = next_chunk;
  }

  // capacity
  uint32_t capacity() const {
    return *layout_type::Partial(1u).Pointer<kCapacity>(ptr());
  }
  void set_capacity(uint32_t capacity) {
    *layout_type::Partial(1u).Pointer<kCapacity>(ptr()) = capacity;
  }

  // ids: returns up to size().
  absl::Span<const std::atomic<void*>> ids() const {
    return Layout(capacity()).Slice<kIds>(ptr()).first(safe_size());
  }
  absl::Span<std::atomic<void*>> ids() {
    return Layout(capacity()).Slice<kIds>(ptr()).first(safe_size());
  }
  std::atomic<void*>& id(unsigned i) {
    GOOGLE_DCHECK_LT(i, capacity());
    return Layout(capacity()).Pointer<kIds>(ptr())[i];
  }

  // arenas: returns up to size().
  absl::Span<const std::atomic<SerialArena*>> arenas() const {
    return Layout(capacity()).Slice<kArenas>(ptr()).first(safe_size());
  }
  absl::Span<std::atomic<SerialArena*>> arenas() {
    return Layout(capacity()).Slice<kArenas>(ptr()).first(safe_size());
  }
  std::atomic<SerialArena*>& arena(unsigned i) {
    GOOGLE_DCHECK_LT(i, capacity());
    return Layout(capacity()).Pointer<kArenas>(ptr())[i];
  }

  // Tries to insert {id, serial} to head chunk. Returns false if the head is
  // already full.
  //
  // Note that the updating "size", "id", "arena" is individually atomic but
  // those are not protected by a mutex. This is acceptable because concurrent
  // lookups from SpaceUsed or SpaceAllocated accept inaccuracy due to race. On
  // other paths, either race is not possible (GetSerialArenaFallback) or must
  // be prevented by users (CleanupList, Free).
  bool insert(void* me, SerialArena* serial) {
    uint32_t idx = size().fetch_add(1, std::memory_order_relaxed);
    // Bail out if this chunk is full.
    if (idx >= capacity()) {
      // Write old value back to avoid potential overflow.
      size().store(capacity(), std::memory_order_relaxed);
      return false;
    }

    id(idx).store(me, std::memory_order_relaxed);
    arena(idx).store(serial, std::memory_order_relaxed);
    return true;
  }

  constexpr static size_t AllocSize(size_t n) { return Layout(n).AllocSize(); }

 private:
  constexpr static int kNextChunk = 0;
  constexpr static int kCapacity = 1;
  constexpr static int kSize = 2;
  constexpr static int kIds = 3;
  constexpr static int kArenas = 4;

  using layout_type = absl::container_internal::Layout<
      SerialArenaChunk*, uint32_t, std::atomic<uint32_t>, std::atomic<void*>,
      std::atomic<SerialArena*>>;

  const char* ptr() const { return reinterpret_cast<const char*>(this); }
  char* ptr() { return reinterpret_cast<char*>(this); }

  std::atomic<uint32_t>& size() {
    return *layout_type::Partial(1u, 1u).Pointer<kSize>(ptr());
  }

  const std::atomic<uint32_t>& size() const {
    return *layout_type::Partial(1u, 1u).Pointer<kSize>(ptr());
  }

  // Returns the size capped by the capacity as fetch_add may result in a size
  // greater than capacity.
  uint32_t safe_size() const {
    return std::min(capacity(), size().load(std::memory_order_relaxed));
  }

  constexpr static layout_type Layout(size_t n) {
    return layout_type(
        /*next_chunk*/ 1,
        /*capacity*/ 1,
        /*size*/ 1,
        /*ids*/ n,
        /*arenas*/ n);
  }
};


ThreadSafeArena::CacheAlignedLifecycleIdGenerator
    ThreadSafeArena::lifecycle_id_generator_;
#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
ThreadSafeArena::ThreadCache& ThreadSafeArena::thread_cache() {
  static internal::ThreadLocalStorage<ThreadCache>* thread_cache_ =
      new internal::ThreadLocalStorage<ThreadCache>();
  return *thread_cache_->Get();
}
#elif defined(PROTOBUF_USE_DLLS)
ThreadSafeArena::ThreadCache& ThreadSafeArena::thread_cache() {
  static PROTOBUF_THREAD_LOCAL ThreadCache thread_cache_ = {
      0, static_cast<LifecycleIdAtomic>(-1), nullptr};
  return thread_cache_;
}
#else
PROTOBUF_THREAD_LOCAL ThreadSafeArena::ThreadCache
    ThreadSafeArena::thread_cache_ = {0, static_cast<LifecycleIdAtomic>(-1),
                                      nullptr};
#endif

void ThreadSafeArena::InitializeFrom(void* mem, size_t size) {
  GOOGLE_DCHECK_EQ(reinterpret_cast<uintptr_t>(mem) & 7, 0u);
  GOOGLE_DCHECK(!AllocPolicy());  // Reset should call InitializeWithPolicy instead.
  Init();

  // Ignore initial block if it is too small.
  if (mem != nullptr && size >= kBlockHeaderSize + kSerialArenaSize) {
    alloc_policy_.set_is_user_owned_initial_block(true);
    SetInitialBlock(mem, size);
  }
}

void ThreadSafeArena::InitializeWithPolicy(void* mem, size_t size,
                                           AllocationPolicy policy) {
#ifndef NDEBUG
  const uint64_t old_alloc_policy = alloc_policy_.get_raw();
  // If there was a policy (e.g., in Reset()), make sure flags were preserved.
#define GOOGLE_DCHECK_POLICY_FLAGS_() \
  if (old_alloc_policy > 3)    \
    GOOGLE_CHECK_EQ(old_alloc_policy & 3, alloc_policy_.get_raw() & 3)
#else
#define GOOGLE_DCHECK_POLICY_FLAGS_()
#endif  // NDEBUG

  if (policy.IsDefault()) {
    // Legacy code doesn't use the API above, but provides the initial block
    // through ArenaOptions. I suspect most do not touch the allocation
    // policy parameters.
    InitializeFrom(mem, size);
    GOOGLE_DCHECK_POLICY_FLAGS_();
    return;
  }
  GOOGLE_DCHECK_EQ(reinterpret_cast<uintptr_t>(mem) & 7, 0u);
  Init();

  // Ignore initial block if it is too small. We include an optional
  // AllocationPolicy in this check, so that this can be allocated on the
  // first block.
  constexpr size_t kAPSize = internal::AlignUpTo8(sizeof(AllocationPolicy));
  constexpr size_t kMinimumSize = kBlockHeaderSize + kSerialArenaSize + kAPSize;
  // Make sure we have an initial block to store the AllocationPolicy.
  if (mem != nullptr && size >= kMinimumSize) {
    alloc_policy_.set_is_user_owned_initial_block(true);
  } else {
    auto tmp = AllocateMemory(&policy, 0, kMinimumSize);
    mem = tmp.ptr;
    size = tmp.size;
  }
  SerialArena* sa = SetInitialBlock(mem, size);

  // We ensured enough space so this cannot fail.
  void* p;
  if (!sa || !sa->MaybeAllocateAligned(kAPSize, &p)) {
    GOOGLE_LOG(FATAL) << "MaybeAllocateAligned cannot fail here.";
    return;
  }
  new (p) AllocationPolicy{policy};
  // Low bits store flags, so they mustn't be overwritten.
  GOOGLE_DCHECK_EQ(0, reinterpret_cast<uintptr_t>(p) & 3);
  alloc_policy_.set_policy(reinterpret_cast<AllocationPolicy*>(p));
  GOOGLE_DCHECK_POLICY_FLAGS_();

#undef GOOGLE_DCHECK_POLICY_FLAGS_
}

uint64_t ThreadSafeArena::GetNextLifeCycleId() {
  ThreadCache& tc = thread_cache();
  uint64_t id = tc.next_lifecycle_id;
  // We increment lifecycle_id's by multiples of two so we can use bit 0 as
  // a tag.
  constexpr uint64_t kDelta = 2;
  constexpr uint64_t kInc = ThreadCache::kPerThreadIds * kDelta;
  if (PROTOBUF_PREDICT_FALSE((id & (kInc - 1)) == 0)) {
    constexpr auto relaxed = std::memory_order_relaxed;
    // On platforms that don't support uint64_t atomics we can certainly not
    // afford to increment by large intervals and expect uniqueness due to
    // wrapping, hence we only add by 1.
    id = lifecycle_id_generator_.id.fetch_add(1, relaxed) * kInc;
  }
  tc.next_lifecycle_id = id + kDelta;
  return id;
}

// We assume that #threads / arena is bimodal; i.e. majority small ones are
// single threaded but some big ones are highly concurrent. To balance between
// memory overhead and minimum pointer chasing, we start with few entries and
// exponentially (4x) grow with a limit (255 entries). Note that parameters are
// picked for x64 architectures as hint and the actual size is calculated by
// Layout.
ThreadSafeArena::SerialArenaChunk* ThreadSafeArena::NewSerialArenaChunk(
    uint32_t prev_capacity, void* id, SerialArena* serial) {
  constexpr size_t kMaxBytes = 4096;  // Can hold up to 255 entries.
  constexpr size_t kGrowthFactor = 4;
  constexpr size_t kHeaderSize = SerialArenaChunk::AllocSize(0);
  constexpr size_t kEntrySize = SerialArenaChunk::AllocSize(1) - kHeaderSize;

  // On x64 arch: {4, 16, 64, 256, 256, ...} * 16.
  size_t prev_bytes = SerialArenaChunk::AllocSize(prev_capacity);
  size_t next_bytes = std::min(kMaxBytes, prev_bytes * kGrowthFactor);
  uint32_t next_capacity =
      static_cast<uint32_t>(next_bytes - kHeaderSize) / kEntrySize;
  // Growth based on bytes needs to be adjusted by AllocSize.
  next_bytes = SerialArenaChunk::AllocSize(next_capacity);
  void* mem;
  mem = ::operator new(next_bytes);
  if (serial == nullptr) {
    return new (mem) SerialArenaChunk{next_capacity};
  }

  return new (mem) SerialArenaChunk{next_capacity, id, serial};
}

// Tries to reserve an entry by atomic fetch_add. If the head chunk is already
// full (size >= capacity), acquires the mutex and adds a new head.
void ThreadSafeArena::AddSerialArena(void* id, SerialArena* serial) {
  SerialArenaChunk* head = head_.load(std::memory_order_acquire);
  GOOGLE_DCHECK_NE(head, nullptr);
  // Fast path without acquiring mutex.
  if (head->insert(id, serial)) {
    return;
  }

  // Slow path with acquiring mutex.
  absl::MutexLock lock(&mutex_);

  // Refetch and if someone else installed a new head, try allocating on that!
  SerialArenaChunk* new_head = head_.load(std::memory_order_acquire);
  if (new_head != head) {
    if (new_head->insert(id, serial)) return;
    // Update head to link to the latest one.
    head = new_head;
  }

  new_head = NewSerialArenaChunk(head->capacity(), id, serial);
  new_head->set_next(head);

  // Use "std::memory_order_release" to make sure prior stores are visible after
  // this one.
  head_.store(new_head, std::memory_order_release);
}

void ThreadSafeArena::Init() {
  const bool message_owned = IsMessageOwned();
  if (!message_owned) {
    // Message-owned arenas bypass thread cache and do not need life cycle ID.
    tag_and_id_ = GetNextLifeCycleId();
  } else {
    GOOGLE_DCHECK_EQ(tag_and_id_, kMessageOwnedArena);
  }
  auto* empty_chunk = NewSerialArenaChunk(0, nullptr, nullptr);
  head_.store(empty_chunk, std::memory_order_relaxed);
  GOOGLE_DCHECK_EQ(message_owned, IsMessageOwned());
  arena_stats_ = Sample();
}

SerialArena* ThreadSafeArena::SetInitialBlock(void* mem, size_t size) {
  SerialArena* serial = SerialArena::New({mem, size}, *this);
  AddSerialArena(&thread_cache(), serial);
  CacheSerialArena(serial);
  return serial;
}

ThreadSafeArena::~ThreadSafeArena() {
  // Have to do this in a first pass, because some of the destructors might
  // refer to memory in other blocks.
  CleanupList();

  size_t space_allocated = 0;
  auto mem = Free(&space_allocated);
  if (alloc_policy_.is_user_owned_initial_block()) {
#ifdef ADDRESS_SANITIZER
    // Unpoison the initial block, now that it's going back to the user.
    ASAN_UNPOISON_MEMORY_REGION(mem.ptr, mem.size);
#endif  // ADDRESS_SANITIZER
    space_allocated += mem.size;
  } else {
    GetDeallocator(alloc_policy_.get(), &space_allocated)(mem);
  }
}

SerialArena::Memory ThreadSafeArena::Free(size_t* space_allocated) {
  SerialArena::Memory mem = {nullptr, 0};
  auto deallocator = GetDeallocator(alloc_policy_.get(), space_allocated);
  PerSerialArena([deallocator, &mem](SerialArena* a) {
    if (mem.ptr) deallocator(mem);
    mem = a->Free(deallocator);
  });
  // Free chunks that stored SerialArena.
  SerialArenaChunk* chunk = head_.load(std::memory_order_relaxed);
  while (chunk != nullptr) {
    SerialArenaChunk* next_chunk = chunk->next_chunk();
    internal::SizedDelete(chunk,
                          SerialArenaChunk::AllocSize(chunk->capacity()));
    chunk = next_chunk;
  }
  return mem;
}

uint64_t ThreadSafeArena::Reset() {
  // Have to do this in a first pass, because some of the destructors might
  // refer to memory in other blocks.
  CleanupList();

  // Discard all blocks except the special block (if present).
  size_t space_allocated = 0;
  auto mem = Free(&space_allocated);

  AllocationPolicy* policy = alloc_policy_.get();
  if (policy) {
    auto saved_policy = *policy;
    if (alloc_policy_.is_user_owned_initial_block()) {
      space_allocated += mem.size;
    } else {
      GetDeallocator(alloc_policy_.get(), &space_allocated)(mem);
      mem.ptr = nullptr;
      mem.size = 0;
    }
    InitializeWithPolicy(mem.ptr, mem.size, saved_policy);
  } else {
    // Nullptr policy
    if (alloc_policy_.is_user_owned_initial_block()) {
      space_allocated += mem.size;
      InitializeFrom(mem.ptr, mem.size);
    } else {
      GetDeallocator(alloc_policy_.get(), &space_allocated)(mem);
      Init();
    }
  }

  return space_allocated;
}

void* ThreadSafeArena::AllocateAlignedWithCleanup(size_t n, size_t align,
                                                  void (*destructor)(void*)) {
  SerialArena* arena;
  if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
    return arena->AllocateAlignedWithCleanup(n, align, destructor);
  } else {
    return AllocateAlignedWithCleanupFallback(n, align, destructor);
  }
}

void ThreadSafeArena::AddCleanup(void* elem, void (*cleanup)(void*)) {
  SerialArena* arena;
  if (PROTOBUF_PREDICT_FALSE(!GetSerialArenaFast(&arena))) {
    arena = GetSerialArenaFallback();
  }
  arena->AddCleanup(elem, cleanup, AllocPolicy(), arena_stats_.MutableStats());
}

PROTOBUF_NOINLINE
void* ThreadSafeArena::AllocateAlignedWithCleanupFallback(
    size_t n, size_t align, void (*destructor)(void*)) {
  return GetSerialArenaFallback()->AllocateAlignedWithCleanup(n, align,
                                                              destructor);
}

template <typename Functor>
void ThreadSafeArena::PerConstSerialArena(Functor fn) const {
  const SerialArenaChunk* chunk = head_.load(std::memory_order_acquire);

  for (; chunk; chunk = chunk->next_chunk()) {
    absl::Span<const std::atomic<SerialArena*>> span = chunk->arenas();
    // Walks arenas backward to handle the first serial arena the last. This is
    // necessary to special-case the initial block.
    for (auto it = span.crbegin(); it != span.crend(); ++it) {
      const SerialArena* serial = it->load(std::memory_order_relaxed);
      // It is possible that newly added SerialArena is not updated although
      // size was. This is acceptable for SpaceAllocated and SpaceUsed.
      if (serial == nullptr) continue;
      fn(serial);
    }
  }
}

template <typename Functor>
void ThreadSafeArena::PerSerialArena(Functor fn) {
  // By omitting an Acquire barrier we help the sanitizer that any user code
  // that doesn't properly synchronize Reset() or the destructor will throw a
  // TSAN warning.
  SerialArenaChunk* chunk = head_.load(std::memory_order_relaxed);

  for (; chunk; chunk = chunk->next_chunk()) {
    absl::Span<std::atomic<SerialArena*>> span = chunk->arenas();
    // Walks arenas backward to handle the first serial arena the last. This is
    // necessary to special-case the initial block.
    for (auto it = span.rbegin(); it != span.rend(); ++it) {
      SerialArena* serial = it->load(std::memory_order_relaxed);
      GOOGLE_DCHECK_NE(serial, nullptr);
      if (serial == nullptr) continue;
      fn(serial);
    }
  }
}

uint64_t ThreadSafeArena::SpaceAllocated() const {
  uint64_t space_allocated = 0;
  PerConstSerialArena([&space_allocated](const SerialArena* serial) {
    space_allocated += serial->SpaceAllocated();
  });
  return space_allocated;
}

template <AllocationClient alloc_client>
PROTOBUF_NOINLINE void* ThreadSafeArena::AllocateAlignedFallback(size_t n) {
  return GetSerialArenaFallback()->AllocateAligned<alloc_client>(n);
}

template void* ThreadSafeArena::AllocateAlignedFallback<
    AllocationClient::kDefault>(size_t);
template void*
    ThreadSafeArena::AllocateAlignedFallback<AllocationClient::kArray>(size_t);

uint64_t ThreadSafeArena::SpaceUsed() const {
  uint64_t space_used = 0;
  PerConstSerialArena([&space_used](const SerialArena* serial) {
    space_used += serial->SpaceUsed();
  });
  return space_used - (alloc_policy_.get() ? sizeof(AllocationPolicy) : 0);
}

void ThreadSafeArena::CleanupList() {
  PerSerialArena([](SerialArena* a) { a->CleanupList(); });
}

PROTOBUF_NOINLINE
SerialArena* ThreadSafeArena::GetSerialArenaFallback() {
  void* const id = &thread_cache();
  SerialArena* serial = nullptr;

  // Search matching SerialArena.
  SerialArenaChunk* chunk = head_.load(std::memory_order_acquire);
  for (; chunk; chunk = chunk->next_chunk()) {
    absl::Span<std::atomic<void*>> ids = chunk->ids();
    for (unsigned i = 0; i < ids.size(); ++i) {
      if (ids[i].load(std::memory_order_relaxed) == id) {
        serial = chunk->arena(i).load(std::memory_order_relaxed);
        GOOGLE_DCHECK_NE(serial, nullptr);
        break;
      }
    }
  }

  if (!serial) {
    // This thread doesn't have any SerialArena, which also means it doesn't
    // have any blocks yet.  So we'll allocate its first block now.
    serial = SerialArena::New(
        AllocateMemory(alloc_policy_.get(), 0, kSerialArenaSize), *this);

    AddSerialArena(id, serial);
  }

  CacheSerialArena(serial);
  return serial;
}

}  // namespace internal

void* Arena::Allocate(size_t n) { return impl_.AllocateAligned(n); }

void* Arena::AllocateForArray(size_t n) {
  return impl_.AllocateAligned<internal::AllocationClient::kArray>(n);
}

void* Arena::AllocateAlignedWithCleanup(size_t n, size_t align,
                                        void (*destructor)(void*)) {
  return impl_.AllocateAlignedWithCleanup(n, align, destructor);
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
