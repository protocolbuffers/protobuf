// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/arena.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/base/dynamic_annotations.h"
#include "absl/base/optimization.h"
#include "absl/base/prefetch.h"
#include "absl/container/internal/layout.h"
#include "absl/flags/flag.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/synchronization/mutex.h"
#include "absl/types/span.h"
#include "google/protobuf/arena_allocation_policy.h"
#include "google/protobuf/arena_cleanup.h"
#include "google/protobuf/arenaz_sampler.h"
#include "google/protobuf/port.h"
#include "google/protobuf/serial_arena.h"
#include "google/protobuf/string_block.h"
#include "google/protobuf/thread_safe_arena.h"



alignas(kCacheAlignment) ABSL_CONST_INIT
    std::atomic<ThreadSafeArena::LifecycleId> ThreadSafeArena::lifecycle_id_{0};
#if defined(PROTOBUF_NO_THREADLOCAL)
ThreadSafeArena::ThreadCache& ThreadSafeArena::thread_cache() {
  static internal::ThreadLocalStorage<ThreadCache>* thread_cache_ =
      new internal::ThreadLocalStorage<ThreadCache>();
  return *thread_cache_->Get();
}
#elif defined(PROTOBUF_USE_DLLS) && defined(_WIN32)
ThreadSafeArena::ThreadCache& ThreadSafeArena::thread_cache() {
  static PROTOBUF_THREAD_LOCAL ThreadCache thread_cache;
  return thread_cache;
}
#else
PROTOBUF_CONSTINIT PROTOBUF_THREAD_LOCAL
    ThreadSafeArena::ThreadCache ThreadSafeArena::thread_cache_;
#endif

ThreadSafeArena::ThreadSafeArena() : first_arena_(*this) { Init(); }

ThreadSafeArena::ThreadSafeArena(char* mem, size_t size)
    : first_arena_(FirstSerialArena{}, FirstBlock(mem, size), *this) {
  Init();
}

ThreadSafeArena::ThreadSafeArena(void* mem, size_t size,
                                 const AllocationPolicy& policy)
    : first_arena_(FirstSerialArena{}, FirstBlock(mem, size, policy), *this) {
  InitializeWithPolicy(policy);
}

ArenaBlock* ThreadSafeArena::FirstBlock(void* buf, size_t size) {
  ABSL_DCHECK_EQ(reinterpret_cast<uintptr_t>(buf) & 7, 0u);
  if (buf == nullptr || size <= kBlockHeaderSize) {
    return SentryArenaBlock();
  }
  // Record user-owned block.

  if constexpr (internal::PerformDebugChecks()) {
    // Touch block to verify it is addressable.
    if (size > 0) {
      static_cast<char*>(buf)[0] = 0;
      static_cast<char*>(buf)[size - 1] = 0;
    }
  }

  ABSL_ANNOTATE_MEMORY_IS_UNINITIALIZED(buf, size);
  alloc_policy_.set_is_user_owned_initial_block(true);
  return new (buf) ArenaBlock{nullptr, size};
}

ArenaBlock* ThreadSafeArena::FirstBlock(void* buf, size_t size,
                                        const AllocationPolicy& policy) {
  if (policy.IsDefault()) return FirstBlock(buf, size);

  ABSL_DCHECK_EQ(reinterpret_cast<uintptr_t>(buf) & 7, 0u);

  SizedPtr mem;
  if (buf == nullptr || size < kBlockHeaderSize + kAllocPolicySize) {
    mem = AllocateBlock(&policy, 0, kAllocPolicySize, AllocationHint::kHot);
  } else {
    mem = {buf, size};
    // Record user-owned block.
    if constexpr (internal::PerformDebugChecks()) {
      // Touch block to verify it is addressable.
      if (size > 0) {
        static_cast<char*>(buf)[0] = 0;
        static_cast<char*>(buf)[size - 1] = 0;
      }
    }
    ABSL_ANNOTATE_MEMORY_IS_UNINITIALIZED(buf, size);
    alloc_policy_.set_is_user_owned_initial_block(true);
  }

  return new (mem.p) ArenaBlock{nullptr, mem.n};
}

