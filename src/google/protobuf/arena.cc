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
#include <cstddef>
#include <cstdint>
#include <limits>
#include <typeinfo>

#include <google/protobuf/arena_impl.h>
#include <google/protobuf/arenaz_sampler.h>
#include <google/protobuf/port.h>

#include <google/protobuf/stubs/mutex.h>
#ifdef ADDRESS_SANITIZER
#include <sanitizer/asan_interface.h>
#endif  // ADDRESS_SANITIZER

// Must be included last.
#include <google/protobuf/port_def.inc>

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

SerialArena::SerialArena(Block* b, void* owner, ThreadSafeArenaStats* stats)
    : space_allocated_(b->size()) {
  owner_ = owner;
  set_head(b);
  set_ptr(b->Pointer(kBlockHeaderSize + ThreadSafeArena::kSerialArenaSize));
  limit_ = b->Pointer(b->size() & static_cast<size_t>(-8));
  arena_stats_ = stats;
}

SerialArena* SerialArena::New(Memory mem, void* owner,
                              ThreadSafeArenaStats* stats) {
  GOOGLE_DCHECK_LE(kBlockHeaderSize + ThreadSafeArena::kSerialArenaSize, mem.size);
  ThreadSafeArenaStats::RecordAllocateStats(
      stats, /*requested=*/mem.size, /*allocated=*/mem.size, /*wasted=*/0);
  auto b = new (mem.ptr) Block{nullptr, mem.size};
  return new (b->Pointer(kBlockHeaderSize)) SerialArena(b, owner, stats);
}

template <typename Deallocator>
SerialArena::Memory SerialArena::Free(Deallocator deallocator) {
  Block* b = head();
  Memory mem = {b, b->size()};
  while (b->next) {
    b = b->next;  // We must first advance before deleting this block
    deallocator(mem);
    mem = {b, b->size()};
  }
  return mem;
}

PROTOBUF_NOINLINE
void* SerialArena::AllocateAlignedFallback(size_t n,
                                           const AllocationPolicy* policy) {
  AllocateNewBlock(n, policy);
  return AllocateFromExisting(n);
}

PROTOBUF_NOINLINE
void* SerialArena::AllocateAlignedWithCleanupFallback(
    size_t n, size_t align, void (*destructor)(void*),
    const AllocationPolicy* policy) {
  size_t required = AlignUpTo(n, align) + cleanup::Size(destructor);
  AllocateNewBlock(required, policy);
  return AllocateFromExistingWithCleanupFallback(n, align, destructor);
}

PROTOBUF_NOINLINE
void SerialArena::AddCleanupFallback(void* elem, void (*destructor)(void*),
                                     const AllocationPolicy* policy) {
  size_t required = cleanup::Size(destructor);
  AllocateNewBlock(required, policy);
  AddCleanupFromExisting(elem, destructor);
}

void SerialArena::AllocateNewBlock(size_t n, const AllocationPolicy* policy) {
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

  auto mem = AllocateMemory(policy, head()->size(), n);
  // We don't want to emit an expensive RMW instruction that requires
  // exclusive access to a cacheline. Hence we write it in terms of a
  // regular add.
  space_allocated_.store(
      space_allocated_.load(std::memory_order_relaxed) + mem.size,
      std::memory_order_relaxed);
  ThreadSafeArenaStats::RecordAllocateStats(arena_stats_, /*used=*/used,
                                            /*allocated=*/mem.size, wasted);
  set_head(new (mem.ptr) Block{head(), mem.size});
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
          ptr() - const_cast<Block*>(head())->Pointer(kBlockHeaderSize)),
      current_block_size);
  space_used += space_used_.load(std::memory_order_relaxed);
  // Remove the overhead of the SerialArena itself.
  space_used -= ThreadSafeArena::kSerialArenaSize;
  return space_used;
}

