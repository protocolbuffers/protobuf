// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mem/arena.h"

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/alloc.h"
#include "upb/mem/internal/arena.h"
#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct upb_ArenaInternal upb_ArenaInternal;

typedef struct upb_MemBlock {
  // Atomic only for the benefit of SpaceAllocated().
  UPB_ATOMIC(struct upb_MemBlock*) next;
  uint32_t size;

  // Data follows.
} upb_MemBlock;

struct upb_ArenaInternal {
  // upb_alloc* together with a low bit which signals whether there is an
  // initial block.
  uintptr_t block_alloc;

  // When multiple arenas are fused together, each arena points to a parent
  // arena (root points to itself). The root tracks how many live arenas
  // reference it.

  // The low bit is tagged:
  //   0: pointer to parent
  //   1: count, left shifted by one
  UPB_ATOMIC(uintptr_t) parent_or_count;

  // All nodes that are fused together are in a singly-linked list.
  UPB_ATOMIC(struct upb_Arena*) next;  // NULL at end of list.

  // The last element of the linked list.  This is present only as an
  // optimization, so that we do not have to iterate over all members for every
  // fuse.  Only significant for an arena root.  In other cases it is ignored.
  UPB_ATOMIC(struct upb_Arena*) tail;  // == self when no other list members.

  // Linked list of blocks to free/cleanup.  Atomic only for the benefit of
  // upb_Arena_SpaceAllocated().
  UPB_ATOMIC(upb_MemBlock*) blocks;
};

typedef struct {
  upb_Arena head;  // Must be the first field so we can alias pointers.
  upb_ArenaInternal tail;
} upb_ArenaBox;

typedef struct {
  upb_Arena* root;
  uintptr_t tagged_count;
} upb_ArenaRoot;

static const size_t kUpb_MemblockReserve =
    UPB_ALIGN_UP(sizeof(upb_MemBlock), UPB_MALLOC_ALIGN);

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

static struct upb_Arena* _upb_Arena_PointerFromTagged(
    uintptr_t parent_or_count) {
  UPB_ASSERT(_upb_Arena_IsTaggedPointer(parent_or_count));
  return (struct upb_Arena*)parent_or_count;
}

static uintptr_t _upb_Arena_TaggedFromPointer(struct upb_Arena* a) {
  uintptr_t parent_or_count = (uintptr_t)a;
  UPB_ASSERT(_upb_Arena_IsTaggedPointer(parent_or_count));
  return parent_or_count;
}

static upb_alloc* _upb_Arena_BlockAlloc(struct upb_Arena* a) {
  return (upb_alloc*)(a->internal->block_alloc & ~0x1);
}

static bool _upb_Arena_HasInitialBlock(struct upb_Arena* a) {
  return a->internal->block_alloc & 0x1;
}

static uintptr_t _upb_Arena_MakeBlockAlloc(upb_alloc* alloc, bool has_initial) {
  uintptr_t alloc_uint = (uintptr_t)alloc;
  UPB_ASSERT((alloc_uint & 1) == 0);
  return alloc_uint | (has_initial ? 1 : 0);
}

static upb_ArenaRoot _upb_Arena_FindRoot(upb_Arena* a) {
  struct upb_ArenaInternal* ai = a->internal;
  uintptr_t poc = upb_Atomic_Load(&ai->parent_or_count, memory_order_acquire);

  while (_upb_Arena_IsTaggedPointer(poc)) {
    upb_Arena* next = _upb_Arena_PointerFromTagged(poc);
    UPB_ASSERT(a != next);
    uintptr_t next_poc =
        upb_Atomic_Load(&next->internal->parent_or_count, memory_order_acquire);

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
      UPB_ASSERT(a != _upb_Arena_PointerFromTagged(next_poc));
      upb_Atomic_Store(&ai->parent_or_count, next_poc, memory_order_relaxed);
    }
    a = next;
    ai = a->internal;
    poc = next_poc;
  }
  return (upb_ArenaRoot){.root = a, .tagged_count = poc};
}

size_t upb_Arena_SpaceAllocated(upb_Arena* a) {
  a = _upb_Arena_FindRoot(a).root;
  size_t memsize = 0;

  while (a != NULL) {
    upb_MemBlock* block =
        upb_Atomic_Load(&a->internal->blocks, memory_order_relaxed);
    while (block != NULL) {
      memsize += sizeof(upb_MemBlock) + block->size;
      block = upb_Atomic_Load(&block->next, memory_order_relaxed);
    }
    a = upb_Atomic_Load(&a->internal->next, memory_order_relaxed);
  }

  return memsize;
}

uint32_t UPB_PRIVATE(_upb_Arena_DebugRefCount)(upb_Arena* a) {
  // These loads could probably be relaxed, but given that this is debug-only,
  // it's not worth introducing a new variant for it.
  uintptr_t poc =
      upb_Atomic_Load(&a->internal->parent_or_count, memory_order_acquire);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    a = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_Load(&a->internal->parent_or_count, memory_order_acquire);
  }
  return _upb_Arena_RefCountFromTagged(poc);
}

