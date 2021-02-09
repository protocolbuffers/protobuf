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

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/logging.h>

#ifdef ADDRESS_SANITIZER
#include <sanitizer/asan_interface.h>
#endif  // ADDRESS_SANITIZER

#include <google/protobuf/port_def.inc>


namespace google {
namespace protobuf {

struct ArenaOptions;

namespace internal {

inline size_t AlignUpTo8(size_t n) {
  // Align n to next multiple of 8 (from Hacker's Delight, Chapter 3.)
  return (n + 7) & static_cast<size_t>(-8);
}

using LifecycleIdAtomic = uint64_t;

void PROTOBUF_EXPORT ArenaFree(void* object, size_t size);

// MetricsCollector collects stats for a particular arena.
class PROTOBUF_EXPORT ArenaMetricsCollector {
 public:
  virtual ~ArenaMetricsCollector();

  // Invoked when the arena is about to be destroyed. This method will
  // typically finalize any metric collection and delete the collector.
  // space_allocated is the space used by the arena.
  virtual void OnDestroy(uint64 space_allocated) = 0;

  // OnReset() is called when the associated arena is reset.
  // space_allocated is the space used by the arena just before the reset.
  virtual void OnReset(uint64 space_allocated) = 0;

  // Does OnAlloc() need to be called?  If false, metric collection overhead
  // will be reduced since we will not do extra work per allocation.
  virtual bool RecordAllocs() = 0;

  // OnAlloc is called when an allocation happens.
  // type_info is promised to be static - its lifetime extends to
  // match program's lifetime (It is given by typeid operator).
  // Note: typeid(void) will be passed as allocated_type every time we
  // intentionally want to avoid monitoring an allocation. (i.e. internal
  // allocations for managing the arena)
  virtual void OnAlloc(const std::type_info* allocated_type,
                       uint64 alloc_size) = 0;
};

class ArenaImpl;

// A thread-unsafe Arena that can only be used within its owning thread.
class PROTOBUF_EXPORT SerialArena {
 public:
  // Blocks are variable length malloc-ed objects.  The following structure
  // describes the common header for all blocks.
  class PROTOBUF_EXPORT Block {
   public:
    Block(size_t size, Block* next, bool special, bool user_owned)
        : next_and_bits_(reinterpret_cast<uintptr_t>(next) | (special ? 1 : 0) |
                         (user_owned ? 2 : 0)),
          pos_(kBlockHeaderSize),
          size_(size) {
      GOOGLE_DCHECK_EQ(reinterpret_cast<uintptr_t>(next) & 3, 0u);
    }

    char* Pointer(size_t n) {
      GOOGLE_DCHECK(n <= size_);
      return reinterpret_cast<char*>(this) + n;
    }

    // One of the blocks may be special. This is either a user-supplied
    // initial block, or a block we created at startup to hold Options info.
    // A special block is not deleted by Reset.
    bool special() const { return (next_and_bits_ & 1) != 0; }

    // Whether or not this current block is owned by the user.
    // Only special blocks can be user_owned.
    bool user_owned() const { return (next_and_bits_ & 2) != 0; }

    Block* next() const {
      const uintptr_t bottom_bits = 3;
      return reinterpret_cast<Block*>(next_and_bits_ & ~bottom_bits);
    }

    void clear_next() {
      next_and_bits_ &= 3;  // Set next to nullptr, preserve bottom bits.
    }

    size_t pos() const { return pos_; }
    size_t size() const { return size_; }
    void set_pos(size_t pos) { pos_ = pos; }

   private:
    // Holds pointer to next block for this thread + special/user_owned bits.
    uintptr_t next_and_bits_;

    size_t pos_;
    size_t size_;
    // data follows
  };

  // The allocate/free methods here are a little strange, since SerialArena is
  // allocated inside a Block which it also manages.  This is to avoid doing
  // an extra allocation for the SerialArena itself.

  // Creates a new SerialArena inside Block* and returns it.
  static SerialArena* New(Block* b, void* owner, ArenaImpl* arena);

  void CleanupList();
  uint64 SpaceUsed() const;

  bool HasSpace(size_t n) { return n <= static_cast<size_t>(limit_ - ptr_); }

