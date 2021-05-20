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
#include <typeinfo>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/logging.h>

#ifdef ADDRESS_SANITIZER
#include <sanitizer/asan_interface.h>
#endif  // ADDRESS_SANITIZER

#include <google/protobuf/port_def.inc>


namespace google {
namespace protobuf {
namespace internal {

inline constexpr size_t AlignUpTo8(size_t n) {
  // Align n to next multiple of 8 (from Hacker's Delight, Chapter 3.)
  return (n + 7) & static_cast<size_t>(-8);
}

using LifecycleIdAtomic = uint64_t;

// MetricsCollector collects stats for a particular arena.
class PROTOBUF_EXPORT ArenaMetricsCollector {
 public:
  ArenaMetricsCollector(bool record_allocs) : record_allocs_(record_allocs) {}

  // Invoked when the arena is about to be destroyed. This method will
  // typically finalize any metric collection and delete the collector.
  // space_allocated is the space used by the arena.
  virtual void OnDestroy(uint64 space_allocated) = 0;

  // OnReset() is called when the associated arena is reset.
  // space_allocated is the space used by the arena just before the reset.
  virtual void OnReset(uint64 space_allocated) = 0;

  // OnAlloc is called when an allocation happens.
  // type_info is promised to be static - its lifetime extends to
  // match program's lifetime (It is given by typeid operator).
  // Note: typeid(void) will be passed as allocated_type every time we
  // intentionally want to avoid monitoring an allocation. (i.e. internal
  // allocations for managing the arena)
  virtual void OnAlloc(const std::type_info* allocated_type,
                       uint64 alloc_size) = 0;

  // Does OnAlloc() need to be called?  If false, metric collection overhead
  // will be reduced since we will not do extra work per allocation.
  bool RecordAllocs() { return record_allocs_; }

 protected:
  // This class is destructed by the call to OnDestroy().
  ~ArenaMetricsCollector() = default;
  const bool record_allocs_;
};

struct AllocationPolicy {
  static constexpr size_t kDefaultStartBlockSize = 256;
  static constexpr size_t kDefaultMaxBlockSize = 8192;

  size_t start_block_size = kDefaultStartBlockSize;
  size_t max_block_size = kDefaultMaxBlockSize;
  void* (*block_alloc)(size_t) = nullptr;
  void (*block_dealloc)(void*, size_t) = nullptr;
  ArenaMetricsCollector* metrics_collector = nullptr;

  bool IsDefault() const {
    return start_block_size == kDefaultMaxBlockSize &&
           max_block_size == kDefaultMaxBlockSize && block_alloc == nullptr &&
           block_dealloc == nullptr && metrics_collector == nullptr;
  }
};

// A simple arena allocator. Calls to allocate functions must be properly
// serialized by the caller, hence this class cannot be used as a general
// purpose allocator in a multi-threaded program. It serves as a building block
// for ThreadSafeArena, which provides a thread-safe arena allocator.
//
// This class manages
// 1) Arena bump allocation + owning memory blocks.
// 2) Maintaining a cleanup list.
// It delagetes the actual memory allocation back to ThreadSafeArena, which
// contains the information on block growth policy and backing memory allocation
// used.
class PROTOBUF_EXPORT SerialArena {
 public:
  struct Memory {
    void* ptr;
    size_t size;
  };

  // Node contains the ptr of the object to be cleaned up and the associated
  // cleanup function ptr.
  struct CleanupNode {
    void* elem;              // Pointer to the object to be cleaned up.
    void (*cleanup)(void*);  // Function pointer to the destructor or deleter.
  };

  // Creates a new SerialArena inside mem using the remaining memory as for
  // future allocations.
  static SerialArena* New(SerialArena::Memory mem, void* owner);
  // Free SerialArena returning the memory passed in to New
  template <typename Deallocator>
  Memory Free(Deallocator deallocator);

  void CleanupList();
  uint64 SpaceAllocated() const {
    return space_allocated_.load(std::memory_order_relaxed);
  }
  uint64 SpaceUsed() const;

  bool HasSpace(size_t n) { return n <= static_cast<size_t>(limit_ - ptr_); }