static void upb_Arena_AddBlock(upb_Arena* a, void* ptr, size_t size) {
  upb_MemBlock* block = ptr;

  // Insert into linked list.
  block->size = (uint32_t)size;
  upb_Atomic_Init(&block->next, a->internal->blocks);
  upb_Atomic_Store(&a->internal->blocks, block, memory_order_release);

  a->UPB_PRIVATE(ptr) = UPB_PTR_AT(block, kUpb_MemblockReserve, char);
  a->UPB_PRIVATE(end) = UPB_PTR_AT(block, size, char);

  UPB_POISON_MEMORY_REGION(a->UPB_PRIVATE(ptr),
                           a->UPB_PRIVATE(end) - a->UPB_PRIVATE(ptr));
}

bool UPB_PRIVATE(_upb_Arena_AllocBlock)(upb_Arena* a, size_t size) {
  struct upb_ArenaInternal* ai = a->internal;
  if (!ai->block_alloc) return false;
  upb_MemBlock* last_block = upb_Atomic_Load(&ai->blocks, memory_order_acquire);
  size_t last_size = last_block != NULL ? last_block->size : 128;
  size_t block_size = UPB_MAX(size, last_size * 2) + kUpb_MemblockReserve;
  upb_MemBlock* block = upb_malloc(_upb_Arena_BlockAlloc(a), block_size);

  if (!block) return false;
  upb_Arena_AddBlock(a, block, block_size);
  UPB_ASSERT(UPB_PRIVATE(_upb_ArenaHas)(a) >= size);
  return true;
}

// Public Arena API ////////////////////////////////////////////////////////////

static upb_Arena* upb_Arena_InitSlow(upb_alloc* alloc) {
  const size_t first_block_overhead =
      sizeof(upb_ArenaBox) + kUpb_MemblockReserve;

  // We need to malloc the initial block.
  char* mem;
  size_t n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  upb_ArenaBox* b = UPB_PTR_AT(mem, n - sizeof(upb_ArenaBox), upb_ArenaBox);
  n -= sizeof(upb_ArenaBox);

  upb_Arena* a = &b->head;
  UPB_ASSERT((void*)a == (void*)b);

  upb_ArenaInternal* ai = a->internal = &b->tail;

  ai->block_alloc = _upb_Arena_MakeBlockAlloc(alloc, 0);
  upb_Atomic_Init(&ai->parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&ai->next, NULL);
  upb_Atomic_Init(&ai->tail, a);
  upb_Atomic_Init(&ai->blocks, NULL);

  upb_Arena_AddBlock(a, mem, n);

  return a;
}

upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc) {
  if (n) {
    // Align initial pointer up so that we return properly-aligned pointers.
    void* aligned = (void*)UPB_ALIGN_UP((uintptr_t)mem, UPB_MALLOC_ALIGN);
    size_t delta = (uintptr_t)aligned - (uintptr_t)mem;
    n = delta <= n ? n - delta : 0;
    mem = aligned;
  }

  // Round block size down to alignof(*a) since we will allocate the arena
  // itself at the end.
  n = UPB_ALIGN_DOWN(n, UPB_ALIGN_OF(upb_ArenaBox));

  if (UPB_UNLIKELY(n < sizeof(upb_ArenaBox))) {
    return upb_Arena_InitSlow(alloc);
  }

  upb_ArenaBox* b = UPB_PTR_AT(mem, n - sizeof(upb_ArenaBox), upb_ArenaBox);
  upb_Arena* a = &b->head;
  UPB_ASSERT((void*)a == (void*)b);

  upb_ArenaInternal* ai = a->internal = &b->tail;

  upb_Atomic_Init(&ai->parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&ai->next, NULL);
  upb_Atomic_Init(&ai->tail, a);
  upb_Atomic_Init(&ai->blocks, NULL);
  ai->block_alloc = _upb_Arena_MakeBlockAlloc(alloc, 1);
  a->UPB_PRIVATE(ptr) = mem;
  a->UPB_PRIVATE(end) = (char*)a;

  return a;
}

static void arena_dofree(upb_Arena* a) {
  UPB_ASSERT(_upb_Arena_RefCountFromTagged(a->internal->parent_or_count) == 1);

  while (a != NULL) {
    // Load first since arena itself is likely from one of its blocks.
    upb_Arena* next_arena =
        (upb_Arena*)upb_Atomic_Load(&a->internal->next, memory_order_acquire);
    upb_alloc* block_alloc = _upb_Arena_BlockAlloc(a);
    upb_MemBlock* block =
        upb_Atomic_Load(&a->internal->blocks, memory_order_acquire);
    while (block != NULL) {
      // Load first since we are deleting block.
      upb_MemBlock* next_block =
          upb_Atomic_Load(&block->next, memory_order_acquire);
      upb_free(block_alloc, block);
      block = next_block;
    }
    a = next_arena;
  }
}

