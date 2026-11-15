// Protocol Buffers - Google's data interchange format
// Copyright 2022 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//
// This file defines the internal class SerialArena

#ifndef GOOGLE_PROTOBUF_SERIAL_ARENA_H__
#define GOOGLE_PROTOBUF_SERIAL_ARENA_H__

#include <algorithm>
#include <atomic>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <string>
#include <type_traits>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/base/prefetch.h"
#include "absl/log/absl_check.h"
#include "absl/numeric/bits.h"
#include "google/protobuf/arena_alloc_result.h"
#include "google/protobuf/arena_allocation_policy.h"
#include "google/protobuf/arena_cleanup.h"
#include "google/protobuf/arena_seed.h"
#include "google/protobuf/memory_align.h"
#include "google/protobuf/port.h"
#include "google/protobuf/string_block.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

class Arena;

namespace internal {

// Arena blocks are variable length malloc-ed objects.  The following structure
// describes the common header for all blocks.
struct alignas(ArenaAlignDefault::kAlign) ArenaBlock {
  // For the sentry block with zero-size where ptr_/limit_ both point to `this`.
  constexpr ArenaBlock()
      : next(nullptr),
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
        is_arena_seeded(false),
#endif
        size(0) {
  }

  ArenaBlock(ArenaBlock* next, size_t size, bool is_arena_seeded)
      : next(next),
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
        is_arena_seeded(is_arena_seeded),
#endif
        size(size) {
    ABSL_DCHECK_GT(size, sizeof(ArenaBlock));
    ABSL_DCHECK(ArenaSeedPolicy::kEnableArenaSeeding || !is_arena_seeded);
  }

  size_t allocation_size() const {
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    return size + is_arena_seeded * ArenaSeedPolicy::kArenaPtrSize;
#else
    return size;
#endif
  }

  SizedPtr MemPtr() {
    char* ptr = reinterpret_cast<char*>(this);
    size_t mem_size = size;
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    // If the block has an arena seed, that arena seed is placed at the
    // beginning of the block (so that it lies on an aligned address), before
    // this `ArenaBlock` header.
    if (is_arena_seeded) {
      ptr -= ArenaSeedPolicy::kArenaPtrSize;
      ABSL_DCHECK(ArenaSeedPolicy::kMinArenaSeedAlignment.IsAligned(ptr));
      mem_size += ArenaSeedPolicy::kArenaPtrSize;
    }
#endif
    return {ptr, mem_size};
  }

  char* Pointer(size_t n) {
    ABSL_DCHECK_LE(n, size);
    return reinterpret_cast<char*>(this) + n;
  }
  char* Limit() { return Pointer(ArenaAlignDefault{}.Floor(size)); }

  bool IsSentry() const { return size == 0; }

  ArenaBlock* const next;
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  const bool is_arena_seeded : 1;
  const size_t size : sizeof(size_t) * CHAR_BIT - 1;
#else
  const size_t size;
#endif
  // data follows
};

enum class AllocationClient { kDefault, kArray };

class ThreadSafeArena;

// Tag type used to invoke the constructor of the first SerialArena.
struct FirstSerialArena {
  explicit FirstSerialArena() = default;
};

// A simple arena allocator. Calls to allocate functions must be properly
// serialized by the caller, hence this class cannot be used as a general
// purpose allocator in a multi-threaded program. It serves as a building block
// for ThreadSafeArena, which provides a thread-safe arena allocator.
//
// This class manages
// 1) Arena bump allocation + owning memory blocks.
// 2) Maintaining a cleanup list.
// It delegates the actual memory allocation back to ThreadSafeArena, which
// contains the information on block growth policy and backing memory allocation
// used.
class PROTOBUF_EXPORT SerialArena {
 public:
  static constexpr size_t kBlockHeaderSize =
      ArenaAlignDefault{}.Ceil(sizeof(ArenaBlock));
  static constexpr size_t kMinimumReturnedArraySize = 16;

