// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/arena.h"

#ifdef UPB_TRACING_ENABLED
#include <stdatomic.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/alloc.h"
#include "upb/mem/internal/arena.h"
#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"

static UPB_ATOMIC(size_t) g_max_block_size = UPB_DEFAULT_MAX_BLOCK_SIZE;

void upb_Arena_SetMaxBlockSize(size_t max) {
  upb_Atomic_Store(&g_max_block_size, max, memory_order_relaxed);
}

typedef struct upb_MemBlock {
  struct upb_MemBlock* next;
  size_t size;
  // Data follows.
} upb_MemBlock;

typedef struct upb_ArenaInternal {
  // upb_alloc* together with a low bit which signals if there is an initial
  // block.
  uintptr_t block_alloc;

  // The cleanup for the allocator. This is called after all the blocks are
  // freed in an arena.
  upb_AllocCleanupFunc* upb_alloc_cleanup;

  // When multiple arenas are fused together, each arena points to a parent
  // arena (root points to itself). The root tracks how many live arenas
  // reference it.

  // The low bit is tagged:
  //   0: pointer to parent
  //   1: count, left shifted by one
  UPB_ATOMIC(uintptr_t) parent_or_count;

  // All nodes that are fused together are in a singly-linked list.
  // == NULL at end of list.
  UPB_ATOMIC(struct upb_ArenaInternal*) next;

  // If the low bit is set, is a pointer to the tail of the list (populated for
  // roots, set to self for roots with no fused arenas). If the low bit is not
  // set, is a pointer to the previous node in the list, such that
  // a->previous_or_tail->next == a.
  UPB_ATOMIC(uintptr_t) previous_or_tail;

  // Linked list of blocks to free/cleanup.
  upb_MemBlock* blocks;

  // Total space allocated in blocks, atomic only for SpaceAllocated
  UPB_ATOMIC(uintptr_t) space_allocated;

  UPB_TSAN_PUBLISHED_MEMBER
} upb_ArenaInternal;

// All public + private state for an arena.
typedef struct {
  upb_Arena head;
  upb_ArenaInternal body;
} upb_ArenaState;

typedef struct {
  upb_ArenaInternal* root;
  uintptr_t tagged_count;
} upb_ArenaRoot;

static const size_t kUpb_MemblockReserve =
    UPB_ALIGN_MALLOC(sizeof(upb_MemBlock));

// Extracts the (upb_ArenaInternal*) from a (upb_Arena*)
static upb_ArenaInternal* upb_Arena_Internal(const upb_Arena* a) {
  return &((upb_ArenaState*)a)->body;
}

static bool _upb_Arena_IsTaggedRefcount(uintptr_t parent_or_count) {
  return (parent_or_count & 1) == 1;
}

static bool _upb_Arena_IsTaggedPointer(uintptr_t parent_or_count) {
  return (parent_or_count & 1) == 0;
}

static uintptr_t _upb_Arena_RefCountFromTagged(uintptr_t parent_or_count) {
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(parent_or_count));
  return parent_or_count >> 1;
}

static uintptr_t _upb_Arena_TaggedFromRefcount(uintptr_t refcount) {
  uintptr_t parent_or_count = (refcount << 1) | 1;
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(parent_or_count));
  return parent_or_count;
}

static upb_ArenaInternal* _upb_Arena_PointerFromTagged(
    uintptr_t parent_or_count) {
  UPB_ASSERT(_upb_Arena_IsTaggedPointer(parent_or_count));
  return (upb_ArenaInternal*)parent_or_count;
}

static uintptr_t _upb_Arena_TaggedFromPointer(upb_ArenaInternal* ai) {
  uintptr_t parent_or_count = (uintptr_t)ai;
  UPB_ASSERT(_upb_Arena_IsTaggedPointer(parent_or_count));
  return parent_or_count;
}

static bool _upb_Arena_IsTaggedTail(uintptr_t previous_or_tail) {
  return (previous_or_tail & 1) == 1;
}

static bool _upb_Arena_IsTaggedPrevious(uintptr_t previous_or_tail) {
  return (previous_or_tail & 1) == 0;
}

static upb_ArenaInternal* _upb_Arena_TailFromTagged(
    uintptr_t previous_or_tail) {
  UPB_ASSERT(_upb_Arena_IsTaggedTail(previous_or_tail));
  return (upb_ArenaInternal*)(previous_or_tail ^ 1);
}