  void* AllocateAligned(size_t n) {
    GOOGLE_DCHECK_EQ(internal::AlignUpTo8(n), n);  // Must be already aligned.
    GOOGLE_DCHECK_GE(limit_, ptr_);
    if (PROTOBUF_PREDICT_FALSE(!HasSpace(n))) {
      return AllocateAlignedFallback(n);
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

  void AddCleanup(void* elem, void (*cleanup)(void*)) {
    if (PROTOBUF_PREDICT_FALSE(cleanup_ptr_ == cleanup_limit_)) {
      AddCleanupFallback(elem, cleanup);
      return;
    }
    cleanup_ptr_->elem = elem;
    cleanup_ptr_->cleanup = cleanup;
    cleanup_ptr_++;
  }

  void* AllocateAlignedAndAddCleanup(size_t n, void (*cleanup)(void*)) {
    void* ret = AllocateAligned(n);
    AddCleanup(ret, cleanup);
    return ret;
  }

  Block* head() const { return head_; }
  void* owner() const { return owner_; }
  SerialArena* next() const { return next_; }
  void set_next(SerialArena* next) { next_ = next; }
  static Block* NewBlock(Block* last_block, size_t min_bytes, ArenaImpl* arena);

 private:
  // Node contains the ptr of the object to be cleaned up and the associated
  // cleanup function ptr.
  struct CleanupNode {
    void* elem;              // Pointer to the object to be cleaned up.
    void (*cleanup)(void*);  // Function pointer to the destructor or deleter.
  };

  // Cleanup uses a chunked linked list, to reduce pointer chasing.
  struct CleanupChunk {
    static size_t SizeOf(size_t i) {
      return sizeof(CleanupChunk) + (sizeof(CleanupNode) * (i - 1));
    }
    size_t size;           // Total elements in the list.
    CleanupChunk* next;    // Next node in the list.
    CleanupNode nodes[1];  // True length is |size|.
  };

  ArenaImpl* arena_;       // Containing arena.
  void* owner_;            // &ThreadCache of this thread;
  Block* head_;            // Head of linked list of blocks.
  CleanupChunk* cleanup_;  // Head of cleanup list.
  SerialArena* next_;      // Next SerialArena in this linked list.

  // Next pointer to allocate from.  Always 8-byte aligned.  Points inside
  // head_ (and head_->pos will always be non-canonical).  We keep these
  // here to reduce indirection.
  char* ptr_;
  char* limit_;

  // Next CleanupList members to append to.  These point inside cleanup_.
  CleanupNode* cleanup_ptr_;
  CleanupNode* cleanup_limit_;

  void* AllocateAlignedFallback(size_t n);
  void AddCleanupFallback(void* elem, void (*cleanup)(void*));
  void CleanupListFallback();

 public:
  static constexpr size_t kBlockHeaderSize =
      (sizeof(Block) + 7) & static_cast<size_t>(-8);
};

// This class provides the core Arena memory allocation library. Different
// implementations only need to implement the public interface below.
// Arena is not a template type as that would only be useful if all protos
// in turn would be templates, which will/cannot happen. However separating
// the memory allocation part from the cruft of the API users expect we can
// use #ifdef the select the best implementation based on hardware / OS.
class PROTOBUF_EXPORT ArenaImpl {
 public:
  static const size_t kDefaultStartBlockSize = 256;
  static const size_t kDefaultMaxBlockSize = 8192;

  ArenaImpl() { Init(false); }

  ArenaImpl(char* mem, size_t size) {
    GOOGLE_DCHECK_EQ(reinterpret_cast<uintptr_t>(mem) & 7, 0u);
    Init(false);

    // Ignore initial block if it is too small.
    if (mem != nullptr && size >= kBlockHeaderSize + kSerialArenaSize) {
      SetInitialBlock(new (mem) SerialArena::Block(size, nullptr, true, true));
    }
  }

  explicit ArenaImpl(const ArenaOptions& options);

  // Destructor deletes all owned heap allocated objects, and destructs objects
  // that have non-trivial destructors, except for proto2 message objects whose
  // destructors can be skipped. Also, frees all blocks except the initial block
  // if it was passed in.
  ~ArenaImpl();

  uint64 Reset();

  uint64 SpaceAllocated() const;
  uint64 SpaceUsed() const;

  void* AllocateAligned(size_t n) {
    SerialArena* arena;
    if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFast(&arena))) {
      return arena->AllocateAligned(n);
    } else {
      return AllocateAlignedFallback(n);
    }
  }

  // This function allocates n bytes if the common happy case is true and
  // returns true. Otherwise does nothing and returns false. This strange
  // semantics is necessary to allow callers to program functions that only
  // have fallback function calls in tail position. This substantially improves
  // code for the happy path.
  PROTOBUF_ALWAYS_INLINE bool MaybeAllocateAligned(size_t n, void** out) {
    SerialArena* a;
    if (PROTOBUF_PREDICT_TRUE(GetSerialArenaFromThreadCache(&a))) {
      return a->MaybeAllocateAligned(n, out);
    }
    return false;
  }