  template <bool needs_arena_ptr>
  using AllocResult =
      std::conditional_t<needs_arena_ptr, MessageAllocationResult,
                         AllocationResult>;

  void CleanupList() { cleanup_list_.Cleanup(*this); }
  uint64_t SpaceAllocated() const {
    return space_allocated_.load(std::memory_order_relaxed);
  }
  uint64_t SpaceUsed() const;

  // See comments on `cached_blocks_` member for details.
  PROTOBUF_ALWAYS_INLINE AllocationResult
  TryAllocateFromCachedBlock(size_t size) {
    if (ABSL_PREDICT_FALSE(size < kMinimumReturnedArraySize)) return {nullptr};
    // We round up to the next larger block in case the memory doesn't match
    // the pattern we are looking for.
    const size_t index = absl::bit_width(size - 1) -
                         absl::bit_width(kMinimumReturnedArraySize - 1);

    if (ABSL_PREDICT_FALSE(index >= cached_block_length_)) return {nullptr};
    auto& cached_head = cached_blocks_[index];
    if (cached_head == nullptr) return {nullptr};

    void* ret = cached_head;
    internal::UnpoisonMemoryRegion(ret, size);
    cached_head = cached_head->next;
    return {ret};
  }

  // In kArray mode we look through cached blocks.
  // We do not do this by default because most non-array allocations will not
  // have the right size and will fail to find an appropriate cached block.
  //
  // TODO: Evaluate if we should use cached blocks for message types of
  // the right size. We can statically know if the allocation size can benefit
  // from it.
  template <AllocationClient alloc_client = AllocationClient::kDefault>
  AllocationResult AllocateDefaultAligned(size_t n) {
    ABSL_DCHECK(internal::ArenaAlignDefault{}.IsAligned(n));
    ABSL_DCHECK_GE(limit_, ptr());

    if constexpr (alloc_client == AllocationClient::kArray) {
      if (auto res = TryAllocateFromCachedBlock(n); res.ptr != nullptr) {
        return res;
      }
    }

    return AllocateNonMessageImpl<AllocationMode::kDefault>(n);
  }

  // See comments on `cached_blocks_` member for details.
  void ReturnArrayMemory(void* p, size_t size) {
    // We only need to check for 32-bit platforms.
    // In 64-bit platforms the minimum allocation size from Repeated*Field will
    // be 16 guaranteed.
    if (sizeof(void*) < 8) {
      if (ABSL_PREDICT_FALSE(size < kMinimumReturnedArraySize)) return;
    } else {
      PROTOBUF_ASSUME(size >= kMinimumReturnedArraySize);
    }

    // We round down to the next smaller block in case the memory doesn't match
    // the pattern we are looking for. eg, someone might have called Reserve()
    // on the repeated field.
    const size_t index = absl::bit_width(size) - 5;

    if (ABSL_PREDICT_FALSE(index >= cached_block_length_)) {
      // We can't put this object on the freelist so make this object the
      // freelist. It is guaranteed it is larger than the one we have, and
      // large enough to hold another allocation of `size`.
      CachedBlock** new_list = static_cast<CachedBlock**>(p);
      size_t new_size = size / sizeof(CachedBlock*);

      std::copy(cached_blocks_, cached_blocks_ + cached_block_length_,
                new_list);

      // We need to unpoison this memory before filling it in case it has been
      // poisoned by another sanitizer client.
      internal::UnpoisonMemoryRegion(
          new_list + cached_block_length_,
          (new_size - cached_block_length_) * sizeof(CachedBlock*));

      std::fill(new_list + cached_block_length_, new_list + new_size, nullptr);

      cached_blocks_ = new_list;
      // Make the size fit in uint8_t. This is the power of two, so we don't
      // need anything larger.
      cached_block_length_ =
          static_cast<uint8_t>(std::min(size_t{64}, new_size));

      return;
    }

    auto& cached_head = cached_blocks_[index];
    auto* new_node = static_cast<CachedBlock*>(p);
    new_node->next = cached_head;
    cached_head = new_node;
    internal::PoisonMemoryRegion(p, size);
  }

