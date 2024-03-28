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

typedef struct upb_MemBlock {
  // Atomic only for the benefit of SpaceAllocated().
  UPB_ATOMIC(struct upb_MemBlock*) next;
  uint32_t size;
  // Data follows.
} upb_MemBlock;

typedef struct upb_ArenaInternal {
  // upb_alloc* together with a low bit which signals if there is an initial
  // block.
  uintptr_t block_alloc;

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

  // The last element of the linked list. This is present only as an
  // optimization, so that we do not have to iterate over all members for every
  // fuse.  Only significant for an arena root. In other cases it is ignored.
  // == self when no other list members.
  UPB_ATOMIC(struct upb_ArenaInternal*) tail;

  // Linked list of blocks to free/cleanup. Atomic only for the benefit of
  // upb_Arena_SpaceAllocated().
  UPB_ATOMIC(upb_MemBlock*) blocks;
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
    UPB_ALIGN_UP(sizeof(upb_MemBlock), UPB_MALLOC_ALIGN);

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

static upb_ArenaRoot _upb_Arena_FindRoot(upb_Arena* a) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  uintptr_t poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_acquire);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    upb_ArenaInternal* next = _upb_Arena_PointerFromTagged(poc);
    UPB_ASSERT(ai != next);
    uintptr_t next_poc =
        upb_Atomic_Load(&next->parent_or_count, memory_order_acquire);

    if (_upb_Arena_IsTaggedPointer(next_poc)) {
      // To keep complexity down, we lazily collapse levels of the tree.  This
      // keeps it flat in the final case, but doesn't cost much incrementally.
      //
      // Path splitting keeps time complexity down, see:
      //   https://en.wikipedia.org/wiki/Disjoint-set_data_structure
      //
      // We can safely use a relaxed atomic here because all threads doing this
      // will converge on the same value and we don't need memory orderings to
      // be visible.
      //
      // This is true because:
      // - If no fuses occur, this will eventually become the root.
      // - If fuses are actively occurring, the root may change, but the
      //   invariant is that `parent_or_count` merely points to *a* parent.
      //
      // In other words, it is moving towards "the" root, and that root may move
      // further away over time, but the path towards that root will continue to
      // be valid and the creation of the path carries all the memory orderings
      // required.
      UPB_ASSERT(ai != _upb_Arena_PointerFromTagged(next_poc));
      upb_Atomic_Store(&ai->parent_or_count, next_poc, memory_order_relaxed);
    }
    ai = next;
    poc = next_poc;
  }
  return (upb_ArenaRoot){.root = ai, .tagged_count = poc};
}

size_t upb_Arena_SpaceAllocated(upb_Arena* arena, size_t* fused_count) {
  upb_ArenaInternal* ai = _upb_Arena_FindRoot(arena).root;
  size_t memsize = 0;
  size_t local_fused_count = 0;

  while (ai != NULL) {
    upb_MemBlock* block = upb_Atomic_Load(&ai->blocks, memory_order_relaxed);
    while (block != NULL) {
      memsize += sizeof(upb_MemBlock) + block->size;
      block = upb_Atomic_Load(&block->next, memory_order_relaxed);
    }
    ai = upb_Atomic_Load(&ai->next, memory_order_relaxed);
    local_fused_count++;
  }

  if (fused_count) *fused_count = local_fused_count;
  return memsize;
}

bool UPB_PRIVATE(_upb_Arena_Contains)(const upb_Arena* a, void* ptr) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  UPB_ASSERT(ai);

  upb_MemBlock* block = upb_Atomic_Load(&ai->blocks, memory_order_relaxed);
  while (block) {
    uintptr_t beg = (uintptr_t)block;
    uintptr_t end = beg + block->size;
    if ((uintptr_t)ptr >= beg && (uintptr_t)ptr < end) return true;
    block = upb_Atomic_Load(&block->next, memory_order_relaxed);
  }

  return false;
}

uint32_t upb_Arena_DebugRefCount(upb_Arena* a) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  // These loads could probably be relaxed, but given that this is debug-only,
  // it's not worth introducing a new variant for it.
  uintptr_t poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_acquire);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    ai = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_acquire);
  }
  return _upb_Arena_RefCountFromTagged(poc);
}

static void _upb_Arena_AddBlock(upb_Arena* a, void* ptr, size_t size) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  upb_MemBlock* block = ptr;

  // Insert into linked list.
  block->size = (uint32_t)size;
  upb_Atomic_Init(&block->next, ai->blocks);
  upb_Atomic_Store(&ai->blocks, block, memory_order_release);

  a->UPB_PRIVATE(ptr) = UPB_PTR_AT(block, kUpb_MemblockReserve, char);
  a->UPB_PRIVATE(end) = UPB_PTR_AT(block, size, char);

  UPB_POISON_MEMORY_REGION(a->UPB_PRIVATE(ptr),
                           a->UPB_PRIVATE(end) - a->UPB_PRIVATE(ptr));
}

