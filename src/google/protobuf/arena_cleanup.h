// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
#define GOOGLE_PROTOBUF_ARENA_CLEANUP_H__

#include <cstddef>
#include <cstring>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/base/optimization.h"
#include "absl/base/prefetch.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

class SerialArena;

namespace cleanup {

// Helper function invoking the destructor of `object`
template <typename T>
void arena_destruct_object(void* PROTOBUF_NONNULL object) {
  reinterpret_cast<T*>(object)->~T();
}

// CleanupNode contains the object (`elem`) that needs to be
// destroyed, and the function to destroy it (`destructor`)
// elem must be aligned at minimum on a 4 byte boundary.
struct CleanupNode {
  // Optimization: performs a prefetch on the elem for the cleanup node. We
  // explicitly use NTA prefetch here to avoid polluting remote caches: we are
  // destroying these instances, there is no purpose for these cache lines to
  // linger around in remote caches.
  ABSL_ATTRIBUTE_ALWAYS_INLINE void Prefetch() {
    // TODO: we should also prefetch the destructor code once
    // processors support code prefetching.
    absl::PrefetchToLocalCacheNta(elem);
  }

  // Destroys the object referenced by the cleanup node.
  ABSL_ATTRIBUTE_ALWAYS_INLINE void Destroy() { destructor(elem); }

  void* PROTOBUF_NONNULL elem;
  void (*PROTOBUF_NONNULL destructor)(void* PROTOBUF_NONNULL);
};

// Manages the list of cleanup nodes in a chunked linked list. Chunks grow by
// factors of two up to a limit. Trivially destructible, but Cleanup() must be
// called before destruction.
class ChunkList {
 public:
  PROTOBUF_ALWAYS_INLINE void Add(
      void* PROTOBUF_NONNULL elem,
      void (*PROTOBUF_NONNULL destructor)(void* PROTOBUF_NONNULL),
      SerialArena& arena) {
    if (ABSL_PREDICT_TRUE(next_ < limit_)) {
      AddFromExisting(elem, destructor);
      return;
    }
    AddFallback(elem, destructor, arena);
  }

  // Runs all inserted cleanups and frees allocated chunks. Must be called
  // before destruction.
  void Cleanup(const SerialArena& arena);

 private:
  struct Chunk;
  friend class internal::SerialArena;

  void AddFallback(void* PROTOBUF_NONNULL elem,
                   void (*PROTOBUF_NONNULL destructor)(void* PROTOBUF_NONNULL),
                   SerialArena& arena);
  ABSL_ATTRIBUTE_ALWAYS_INLINE void AddFromExisting(
      void* PROTOBUF_NONNULL elem,
      void (*PROTOBUF_NONNULL destructor)(void* PROTOBUF_NONNULL)) {
    *next_++ = CleanupNode{elem, destructor};
  }

  // Returns the pointers to the to-be-cleaned objects.
  std::vector<void*> PeekForTesting();

  Chunk* PROTOBUF_NULLABLE head_ = nullptr;
  CleanupNode* PROTOBUF_NULLABLE next_ = nullptr;
  CleanupNode* PROTOBUF_NULLABLE limit_ = nullptr;
  // Current prefetch position. Data from `next_` up to but not including
  // `prefetch_ptr_` is software prefetched. Used in SerialArena prefetching.
  const char* PROTOBUF_NULLABLE prefetch_ptr_ = nullptr;
};

}  // namespace cleanup
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
