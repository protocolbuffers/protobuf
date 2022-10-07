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

// This file defines an Arena allocator for better allocation performance.

#ifndef GOOGLE_PROTOBUF_ARENA_IMPL_H__
#define GOOGLE_PROTOBUF_ARENA_IMPL_H__

#include <atomic>
#include <limits>
#include <string>
#include <type_traits>
#include <typeinfo>

#include "google/protobuf/stubs/common.h"
#include "google/protobuf/stubs/logging.h"
#include "absl/numeric/bits.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/arena_allocation_policy.h"
#include "google/protobuf/arena_cleanup.h"
#include "google/protobuf/arena_config.h"
#include "google/protobuf/arenaz_sampler.h"
#include "google/protobuf/port.h"


// Must be included last.
#include "google/protobuf/port_def.inc"


namespace google {
namespace protobuf {
namespace internal {

// To prevent sharing cache lines between threads
#ifdef __cpp_aligned_new
enum { kCacheAlignment = 64 };
#else
enum { kCacheAlignment = alignof(max_align_t) };  // do the best we can
#endif

inline PROTOBUF_ALWAYS_INLINE constexpr size_t AlignUpTo8(size_t n) {
  // Align n to next multiple of 8 (from Hacker's Delight, Chapter 3.)
  return (n + 7) & static_cast<size_t>(-8);
}

inline PROTOBUF_ALWAYS_INLINE constexpr size_t AlignUpTo(size_t n, size_t a) {
  // We are wasting space by over allocating align - 8 bytes. Compared to a
  // dedicated function that takes current alignment in consideration.  Such a
  // scheme would only waste (align - 8)/2 bytes on average, but requires a
  // dedicated function in the outline arena allocation functions. Possibly
  // re-evaluate tradeoffs later.
  return a <= 8 ? AlignUpTo8(n) : n + a - 8;
}

inline PROTOBUF_ALWAYS_INLINE void* AlignTo(void* p, size_t a) {
  if (a <= 8) {
    return p;
  } else {
    auto u = reinterpret_cast<uintptr_t>(p);
    return reinterpret_cast<void*>((u + a - 1) & (~a + 1));
  }
}

// Arena blocks are variable length malloc-ed objects.  The following structure
// describes the common header for all blocks.
struct ArenaBlock {
  // For the sentry block with zero-size where ptr_, limit_, cleanup_nodes all
  // point to "this".
  constexpr ArenaBlock()
      : next(nullptr), cleanup_nodes(this), size(0) {}

  ArenaBlock(ArenaBlock* next, size_t size)
      : next(next), cleanup_nodes(nullptr), size(size) {
    GOOGLE_DCHECK_GT(size, sizeof(ArenaBlock));
  }

  char* Pointer(size_t n) {
    GOOGLE_DCHECK_LE(n, size);
    return reinterpret_cast<char*>(this) + n;
  }
  char* Limit() { return Pointer(size & static_cast<size_t>(-8)); }

  bool IsSentry() const { return size == 0; }

  ArenaBlock* const next;
  void* cleanup_nodes;
  const size_t size;
  // data follows
};

using LifecycleIdAtomic = uint64_t;

// MetricsCollector collects stats for a particular arena.
class PROTOBUF_EXPORT ArenaMetricsCollector {
 public:
  ArenaMetricsCollector(bool record_allocs) : record_allocs_(record_allocs) {}

  // Invoked when the arena is about to be destroyed. This method will
  // typically finalize any metric collection and delete the collector.
  // space_allocated is the space used by the arena.
  virtual void OnDestroy(uint64_t space_allocated) = 0;

  // OnReset() is called when the associated arena is reset.
  // space_allocated is the space used by the arena just before the reset.
  virtual void OnReset(uint64_t space_allocated) = 0;

  // OnAlloc is called when an allocation happens.
  // type_info is promised to be static - its lifetime extends to
  // match program's lifetime (It is given by typeid operator).
  // Note: typeid(void) will be passed as allocated_type every time we
  // intentionally want to avoid monitoring an allocation. (i.e. internal
  // allocations for managing the arena)
  virtual void OnAlloc(const std::type_info* allocated_type,
                       uint64_t alloc_size) = 0;