void upb_Arena_Free(upb_Arena* a) {
  uintptr_t poc =
      upb_Atomic_Load(&a->internal->parent_or_count, memory_order_acquire);
retry:
  while (_upb_Arena_IsTaggedPointer(poc)) {
    a = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_Load(&a->internal->parent_or_count, memory_order_acquire);
  }

  // compare_exchange or fetch_sub are RMW operations, which are more
  // expensive then direct loads.  As an optimization, we only do RMW ops
  // when we need to update things for other threads to see.
  if (poc == _upb_Arena_TaggedFromRefcount(1)) {
    arena_dofree(a);
    return;
  }

  if (upb_Atomic_CompareExchangeWeak(
          &a->internal->parent_or_count, &poc,
          _upb_Arena_TaggedFromRefcount(_upb_Arena_RefCountFromTagged(poc) - 1),
          memory_order_release, memory_order_acquire)) {
    // We were >1 and we decremented it successfully, so we are done.
    return;
  }

  // We failed our update, so someone has done something, retry the whole
  // process, but the failed exchange reloaded `poc` for us.
  goto retry;
}

static void _upb_Arena_DoFuseArenaLists(upb_Arena* const parent,
                                        upb_Arena* child) {
  upb_Arena* parent_tail =
      upb_Atomic_Load(&parent->internal->tail, memory_order_relaxed);
  do {
    // Our tail might be stale, but it will always converge to the true tail.
    upb_Arena* parent_tail_next =
        upb_Atomic_Load(&parent_tail->internal->next, memory_order_relaxed);
    while (parent_tail_next != NULL) {
      parent_tail = parent_tail_next;
      parent_tail_next =
          upb_Atomic_Load(&parent_tail->internal->next, memory_order_relaxed);
    }

    upb_Arena* displaced = upb_Atomic_Exchange(&parent_tail->internal->next,
                                               child, memory_order_relaxed);
    parent_tail = upb_Atomic_Load(&child->internal->tail, memory_order_relaxed);

    // If we displaced something that got installed racily, we can simply
    // reinstall it on our new tail.
    child = displaced;
  } while (child != NULL);

  upb_Atomic_Store(&parent->internal->tail, parent_tail, memory_order_relaxed);
}

static upb_Arena* _upb_Arena_DoFuse(upb_Arena* a1, upb_Arena* a2,
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
          &r1.root->internal->parent_or_count, &r1.tagged_count, with_r2_refs,
          memory_order_release, memory_order_acquire)) {
    return NULL;
  }

  // Perform the actual fuse by removing the refs from `r2` and swapping in the
  // parent pointer.
  if (!upb_Atomic_CompareExchangeStrong(
          &r2.root->internal->parent_or_count, &r2.tagged_count,
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

static bool _upb_Arena_FixupRefs(upb_Arena* new_root, uintptr_t ref_delta) {
  if (ref_delta == 0) return true;  // No fixup required.
  uintptr_t poc = upb_Atomic_Load(&new_root->internal->parent_or_count,
                                  memory_order_relaxed);
  if (_upb_Arena_IsTaggedPointer(poc)) return false;
  uintptr_t with_refs = poc - ref_delta;
  UPB_ASSERT(!_upb_Arena_IsTaggedPointer(with_refs));
  return upb_Atomic_CompareExchangeStrong(&new_root->internal->parent_or_count,
                                          &poc, with_refs, memory_order_relaxed,
                                          memory_order_relaxed);
}

bool upb_Arena_Fuse(upb_Arena* a1, upb_Arena* a2) {
  if (a1 == a2) return true;  // trivial fuse

  // Do not fuse initial blocks since we cannot lifetime extend them.
  // Any other fuse scenario is allowed.
  if (_upb_Arena_HasInitialBlock(a1) || _upb_Arena_HasInitialBlock(a2)) {
    return false;
  }

  // The number of refs we ultimately need to transfer to the new root.
  uintptr_t ref_delta = 0;
  while (true) {
    upb_Arena* new_root = _upb_Arena_DoFuse(a1, a2, &ref_delta);
    if (new_root != NULL && _upb_Arena_FixupRefs(new_root, ref_delta)) {
      return true;
    }
  }
}

bool upb_Arena_IncRefFor(upb_Arena* a, const void* owner) {
  upb_ArenaRoot r;
  if (_upb_Arena_HasInitialBlock(a)) return false;

retry:
  r = _upb_Arena_FindRoot(a);
  if (upb_Atomic_CompareExchangeWeak(
          &r.root->internal->parent_or_count, &r.tagged_count,
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