  enum class AllocationMode {
    // Attempt to allocate in the current block. If there is not enough space,
    // call the out-of-line fallback allocator to allocate a new block. Always
    // succeeds.
    kDefault,
    // Attempt to allocate in the current block. If there is not enough space,
    // return `nullptr`.
    kNoFallback,
    // Allocate in the current block assuming there is enough space. Callers are
    // responsible for ensuring there is enough space.
    kUnchecked,
  };

  struct NoCleanup {
    void AddCleanup(SerialArena& arena, void* elem) {}

    static constexpr size_t MaxSize();
  };

  struct WithCleanup {
    void (*destructor)(void*);

    void AddCleanup(SerialArena& arena, void* elem) {
      arena.AddCleanup(elem, destructor);
    }

    static constexpr size_t MaxSize();
  };

  // The fast path for allocations within a seeded region which don't cross an
  // alignment boundary.
  template <bool needs_arena_ptr = false, typename Align = ArenaAlignDefault,
            typename Cleanup = NoCleanup>
  PROTOBUF_ALWAYS_INLINE AllocResult<needs_arena_ptr> AllocateFast(
      char* ptr, size_t n, Align align, Cleanup cleanup) {
    ABSL_DCHECK_EQ(ptr, this->ptr());

    ptr = align.CeilDefaultAligned(ptr);
    ABSL_DCHECK_LE(ptr + n, limit_);

    internal::UnpoisonMemoryRegion(ptr, n);
    char* next = ptr + n;

    set_ptr(next);
    cleanup.AddCleanup(*this, ptr);

    MaybePrefetchData(next);

    PROTOBUF_ASSUME(ptr != nullptr);
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    if constexpr (needs_arena_ptr) {
      return {ptr, encoded_arena_alignment_};
    } else {
      return {ptr};
    }
#else
    return {ptr};
#endif
  }

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  // Places an `Arena*` seed at `ptr` and returns the next available address
  // after the seed.
  inline char* SeedArena(char* ptr);

  // Slow fallback for allocating a message which won't cross the next
  // `kMinArenaSeedAlignment` alignment boundary in an unseeded region.
  inline MessageAllocationResult AllocateMessageInUnseededRegion(size_t n,
                                                                 char* ptr);

  // Fallback allocation function for when this allocation request would cross
  // `alignment_limit_`. This fallback is called either when crossing an
  // alignment boundary (and we should consider re-seeding the arena), or
  // because we are currently in an unseeded region.
  template <bool needs_arena_ptr, typename Align, typename Cleanup>
  AllocResult<needs_arena_ptr> AllocateSlow(size_t n, Align align,
                                            Cleanup cleanup);

  // Core arena allocation function. Configurable based on alignment and
  // cleanup registration.
  template <bool needs_arena_ptr, typename Align, typename Cleanup>
  PROTOBUF_ALWAYS_INLINE AllocResult<needs_arena_ptr> AllocateImpl(
      size_t n, Align align, Cleanup cleanup) {
    ABSL_DCHECK(internal::ArenaAlignDefault{}.IsAligned(n));
    ABSL_DCHECK_LE(alignment_limit_, limit_);
    DebugAssertCachedAlignmentLimitIsCorrect();

    char* ptr = this->ptr();

    // Comparing `alignment_limit_` to aligned `ptr` is precarious because
    // aligning `ptr` may overflow. Instead, we conservatively compare the total
    // remaining space against `n` plus the maximum possible padding we might
    // incur from aligning `ptr`. Doing it this way is slightly faster than
    // calculating how much alignment padding we need before doing the
    // comparison.
    const size_t max_alignment_padding =
        align.align() - ArenaAlignDefault{}.align();
    if (ABSL_PREDICT_TRUE(static_cast<ptrdiff_t>(n + max_alignment_padding) <=
                          alignment_limit_ - ptr)) {
      return AllocateFast<needs_arena_ptr>(ptr, n, align, cleanup);
    } else {
      PROTOBUF_MUSTTAIL return AllocateSlow<needs_arena_ptr>(n, align, cleanup);
    }
  }
#else