  // Does OnAlloc() need to be called?  If false, metric collection overhead
  // will be reduced since we will not do extra work per allocation.
  bool RecordAllocs() { return record_allocs_; }

 protected:
  // This class is destructed by the call to OnDestroy().
  ~ArenaMetricsCollector() = default;
  const bool record_allocs_;
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
  struct Memory {
    void* ptr;
    size_t size;
  };

  void CleanupList();
  uint64_t SpaceAllocated() const {
    return space_allocated_.load(std::memory_order_relaxed);
  }
  uint64_t SpaceUsed() const;

  bool HasSpace(size_t n) const {
    return n <= static_cast<size_t>(limit_ - ptr());
  }

  // See comments on `cached_blocks_` member for details.
  PROTOBUF_ALWAYS_INLINE void* TryAllocateFromCachedBlock(size_t size) {
    if (PROTOBUF_PREDICT_FALSE(size < 16)) return nullptr;
    // We round up to the next larger block in case the memory doesn't match
    // the pattern we are looking for.
    const size_t index = absl::bit_width(size - 1) - 4;

    if (index >= cached_block_length_) return nullptr;
    auto& cached_head = cached_blocks_[index];
    if (cached_head == nullptr) return nullptr;

    void* ret = cached_head;
    PROTOBUF_UNPOISON_MEMORY_REGION(ret, size);
    cached_head = cached_head->next;
    return ret;
  }

  // In kArray mode we look through cached blocks.
  // We do not do this by default because most non-array allocations will not
  // have the right size and will fail to find an appropriate cached block.
  //
  // TODO(sbenza): Evaluate if we should use cached blocks for message types of
  // the right size. We can statically know if the allocation size can benefit
  // from it.
  template <AllocationClient alloc_client = AllocationClient::kDefault>
  void* AllocateAligned(size_t n) {
    GOOGLE_DCHECK_EQ(internal::AlignUpTo8(n), n);  // Must be already aligned.
    GOOGLE_DCHECK_GE(limit_, ptr());

    if (alloc_client == AllocationClient::kArray) {
      if (void* res = TryAllocateFromCachedBlock(n)) {
        return res;
      }
    }

    if (PROTOBUF_PREDICT_FALSE(!HasSpace(n))) {
      return AllocateAlignedFallback(n);
    }
    return AllocateFromExisting(n);
  }

 private:
  void* AllocateFromExisting(size_t n) {
    PROTOBUF_UNPOISON_MEMORY_REGION(ptr(), n);
    void* ret = ptr();
    set_ptr(static_cast<char*>(ret) + n);
    return ret;
  }