  void* AllocateAligned(size_t n, const AllocationPolicy* policy) {
    GOOGLE_DCHECK_EQ(internal::AlignUpTo8(n), n);  // Must be already aligned.
    GOOGLE_DCHECK_GE(limit_, ptr_);
    if (PROTOBUF_PREDICT_FALSE(!HasSpace(n))) {
      return AllocateAlignedFallback(n, policy);
    }
    void* ret = ptr_;
    ptr_ += n;
#ifdef ADDRESS_SANITIZER
    ASAN_UNPOISON_MEMORY_REGION(ret, n);
#endif  // ADDRESS_SANITIZER
    return ret;
  }

  // Allocate space if the current region provides enough space.
  bool MaybeAllocateAligned(size_t n, void** out) {
    GOOGLE_DCHECK_EQ(internal::AlignUpTo8(n), n);  // Must be already aligned.
    GOOGLE_DCHECK_GE(limit_, ptr_);
    if (PROTOBUF_PREDICT_FALSE(!HasSpace(n))) return false;
    void* ret = ptr_;
    ptr_ += n;
#ifdef ADDRESS_SANITIZER
    ASAN_UNPOISON_MEMORY_REGION(ret, n);
#endif  // ADDRESS_SANITIZER
    *out = ret;
    return true;
  }

  std::pair<void*, CleanupNode*> AllocateAlignedWithCleanup(
      size_t n, const AllocationPolicy* policy) {
    if (PROTOBUF_PREDICT_FALSE(!HasSpace(n + kCleanupSize))) {
      return AllocateAlignedWithCleanupFallback(n, policy);
    }
    void* ret = ptr_;
    ptr_ += n;
    limit_ -= kCleanupSize;
#ifdef ADDRESS_SANITIZER
    ASAN_UNPOISON_MEMORY_REGION(ret, n);
    ASAN_UNPOISON_MEMORY_REGION(limit_, kCleanupSize);
#endif  // ADDRESS_SANITIZER
    return CreatePair(ret, reinterpret_cast<CleanupNode*>(limit_));
  }

  void AddCleanup(void* elem, void (*cleanup)(void*),
                  const AllocationPolicy* policy) {
    auto res = AllocateAlignedWithCleanup(0, policy);
    res.second->elem = elem;
    res.second->cleanup = cleanup;
  }

  void* owner() const { return owner_; }
  SerialArena* next() const { return next_; }
  void set_next(SerialArena* next) { next_ = next; }

 private:
  // Blocks are variable length malloc-ed objects.  The following structure
  // describes the common header for all blocks.
  struct Block {
    char* Pointer(size_t n) {
      GOOGLE_DCHECK(n <= size);
      return reinterpret_cast<char*>(this) + n;
    }

    Block* next;
    size_t size;
    CleanupNode* start;
    // data follows
  };

  void* owner_;            // &ThreadCache of this thread;
  Block* head_;            // Head of linked list of blocks.
  SerialArena* next_;      // Next SerialArena in this linked list.
  size_t space_used_ = 0;  // Necessary for metrics.
  std::atomic<size_t> space_allocated_;

  // Next pointer to allocate from.  Always 8-byte aligned.  Points inside
  // head_ (and head_->pos will always be non-canonical).  We keep these
  // here to reduce indirection.
  char* ptr_;
  char* limit_;

  // Constructor is private as only New() should be used.
  inline SerialArena(Block* b, void* owner);
  void* AllocateAlignedFallback(size_t n, const AllocationPolicy* policy);
  std::pair<void*, CleanupNode*> AllocateAlignedWithCleanupFallback(
      size_t n, const AllocationPolicy* policy);
  void AllocateNewBlock(size_t n, const AllocationPolicy* policy);

  std::pair<void*, CleanupNode*> CreatePair(void* ptr, CleanupNode* node) {
    return {ptr, node};
  }

 public:
  static constexpr size_t kBlockHeaderSize = AlignUpTo8(sizeof(Block));
  static constexpr size_t kCleanupSize = AlignUpTo8(sizeof(CleanupNode));
};

// This class provides the core Arena memory allocation library. Different
// implementations only need to implement the public interface below.
// Arena is not a template type as that would only be useful if all protos
// in turn would be templates, which will/cannot happen. However separating
// the memory allocation part from the cruft of the API users expect we can
// use #ifdef the select the best implementation based on hardware / OS.
class PROTOBUF_EXPORT ThreadSafeArena {
 public:
  ThreadSafeArena() { Init(false); }

  ThreadSafeArena(char* mem, size_t size) { InitializeFrom(mem, size); }