static bool _upb_Arena_AllocBlock(upb_Arena* a, size_t size) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  if (!ai->block_alloc) return false;
  upb_MemBlock* last_block = upb_Atomic_Load(&ai->blocks, memory_order_acquire);
  size_t last_size = last_block != NULL ? last_block->size : 128;
  size_t block_size = UPB_MAX(size, last_size * 2) + kUpb_MemblockReserve;
  upb_MemBlock* block =
      upb_malloc(_upb_ArenaInternal_BlockAlloc(ai), block_size);

  if (!block) return false;
  _upb_Arena_AddBlock(a, block, block_size);
  UPB_ASSERT(UPB_PRIVATE(_upb_ArenaHas)(a) >= size);
  return true;
}

void* UPB_PRIVATE(_upb_Arena_SlowMalloc)(upb_Arena* a, size_t size) {
  if (!_upb_Arena_AllocBlock(a, size)) return NULL;  // OOM
  return upb_Arena_Malloc(a, size - UPB_ASAN_GUARD_SIZE);
}

static upb_Arena* _upb_Arena_InitSlow(upb_alloc* alloc) {
  const size_t first_block_overhead =
      sizeof(upb_ArenaState) + kUpb_MemblockReserve;
  upb_ArenaState* a;

  // We need to malloc the initial block.
  char* mem;
  size_t n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, n - sizeof(upb_ArenaState), upb_ArenaState);
  n -= sizeof(upb_ArenaState);

  a->body.block_alloc = _upb_Arena_MakeBlockAlloc(alloc, 0);
  upb_Atomic_Init(&a->body.parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&a->body.next, NULL);
  upb_Atomic_Init(&a->body.tail, &a->body);
  upb_Atomic_Init(&a->body.blocks, NULL);

  _upb_Arena_AddBlock(&a->head, mem, n);

  return &a->head;
}

upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc) {
  UPB_ASSERT(sizeof(void*) * UPB_ARENA_SIZE_HACK >= sizeof(upb_ArenaState));
  upb_ArenaState* a;

  if (n) {
    /* Align initial pointer up so that we return properly-aligned pointers. */
    void* aligned = (void*)UPB_ALIGN_UP((uintptr_t)mem, UPB_MALLOC_ALIGN);
    size_t delta = (uintptr_t)aligned - (uintptr_t)mem;
    n = delta <= n ? n - delta : 0;
    mem = aligned;
  }

  /* Round block size down to alignof(*a) since we will allocate the arena
   * itself at the end. */
  n = UPB_ALIGN_DOWN(n, UPB_ALIGN_OF(upb_ArenaState));

  if (UPB_UNLIKELY(n < sizeof(upb_ArenaState))) {
#ifdef UPB_TRACING_ENABLED
    upb_Arena* ret = _upb_Arena_InitSlow(alloc);
    upb_Arena_LogInit(ret, n);
    return ret;
#else
    return _upb_Arena_InitSlow(alloc);
#endif
  }

  a = UPB_PTR_AT(mem, n - sizeof(upb_ArenaState), upb_ArenaState);

  upb_Atomic_Init(&a->body.parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&a->body.next, NULL);
  upb_Atomic_Init(&a->body.tail, &a->body);
  upb_Atomic_Init(&a->body.blocks, NULL);

  a->body.block_alloc = _upb_Arena_MakeBlockAlloc(alloc, 1);
  a->head.UPB_PRIVATE(ptr) = mem;
  a->head.UPB_PRIVATE(end) = UPB_PTR_AT(mem, n - sizeof(upb_ArenaState), char);
#ifdef UPB_TRACING_ENABLED
  upb_Arena_LogInit(&a->head, n);
#endif
  return &a->head;
}

static void _upb_Arena_DoFree(upb_ArenaInternal* ai) {
  UPB_ASSERT(_upb_Arena_RefCountFromTagged(ai->parent_or_count) == 1);
  while (ai != NULL) {
    // Load first since arena itself is likely from one of its blocks.
    upb_ArenaInternal* next_arena =
        (upb_ArenaInternal*)upb_Atomic_Load(&ai->next, memory_order_acquire);
    upb_alloc* block_alloc = _upb_ArenaInternal_BlockAlloc(ai);
    upb_MemBlock* block = upb_Atomic_Load(&ai->blocks, memory_order_acquire);
    while (block != NULL) {
      // Load first since we are deleting block.
      upb_MemBlock* next_block =
          upb_Atomic_Load(&block->next, memory_order_acquire);
      upb_free(block_alloc, block);
      block = next_block;
    }
    ai = next_arena;
  }
}