  // See comments on `cached_blocks_` member for details.
  void ReturnArrayMemory(void* p, size_t size) {
    // We only need to check for 32-bit platforms.
    // In 64-bit platforms the minimum allocation size from Repeated*Field will
    // be 16 guaranteed.
    if (sizeof(void*) < 8) {
      if (PROTOBUF_PREDICT_FALSE(size < 16)) return;
    } else {
      PROTOBUF_ASSUME(size >= 16);
    }

    // We round down to the next smaller block in case the memory doesn't match
    // the pattern we are looking for. eg, someone might have called Reserve()
    // on the repeated field.
    const size_t index = absl::bit_width(size) - 5;

    if (PROTOBUF_PREDICT_FALSE(index >= cached_block_length_)) {
      // We can't put this object on the freelist so make this object the
      // freelist. It is guaranteed it is larger than the one we have, and
      // large enough to hold another allocation of `size`.
      CachedBlock** new_list = static_cast<CachedBlock**>(p);
      size_t new_size = size / sizeof(CachedBlock*);

      std::copy(cached_blocks_, cached_blocks_ + cached_block_length_,
                new_list);

      // We need to unpoison this memory before filling it in case it has been
      // poisoned by another santizer client.
      PROTOBUF_UNPOISON_MEMORY_REGION(
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
    PROTOBUF_POISON_MEMORY_REGION(p, size);
  }

 public:
  // Allocate space if the current region provides enough space.
  bool MaybeAllocateAligned(size_t n, void** out) {
    GOOGLE_DCHECK_EQ(internal::AlignUpTo8(n), n);  // Must be already aligned.
    GOOGLE_DCHECK_GE(limit_, ptr());
    if (PROTOBUF_PREDICT_FALSE(!HasSpace(n))) return false;
    *out = AllocateFromExisting(n);
    return true;
  }

  // If there is enough space in the current block, allocate space for one `T`
  // object and register for destruction. The object has not been constructed
  // and the memory returned is uninitialized.
  template <typename T>
  PROTOBUF_ALWAYS_INLINE void* MaybeAllocateWithCleanup() {
    GOOGLE_DCHECK_GE(limit_, ptr());
    static_assert(!std::is_trivially_destructible<T>::value,
                  "This function is only for non-trivial types.");

    constexpr int aligned_size = AlignUpTo8(sizeof(T));
    constexpr auto destructor = cleanup::arena_destruct_object<T>;
    size_t required = aligned_size + cleanup::Size(destructor);
    if (PROTOBUF_PREDICT_FALSE(!HasSpace(required))) {
      return nullptr;
    }
    void* ptr = AllocateFromExistingWithCleanupFallback(aligned_size,
                                                        alignof(T), destructor);
    PROTOBUF_ASSUME(ptr != nullptr);
    return ptr;
  }

  PROTOBUF_ALWAYS_INLINE
  void* AllocateAlignedWithCleanup(size_t n, size_t align,
                                   void (*destructor)(void*)) {
    size_t required = AlignUpTo(n, align) + cleanup::Size(destructor);
    if (PROTOBUF_PREDICT_FALSE(!HasSpace(required))) {
      return AllocateAlignedWithCleanupFallback(n, align, destructor);
    }
    return AllocateFromExistingWithCleanupFallback(n, align, destructor);
  }

  PROTOBUF_ALWAYS_INLINE
  void AddCleanup(void* elem, void (*destructor)(void*)) {
    size_t required = cleanup::Size(destructor);
    if (PROTOBUF_PREDICT_FALSE(!HasSpace(required))) {
      return AddCleanupFallback(elem, destructor);
    }
    AddCleanupFromExisting(elem, destructor);
  }

 private:
  void* AllocateFromExistingWithCleanupFallback(size_t n, size_t align,
                                                void (*destructor)(void*)) {
    n = AlignUpTo(n, align);
    PROTOBUF_UNPOISON_MEMORY_REGION(ptr(), n);
    void* ret = internal::AlignTo(ptr(), align);
    set_ptr(ptr() + n);
    GOOGLE_DCHECK_GE(limit_, ptr());
    AddCleanupFromExisting(ret, destructor);
    return ret;
  }

  PROTOBUF_ALWAYS_INLINE
  void AddCleanupFromExisting(void* elem, void (*destructor)(void*)) {
    cleanup::Tag tag = cleanup::Type(destructor);
    size_t n = cleanup::Size(tag);

    PROTOBUF_UNPOISON_MEMORY_REGION(limit_ - n, n);
    limit_ -= n;
    GOOGLE_DCHECK_GE(limit_, ptr());
    cleanup::CreateNode(tag, limit_, elem, destructor);
  }

 private:
  friend class ThreadSafeArena;

  // Creates a new SerialArena inside mem using the remaining memory as for
  // future allocations.
  // The `parent` arena must outlive the serial arena, which is guaranteed
  // because the parent manages the lifetime of the serial arenas.
  static SerialArena* New(SerialArena::Memory mem, ThreadSafeArena& parent);
  // Free SerialArena returning the memory passed in to New
  template <typename Deallocator>
  Memory Free(Deallocator deallocator);

  // Members are declared here to track sizeof(SerialArena) and hotness
  // centrally. They are (roughly) laid out in descending order of hotness.

  // Next pointer to allocate from.  Always 8-byte aligned.  Points inside
  // head_ (and head_->pos will always be non-canonical).  We keep these
  // here to reduce indirection.
  std::atomic<char*> ptr_{nullptr};
  // Limiting address up to which memory can be allocated from the head block.
  char* limit_ = nullptr;

  std::atomic<ArenaBlock*> head_{nullptr};  // Head of linked list of blocks.
  std::atomic<size_t> space_used_{0};       // Necessary for metrics.
  std::atomic<size_t> space_allocated_{0};
  ThreadSafeArena& parent_;

  // Repeated*Field and Arena play together to reduce memory consumption by
  // reusing blocks. Currently, natural growth of the repeated field types makes
  // them allocate blocks of size `8 + 2^N, N>=3`.
  // When the repeated field grows returns the previous block and we put it in
  // this free list.
  // `cached_blocks_[i]` points to the free list for blocks of size `8+2^(i+3)`.
  // The array of freelists is grown when needed in `ReturnArrayMemory()`.
  struct CachedBlock {
    // Simple linked list.
    CachedBlock* next;
  };
  uint8_t cached_block_length_ = 0;
  CachedBlock** cached_blocks_ = nullptr;

  // Helper getters/setters to handle relaxed operations on atomic variables.
  ArenaBlock* head() { return head_.load(std::memory_order_relaxed); }
  const ArenaBlock* head() const {
    return head_.load(std::memory_order_relaxed);
  }

  char* ptr() { return ptr_.load(std::memory_order_relaxed); }
  const char* ptr() const { return ptr_.load(std::memory_order_relaxed); }
  void set_ptr(char* ptr) { return ptr_.store(ptr, std::memory_order_relaxed); }

  // Constructor is private as only New() should be used.
  inline SerialArena(ArenaBlock* b, ThreadSafeArena& parent);

  // Constructors to handle the first SerialArena.
  inline explicit SerialArena(ThreadSafeArena& parent);
  inline SerialArena(FirstSerialArena, ArenaBlock* b, ThreadSafeArena& parent);

  void* AllocateAlignedFallback(size_t n);
  void* AllocateAlignedWithCleanupFallback(size_t n, size_t align,
                                           void (*destructor)(void*));
  void AddCleanupFallback(void* elem, void (*destructor)(void*));
  inline void AllocateNewBlock(size_t n);
  inline void Init(ArenaBlock* b, size_t offset);

 public:
  static constexpr size_t kBlockHeaderSize = AlignUpTo8(sizeof(ArenaBlock));
};

// Tag type used to invoke the constructor of message-owned arena.
// Only message-owned arenas use this constructor for creation.
// Such constructors are internal implementation details of the library.
struct MessageOwned {
  explicit MessageOwned() = default;
};

// This class provides the core Arena memory allocation library. Different
// implementations only need to implement the public interface below.
// Arena is not a template type as that would only be useful if all protos
// in turn would be templates, which will/cannot happen. However separating
// the memory allocation part from the cruft of the API users expect we can
// use #ifdef the select the best implementation based on hardware / OS.
class PROTOBUF_EXPORT ThreadSafeArena {
 public:
  ThreadSafeArena();

  // Constructor solely used by message-owned arena.
  explicit ThreadSafeArena(internal::MessageOwned);

  ThreadSafeArena(char* mem, size_t size);

  explicit ThreadSafeArena(void* mem, size_t size,
                           const AllocationPolicy& policy);

  // All protos have pointers back to the arena hence Arena must have
  // pointer stability.
  ThreadSafeArena(const ThreadSafeArena&) = delete;
  ThreadSafeArena& operator=(const ThreadSafeArena&) = delete;
  ThreadSafeArena(ThreadSafeArena&&) = delete;
  ThreadSafeArena& operator=(ThreadSafeArena&&) = delete;

  // Destructor deletes all owned heap allocated objects, and destructs objects
  // that have non-trivial destructors, except for proto2 message objects whose
  // destructors can be skipped. Also, frees all blocks except the initial block
  // if it was passed in.
  ~ThreadSafeArena();

  uint64_t Reset();

  uint64_t SpaceAllocated() const;
  uint64_t SpaceUsed() const;

  template <AllocationClient alloc_client = AllocationClient::kDefault>
  void* AllocateAligned(size_t n) {
    SerialArena* arena;
    if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
      return arena->AllocateAligned<alloc_client>(n);
    } else {
      return AllocateAlignedFallback<alloc_client>(n);
    }
  }

  void ReturnArrayMemory(void* p, size_t size) {
    SerialArena* arena;
    if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
      arena->ReturnArrayMemory(p, size);
    }
  }