  void* AllocateAlignedAndAddCleanup(size_t n, void (*cleanup)(void*));

  // Add object pointer and cleanup function pointer to the list.
  void AddCleanup(void* elem, void (*cleanup)(void*));

  inline void RecordAlloc(const std::type_info* allocated_type,
                          size_t n) const {
    if (PROTOBUF_PREDICT_FALSE(record_allocs())) {
      options_->metrics_collector->OnAlloc(allocated_type, n);
    }
  }

  std::pair<void*, size_t> NewBuffer(size_t last_size, size_t min_bytes);

 private:
  // Pointer to a linked list of SerialArena.
  std::atomic<SerialArena*> threads_;
  std::atomic<SerialArena*> hint_;       // Fast thread-local block access
  std::atomic<size_t> space_allocated_;  // Total size of all allocated blocks.

  // Unique for each arena. Changes on Reset().
  // Least-significant-bit is 1 iff allocations should be recorded.
  uint64 lifecycle_id_;

  struct Options {
    size_t start_block_size;
    size_t max_block_size;
    void* (*block_alloc)(size_t);
    void (*block_dealloc)(void*, size_t);
    ArenaMetricsCollector* metrics_collector;
  };

  Options* options_ = nullptr;

  void* AllocateAlignedFallback(size_t n);
  void* AllocateAlignedAndAddCleanupFallback(size_t n, void (*cleanup)(void*));
  void AddCleanupFallback(void* elem, void (*cleanup)(void*));

  void Init(bool record_allocs);
  void SetInitialBlock(
      SerialArena::Block* block);  // Can be called right after Init()

  // Return true iff allocations should be recorded in a metrics collector.
  inline bool record_allocs() const { return lifecycle_id_ & 1; }

  // Invoke fn(b) for every Block* b.
  template <typename Functor>
  void PerBlock(Functor fn) {
    // By omitting an Acquire barrier we ensure that any user code that doesn't
    // properly synchronize Reset() or the destructor will throw a TSAN warning.
    SerialArena* serial = threads_.load(std::memory_order_relaxed);
    while (serial) {
      // fn() may delete blocks and arenas, so fetch next pointers before fn();
      SerialArena* cur = serial;
      serial = serial->next();
      for (auto* block = cur->head(); block != nullptr;) {
        auto* b = block;
        block = b->next();
        fn(b);
      }
    }
  }

  // Delete or Destruct all objects owned by the arena.
  void CleanupList();

  inline void CacheSerialArena(SerialArena* serial) {
    thread_cache().last_serial_arena = serial;
    thread_cache().last_lifecycle_id_seen = lifecycle_id_;
    // TODO(haberman): evaluate whether we would gain efficiency by getting rid
    // of hint_.  It's the only write we do to ArenaImpl in the allocation path,
    // which will dirty the cache line.

    hint_.store(serial, std::memory_order_release);
  }

  PROTOBUF_ALWAYS_INLINE bool GetSerialArenaFast(SerialArena** arena) {
    if (GetSerialArenaFromThreadCache(arena)) return true;

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

  PROTOBUF_ALWAYS_INLINE bool GetSerialArenaFromThreadCache(
      SerialArena** arena) {
    // If this thread already owns a block in this arena then try to use that.
    // This fast path optimizes the case where multiple threads allocate from
    // the same arena.
    ThreadCache* tc = &thread_cache();
    if (PROTOBUF_PREDICT_TRUE(tc->last_lifecycle_id_seen == lifecycle_id_)) {
      *arena = tc->last_serial_arena;
      return true;
    }
    return false;
  }
  SerialArena* GetSerialArenaFallback(void* me);

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

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ArenaImpl);
  // All protos have pointers back to the arena hence Arena must have
  // pointer stability.
  ArenaImpl(ArenaImpl&&) = delete;
  ArenaImpl& operator=(ArenaImpl&&) = delete;

 public:
  // kBlockHeaderSize is sizeof(Block), aligned up to the nearest multiple of 8
  // to protect the invariant that pos is always at a multiple of 8.
  static constexpr size_t kBlockHeaderSize = SerialArena::kBlockHeaderSize;
  static constexpr size_t kSerialArenaSize =
      (sizeof(SerialArena) + 7) & static_cast<size_t>(-8);
  static constexpr size_t kOptionsSize =
      (sizeof(Options) + 7) & static_cast<size_t>(-8);
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