  // Core arena allocation function. Configurable based on overflow handling,
  // alignment, and cleanup registration.
  template <AllocationMode mode, typename Align, typename Cleanup>
  PROTOBUF_ALWAYS_INLINE AllocationResult AllocateImpl(size_t n, Align align,
                                                       Cleanup cleanup) {
    static_assert(std::is_same_v<Align, ArenaAlignDefault> ||
                  std::is_same_v<Align, MemoryAlign>);
    static_assert(std::is_same_v<Cleanup, NoCleanup> ||
                  std::is_same_v<Cleanup, WithCleanup>);
    ABSL_DCHECK(align.IsAligned(n));
    ABSL_DCHECK_GE(limit_, ptr());

    char* ptr = this->ptr();

    // Comparing `limit_` to aligned `ptr` is precarious because aligning `ptr`
    // may overflow. Instead, we conservatively compare the total remaining
    // space against `n` plus the maximum possible padding we might incur from
    // aligning `ptr`. Doing it this way is slightly faster than calculating how
    // much alignment padding we need before doing the comparison.
    const size_t max_alignment_padding =
        align.align() - ArenaAlignDefault{}.align();
    const bool enough_space =
        n + max_alignment_padding <= static_cast<size_t>(limit_ - ptr);
    switch (mode) {
      case AllocationMode::kDefault:
        if (ABSL_PREDICT_FALSE(!enough_space)) {
          PROTOBUF_MUSTTAIL return AllocateFallbackImpl<
              /*needs_arena_ptr=*/false>(n, align, cleanup);
        }
        break;
      case AllocationMode::kNoFallback:
        if (ABSL_PREDICT_FALSE(!enough_space)) {
          return {nullptr};
        }
        break;
      case AllocationMode::kUnchecked:
        ABSL_DCHECK(enough_space);
        break;
    }

    return AllocateFast(ptr, n, align, cleanup);
  }
#endif

  template <AllocationMode mode>
  PROTOBUF_ALWAYS_INLINE MessageAllocationResult AllocateMessageImpl(size_t n) {
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    (void)mode;
    auto res = AllocateImpl</*needs_arena_ptr=*/true>(n, ArenaAlignDefault{},
                                                      NoCleanup{});
    ABSL_DCHECK_EQ(res.encoded_arena_alignment.FindArena(res.ptr), parent());
    return res;
#else
    auto res = AllocateImpl<mode>(n, ArenaAlignDefault{}, NoCleanup{});
    return {res.ptr};
#endif
  }

  template <AllocationMode mode, typename Align = ArenaAlignDefault,
            typename Cleanup = NoCleanup>
  PROTOBUF_ALWAYS_INLINE AllocationResult
  AllocateNonMessageImpl(size_t n, Align align = {}, Cleanup cleanup = {}) {
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    (void)mode;
    return AllocateImpl</*needs_arena_ptr=*/false>(n, align, cleanup);
#else
    return AllocateImpl<mode>(n, align, cleanup);
#endif
  }

  PROTOBUF_ALWAYS_INLINE
  void AddCleanup(void* elem, void (*destructor)(void*)) {
    cleanup_list_.Add(elem, destructor, *this);
    MaybePrefetchCleanup();
  }

  AllocationResult AllocateFromStringBlock();

  std::vector<void*> PeekCleanupListForTesting();