static uintptr_t _upb_Arena_TaggedFromTail(upb_ArenaInternal* tail) {
  uintptr_t previous_or_tail = (uintptr_t)tail | 1;
  UPB_ASSERT(_upb_Arena_IsTaggedTail(previous_or_tail));
  return previous_or_tail;
}

static upb_ArenaInternal* _upb_Arena_PreviousFromTagged(
    uintptr_t previous_or_tail) {
  UPB_ASSERT(_upb_Arena_IsTaggedPrevious(previous_or_tail));
  return (upb_ArenaInternal*)previous_or_tail;
}

static uintptr_t _upb_Arena_TaggedFromPrevious(upb_ArenaInternal* ai) {
  uintptr_t previous = (uintptr_t)ai;
  UPB_ASSERT(_upb_Arena_IsTaggedPrevious(previous));
  return previous;
}

static upb_alloc* _upb_ArenaInternal_BlockAlloc(upb_ArenaInternal* ai) {
  return (upb_alloc*)(ai->block_alloc & ~0x1);
}

static uintptr_t _upb_Arena_MakeBlockAlloc(upb_alloc* alloc, bool has_initial) {
  uintptr_t alloc_uint = (uintptr_t)alloc;
  UPB_ASSERT((alloc_uint & 1) == 0);
  return alloc_uint | (has_initial ? 1 : 0);
}

static bool _upb_ArenaInternal_HasInitialBlock(upb_ArenaInternal* ai) {
  return ai->block_alloc & 0x1;
}

#ifdef UPB_TRACING_ENABLED
static void (*_init_arena_trace_handler)(const upb_Arena*, size_t size) = NULL;
static void (*_fuse_arena_trace_handler)(const upb_Arena*,
                                         const upb_Arena*) = NULL;
static void (*_free_arena_trace_handler)(const upb_Arena*) = NULL;

void upb_Arena_SetTraceHandler(
    void (*initArenaTraceHandler)(const upb_Arena*, size_t size),
    void (*fuseArenaTraceHandler)(const upb_Arena*, const upb_Arena*),
    void (*freeArenaTraceHandler)(const upb_Arena*)) {
  _init_arena_trace_handler = initArenaTraceHandler;
  _fuse_arena_trace_handler = fuseArenaTraceHandler;
  _free_arena_trace_handler = freeArenaTraceHandler;
}

void upb_Arena_LogInit(const upb_Arena* arena, size_t size) {
  if (_init_arena_trace_handler) {
    _init_arena_trace_handler(arena, size);
  }
}
void upb_Arena_LogFuse(const upb_Arena* arena1, const upb_Arena* arena2) {
  if (_fuse_arena_trace_handler) {
    _fuse_arena_trace_handler(arena1, arena2);
  }
}
void upb_Arena_LogFree(const upb_Arena* arena) {
  if (_free_arena_trace_handler) {
    _free_arena_trace_handler(arena);
  }
}
#endif  // UPB_TRACING_ENABLED

// If the param a is already the root, provides no memory order of refcount.
// If it has a parent, then acquire memory order is provided for both the root
// and the refcount. Thread safe.
static upb_ArenaRoot _upb_Arena_FindRoot(upb_ArenaInternal* ai) {
  uintptr_t poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_relaxed);
  if (_upb_Arena_IsTaggedRefcount(poc)) {
    // Fast, relaxed path - arenas that have never been fused to a parent only
    // need relaxed memory order, since they're returning themselves and the
    // refcount.
    return (upb_ArenaRoot){.root = ai, .tagged_count = poc};
  }
  // Slow path needs acquire order; reloading is cheaper than a fence on ARM
  // (LDA vs DMB ISH). Even though this is a reread, we know it must be a tagged
  // pointer because if this Arena isn't a root, it can't ever become one.
  poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_acquire);
  do {
    upb_ArenaInternal* next = _upb_Arena_PointerFromTagged(poc);
    UPB_TSAN_CHECK_PUBLISHED(next);
    UPB_ASSERT(ai != next);
    poc = upb_Atomic_Load(&next->parent_or_count, memory_order_acquire);

    if (_upb_Arena_IsTaggedPointer(poc)) {
      // To keep complexity down, we lazily collapse levels of the tree.  This
      // keeps it flat in the final case, but doesn't cost much incrementally.
      //
      // Path splitting keeps time complexity down, see:
      //   https://en.wikipedia.org/wiki/Disjoint-set_data_structure
      UPB_ASSERT(ai != _upb_Arena_PointerFromTagged(poc));
      upb_Atomic_Store(&ai->parent_or_count, poc, memory_order_release);
    }
    ai = next;
  } while (_upb_Arena_IsTaggedPointer(poc));
  return (upb_ArenaRoot){.root = ai, .tagged_count = poc};
}