  // This function allocates n bytes if the common happy case is true and
  // returns true. Otherwise does nothing and returns false. This strange
  // semantics is necessary to allow callers to program functions that only
  // have fallback function calls in tail position. This substantially improves
  // code for the happy path.
  PROTOBUF_NDEBUG_INLINE bool MaybeAllocateAligned(size_t n, void** out) {
    SerialArena* arena;
    if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
      return arena->MaybeAllocateAligned(n, out);
    }
    return false;
  }

  void* AllocateAlignedWithCleanup(size_t n, size_t align,
                                   void (*destructor)(void*));

  // Add object pointer and cleanup function pointer to the list.
  void AddCleanup(void* elem, void (*cleanup)(void*));

  // Checks whether this arena is message-owned.
  PROTOBUF_ALWAYS_INLINE bool IsMessageOwned() const {
    return tag_and_id_ & kMessageOwnedArena;
  }

 private:
  friend class ArenaBenchmark;
  friend class TcParser;
  friend class SerialArena;
  friend struct SerialArenaChunkHeader;
  static uint64_t GetNextLifeCycleId();

  class SerialArenaChunk;

  // Returns a new SerialArenaChunk that has {id, serial} at slot 0. It may
  // grow based on "prev_num_slots".
  static SerialArenaChunk* NewSerialArenaChunk(uint32_t prev_capacity, void* id,
                                               SerialArena* serial);
  static SerialArenaChunk* SentrySerialArenaChunk();