  explicit ThreadSafeArena(void* mem, size_t size,
                           const AllocationPolicy& policy) {
    if (policy.IsDefault()) {
      // Legacy code doesn't use the API above, but provides the initial block
      // through ArenaOptions. I suspect most do not touch the allocation
      // policy parameters.
      InitializeFrom(mem, size);
    } else {
      auto collector = policy.metrics_collector;
      bool record_allocs = collector && collector->RecordAllocs();
      InitializeWithPolicy(mem, size, record_allocs, policy);
    }
  }

  // Destructor deletes all owned heap allocated objects, and destructs objects
  // that have non-trivial destructors, except for proto2 message objects whose
  // destructors can be skipped. Also, frees all blocks except the initial block
  // if it was passed in.
  ~ThreadSafeArena();

  uint64 Reset();

  uint64 SpaceAllocated() const;
  uint64 SpaceUsed() const;

  void* AllocateAligned(size_t n, const std::type_info* type) {
    SerialArena* arena;
    if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(tag_and_id_, &arena))) {
      return arena->AllocateAligned(n, AllocPolicy());
    } else {
      return AllocateAlignedFallback(n, type);
    }
  }

  // This function allocates n bytes if the common happy case is true and
  // returns true. Otherwise does nothing and returns false. This strange
  // semantics is necessary to allow callers to program functions that only
  // have fallback function calls in tail position. This substantially improves
  // code for the happy path.
  PROTOBUF_NDEBUG_INLINE bool MaybeAllocateAligned(size_t n, void** out) {
    SerialArena* a;
    if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFromThreadCache(tag_and_id_, &a))) {
      return a->MaybeAllocateAligned(n, out);
    }
    return false;
  }

  std::pair<void*, SerialArena::CleanupNode*> AllocateAlignedWithCleanup(
      size_t n, const std::type_info* type);

  // Add object pointer and cleanup function pointer to the list.
  void AddCleanup(void* elem, void (*cleanup)(void*));

 private:
  // Unique for each arena. Changes on Reset().
  uint64 tag_and_id_;
  // The LSB of tag_and_id_ indicates if allocs in this arena are recorded.
  enum { kRecordAllocs = 1 };

  intptr_t alloc_policy_ = 0;  // Tagged pointer to AllocPolicy.
  // The LSB of alloc_policy_ indicates if the user owns the initial block.
  enum { kUserOwnedInitialBlock = 1 };

  // Pointer to a linked list of SerialArena.
  std::atomic<SerialArena*> threads_;
  std::atomic<SerialArena*> hint_;  // Fast thread-local block access

  const AllocationPolicy* AllocPolicy() const {
    return reinterpret_cast<const AllocationPolicy*>(alloc_policy_ & -8);
  }
  void InitializeFrom(void* mem, size_t size);
  void InitializeWithPolicy(void* mem, size_t size, bool record_allocs,
                            AllocationPolicy policy);
  void* AllocateAlignedFallback(size_t n, const std::type_info* type);
  std::pair<void*, SerialArena::CleanupNode*>
  AllocateAlignedWithCleanupFallback(size_t n, const std::type_info* type);
  void AddCleanupFallback(void* elem, void (*cleanup)(void*));

  void Init(bool record_allocs);
  void SetInitialBlock(void* mem, size_t size);

  // Delete or Destruct all objects owned by the arena.
  void CleanupList();

  inline bool ShouldRecordAlloc() const { return tag_and_id_ & kRecordAllocs; }

  inline uint64 LifeCycleId() const {
    return tag_and_id_ & (-kRecordAllocs - 1);
  }

  inline void RecordAlloc(const std::type_info* allocated_type,
                          size_t n) const {
    AllocPolicy()->metrics_collector->OnAlloc(allocated_type, n);
  }

  inline void CacheSerialArena(SerialArena* serial) {
    thread_cache().last_serial_arena = serial;
    thread_cache().last_lifecycle_id_seen = LifeCycleId();
    // TODO(haberman): evaluate whether we would gain efficiency by getting rid
    // of hint_.  It's the only write we do to ThreadSafeArena in the allocation
    // path, which will dirty the cache line.

    hint_.store(serial, std::memory_order_release);
  }

  PROTOBUF_NDEBUG_INLINE bool GetSerialArenaFast(uint64 lifecycle_id,
                                                 SerialArena** arena) {
    if (GetSerialArenaFromThreadCache(lifecycle_id, arena)) return true;
    if (lifecycle_id & kRecordAllocs) return false;

    // Check whether we own the last accessed SerialArena on this arena.  This
    // fast path optimizes the case where a single thread uses multiple arenas.
    ThreadCache* tc = &thread_cache();
    SerialArena* serial = hint_.load(std::memory_order_acquire);
    if (PROTOBUF_PREDICT_TRUE(serial != NULL && serial->owner() == tc)) {
      *arena = serial;
      return true;
    }
    return false;
  }

  PROTOBUF_NDEBUG_INLINE bool GetSerialArenaFromThreadCache(
      uint64 lifecycle_id, SerialArena** arena) {
    // If this thread already owns a block in this arena then try to use that.
    // This fast path optimizes the case where multiple threads allocate from
    // the same arena.
    ThreadCache* tc = &thread_cache();
    if (PROTOBUF_PREDICT_TRUE(tc->last_lifecycle_id_seen == lifecycle_id)) {
      *arena = tc->last_serial_arena;
      return true;
    }
    return false;
  }
  SerialArena* GetSerialArenaFallback(void* me);

  template <typename Functor>
  void PerSerialArena(Functor fn) {
    // By omitting an Acquire barrier we ensure that any user code that doesn't
    // properly synchronize Reset() or the destructor will throw a TSAN warning.
    SerialArena* serial = threads_.load(std::memory_order_relaxed);

    for (; serial; serial = serial->next()) fn(serial);
  }

  // Releases all memory except the first block which it returns. The first
  // block might be owned by the user and thus need some extra checks before
  // deleting.
  SerialArena::Memory Free(size_t* space_allocated);

