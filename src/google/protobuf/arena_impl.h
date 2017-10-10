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

#include <limits>

#include <google/protobuf/stubs/atomic_sequence_num.h>
#include <google/protobuf/stubs/atomicops.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/mutex.h>
#include <google/protobuf/stubs/type_traits.h>

namespace google {

namespace protobuf {
namespace internal {

inline size_t AlignUpTo8(size_t n) {
  // Align n to next multiple of 8 (from Hacker's Delight, Chapter 3.)
  return (n + 7) & -8;
}

// This class provides the core Arena memory allocation library. Different
// implementations only need to implement the public interface below.
// Arena is not a template type as that would only be useful if all protos
// in turn would be templates, which will/cannot happen. However separating
// the memory allocation part from the cruft of the API users expect we can
// use #ifdef the select the best implementation based on hardware / OS.
class LIBPROTOBUF_EXPORT ArenaImpl {
 public:
  struct Options {
    size_t start_block_size;
    size_t max_block_size;
    char* initial_block;
    size_t initial_block_size;
    void* (*block_alloc)(size_t);
    void (*block_dealloc)(void*, size_t);

    template <typename O>
    explicit Options(const O& options)
      : start_block_size(options.start_block_size),
        max_block_size(options.max_block_size),
        initial_block(options.initial_block),
        initial_block_size(options.initial_block_size),
        block_alloc(options.block_alloc),
        block_dealloc(options.block_dealloc) {}
  };

  template <typename O>
  explicit ArenaImpl(const O& options) : options_(options) {
    Init();
  }

  // Destructor deletes all owned heap allocated objects, and destructs objects
  // that have non-trivial destructors, except for proto2 message objects whose
  // destructors can be skipped. Also, frees all blocks except the initial block
  // if it was passed in.
  ~ArenaImpl();

  uint64 Reset();

  uint64 SpaceAllocated() const;
  uint64 SpaceUsed() const;

  void* AllocateAligned(size_t n);

  void* AllocateAlignedAndAddCleanup(size_t n, void (*cleanup)(void*));

  // Add object pointer and cleanup function pointer to the list.
  void AddCleanup(void* elem, void (*cleanup)(void*));

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
    size_t len;            // Number of elements currently present.
    size_t size;           // Total elements in the list.
    CleanupChunk* next;    // Next node in the list.
    CleanupNode nodes[1];  // True length is |size|.
  };

  // Blocks are variable length malloc-ed objects.  The following structure
  // describes the common header for all blocks.
  struct Block {
    void* owner;            // &ThreadCache of thread that owns this block.
    Block* next;            // Next block in arena (may have different owner)
    CleanupChunk* cleanup;  // Head of cleanup list (may point to another block,
                            // but it must have the same owner).
    // ((char*) &block) + pos is next available byte. It is always
    // aligned at a multiple of 8 bytes.
    size_t pos;
    size_t size;  // total size of the block.
    GOOGLE_ATTRIBUTE_ALWAYS_INLINE size_t avail() const { return size - pos; }
    // data follows
  };

  struct ThreadCache {
    // The ThreadCache is considered valid as long as this matches the
    // lifecycle_id of the arena being used.
    int64 last_lifecycle_id_seen;
    Block* last_block_used_;
  };

  // kHeaderSize is sizeof(Block), aligned up to the nearest multiple of 8 to
  // protect the invariant that pos is always at a multiple of 8.
  static const size_t kHeaderSize = (sizeof(Block) + 7) & -8;
#if LANG_CXX11
  static_assert(kHeaderSize % 8 == 0, "kHeaderSize must be a multiple of 8.");
#endif
  static google::protobuf::internal::SequenceNumber lifecycle_id_generator_;
#if defined(GOOGLE_PROTOBUF_NO_THREADLOCAL)
  // Android ndk does not support GOOGLE_THREAD_LOCAL keyword so we use a custom thread
  // local storage class we implemented.
  // iOS also does not support the GOOGLE_THREAD_LOCAL keyword.
  static ThreadCache& thread_cache();
#elif defined(PROTOBUF_USE_DLLS)
  // Thread local variables cannot be exposed through DLL interface but we can
  // wrap them in static functions.
  static ThreadCache& thread_cache();
#else
  static GOOGLE_THREAD_LOCAL ThreadCache thread_cache_;
  static ThreadCache& thread_cache() { return thread_cache_; }
#endif

  void Init();

  // Free all blocks and return the total space used which is the sums of sizes
  // of the all the allocated blocks.
  uint64 FreeBlocks(Block* head);

  void AddCleanupInBlock(Block* b, void* elem, void (*cleanup)(void*));
  Block* ExpandCleanupList(Block* b);
  // Delete or Destruct all objects owned by the arena.
  void CleanupList(Block* head);
  uint64 ResetInternal();

  inline void CacheBlock(Block* block) {
    thread_cache().last_block_used_ = block;
    thread_cache().last_lifecycle_id_seen = lifecycle_id_;
    google::protobuf::internal::Release_Store(&hint_, reinterpret_cast<google::protobuf::internal::AtomicWord>(block));
  }

  google::protobuf::internal::AtomicWord blocks_;       // Head of linked list of all allocated blocks
  google::protobuf::internal::AtomicWord hint_;         // Fast thread-local block access
  uint64 space_allocated_;  // Sum of sizes of all allocated blocks.

  bool owns_first_block_;  // Indicates that arena owns the first block
  mutable Mutex blocks_lock_;

  void AddBlock(Block* b);
  // Access must be synchronized, either by blocks_lock_ or by being called from
  // Init()/Reset().
  void AddBlockInternal(Block* b);
  // Returns a block owned by this thread.
  Block* GetBlock(size_t n);
  Block* GetBlockSlow(void* me, Block* my_full_block, size_t n);
  Block* FindBlock(void* me);
  Block* NewBlock(void* me, Block* my_last_block, size_t min_bytes,
                  size_t start_block_size, size_t max_block_size);
  static void* AllocFromBlock(Block* b, size_t n);

  int64 lifecycle_id_;  // Unique for each arena. Changes on Reset().

  Options options_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ArenaImpl);
};

}  // namespace internal
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_ARENA_IMPL_H__