  // Attempts to grow the most recent allocation in place by `desired_growth`
  // bytes. Succeeds only if `alloc_end` matches the current tail pointer and
  // the current arena block has enough remaining space. Returns true if the
  // allocation was grown in place, false otherwise.
  bool TryGrowTail(void* alloc_end, size_t desired_growth) {
    char* tail = ptr();
    if (static_cast<char*>(alloc_end) != tail) {
      return false;
    }
    if (static_cast<size_t>(limit_ - tail) < desired_growth) {
      return false;
    }
    internal::UnpoisonMemoryRegion(tail, desired_growth);
    set_ptr(tail + desired_growth);
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    if (tail + desired_growth > alignment_limit_) {
      UpdateCachedAlignments(tail + desired_growth, /*arena_alignment=*/0);
    }
#endif
    return true;
  }

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  static bool CrossesAlignmentBoundary(
      const void* begin, const void* end,
      EncodedArenaAlignment encoded_arena_alignment) {
    uintptr_t begin_val = reinterpret_cast<uintptr_t>(begin);
    uintptr_t end_val = reinterpret_cast<uintptr_t>(end);
    uint32_t align_shift = std::max<uint32_t>(
        encoded_arena_alignment.encoded_alignment(),
        absl::countr_zero(ArenaSeedPolicy::kMinArenaSeedAlignmentValue));
    // We actually check if begin_val - 1 to end_val - 1 crosses alignment. We
    // check the lower bound of begin_val - 1 because, if begin_val is exactly
    // on an alignment boundary, we need to fall back to the slow path to decide
    // if we want to re-seed the arena. And we check end_val - 1 as an upper
    // bound in case `end_val - begin_val` is exactly the remaining space in the
    // aligned region.
    return (((begin_val - 1) ^ (end_val - 1)) >> align_shift) != 0;
  };
#endif

  // Attempts to trim the most recent allocation in place down to `desired_end`.
  // Succeeds only if `alloc_end` matches the current tail pointer. Returns true
  // if the allocation was trimmed in place, false otherwise.
  // REQUIRES: desired_end <= alloc_end
  // REQUIRES: desired_end is an aligned pointer within a previous allocation.
  bool TryTrimTail(void* alloc_end, void* desired_end) {
    ABSL_DCHECK_LE(desired_end, alloc_end);
    ABSL_DCHECK(ArenaAlignDefault{}.IsAligned(desired_end));

    char* tail = ptr();
    if (static_cast<char*>(alloc_end) != tail) {
      return false;
    }

    ABSL_DCHECK_LE(static_cast<void*>(head()), desired_end);
    char* desired = static_cast<char*>(desired_end);
    internal::PoisonMemoryRegion(desired, tail - desired);

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
    // If this block lies within a seeded region, then shrinking the block can't
    // have moved `ptr_` to an unseeded region. Therefore, we can only cross an
    // alignment boundary if we are already in an unseeded region.
    if (ABSL_PREDICT_FALSE(encoded_arena_alignment_.is_unseeded()) &&
        CrossesAlignmentBoundary(desired, alloc_end,
                                 EncodedArenaAlignment::ArenaPrefix())) {
      alignment_limit_ =
          ArenaSeedPolicy::kMinArenaSeedAlignment.Floor(desired - 1);
    }
#endif
    set_ptr(desired);
    return true;
  }

  Arena* parent() const {
    Arena* parent = reinterpret_cast<Arena*>(&parent_);
    PROTOBUF_ASSUME(parent != nullptr);
    return parent;
  }

 private:
  friend class ThreadSafeArena;
  friend class cleanup::ChunkList;

  // See comments for cached_blocks_.
  struct CachedBlock {
    // Simple linked list.
    CachedBlock* next;
  };

  static constexpr ptrdiff_t kPrefetchDataDegree = ABSL_CACHELINE_SIZE * 16;
  static constexpr ptrdiff_t kPrefetchCleanupDegree = ABSL_CACHELINE_SIZE * 6;

  // Constructor is private as only New() should be used.
  inline SerialArena(ArenaBlock* b, ThreadSafeArena& parent,
                     uint32_t arena_alignment);

  inline SerialArena(FirstSerialArena, SizedPtr mem,
                     const AllocationPolicy& policy, ThreadSafeArena& parent);

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  // Debug-only function to verify that `alignment_limit_` is set correctly.
  void DebugAssertCachedAlignmentLimitIsCorrect() {
#if !defined(NDEBUG)
    char* alignment_limit =
        encoded_arena_alignment_.is_unseeded()
            ? ArenaSeedPolicy::kMinArenaSeedAlignment.Floor(ptr() - 1)
            : MemoryAlign{encoded_arena_alignment_.alignment()}.Ceil(ptr());
    alignment_limit = std::min(alignment_limit, limit_);

    ABSL_CHECK_EQ(alignment_limit_, alignment_limit);
#endif
  }