uintptr_t upb_Arena_SpaceAllocated(const upb_Arena* arena,
                                   size_t* fused_count) {
  upb_ArenaInternal* ai = upb_Arena_Internal(arena);
  uintptr_t memsize = 0;
  size_t local_fused_count = 0;
  // Our root would get updated by any racing fuses before our target arena
  // became reachable from the root via the linked list; in order to preserve
  // monotonic output (any arena counted by a previous invocation is counted by
  // this one), we instead iterate forwards and backwards so that we only see
  // the results of completed fuses.
  uintptr_t previous_or_tail =
      upb_Atomic_Load(&ai->previous_or_tail, memory_order_acquire);
  while (_upb_Arena_IsTaggedPrevious(previous_or_tail)) {
    upb_ArenaInternal* previous =
        _upb_Arena_PreviousFromTagged(previous_or_tail);
    UPB_ASSERT(previous != ai);
    UPB_TSAN_CHECK_PUBLISHED(previous);
    // Unfortunate macro behavior; prior to C11 when using nonstandard atomics
    // this returns a void* and can't be used with += without an intermediate
    // conversion to an integer.
    // Relaxed is safe - no subsequent reads depend this one
    uintptr_t allocated =
        upb_Atomic_Load(&previous->space_allocated, memory_order_relaxed);
    memsize += allocated;
    previous_or_tail =
        upb_Atomic_Load(&previous->previous_or_tail, memory_order_acquire);
    local_fused_count++;
  }
  while (ai != NULL) {
    UPB_TSAN_CHECK_PUBLISHED(ai);
    // Unfortunate macro behavior; prior to C11 when using nonstandard atomics
    // this returns a void* and can't be used with += without an intermediate
    // conversion to an integer.
    // Relaxed is safe - no subsequent reads depend this one
    uintptr_t allocated =
        upb_Atomic_Load(&ai->space_allocated, memory_order_relaxed);
    memsize += allocated;
    ai = upb_Atomic_Load(&ai->next, memory_order_acquire);
    local_fused_count++;
  }

  if (fused_count) *fused_count = local_fused_count;
  return memsize;
}

uint32_t upb_Arena_DebugRefCount(const upb_Arena* a) {
  uintptr_t tagged = _upb_Arena_FindRoot(upb_Arena_Internal(a)).tagged_count;
  return (uint32_t)_upb_Arena_RefCountFromTagged(tagged);
}

static void _upb_Arena_AddBlock(upb_Arena* a, void* ptr, size_t offset,
                                size_t block_size) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  upb_MemBlock* block = ptr;

  block->size = block_size;
  // Insert into linked list.
  block->next = ai->blocks;
  ai->blocks = block;

  UPB_ASSERT(offset >= kUpb_MemblockReserve);
  a->UPB_PRIVATE(ptr) = UPB_PTR_AT(block, offset, char);
  a->UPB_PRIVATE(end) = UPB_PTR_AT(block, block_size, char);

  UPB_POISON_MEMORY_REGION(a->UPB_PRIVATE(ptr),
                           a->UPB_PRIVATE(end) - a->UPB_PRIVATE(ptr));
}

static bool _upb_Arena_AllocBlock(upb_Arena* a, size_t size) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  if (!ai->block_alloc) return false;
  size_t last_size = 128;
  upb_MemBlock* last_block = ai->blocks;
  if (last_block) {
    last_size = a->UPB_PRIVATE(end) - (char*)last_block;
  }

  // Relaxed order is safe here as we don't need any ordering with the setter.
  size_t max_block_size =
      upb_Atomic_Load(&g_max_block_size, memory_order_relaxed);

  // Don't naturally grow beyond the max block size.
  size_t clamped_size = UPB_MIN(last_size * 2, max_block_size);

  // We may need to exceed the max block size if the user requested a large
  // allocation.
  size_t block_size = UPB_MAX(kUpb_MemblockReserve + size, clamped_size);

  upb_MemBlock* block =
      upb_malloc(_upb_ArenaInternal_BlockAlloc(ai), block_size);

  if (!block) return false;
  _upb_Arena_AddBlock(a, block, kUpb_MemblockReserve, block_size);
  // Atomic add not required here, as threads won't race allocating blocks, plus
  // atomic fetch-add is slower than load/add/store on arm devices compiled
  // targetting pre-v8.1. Relaxed order is safe as nothing depends on order of
  // size allocated.

  uintptr_t old_space_allocated =
      upb_Atomic_Load(&ai->space_allocated, memory_order_relaxed);
  upb_Atomic_Store(&ai->space_allocated, old_space_allocated + block_size,
                   memory_order_relaxed);
  UPB_ASSERT(UPB_PRIVATE(_upb_ArenaHas)(a) >= size);
  return true;
}