  // Returns the first ArenaBlock* for the first SerialArena. If users provide
  // one, use it if it's acceptable. Otherwise returns a sentry block.
  ArenaBlock* FirstBlock(void* buf, size_t size);
  // Same as the above but returns a valid block if "policy" is not default.
  ArenaBlock* FirstBlock(void* buf, size_t size,
                         const AllocationPolicy& policy);

  // Adds SerialArena to the chunked list. May create a new chunk.
  void AddSerialArena(void* id, SerialArena* serial);

  // Members are declared here to track sizeof(ThreadSafeArena) and hotness
  // centrally.

  // Unique for each arena. Changes on Reset().
  uint64_t tag_and_id_ = 0;

  TaggedAllocationPolicyPtr alloc_policy_;  // Tagged pointer to AllocPolicy.
  ThreadSafeArenaStatsHandle arena_stats_;

  // Adding a new chunk to head_ must be protected by mutex_.
  absl::Mutex mutex_;
  // Pointer to a linked list of SerialArenaChunk.
  std::atomic<SerialArenaChunk*> head_{nullptr};

  void* first_owner_;
  // Must be declared after alloc_policy_; otherwise, it may lose info on
  // user-provided initial block.
  SerialArena first_arena_;

  // The LSB of tag_and_id_ indicates if the arena is message-owned.
  enum : uint64_t { kMessageOwnedArena = 1 };

  static_assert(std::is_trivially_destructible<SerialArena>{},
                "SerialArena needs to be trivially destructible.");

  const AllocationPolicy* AllocPolicy() const { return alloc_policy_.get(); }
  void InitializeWithPolicy(const AllocationPolicy& policy);
  void* AllocateAlignedWithCleanupFallback(size_t n, size_t align,
                                           void (*destructor)(void*));

  void Init();

  // Delete or Destruct all objects owned by the arena.
  void CleanupList();

  inline void CacheSerialArena(SerialArena* serial) {
    if (!IsMessageOwned()) {
      thread_cache().last_serial_arena = serial;
      thread_cache().last_lifecycle_id_seen = tag_and_id_;
    }
  }

  PROTOBUF_NDEBUG_INLINE bool GetSerialArenaFast(SerialArena** arena) {
    // If this thread already owns a block in this arena then try to use that.
    // This fast path optimizes the case where multiple threads allocate from
    // the same arena.
    ThreadCache* tc = &thread_cache();
    if (PROTOBUF_PREDICT_TRUE(tc->last_lifecycle_id_seen == tag_and_id_)) {
      *arena = tc->last_serial_arena;
      return true;
    }
    return false;
  }