void SerialArena::CleanupList() {
  Block* b = head();
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

  // The value for alloc_policy_ stores whether or not allocations should be
  // recorded.
  alloc_policy_.set_should_record_allocs(
      policy.metrics_collector != nullptr &&
      policy.metrics_collector->RecordAllocs());
  // Make sure we have an initial block to store the AllocationPolicy.
  if (mem != nullptr && size >= kMinimumSize) {
    alloc_policy_.set_is_user_owned_initial_block(true);
  } else {
    auto tmp = AllocateMemory(&policy, 0, kMinimumSize);
    mem = tmp.ptr;
    size = tmp.size;
  }
  SetInitialBlock(mem, size);

  auto sa = threads_.load(std::memory_order_relaxed);
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

void ThreadSafeArena::Init() {
  const bool message_owned = IsMessageOwned();
  if (!message_owned) {
    // Message-owned arenas bypass thread cache and do not need life cycle ID.
    tag_and_id_ = GetNextLifeCycleId();
  } else {
    GOOGLE_DCHECK_EQ(tag_and_id_, kMessageOwnedArena);
  }
  threads_.store(nullptr, std::memory_order_relaxed);
  GOOGLE_DCHECK_EQ(message_owned, IsMessageOwned());
  arena_stats_ = Sample();
}

void ThreadSafeArena::SetInitialBlock(void* mem, size_t size) {
  SerialArena* serial = SerialArena::New({mem, size}, &thread_cache(),
                                         arena_stats_.MutableStats());
  serial->set_next(nullptr);
  threads_.store(serial, std::memory_order_relaxed);
  CacheSerialArena(serial);
}

ThreadSafeArena::~ThreadSafeArena() {
  // Have to do this in a first pass, because some of the destructors might
  // refer to memory in other blocks.
  CleanupList();

  size_t space_allocated = 0;
  auto mem = Free(&space_allocated);

  // Policy is about to get deleted.
  auto* p = alloc_policy_.get();
  ArenaMetricsCollector* collector = p ? p->metrics_collector : nullptr;

  if (alloc_policy_.is_user_owned_initial_block()) {
#ifdef ADDRESS_SANITIZER
    // Unpoison the initial block, now that it's going back to the user.
    ASAN_UNPOISON_MEMORY_REGION(mem.ptr, mem.size);
#endif  // ADDRESS_SANITIZER
    space_allocated += mem.size;
  } else {
    GetDeallocator(alloc_policy_.get(), &space_allocated)(mem);
  }

  if (collector) collector->OnDestroy(space_allocated);
}

SerialArena::Memory ThreadSafeArena::Free(size_t* space_allocated) {
  SerialArena::Memory mem = {nullptr, 0};
  auto deallocator = GetDeallocator(alloc_policy_.get(), space_allocated);
  PerSerialArena([deallocator, &mem](SerialArena* a) {
    if (mem.ptr) deallocator(mem);
    mem = a->Free(deallocator);
  });
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
    ArenaMetricsCollector* collector = saved_policy.metrics_collector;
    if (collector) collector->OnReset(space_allocated);
    InitializeWithPolicy(mem.ptr, mem.size, saved_policy);
  } else {
    GOOGLE_DCHECK(!alloc_policy_.should_record_allocs());
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
                                                  void (*destructor)(void*),
                                                  const std::type_info* type) {
  SerialArena* arena;
  if (PROTOBUF_PREDICT_TRUE(!alloc_policy_.should_record_allocs() &&
                            GetSerialArenaFast(&arena))) {
    return arena->AllocateAlignedWithCleanup(n, align, destructor,
                                             alloc_policy_.get());
  } else {
    return AllocateAlignedWithCleanupFallback(n, align, destructor, type);
  }
}

void ThreadSafeArena::AddCleanup(void* elem, void (*cleanup)(void*)) {
  SerialArena* arena;
  if (PROTOBUF_PREDICT_FALSE(!GetSerialArenaFast(&arena))) {
    arena = GetSerialArenaFallback(&thread_cache());
  }
  arena->AddCleanup(elem, cleanup, AllocPolicy());
}

PROTOBUF_NOINLINE
void* ThreadSafeArena::AllocateAlignedFallback(size_t n,
                                               const std::type_info* type) {
  if (alloc_policy_.should_record_allocs()) {
    alloc_policy_.RecordAlloc(type, n);
    SerialArena* arena;
    if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
      return arena->AllocateAligned(n, alloc_policy_.get());
    }
  }
  return GetSerialArenaFallback(&thread_cache())
      ->AllocateAligned(n, alloc_policy_.get());
}

PROTOBUF_NOINLINE
void* ThreadSafeArena::AllocateAlignedWithCleanupFallback(
    size_t n, size_t align, void (*destructor)(void*),
    const std::type_info* type) {
  if (alloc_policy_.should_record_allocs()) {
    alloc_policy_.RecordAlloc(type, internal::AlignUpTo(n, align));
    SerialArena* arena;
    if (GetSerialArenaFast(&arena)) {
      return arena->AllocateAlignedWithCleanup(n, align, destructor,
                                               alloc_policy_.get());
    }
  }
  return GetSerialArenaFallback(&thread_cache())
      ->AllocateAlignedWithCleanup(n, align, destructor, alloc_policy_.get());
}

uint64_t ThreadSafeArena::SpaceAllocated() const {
  SerialArena* serial = threads_.load(std::memory_order_acquire);
  uint64_t res = 0;
  for (; serial; serial = serial->next()) {
    res += serial->SpaceAllocated();
  }
  return res;
}

uint64_t ThreadSafeArena::SpaceUsed() const {
  SerialArena* serial = threads_.load(std::memory_order_acquire);
  uint64_t space_used = 0;
  for (; serial; serial = serial->next()) {
    space_used += serial->SpaceUsed();
  }
  return space_used - (alloc_policy_.get() ? sizeof(AllocationPolicy) : 0);
}

void ThreadSafeArena::CleanupList() {
  PerSerialArena([](SerialArena* a) { a->CleanupList(); });
}

PROTOBUF_NOINLINE
SerialArena* ThreadSafeArena::GetSerialArenaFallback(void* me) {
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
    serial = SerialArena::New(
        AllocateMemory(alloc_policy_.get(), 0, kSerialArenaSize), me,
        arena_stats_.MutableStats());

    SerialArena* head = threads_.load(std::memory_order_relaxed);
    do {
      serial->set_next(head);
    } while (!threads_.compare_exchange_weak(
        head, serial, std::memory_order_release, std::memory_order_relaxed));
  }

  CacheSerialArena(serial);
  return serial;
}

}  // namespace internal

PROTOBUF_FUNC_ALIGN(32)
void* Arena::AllocateAlignedNoHook(size_t n) {
  return impl_.AllocateAligned(n, nullptr);
}

PROTOBUF_FUNC_ALIGN(32)
void* Arena::AllocateAlignedWithHook(size_t n, const std::type_info* type) {
  return impl_.AllocateAligned(n, type);
}

PROTOBUF_FUNC_ALIGN(32)
void* Arena::AllocateAlignedWithHookForArray(size_t n,
                                             const std::type_info* type) {
  return impl_.AllocateAligned<internal::AllocationClient::kArray>(n, type);
}

PROTOBUF_FUNC_ALIGN(32)
void* Arena::AllocateAlignedWithCleanup(size_t n, size_t align,
                                        void (*destructor)(void*),
                                        const std::type_info* type) {
  return impl_.AllocateAlignedWithCleanup(n, align, destructor, type);
}

}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>