void* UPB_PRIVATE(_upb_Arena_SlowMalloc)(upb_Arena* a, size_t size) {
  if (!_upb_Arena_AllocBlock(a, size)) return NULL;  // OOM
  return upb_Arena_Malloc(a, size - UPB_ASAN_GUARD_SIZE);
}

static upb_Arena* _upb_Arena_InitSlow(upb_alloc* alloc, size_t first_size) {
  const size_t first_block_overhead =
      UPB_ALIGN_MALLOC(kUpb_MemblockReserve + sizeof(upb_ArenaState));
  upb_ArenaState* a;

  // We need to malloc the initial block.
  char* mem;
  size_t block_size =
      first_block_overhead +
      UPB_MAX(256, UPB_ALIGN_MALLOC(first_size) + UPB_ASAN_GUARD_SIZE);
  if (!alloc || !(mem = upb_malloc(alloc, block_size))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, kUpb_MemblockReserve, upb_ArenaState);

  a->body.block_alloc = _upb_Arena_MakeBlockAlloc(alloc, 0);
  upb_Atomic_Init(&a->body.parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&a->body.next, NULL);
  upb_Atomic_Init(&a->body.previous_or_tail,
                  _upb_Arena_TaggedFromTail(&a->body));
  upb_Atomic_Init(&a->body.space_allocated, block_size);
  a->body.blocks = NULL;
  a->body.upb_alloc_cleanup = NULL;
  UPB_TSAN_INIT_PUBLISHED(&a->body);

  _upb_Arena_AddBlock(&a->head, mem, first_block_overhead, block_size);

  return &a->head;
}

upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc) {
  UPB_ASSERT(sizeof(void*) * UPB_ARENA_SIZE_HACK >= sizeof(upb_ArenaState));
  upb_ArenaState* a;

  if (mem) {
    /* Align initial pointer up so that we return properly-aligned pointers. */
    void* aligned = (void*)UPB_ALIGN_MALLOC((uintptr_t)mem);
    size_t delta = (uintptr_t)aligned - (uintptr_t)mem;
    n = delta <= n ? n - delta : 0;
    mem = aligned;
  }
  if (UPB_UNLIKELY(n < sizeof(upb_ArenaState) || !mem)) {
    upb_Arena* ret = _upb_Arena_InitSlow(alloc, mem ? 0 : n);
#ifdef UPB_TRACING_ENABLED
    upb_Arena_LogInit(ret, n);
#endif
    return ret;
  }

  a = mem;

  upb_Atomic_Init(&a->body.parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&a->body.next, NULL);
  upb_Atomic_Init(&a->body.previous_or_tail,
                  _upb_Arena_TaggedFromTail(&a->body));
  upb_Atomic_Init(&a->body.space_allocated, 0);
  a->body.blocks = NULL;
  a->body.upb_alloc_cleanup = NULL;
  a->body.block_alloc = _upb_Arena_MakeBlockAlloc(alloc, 1);
  a->head.UPB_PRIVATE(ptr) = (void*)UPB_ALIGN_MALLOC((uintptr_t)(a + 1));
  a->head.UPB_PRIVATE(end) = UPB_PTR_AT(mem, n, char);
  UPB_TSAN_INIT_PUBLISHED(&a->body);
#ifdef UPB_TRACING_ENABLED
  upb_Arena_LogInit(&a->head, n);
#endif
  return &a->head;
}