  // Finds SerialArena or creates one if not found. When creating a new one,
  // create a big enough block to accommodate n bytes.
  SerialArena* GetSerialArenaFallback(size_t n);

  template <AllocationClient alloc_client = AllocationClient::kDefault>
  void* AllocateAlignedFallback(size_t n);

  // Executes callback function over SerialArenaChunk. Passes const
  // SerialArenaChunk*.
  template <typename Functor>
  void WalkConstSerialArenaChunk(Functor fn) const;

  // Executes callback function over SerialArenaChunk.
  template <typename Functor>
  void WalkSerialArenaChunk(Functor fn);

  // Executes callback function over SerialArena in chunked list in reverse
  // chronological order. Passes const SerialArena*.
  template <typename Functor>
  void PerConstSerialArenaInChunk(Functor fn) const;

  // Releases all memory except the first block which it returns. The first
  // block might be owned by the user and thus need some extra checks before
  // deleting.
  SerialArena::Memory Free(size_t* space_allocated);

#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif
  struct alignas(kCacheAlignment) ThreadCache {
#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
    // If we are using the ThreadLocalStorage class to store the ThreadCache,
    // then the ThreadCache's default constructor has to be responsible for
    // initializing it.
    ThreadCache()
        : next_lifecycle_id(0),
          last_lifecycle_id_seen(-1),
          last_serial_arena(nullptr) {}
#endif

    // Number of per-thread lifecycle IDs to reserve. Must be power of two.
    // To reduce contention on a global atomic, each thread reserves a batch of
    // IDs.  The following number is calculated based on a stress test with
    // ~6500 threads all frequently allocating a new arena.
    static constexpr size_t kPerThreadIds = 256;
    // Next lifecycle ID available to this thread. We need to reserve a new
    // batch, if `next_lifecycle_id & (kPerThreadIds - 1) == 0`.
    uint64_t next_lifecycle_id;
    // The ThreadCache is considered valid as long as this matches the
    // lifecycle_id of the arena being used.
    uint64_t last_lifecycle_id_seen;
    SerialArena* last_serial_arena;
  };

  // Lifecycle_id can be highly contended variable in a situation of lots of
  // arena creation. Make sure that other global variables are not sharing the
  // cacheline.
#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif
  struct alignas(kCacheAlignment) CacheAlignedLifecycleIdGenerator {
    constexpr CacheAlignedLifecycleIdGenerator() : id{0} {}

    std::atomic<LifecycleIdAtomic> id;
  };
  static CacheAlignedLifecycleIdGenerator lifecycle_id_generator_;
#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
  // iOS does not support __thread keyword so we use a custom thread local
  // storage class we implemented.
  static ThreadCache& thread_cache();
#elif defined(PROTOBUF_USE_DLLS)
  // Thread local variables cannot be exposed through DLL interface but we can
  // wrap them in static functions.
  static ThreadCache& thread_cache();
#else
  static PROTOBUF_THREAD_LOCAL ThreadCache thread_cache_;
  static ThreadCache& thread_cache() { return thread_cache_; }
#endif

 public:
  // kBlockHeaderSize is sizeof(ArenaBlock), aligned up to the nearest multiple
  // of 8 to protect the invariant that pos is always at a multiple of 8.
  static constexpr size_t kBlockHeaderSize = SerialArena::kBlockHeaderSize;
  static constexpr size_t kSerialArenaSize =
      (sizeof(SerialArena) + 7) & static_cast<size_t>(-8);
  static constexpr size_t kAllocPolicySize =
      AlignUpTo8(sizeof(AllocationPolicy));
  static constexpr size_t kMaxCleanupNodeSize = 16;
  static_assert(kBlockHeaderSize % 8 == 0,
                "kBlockHeaderSize must be a multiple of 8.");
  static_assert(kSerialArenaSize % 8 == 0,
                "kSerialArenaSize must be a multiple of 8.");
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_IMPL_H__