  PROTOBUF_ALWAYS_INLINE void UpdateCachedAlignments(char* ptr,
                                                     uint32_t arena_alignment) {
    if (arena_alignment == 0) {
      encoded_arena_alignment_ = EncodedArenaAlignment::ArenaPrefix();
      alignment_limit_ = ArenaSeedPolicy::kMinArenaSeedAlignment.Floor(ptr - 1);
    } else {
      encoded_arena_alignment_ = EncodedArenaAlignment::FromAlignment(
          std::align_val_t{arena_alignment});
      alignment_limit_ =
          std::min(MemoryAlign{arena_alignment}.Ceil(ptr), limit_);
    }
  }
#else
  PROTOBUF_ALWAYS_INLINE void UpdateCachedAlignments(char* ptr,
                                                     uint32_t arena_alignment) {
    (void)ptr;
    (void)arena_alignment;
  }
#endif

  AllocationResult MaybeAllocateString();
  AllocationResult AllocateFromStringBlockFallback();

  // Prefetch the next prefetch_degree bytes after `prefetch_ptr` and
  // up to `limit`, if `next` is within prefetch_degree bytes of `prefetch_ptr`.
  PROTOBUF_ALWAYS_INLINE
  static const char* MaybePrefetchImpl(const ptrdiff_t prefetch_degree,
                                       const char* next, const char* limit,
                                       const char* prefetch_ptr) {
    if (ABSL_PREDICT_TRUE(prefetch_ptr - next > prefetch_degree))
      return prefetch_ptr;
    if (ABSL_PREDICT_TRUE(prefetch_ptr < limit)) {
      prefetch_ptr = std::max(next, prefetch_ptr);
      ABSL_DCHECK(prefetch_ptr != nullptr);
      const char* end = std::min(limit, prefetch_ptr + prefetch_degree);
      for (; prefetch_ptr < end; prefetch_ptr += ABSL_CACHELINE_SIZE) {
        absl::PrefetchToLocalCacheForWrite(prefetch_ptr);
      }
    }
    return prefetch_ptr;
  }
  PROTOBUF_ALWAYS_INLINE
  void MaybePrefetchData(const char* next) {
    ABSL_DCHECK(static_cast<const void*>(prefetch_ptr_) == ptr() ||
                static_cast<const void*>(prefetch_ptr_) >= head());
    prefetch_ptr_ =
        MaybePrefetchImpl(kPrefetchDataDegree, next, limit_, prefetch_ptr_);
  }
  PROTOBUF_ALWAYS_INLINE
  void MaybePrefetchCleanup() {
    ABSL_DCHECK(static_cast<const void*>(cleanup_list_.prefetch_ptr_) ==
                    nullptr ||
                static_cast<const void*>(cleanup_list_.prefetch_ptr_) >=
                    cleanup_list_.head_);
    cleanup_list_.prefetch_ptr_ = MaybePrefetchImpl(
        kPrefetchCleanupDegree, reinterpret_cast<char*>(cleanup_list_.next_),
        reinterpret_cast<char*>(cleanup_list_.limit_),
        cleanup_list_.prefetch_ptr_);
  }

  // Creates a new SerialArena inside mem using the remaining memory as for
  // future allocations.
  // The `parent` arena must outlive the serial arena, which is guaranteed
  // because the parent manages the lifetime of the serial arenas.
  static SerialArena* New(SizedPtr mem, ThreadSafeArena& parent);
  // Free SerialArena returning the memory passed in to New.
  template <typename Deallocator>
  SizedPtr Free(Deallocator deallocator);