void ThreadSafeArena::InitializeWithPolicy(const AllocationPolicy& policy) {
  Init();

  if (policy.IsDefault()) return;

#ifndef NDEBUG
  const uint64_t old_alloc_policy = alloc_policy_.get_raw();
  // If there was a policy (e.g., in Reset()), make sure flags were preserved.
#define ABSL_DCHECK_POLICY_FLAGS_() \
  if (old_alloc_policy > 3)         \
  ABSL_CHECK_EQ(old_alloc_policy & 3, alloc_policy_.get_raw() & 3)
#else
#define ABSL_DCHECK_POLICY_FLAGS_()
#endif  // NDEBUG

  // We ensured enough space so this cannot fail.
  void* p;
  if (!first_arena_.MaybeAllocateAligned(kAllocPolicySize, &p)) {
    ABSL_LOG(FATAL) << "MaybeAllocateAligned cannot fail here.";
    return;
  }
  new (p) AllocationPolicy{policy};
  // Low bits store flags, so they mustn't be overwritten.
  ABSL_DCHECK_EQ(0u, reinterpret_cast<uintptr_t>(p) & 3);
  alloc_policy_.set_policy(reinterpret_cast<AllocationPolicy*>(p));
  ABSL_DCHECK_POLICY_FLAGS_();

#undef ABSL_DCHECK_POLICY_FLAGS_
}

uint64_t ThreadSafeArena::GetNextLifeCycleId() {
  ThreadCache& tc = thread_cache();
  uint64_t id = tc.next_lifecycle_id;
  constexpr uint64_t kInc = ThreadCache::kPerThreadIds;
  if (ABSL_PREDICT_FALSE((id & (kInc - 1)) == 0)) {
    // On platforms that don't support uint64_t atomics we can certainly not
    // afford to increment by large intervals and expect uniqueness due to
    // wrapping, hence we only add by 1.
    id = lifecycle_id_.fetch_add(1, std::memory_order_relaxed) * kInc;
  }
  tc.next_lifecycle_id = id + 1;
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

  // If we allocate bigger memory than requested, we should expand
  // size to use that extra space, and add extra entries permitted
  // by the extra space.
  SizedPtr mem = AllocateAtLeast(next_bytes);
  next_capacity = static_cast<uint32_t>(mem.n - kHeaderSize) / kEntrySize;
  ABSL_DCHECK_LE(SerialArenaChunk::AllocSize(next_capacity), mem.n);
  return new (mem.p) SerialArenaChunk{next_capacity, id, serial};
}