void upb_Arena_Free(upb_Arena* a) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  uintptr_t poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_acquire);
retry:
  while (_upb_Arena_IsTaggedPointer(poc)) {
    ai = _upb_Arena_PointerFromTagged(poc);
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
  upb_ArenaInternal* parent_tail =
      upb_Atomic_Load(&parent->tail, memory_order_relaxed);

  do {
    // Our tail might be stale, but it will always converge to the true tail.
    upb_ArenaInternal* parent_tail_next =
        upb_Atomic_Load(&parent_tail->next, memory_order_relaxed);
    while (parent_tail_next != NULL) {
      parent_tail = parent_tail_next;
      parent_tail_next =
          upb_Atomic_Load(&parent_tail->next, memory_order_relaxed);
    }

    upb_ArenaInternal* displaced =
        upb_Atomic_Exchange(&parent_tail->next, child, memory_order_relaxed);
    parent_tail = upb_Atomic_Load(&child->tail, memory_order_relaxed);

    // If we displaced something that got installed racily, we can simply
    // reinstall it on our new tail.
    child = displaced;
  } while (child != NULL);

  upb_Atomic_Store(&parent->tail, parent_tail, memory_order_relaxed);
}

static upb_ArenaInternal* _upb_Arena_DoFuse(upb_Arena* a1, upb_Arena* a2,
                                            uintptr_t* ref_delta) {
  // `parent_or_count` has two disctint modes
  // -  parent pointer mode
  // -  refcount mode
  //
  // In parent pointer mode, it may change what pointer it refers to in the
  // tree, but it will always approach a root.  Any operation that walks the
  // tree to the root may collapse levels of the tree concurrently.
  upb_ArenaRoot r1 = _upb_Arena_FindRoot(a1);
  upb_ArenaRoot r2 = _upb_Arena_FindRoot(a2);

  if (r1.root == r2.root) return r1.root;  // Already fused.

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

static bool _upb_Arena_FixupRefs(upb_ArenaInternal* new_root,
                                 uintptr_t ref_delta) {
  if (ref_delta == 0) return true;  // No fixup required.
  uintptr_t poc =
      upb_Atomic_Load(&new_root->parent_or_count, memory_order_relaxed);
  if (_upb_Arena_IsTaggedPointer(poc)) return false;
  uintptr_t with_refs = poc - ref_delta;
  UPB_ASSERT(!_upb_Arena_IsTaggedPointer(with_refs));
  return upb_Atomic_CompareExchangeStrong(&new_root->parent_or_count, &poc,
                                          with_refs, memory_order_relaxed,
                                          memory_order_relaxed);
}

bool upb_Arena_Fuse(upb_Arena* a1, upb_Arena* a2) {
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
    upb_ArenaInternal* new_root = _upb_Arena_DoFuse(a1, a2, &ref_delta);
    if (new_root != NULL && _upb_Arena_FixupRefs(new_root, ref_delta)) {
      return true;
    }
  }
}

bool upb_Arena_IncRefFor(upb_Arena* a, const void* owner) {
  upb_ArenaInternal* ai = upb_Arena_Internal(a);
  if (_upb_ArenaInternal_HasInitialBlock(ai)) return false;
  upb_ArenaRoot r;

retry:
  r = _upb_Arena_FindRoot(a);
  if (upb_Atomic_CompareExchangeWeak(
          &r.root->parent_or_count, &r.tagged_count,
          _upb_Arena_TaggedFromRefcount(
              _upb_Arena_RefCountFromTagged(r.tagged_count) + 1),
          memory_order_release, memory_order_acquire)) {
    // We incremented it successfully, so we are done.
    return true;
  }
  // We failed update due to parent switching on the arena.
  goto retry;
}

void upb_Arena_DecRefFor(upb_Arena* a, const void* owner) { upb_Arena_Free(a); }

void UPB_PRIVATE(_upb_Arena_SwapIn)(upb_Arena* des, const upb_Arena* src) {
  upb_ArenaInternal* desi = upb_Arena_Internal(des);
  upb_ArenaInternal* srci = upb_Arena_Internal(src);

  *des = *src;
  desi->block_alloc = srci->block_alloc;
  upb_MemBlock* blocks = upb_Atomic_Load(&srci->blocks, memory_order_relaxed);
  upb_Atomic_Init(&desi->blocks, blocks);
}

void UPB_PRIVATE(_upb_Arena_SwapOut)(upb_Arena* des, const upb_Arena* src) {
  upb_ArenaInternal* desi = upb_Arena_Internal(des);
  upb_ArenaInternal* srci = upb_Arena_Internal(src);

  *des = *src;
  upb_MemBlock* blocks = upb_Atomic_Load(&srci->blocks, memory_order_relaxed);
  upb_Atomic_Store(&desi->blocks, blocks, memory_order_relaxed);
}