  size_t FreeStringBlocks() {
    // On the active block delete all strings skipping the unused instances.
    size_t unused_bytes = string_block_unused_.load(std::memory_order_relaxed);
    if (StringBlock* sb = string_block_.load(std::memory_order_relaxed)) {
      return FreeStringBlocks(sb, unused_bytes);
    }
    return 0;
  }
  static size_t FreeStringBlocks(StringBlock* string_block, size_t unused);

  // Adds 'used` to space_used_ in relaxed atomic order.
  void AddSpaceUsed(size_t space_used) {
    space_used_.store(space_used_.load(std::memory_order_relaxed) + space_used,
                      std::memory_order_relaxed);
  }

  // Adds 'allocated` to space_allocated_ in relaxed atomic order.
  void AddSpaceAllocated(size_t space_allocated) {
    space_allocated_.store(
        space_allocated_.load(std::memory_order_relaxed) + space_allocated,
        std::memory_order_relaxed);
  }

  // Helper getters/setters to handle relaxed operations on atomic variables.
  ArenaBlock* head() { return head_.load(std::memory_order_relaxed); }
  const ArenaBlock* head() const {
    return head_.load(std::memory_order_relaxed);
  }

  char* ptr() { return ptr_.load(std::memory_order_relaxed); }
  const char* ptr() const { return ptr_.load(std::memory_order_relaxed); }
  void set_ptr(char* ptr) { return ptr_.store(ptr, std::memory_order_relaxed); }
  PROTOBUF_ALWAYS_INLINE void set_range(char* ptr, char* limit,
                                        uint32_t arena_alignment) {
    set_ptr(ptr);
    prefetch_ptr_ = ptr;
    limit_ = limit;

    UpdateCachedAlignments(ptr, arena_alignment);
  }

  template <bool needs_arena_ptr, typename Align = ArenaAlignDefault,
            typename Cleanup = NoCleanup>
  AllocResult<needs_arena_ptr> AllocateFallbackImpl(size_t n, Align align = {},
                                                    Cleanup cleanup = {});
  inline void AllocateNewBlock(size_t n);
  inline void Init(ArenaBlock* b, size_t offset, size_t allocation_size,
                   uint32_t arena_alignment);

  // Members are declared here to track sizeof(SerialArena) and hotness
  // centrally. They are (roughly) laid out in descending order of hotness.

  // We initialize ptr/limit with an arbitrary valid pointer.
  // This allows Allocate to always return non-null even when asking for zero
  // bytes.
  static char* ArbitraryInternalPointerForInit() {
    // Use rodata to detect potential bugs. No one should be writing here.
    alignas(8) static constexpr char dummy{};
    return const_cast<char*>(&dummy);
  }


  // Repeated*Field/Map/etc and Arena play together to reduce memory consumption
  // by reusing blocks. Currently, natural growth of the repeated field types
  // makes them allocate blocks of size `8 + 2^N, N>=3`. When the repeated field
  // grows returns the previous block and we put it in this free list.
  // `cached_blocks_[i]` points to the free list for blocks of size `8+2^(i+3)`.
  // The array of freelists is grown when needed in `ReturnArrayMemory()`.
  // Note that this optimization somewhat conflicts with TryGrowTail because
  // allocating from the free list means we won't be able to grow in the tail.
  // On average, it works out because the free list tends to be mostly empty.
  uint8_t cached_block_length_ = 0;

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  // If at least `kMinArenaSeedAlignment`, there's an arena pointer at
  // `encoded_arena_alignment_` alignment below `ptr_`. Otherwise, we are in an
  // unseeded region, and this value is `EncodedArenaAlignment::ArenaPrefix()`.
  EncodedArenaAlignment encoded_arena_alignment_ =
      EncodedArenaAlignment::ArenaPrefix();
#endif

  // Current prefetch positions. Data from `ptr_` up to but not including
  // `prefetch_ptr_` is software prefetched.
  const char* prefetch_ptr_ = ArbitraryInternalPointerForInit();

  std::atomic<ArenaBlock*> head_{nullptr};  // Head of linked list of blocks.