static void _upb_Arena_DoFree(upb_ArenaInternal* ai) {
  UPB_ASSERT(_upb_Arena_RefCountFromTagged(ai->parent_or_count) == 1);
  while (ai != NULL) {
    UPB_TSAN_CHECK_PUBLISHED(ai);
    // Load first since arena itself is likely from one of its blocks.
    upb_ArenaInternal* next_arena =
        (upb_ArenaInternal*)upb_Atomic_Load(&ai->next, memory_order_acquire);
    // Freeing may have memory barriers that confuse tsan, so assert immdiately
    // after load here
    if (next_arena) {
      UPB_TSAN_CHECK_PUBLISHED(next_arena);
    }
    upb_alloc* block_alloc = _upb_ArenaInternal_BlockAlloc(ai);
    upb_MemBlock* block = ai->blocks;
    upb_AllocCleanupFunc* alloc_cleanup = *ai->upb_alloc_cleanup;
    while (block != NULL) {
      // Load first since we are deleting block.
      upb_MemBlock* next_block = block->next;
      upb_free_sized(block_alloc, block, block->size);
      block = next_block;
    }
    if (alloc_cleanup != NULL) {
      alloc_cleanup(block_alloc);
    }
    ai = next_arena;
  }
}

void upb_Arena_Free(upb_Arena* a) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  // Cannot be replaced with _upb_Arena_FindRoot, as that provides only a
  // relaxed read of the refcount if ai is already the root.
  uintptr_t poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_acquire);
retry:
  while (_upb_Arena_IsTaggedPointer(poc)) {
    ai = _upb_Arena_PointerFromTagged(poc);
    UPB_TSAN_CHECK_PUBLISHED(ai);
    poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_acquire);
  }

  // compare_exchange or fetch_sub are RMW operations, which are more
  // expensive then direct loads.  As an optimization, we only do RMW ops
  // when we need to update things for other threads to see.
  if (poc == _upb_Arena_TaggedFromRefcount(1)) {
#ifdef UPB_TRACING_ENABLED
    upb_Arena_LogFree(a);
#endif
    _upb_Arena_DoFree(ai);
    return;
  }

  if (upb_Atomic_CompareExchangeWeak(
          &ai->parent_or_count, &poc,
          _upb_Arena_TaggedFromRefcount(_upb_Arena_RefCountFromTagged(poc) - 1),
          memory_order_release, memory_order_acquire)) {
    // We were >1 and we decremented it successfully, so we are done.
    return;
  }

  // We failed our update, so someone has done something, retry the whole
  // process, but the failed exchange reloaded `poc` for us.
  goto retry;
}

static void _upb_Arena_DoFuseArenaLists(upb_ArenaInternal* const parent,
                                        upb_ArenaInternal* child) {
  UPB_TSAN_CHECK_PUBLISHED(parent);
  uintptr_t parent_previous_or_tail =
      upb_Atomic_Load(&parent->previous_or_tail, memory_order_acquire);
  upb_ArenaInternal* parent_tail = parent;
  if (_upb_Arena_IsTaggedTail(parent_previous_or_tail)) {
    // Our tail might be stale, but it will always converge to the true tail.
    parent_tail = _upb_Arena_TailFromTagged(parent_previous_or_tail);
  }

  // Link parent to child going forwards
  while (true) {
    UPB_TSAN_CHECK_PUBLISHED(parent_tail);
    upb_ArenaInternal* parent_tail_next =
        upb_Atomic_Load(&parent_tail->next, memory_order_acquire);
    while (parent_tail_next != NULL) {
      parent_tail = parent_tail_next;
      UPB_TSAN_CHECK_PUBLISHED(parent_tail);
      parent_tail_next =
          upb_Atomic_Load(&parent_tail->next, memory_order_acquire);
    }
    if (upb_Atomic_CompareExchangeWeak(&parent_tail->next, &parent_tail_next,
                                       child, memory_order_release,
                                       memory_order_acquire)) {
      break;
    }
    if (parent_tail_next != NULL) {
      parent_tail = parent_tail_next;
    }
  }

  // Update parent's tail (may be stale).
  uintptr_t child_previous_or_tail =
      upb_Atomic_Load(&child->previous_or_tail, memory_order_acquire);
  upb_ArenaInternal* new_parent_tail =
      _upb_Arena_TailFromTagged(child_previous_or_tail);
  UPB_TSAN_CHECK_PUBLISHED(new_parent_tail);

  // If another thread fused with us, don't overwrite their previous pointer
  // with our tail. Relaxed order is fine here as we only inspect the tag bit
  parent_previous_or_tail =
      upb_Atomic_Load(&parent->previous_or_tail, memory_order_relaxed);
  if (_upb_Arena_IsTaggedTail(parent_previous_or_tail)) {
    upb_Atomic_CompareExchangeStrong(
        &parent->previous_or_tail, &parent_previous_or_tail,
        _upb_Arena_TaggedFromTail(new_parent_tail), memory_order_release,
        memory_order_relaxed);
  }

  // Link child to parent going backwards, for SpaceAllocated
  upb_Atomic_Store(&child->previous_or_tail,
                   _upb_Arena_TaggedFromPrevious(parent_tail),
                   memory_order_release);
}