// Tries to reserve an entry by atomic fetch_add. If the head chunk is already
// full (size >= capacity), acquires the mutex and adds a new head.
void ThreadSafeArena::AddSerialArena(void* id, SerialArena* serial) {
  SerialArenaChunk* head = head_.load(std::memory_order_acquire);
  // Fast path without acquiring mutex.
  if (!head->IsSentry() && head->insert(id, serial)) {
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

void ThreadSafeArena::UnpoisonAllArenaBlocks() const {
  VisitSerialArena([](const SerialArena* serial) {
    for (const auto* b = serial->head(); b != nullptr && !b->IsSentry();
         b = b->next) {
      internal::UnpoisonMemoryRegion(b, b->size);
    }
  });
}

void ThreadSafeArena::Init() {
  tag_and_id_ = GetNextLifeCycleId();
  arena_stats_ = Sample();
  head_.store(SentrySerialArenaChunk(), std::memory_order_relaxed);
  first_owner_ = &thread_cache();

  // Record allocation for the first block that was either user-provided or
  // newly allocated.
  ThreadSafeArenaStats::RecordAllocateStats(
      arena_stats_.MutableStats(),
      /*used=*/0,
      /*allocated=*/first_arena_.SpaceAllocated(),
      /*wasted=*/0);

  CacheSerialArena(&first_arena_);
}

ThreadSafeArena::~ThreadSafeArena() {
  // Have to do this in a first pass, because some of the destructors might
  // refer to memory in other blocks.
  CleanupList();

  auto mem = Free();
  if (alloc_policy_.is_user_owned_initial_block()) {
    // Unpoison the initial block, now that it's going back to the user.
    internal::UnpoisonMemoryRegion(mem.p, mem.n);
  } else if (mem.n > 0) {
    GetDeallocator(alloc_policy_.get())(mem);
  }
}

SizedPtr ThreadSafeArena::Free() {
  auto deallocator = GetDeallocator(alloc_policy_.get());

  WalkSerialArenaChunk([&](SerialArenaChunk* chunk) {
    absl::Span<std::atomic<SerialArena*>> span = chunk->arenas();
    // Walks arenas backward to handle the first serial arena the last. Freeing
    // in reverse-order to the order in which objects were created may not be
    // necessary to Free and we should revisit this. (b/247560530)
    for (auto it = span.rbegin(); it != span.rend(); ++it) {
      SerialArena* serial = it->load(std::memory_order_relaxed);
      ABSL_DCHECK_NE(serial, nullptr);
      // Always frees the first block of "serial" as it cannot be user-provided.
      SizedPtr mem = serial->Free(deallocator);
      ABSL_DCHECK_NE(mem.p, nullptr);
      deallocator(mem);
    }

    // Delete the chunk as we're done with it.
    internal::SizedDelete(chunk,
                          SerialArenaChunk::AllocSize(chunk->capacity()));
  });

  // The first block of the first arena is special and let the caller handle it.
  return first_arena_.Free(deallocator);
}

uint64_t ThreadSafeArena::Reset() {
  const size_t space_allocated = SpaceAllocated();

  // Have to do this in a first pass, because some of the destructors might
  // refer to memory in other blocks.
  CleanupList();
  // Reset the first arena's cleanup list.
  first_arena_.cleanup_list_ = cleanup::ChunkList();

  // Discard all blocks except the first one. Whether it is user-provided or
  // allocated, always reuse the first block for the first arena.
  auto mem = Free();

  // Reset the first arena with the first block. This avoids redundant
  // free / allocation and re-allocating for AllocationPolicy. Adjust offset if
  // we need to preserve alloc_policy_.
  if (alloc_policy_.is_user_owned_initial_block() ||
      alloc_policy_.get() != nullptr) {
    size_t offset = alloc_policy_.get() == nullptr
                        ? kBlockHeaderSize
                        : kBlockHeaderSize + kAllocPolicySize;
    ABSL_ANNOTATE_MEMORY_IS_UNINITIALIZED(static_cast<char*>(mem.p) + offset,
                                          mem.n - offset);
    first_arena_.Init(new (mem.p) ArenaBlock{nullptr, mem.n}, offset);
  } else {
    first_arena_.Init(SentryArenaBlock(), 0);
  }

  // Since the first block and potential alloc_policy on the first block is
  // preserved, this can be initialized by Init().
  Init();

  return space_allocated;
}

void* ThreadSafeArena::AllocateAlignedWithCleanup(size_t n, size_t align,
                                                  void (*destructor)(void*)) {
  SerialArena* arena;
  if (ABSL_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
    return arena->AllocateAlignedWithCleanup(n, align, destructor);
  } else {
    return AllocateAlignedWithCleanupFallback(n, align, destructor);
  }
}

void ThreadSafeArena::AddCleanup(void* elem, void (*cleanup)(void*)) {
  GetSerialArena()->AddCleanup(elem, cleanup);
}

SerialArena* ThreadSafeArena::GetSerialArenaSlow() {
  return GetSerialArenaFallback(0);
}

PROTOBUF_NOINLINE
void* ThreadSafeArena::AllocateAlignedWithCleanupFallback(
    size_t n, size_t align, void (*destructor)(void*)) {
  return GetSerialArenaFallback(n + kMaxCleanupNodeSize)
      ->AllocateAlignedWithCleanup(n, align, destructor);
}

template <typename Callback>
void ThreadSafeArena::WalkConstSerialArenaChunk(Callback fn) const {
  const SerialArenaChunk* chunk = head_.load(std::memory_order_acquire);

  for (; !chunk->IsSentry(); chunk = chunk->next_chunk()) {
    // Prefetch the next chunk.
    absl::PrefetchToLocalCache(chunk->next_chunk());
    fn(chunk);
  }
}

template <typename Callback>
void ThreadSafeArena::WalkSerialArenaChunk(Callback fn) {
  // By omitting an Acquire barrier we help the sanitizer that any user code
  // that doesn't properly synchronize Reset() or the destructor will throw a
  // TSAN warning.
  SerialArenaChunk* chunk = head_.load(std::memory_order_relaxed);

  while (!chunk->IsSentry()) {
    // Cache next chunk in case this chunk is destroyed.
    SerialArenaChunk* next_chunk = chunk->next_chunk();
    // Prefetch the next chunk.
    absl::PrefetchToLocalCache(next_chunk);
    fn(chunk);
    chunk = next_chunk;
  }
}

template <typename Callback>
void ThreadSafeArena::VisitSerialArena(Callback fn) const {
  // In most cases, arenas are single-threaded and "first_arena_" should be
  // sufficient.
  fn(&first_arena_);

  WalkConstSerialArenaChunk([&fn](const SerialArenaChunk* chunk) {
    for (const auto& each : chunk->arenas()) {
      const SerialArena* serial = each.load(std::memory_order_acquire);
      // It is possible that newly added SerialArena is not updated although
      // size was. This is acceptable for SpaceAllocated and SpaceUsed.
      if (serial == nullptr) continue;
      fn(serial);
    }
  });
}

uint64_t ThreadSafeArena::SpaceAllocated() const {
  uint64_t space_allocated = 0;
  VisitSerialArena([&space_allocated](const SerialArena* serial) {
    space_allocated += serial->SpaceAllocated();
  });
  return space_allocated;
}

uint64_t ThreadSafeArena::SpaceUsed() const {
  // `first_arena_` doesn't have kSerialArenaSize overhead, so adjust it here.
  uint64_t space_used = kSerialArenaSize;
  VisitSerialArena([&space_used](const SerialArena* serial) {
    // SerialArena on chunks directly allocated from the block and needs to be
    // subtracted from SpaceUsed.
    space_used += serial->SpaceUsed() - kSerialArenaSize;
  });
  return space_used - (alloc_policy_.get() ? sizeof(AllocationPolicy) : 0);
}

template <AllocationClient alloc_client>
PROTOBUF_NOINLINE void* ThreadSafeArena::AllocateAlignedFallback(size_t n) {
  return GetSerialArenaFallback(n)->AllocateAligned<alloc_client>(n);
}

template void*
ThreadSafeArena::AllocateAlignedFallback<AllocationClient::kDefault>(size_t);
template void*
ThreadSafeArena::AllocateAlignedFallback<AllocationClient::kArray>(size_t);

void* ThreadSafeArena::AllocateAlignedFallback(size_t n, AllocationHint hint) {
  return GetSerialArenaFallback(n)->AllocateAligned(n, hint);
}

void ThreadSafeArena::CleanupList() {
  if constexpr (HasMemoryPoisoning()) {
    UnpoisonAllArenaBlocks();
  }

  WalkSerialArenaChunk([](SerialArenaChunk* chunk) {
    absl::Span<std::atomic<SerialArena*>> span = chunk->arenas();
    // Walks arenas backward to handle the first serial arena the last.
    // Destroying in reverse-order to the construction is often assumed by users
    // and required not to break inter-object dependencies. (b/247560530)
    for (auto it = span.rbegin(); it != span.rend(); ++it) {
      SerialArena* serial = it->load(std::memory_order_relaxed);
      ABSL_DCHECK_NE(serial, nullptr);
      serial->CleanupList();
    }
  });
  // First arena must be cleaned up last. (b/247560530)
  first_arena_.CleanupList();
}

PROTOBUF_NOINLINE
SerialArena* ThreadSafeArena::GetSerialArenaFallback(size_t n) {
  void* const id = &thread_cache();
  if (id == first_owner_) {
    CacheSerialArena(&first_arena_);
    return &first_arena_;
  }

  // Search matching SerialArena.
  SerialArena* serial = nullptr;
  WalkConstSerialArenaChunk([&serial, id](const SerialArenaChunk* chunk) {
    absl::Span<const std::atomic<void*>> ids = chunk->ids();
    for (uint32_t i = 0; i < ids.size(); ++i) {
      if (ids[i].load(std::memory_order_relaxed) == id) {
        serial = chunk->arena(i).load(std::memory_order_relaxed);
        ABSL_DCHECK_NE(serial, nullptr);
        break;
      }
    }
  });

  if (!serial) {
    // This thread doesn't have any SerialArena, which also means it doesn't
    // have any blocks yet.  So we'll allocate its first block now. It must be
    // big enough to host SerialArena and the pending request.
    serial = SerialArena::New(
        AllocateBlock(alloc_policy_.get(), 0, n + kSerialArenaSize,
                      AllocationHint::kHot),
        *this);

    AddSerialArena(id, serial);
  }

  CacheSerialArena(serial);
  return serial;
}

}  // namespace internal

void* PROTOBUF_NONNULL Arena::Allocate(size_t n) {
  return impl_.AllocateAligned(n);
}

void* PROTOBUF_NONNULL Arena::AllocateForArray(size_t n) {
  return impl_.AllocateAligned<internal::AllocationClient::kArray>(n);
}

void* PROTOBUF_NONNULL Arena::AllocateAlignedWithCleanup(
    size_t n, size_t align,
    void (*PROTOBUF_NONNULL destructor)(void* PROTOBUF_NONNULL)) {
  return impl_.AllocateAlignedWithCleanup(n, align, destructor);
}

void* PROTOBUF_NONNULL
Arena::AllocateAligned(size_t size, size_t align, internal::AllocationHint hint,
                       internal::InternalVisibility visibility) {
  (void)visibility;
  if (align <= internal::ArenaAlignDefault::align) {
    return impl_.AllocateAligned(internal::ArenaAlignDefault::Ceil(size), hint);
  } else {
    auto align_as = internal::ArenaAlignAs(align);
    return align_as.Ceil(impl_.AllocateAligned(align_as.Padded(size), hint));
  }
}

namespace internal {

void* SerialArena::AllocateAligned(size_t n, AllocationHint hint) {
#ifndef PROTO2_OPENSOURCE
  if (absl::GetFlag(FLAGS_protobuf_enable_hot_cold_split)) {
    if (hint == AllocationHint::kCold) {
      return AllocateAlignedCold(n);
    }
  }
#endif
  return AllocateAligned(n);
}

void* SerialArena::AllocateAlignedFallback(size_t n, AllocationHint hint) {
#ifndef PROTO2_OPENSOURCE
  if (absl::GetFlag(FLAGS_protobuf_enable_hot_cold_split)) {
    if (hint == AllocationHint::kCold) {
      return AllocateAlignedFallbackCold(n);
    }
  }
#endif
  return AllocateAlignedFallback(n);
}

void* SerialArena::AllocateAlignedCold(size_t n) {
  ABSL_DCHECK(internal::ArenaAlignDefault::IsAligned(n));
  void* ret = nullptr;
  if (ABSL_PREDICT_TRUE(MaybeAllocateAlignedCold(n, &ret))) {
    return ret;
  }
  return AllocateAlignedFallbackCold(n);
}

bool SerialArena::MaybeAllocateAlignedCold(size_t n, void** out) {
  char* ret = cold_ptr_.load(std::memory_order_relaxed);
  if (ABSL_PREDICT_FALSE(cold_limit_ - ret < static_cast<ptrdiff_t>(n))) {
    return false;
  }
  internal::UnpoisonMemoryRegion(ret, n);
  *out = ret;
  char* next = ret + n;
  cold_ptr_.store(next, std::memory_order_relaxed);
  return true;
}

void* SerialArena::AllocateAlignedFallbackCold(size_t n) {
  AllocateNewBlockCold(n);
  void* ret = nullptr;
  bool res = MaybeAllocateAlignedCold(n, &ret);
  ABSL_DCHECK(res);
  return ret;
}

void SerialArena::AllocateNewBlockCold(size_t n) {
  size_t used = 0;
  size_t wasted = 0;
  ArenaBlock* old_head = cold_head_.load(std::memory_order_relaxed);
  if (old_head && !old_head->IsSentry()) {
    used = static_cast<size_t>(cold_ptr_.load(std::memory_order_relaxed) -
                               old_head->Pointer(kBlockHeaderSize));
    wasted = old_head->size - used - kBlockHeaderSize;
    AddSpaceUsed(used);
  }

  auto mem = AllocateBlock(parent_.AllocPolicy(), old_head ? old_head->size : 0,
                           n, AllocationHint::kCold);
  AddSpaceAllocated(mem.n);
  // TODO: stats for cold blocks?
  auto* new_head = new (mem.p) ArenaBlock{old_head, mem.n};
  cold_head_.store(new_head, std::memory_order_release);
  cold_ptr_.store(new_head->Pointer(kBlockHeaderSize),
                  std::memory_order_relaxed);
  cold_limit_ = new_head->Limit();
  cold_prefetch_ptr_ = cold_ptr_.load(std::memory_order_relaxed);

  internal::PoisonMemoryRegion(
      cold_ptr_.load(std::memory_order_relaxed),
      cold_limit_ - cold_ptr_.load(std::memory_order_relaxed));
}

bool SerialArena::MaybeAllocateAligned(size_t n, void** out,
                                       AllocationHint hint) {
#ifndef PROTO2_OPENSOURCE
  if (absl::GetFlag(FLAGS_protobuf_enable_hot_cold_split)) {
    if (hint == AllocationHint::kCold) {
      return MaybeAllocateAlignedCold(n, out);
    }
  }
#endif
  return MaybeAllocateAligned(n, out);
}

}  // namespace internal

std::vector<void*> Arena::PeekCleanupListForTesting() {
  return impl_.PeekCleanupListForTesting();
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