  // Next pointer to allocate from.  Always 8-byte aligned.  Points inside
  // head_ (and head_->pos will always be non-canonical).  We keep these
  // here to reduce indirection.
  std::atomic<char*> ptr_{ArbitraryInternalPointerForInit()};

  // Limiting address up to which memory can be allocated from the head block.
  char* limit_ = ArbitraryInternalPointerForInit();

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
  // The next point at which we need to consider re-seeding the arena. Before
  // this point, we know we are within the range of the current arena seed.
  //
  // If we are in an unseeded region, this value is `kMinArenaSeedAlignment`
  // below the next `kMinArenaSeedAlignment` alignment boundary.
  //
  // This value is never greater than `limit_`.
  char* alignment_limit_ = ArbitraryInternalPointerForInit();
#endif

  std::atomic<size_t> space_allocated_{0};

  CachedBlock** cached_blocks_ = nullptr;

  // The active string block.
  std::atomic<StringBlock*> string_block_{nullptr};

  ThreadSafeArena& parent_;
  // The number of unused bytes in string_block_.
  // We allocate from `effective_size()` down to 0 inside `string_block_`.
  // `unused  == 0` means that `string_block_` is exhausted. (or null).
  std::atomic<size_t> string_block_unused_{0};

  // Chunked linked list for managing cleanup for arena elements.
  cleanup::ChunkList cleanup_list_;

  std::atomic<size_t> space_used_{0};  // Necessary for metrics.
};

PROTOBUF_ALWAYS_INLINE AllocationResult SerialArena::MaybeAllocateString() {
  // Check how many unused instances are in the current block.
  size_t unused_bytes = string_block_unused_.load(std::memory_order_relaxed);
  if (ABSL_PREDICT_FALSE(unused_bytes == 0)) {
    return {nullptr};
  }

  unused_bytes -= sizeof(std::string);
  string_block_unused_.store(unused_bytes, std::memory_order_relaxed);
  void* p =
      string_block_.load(std::memory_order_relaxed)->AtOffset(unused_bytes);
  PROTOBUF_ASSUME(p != nullptr);
  return {p};
}

PROTOBUF_ALWAYS_INLINE AllocationResult SerialArena::AllocateFromStringBlock() {
  if (auto res = MaybeAllocateString(); ABSL_PREDICT_TRUE(res.ptr != nullptr)) {
    return res;
  }
  return AllocateFromStringBlockFallback();
}

inline Arena* ParentArena(SerialArena* arena) {
  if (arena == nullptr) return nullptr;
  return arena->parent();
}

#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<false>
SerialArena::AllocateSlow<false>(size_t, ArenaAlignDefault, NoCleanup);
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<false>
SerialArena::AllocateSlow<false>(size_t, MemoryAlign, NoCleanup);
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<false>
SerialArena::AllocateSlow<false>(size_t, ArenaAlignDefault, WithCleanup);
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<false>
SerialArena::AllocateSlow<false>(size_t, MemoryAlign, WithCleanup);
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<true>
SerialArena::AllocateSlow<true>(size_t, ArenaAlignDefault, NoCleanup);
#endif

extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<false>
SerialArena::AllocateFallbackImpl<false>(size_t, ArenaAlignDefault, NoCleanup);
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<false>
SerialArena::AllocateFallbackImpl<false>(size_t, MemoryAlign, NoCleanup);
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<false>
SerialArena::AllocateFallbackImpl<false>(size_t, ArenaAlignDefault,
                                         WithCleanup);
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<false>
SerialArena::AllocateFallbackImpl<false>(size_t, MemoryAlign, WithCleanup);
#if defined(PROTOBUF_SMALLER_MESSAGE_LITE)
extern template PROTOBUF_EXPORT_TEMPLATE_DECLARE SerialArena::AllocResult<true>
SerialArena::AllocateFallbackImpl<true>(size_t, ArenaAlignDefault, NoCleanup);
#endif

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_SERIAL_ARENA_H__
