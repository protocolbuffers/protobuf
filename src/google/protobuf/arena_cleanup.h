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
#include "absl/log/absl_check.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {

class SerialArena;

namespace cleanup {

struct CleanupNode;
using Callback = void (*)(CleanupNode*, CleanupNode*, CleanupNode*);

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

  void* elem;
  Callback destructor;
};

template <typename Destroy>
void arena_run_destroy(CleanupNode* it, CleanupNode* prefetch,
                       CleanupNode* first) {
  static constexpr auto self = arena_run_destroy<Destroy>;
  ABSL_DCHECK_GE(it, first);
  ABSL_DCHECK_EQ(it->destructor, self);

  for (;;) {
    Destroy{}(it->elem);
    if (it == first || !PROTOBUF_TAILCALL) {
      return;
    }

    --it;
    if (prefetch > first) {
      prefetch->Prefetch();
      --prefetch;
    }
    if (it->destructor != self) {
      PROTOBUF_DEBUG_COUNTER("CleanupListInner.Diff").Inc();
      PROTOBUF_MUSTTAIL return it->destructor(it, prefetch, first);
    }
    PROTOBUF_DEBUG_COUNTER("CleanupListInner.Same").Inc();
  }
}

template <typename T>
struct Destroy {
  void operator()(void* p) const { reinterpret_cast<T*>(p)->~T(); }
};

// Helper function invoking the destructor of `object`
template <typename T>
inline constexpr Callback arena_destruct_object =
    &arena_run_destroy<Destroy<T>>;

// Manages the list of cleanup nodes in a chunked linked list. Chunks grow by
// factors of two up to a limit. Trivially destructible, but Cleanup() must be
// called before destruction.
class ChunkList {
 public:
  PROTOBUF_ALWAYS_INLINE void Add(void* elem, Callback destructor,
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

  void AddFallback(void* elem, Callback destructor, SerialArena& arena);
  ABSL_ATTRIBUTE_ALWAYS_INLINE void AddFromExisting(void* elem,
                                                    Callback destructor) {
    *next_++ = CleanupNode{elem, destructor};
  }

  // Returns the pointers to the to-be-cleaned objects.
  std::vector<void*> PeekForTesting();

  Chunk* head_ = nullptr;
  CleanupNode* next_ = nullptr;
  CleanupNode* limit_ = nullptr;
  // Current prefetch position. Data from `next_` up to but not including
  // `prefetch_ptr_` is software prefetched. Used in SerialArena prefetching.
  const char* prefetch_ptr_ = nullptr;
};

}  // namespace cleanup
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_ARENA_CLEANUP_H__