void upb_Arena_SetAllocCleanup(upb_Arena* a, upb_AllocCleanupFunc* func) {
  UPB_TSAN_CHECK_READ(a->UPB_ONLYBITS(ptr));
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  UPB_ASSERT(ai->upb_alloc_cleanup == NULL);
  ai->upb_alloc_cleanup = func;
}

// Thread safe.
static upb_ArenaInternal* _upb_Arena_DoFuse(upb_ArenaInternal** ai1,
                                            upb_ArenaInternal** ai2,
                                            uintptr_t* ref_delta) {
  // `parent_or_count` has two distinct modes
  // -  parent pointer mode
  // -  refcount mode
  //
  // In parent pointer mode, it may change what pointer it refers to in the
  // tree, but it will always approach a root.  Any operation that walks the
  // tree to the root may collapse levels of the tree concurrently.
  upb_ArenaRoot r1 = _upb_Arena_FindRoot(*ai1);
  upb_ArenaRoot r2 = _upb_Arena_FindRoot(*ai2);

  if (r1.root == r2.root) return r1.root;  // Already fused.

  *ai1 = r1.root;
  *ai2 = r2.root;

  // Avoid cycles by always fusing into the root with the lower address.
  if ((uintptr_t)r1.root > (uintptr_t)r2.root) {
    upb_ArenaRoot tmp = r1;
    r1 = r2;
    r2 = tmp;
  }

  // The moment we install `r1` as the parent for `r2` all racing frees may
  // immediately begin decrementing `r1`'s refcount (including pending
  // increments to that refcount and their frees!).  We need to add `r2`'s refs
  // now, so that `r1` can withstand any unrefs that come from r2.
  //
  // Note that while it is possible for `r2`'s refcount to increase
  // asynchronously, we will not actually do the reparenting operation below
  // unless `r2`'s refcount is unchanged from when we read it.
  //
  // Note that we may have done this previously, either to this node or a
  // different node, during a previous and failed DoFuse() attempt. But we will
  // not lose track of these refs because we always add them to our overall
  // delta.
  uintptr_t r2_untagged_count = r2.tagged_count & ~1;
  uintptr_t with_r2_refs = r1.tagged_count + r2_untagged_count;
  if (!upb_Atomic_CompareExchangeStrong(
          &r1.root->parent_or_count, &r1.tagged_count, with_r2_refs,
          memory_order_release, memory_order_acquire)) {
    return NULL;
  }

  // Perform the actual fuse by removing the refs from `r2` and swapping in the
  // parent pointer.
  if (!upb_Atomic_CompareExchangeStrong(
          &r2.root->parent_or_count, &r2.tagged_count,
          _upb_Arena_TaggedFromPointer(r1.root), memory_order_release,
          memory_order_acquire)) {
    // We'll need to remove the excess refs we added to r1 previously.
    *ref_delta += r2_untagged_count;
    return NULL;
  }

  // Now that the fuse has been performed (and can no longer fail) we need to
  // append `r2` to `r1`'s linked list.
  _upb_Arena_DoFuseArenaLists(r1.root, r2.root);
  return r1.root;
}

// Thread safe.
static bool _upb_Arena_FixupRefs(upb_ArenaInternal* new_root,
                                 uintptr_t ref_delta) {
  if (ref_delta == 0) return true;  // No fixup required.
  // Relaxed order is safe here as if the value is a pointer, we don't deref it
  // or publish it anywhere else. The refcount does provide memory order
  // between allocations on arenas and the eventual free and thus normally
  // requires acquire/release; but in this case any edges provided by the refs
  // we are cleaning up were already provided by the fuse operation itself. It's
  // not valid for a decrement that could cause the overall fused arena to reach
  // a zero refcount to race with this function, as that could result in a
  // use-after-free anyway.
  uintptr_t poc =
      upb_Atomic_Load(&new_root->parent_or_count, memory_order_relaxed);
  if (_upb_Arena_IsTaggedPointer(poc)) return false;
  uintptr_t with_refs = poc - ref_delta;
  UPB_ASSERT(!_upb_Arena_IsTaggedPointer(with_refs));
  // Relaxed order on success is safe here, for the same reasons as the relaxed
  // read above. Relaxed order is safe on failure because the updated value is
  // stored in a local variable which goes immediately out of scope; the retry
  // loop will reread what it needs with proper memory order.
  return upb_Atomic_CompareExchangeStrong(&new_root->parent_or_count, &poc,
                                          with_refs, memory_order_relaxed,
                                          memory_order_relaxed);
}