#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif
  struct alignas(64) ThreadCache {
#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
    // If we are using the ThreadLocalStorage class to store the ThreadCache,
    // then the ThreadCache's default constructor has to be responsible for
    // initializing it.
    ThreadCache()
        : next_lifecycle_id(0),
          last_lifecycle_id_seen(-1),
          last_serial_arena(NULL) {}
#endif

    // Number of per-thread lifecycle IDs to reserve. Must be power of two.
    // To reduce contention on a global atomic, each thread reserves a batch of
    // IDs.  The following number is calculated based on a stress test with
    // ~6500 threads all frequently allocating a new arena.
    static constexpr size_t kPerThreadIds = 256;
    // Next lifecycle ID available to this thread. We need to reserve a new
    // batch, if `next_lifecycle_id & (kPerThreadIds - 1) == 0`.
    uint64 next_lifecycle_id;
    // The ThreadCache is considered valid as long as this matches the
    // lifecycle_id of the arena being used.
    uint64 last_lifecycle_id_seen;
    SerialArena* last_serial_arena;
  };

  // Lifecycle_id can be highly contended variable in a situation of lots of
  // arena creation. Make sure that other global variables are not sharing the
  // cacheline.
#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif
  struct alignas(64) CacheAlignedLifecycleIdGenerator {
    std::atomic<LifecycleIdAtomic> id;
  };
  static CacheAlignedLifecycleIdGenerator lifecycle_id_generator_;
#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
  // Android ndk does not support __thread keyword so we use a custom thread
  // local storage class we implemented.
  // iOS also does not support the __thread keyword.
  static ThreadCache& thread_cache();
#elif defined(PROTOBUF_USE_DLLS)
  // Thread local variables cannot be exposed through DLL interface but we can
  // wrap them in static functions.
  static ThreadCache& thread_cache();
#else
  static PROTOBUF_THREAD_LOCAL ThreadCache thread_cache_;
  static ThreadCache& thread_cache() { return thread_cache_; }
#endif

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ThreadSafeArena);
  // All protos have pointers back to the arena hence Arena must have
  // pointer stability.
  ThreadSafeArena(ThreadSafeArena&&) = delete;
  ThreadSafeArena& operator=(ThreadSafeArena&&) = delete;

 public:
  // kBlockHeaderSize is sizeof(Block), aligned up to the nearest multiple of 8
  // to protect the invariant that pos is always at a multiple of 8.
  static constexpr size_t kBlockHeaderSize = SerialArena::kBlockHeaderSize;
  static constexpr size_t kSerialArenaSize =
      (sizeof(SerialArena) + 7) & static_cast<size_t>(-8);
  static_assert(kBlockHeaderSize % 8 == 0,
                "kBlockHeaderSize must be a multiple of 8.");
  static_assert(kSerialArenaSize % 8 == 0,
                "kSerialArenaSize must be a multiple of 8.");
};

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include <google/protobuf/port_undef.inc>

#endif  // GOOGLE_PROTOBUF_ARENA_IMPL_H__
