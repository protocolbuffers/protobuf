/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/mem/arena_internal.h"
#include "upb/port/atomic.h"

// Must be last.
#include "upb/port/def.inc"

struct _upb_MemBlock {
  // Atomic only for the benefit of SpaceAllocated().
  UPB_ATOMIC(_upb_MemBlock*) next;
  uint32_t size;
  // Data follows.
};

static const size_t memblock_reserve =
    UPB_ALIGN_UP(sizeof(_upb_MemBlock), UPB_MALLOC_ALIGN);

typedef struct _upb_ArenaRoot {
  upb_Arena* root;
  uintptr_t tagged_count;
} _upb_ArenaRoot;

static _upb_ArenaRoot _upb_Arena_FindRoot(upb_Arena* a) {
  uintptr_t poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    upb_Arena* next = _upb_Arena_PointerFromTagged(poc);
    UPB_ASSERT(a != next);
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
      // - If fuses are actively occuring, the root may change, but the
      //   invariant is that `parent_or_count` merely points to *a* parent.
      //
      // In other words, it is moving towards "the" root, and that root may move
      // further away over time, but the path towards that root will continue to
      // be valid and the creation of the path carries all the memory orderings
      // required.
      UPB_ASSERT(a != _upb_Arena_PointerFromTagged(next_poc));
      upb_Atomic_Store(&a->parent_or_count, next_poc, memory_order_relaxed);
    }
    a = next;
    poc = next_poc;
  }
  return (_upb_ArenaRoot){.root = a, .tagged_count = poc};
}

size_t upb_Arena_SpaceAllocated(upb_Arena* arena) {
  arena = _upb_Arena_FindRoot(arena).root;
  size_t memsize = 0;

  while (arena != NULL) {
    _upb_MemBlock* block =
        upb_Atomic_Load(&arena->blocks, memory_order_relaxed);
    while (block != NULL) {
      memsize += sizeof(_upb_MemBlock) + block->size;
      block = upb_Atomic_Load(&block->next, memory_order_relaxed);
    }
    arena = upb_Atomic_Load(&arena->next, memory_order_relaxed);
  }

  return memsize;
}

uint32_t upb_Arena_DebugRefCount(upb_Arena* a) {
  // These loads could probably be relaxed, but given that this is debug-only,
  // it's not worth introducing a new variant for it.
  uintptr_t poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    a = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
  }
  return _upb_Arena_RefCountFromTagged(poc);
}

static void upb_Arena_AddBlock(upb_Arena* a, void* ptr, size_t size) {
  _upb_MemBlock* block = ptr;

  // Insert into linked list.
  block->size = (uint32_t)size;
  upb_Atomic_Init(&block->next, a->blocks);
  upb_Atomic_Store(&a->blocks, block, memory_order_release);

  a->head.ptr = UPB_PTR_AT(block, memblock_reserve, char);
  a->head.end = UPB_PTR_AT(block, size, char);

  UPB_POISON_MEMORY_REGION(a->head.ptr, a->head.end - a->head.ptr);
}

static bool upb_Arena_AllocBlock(upb_Arena* a, size_t size) {
  if (!a->block_alloc) return false;
  _upb_MemBlock* last_block = upb_Atomic_Load(&a->blocks, memory_order_acquire);
  size_t last_size = last_block != NULL ? last_block->size : 128;
  size_t block_size = UPB_MAX(size, last_size * 2) + memblock_reserve;
  _upb_MemBlock* block = upb_malloc(upb_Arena_BlockAlloc(a), block_size);

  if (!block) return false;
  upb_Arena_AddBlock(a, block, block_size);
  return true;
}

void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size) {
  if (!upb_Arena_AllocBlock(a, size)) return NULL; /* Out of memory. */
  UPB_ASSERT(_upb_ArenaHas(a) >= size);
  return upb_Arena_Malloc(a, size);
}

/* Public Arena API ***********************************************************/

static upb_Arena* upb_Arena_InitSlow(upb_alloc* alloc) {
  const size_t first_block_overhead = sizeof(upb_Arena) + memblock_reserve;
  upb_Arena* a;

  /* We need to malloc the initial block. */
  char* mem;
  size_t n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);
  n -= sizeof(*a);

  a->block_alloc = upb_Arena_MakeBlockAlloc(alloc, 0);
  upb_Atomic_Init(&a->parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&a->next, NULL);
  upb_Atomic_Init(&a->tail, a);
  upb_Atomic_Init(&a->blocks, NULL);

  upb_Arena_AddBlock(a, mem, n);

  return a;
}

upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc) {
  upb_Arena* a;

  if (n) {
    /* Align initial pointer up so that we return properly-aligned pointers. */
    void* aligned = (void*)UPB_ALIGN_UP((uintptr_t)mem, UPB_MALLOC_ALIGN);
    size_t delta = (uintptr_t)aligned - (uintptr_t)mem;
    n = delta <= n ? n - delta : 0;
    mem = aligned;
  }

  /* Round block size down to alignof(*a) since we will allocate the arena
   * itself at the end. */
  n = UPB_ALIGN_DOWN(n, UPB_ALIGN_OF(upb_Arena));

  if (UPB_UNLIKELY(n < sizeof(upb_Arena))) {
    return upb_Arena_InitSlow(alloc);
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);

  upb_Atomic_Init(&a->parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  upb_Atomic_Init(&a->next, NULL);
  upb_Atomic_Init(&a->tail, a);
  upb_Atomic_Init(&a->blocks, NULL);
  a->block_alloc = upb_Arena_MakeBlockAlloc(alloc, 1);
  a->head.ptr = mem;
  a->head.end = UPB_PTR_AT(mem, n - sizeof(*a), char);

  return a;
}

static void arena_dofree(upb_Arena* a) {
  UPB_ASSERT(_upb_Arena_RefCountFromTagged(a->parent_or_count) == 1);

  while (a != NULL) {
    // Load first since arena itself is likely from one of its blocks.
    upb_Arena* next_arena =
        (upb_Arena*)upb_Atomic_Load(&a->next, memory_order_acquire);
    upb_alloc* block_alloc = upb_Arena_BlockAlloc(a);
    _upb_MemBlock* block = upb_Atomic_Load(&a->blocks, memory_order_acquire);
    while (block != NULL) {
      // Load first since we are deleting block.
      _upb_MemBlock* next_block =
          upb_Atomic_Load(&block->next, memory_order_acquire);
      upb_free(block_alloc, block);
      block = next_block;
    }
    a = next_arena;
  }
}

void upb_Arena_Free(upb_Arena* a) {
  uintptr_t poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
retry:
  while (_upb_Arena_IsTaggedPointer(poc)) {
    a = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_Load(&a->parent_or_count, memory_order_acquire);
  }

  // compare_exchange or fetch_sub are RMW operations, which are more
  // expensive then direct loads.  As an optimization, we only do RMW ops
  // when we need to update things for other threads to see.
  if (poc == _upb_Arena_TaggedFromRefcount(1)) {
    arena_dofree(a);
    return;
  }

  if (upb_Atomic_CompareExchangeWeak(
          &a->parent_or_count, &poc,
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
  upb_Arena* parent_tail = upb_Atomic_Load(&parent->tail, memory_order_relaxed);
  do {
    // Our tail might be stale, but it will always converge to the true tail.
    upb_Arena* parent_tail_next =
        upb_Atomic_Load(&parent_tail->next, memory_order_relaxed);
    while (parent_tail_next != NULL) {
      parent_tail = parent_tail_next;
      parent_tail_next =
          upb_Atomic_Load(&parent_tail->next, memory_order_relaxed);
    }

    upb_Arena* displaced =
        upb_Atomic_Exchange(&parent_tail->next, child, memory_order_relaxed);
    parent_tail = upb_Atomic_Load(&child->tail, memory_order_relaxed);

    // If we displaced something that got installed racily, we can simply
    // reinstall it on our new tail.
    child = displaced;
  } while (child != NULL);

  upb_Atomic_Store(&parent->tail, parent_tail, memory_order_relaxed);
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
  _upb_ArenaRoot r1 = _upb_Arena_FindRoot(a1);
  _upb_ArenaRoot r2 = _upb_Arena_FindRoot(a2);

  if (r1.root == r2.root) return r1.root;  // Already fused.

  // Avoid cycles by always fusing into the root with the lower address.
  if ((uintptr_t)r1.root > (uintptr_t)r2.root) {
    _upb_ArenaRoot tmp = r1;
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

static bool _upb_Arena_FixupRefs(upb_Arena* new_root, uintptr_t ref_delta) {
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

  // Do not fuse initial blocks since we cannot lifetime extend them.
  // Any other fuse scenario is allowed.
  if (upb_Arena_HasInitialBlock(a1) || upb_Arena_HasInitialBlock(a2)) {
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