bool upb_Arena_Fuse(const upb_Arena* a1, const upb_Arena* a2) {
  if (a1 == a2) return true;  // trivial fuse

#ifdef UPB_TRACING_ENABLED
  upb_Arena_LogFuse(a1, a2);
#endif

  upb_ArenaInternal* ai1 = upb_Arena_Internal(a1);
  upb_ArenaInternal* ai2 = upb_Arena_Internal(a2);

  // Do not fuse initial blocks since we cannot lifetime extend them.
  // Any other fuse scenario is allowed.
  if (_upb_ArenaInternal_HasInitialBlock(ai1) ||
      _upb_ArenaInternal_HasInitialBlock(ai2)) {
    return false;
  }

  // The number of refs we ultimately need to transfer to the new root.
  uintptr_t ref_delta = 0;
  while (true) {
    upb_ArenaInternal* new_root = _upb_Arena_DoFuse(&ai1, &ai2, &ref_delta);
    if (new_root != NULL && _upb_Arena_FixupRefs(new_root, ref_delta)) {
      return true;
    }
  }
}

bool upb_Arena_IsFused(const upb_Arena* a, const upb_Arena* b) {
  if (a == b) return true;  // trivial fuse
  upb_ArenaInternal* ra = _upb_Arena_FindRoot(upb_Arena_Internal(a)).root;
  upb_ArenaInternal* rb = upb_Arena_Internal(b);
  while (true) {
    rb = _upb_Arena_FindRoot(rb).root;
    if (ra == rb) return true;
    upb_ArenaInternal* tmp = _upb_Arena_FindRoot(ra).root;
    if (ra == tmp) return false;
    // a's root changed since we last checked.  Retry.
    ra = tmp;
  }
}

bool upb_Arena_IncRefFor(const upb_Arena* a, const void* owner) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  if (_upb_ArenaInternal_HasInitialBlock(ai)) return false;
  upb_ArenaRoot r;
  r.root = ai;

retry:
  r = _upb_Arena_FindRoot(r.root);
  if (upb_Atomic_CompareExchangeWeak(
          &r.root->parent_or_count, &r.tagged_count,
          _upb_Arena_TaggedFromRefcount(
              _upb_Arena_RefCountFromTagged(r.tagged_count) + 1),
          // Relaxed order is safe on success, incrementing the refcount
          // need not perform any synchronization with the eventual free of the
          // arena - that's provided by decrements.
          memory_order_relaxed,
          // Relaxed order is safe on failure as r.tagged_count is immediately
          // overwritten by retrying the find root operation.
          memory_order_relaxed)) {
    // We incremented it successfully, so we are done.
    return true;
  }
  // We failed update due to parent switching on the arena.
  goto retry;
}

void upb_Arena_DecRefFor(const upb_Arena* a, const void* owner) {
  upb_Arena_Free((upb_Arena*)a);
}

upb_alloc* upb_Arena_GetUpbAlloc(upb_Arena* a) {
  UPB_TSAN_CHECK_READ(a->UPB_ONLYBITS(ptr));
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  return _upb_ArenaInternal_BlockAlloc(ai);
}

void UPB_PRIVATE(_upb_Arena_SwapIn)(upb_Arena* des, const upb_Arena* src) {
  upb_ArenaInternal* desi = upb_Arena_Internal(des);
  upb_ArenaInternal* srci = upb_Arena_Internal(src);

  *des = *src;
  desi->block_alloc = srci->block_alloc;
  desi->blocks = srci->blocks;
}

void UPB_PRIVATE(_upb_Arena_SwapOut)(upb_Arena* des, const upb_Arena* src) {
  upb_ArenaInternal* desi = upb_Arena_Internal(des);
  upb_ArenaInternal* srci = upb_Arena_Internal(src);

  *des = *src;
  desi->blocks = srci->blocks;
}
