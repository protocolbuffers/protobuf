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

static uint32_t* upb_cleanup_pointer(uintptr_t cleanup_metadata) {
  return (uint32_t*)(cleanup_metadata & ~0x1);
}

static bool upb_cleanup_has_initial_block(uintptr_t cleanup_metadata) {
  return cleanup_metadata & 0x1;
}

static uintptr_t upb_cleanup_metadata(uint32_t* cleanup,
                                      bool has_initial_block) {
  return (uintptr_t)cleanup | has_initial_block;
}

struct _upb_MemBlock {
  struct _upb_MemBlock* next;
  uint32_t size;
  uint32_t cleanups;
  // Data follows.
};

typedef struct cleanup_ent {
  upb_CleanupFunc* cleanup;
  void* ud;
} cleanup_ent;

static const size_t memblock_reserve =
    UPB_ALIGN_UP(sizeof(_upb_MemBlock), UPB_MALLOC_ALIGN);

static upb_Arena* _upb_Arena_FindRoot(upb_Arena* a) {
  uintptr_t poc = upb_Atomic_LoadAcquire(&a->parent_or_count);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    upb_Arena* next = _upb_Arena_PointerFromTagged(poc);
    uintptr_t next_poc = upb_Atomic_LoadAcquire(&next->parent_or_count);

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
      upb_Atomic_StoreRelaxed(&a->parent_or_count, next_poc);
    }
    a = next;
    poc = next_poc;
  }
  return a;
}

size_t upb_Arena_SpaceAllocated(upb_Arena* arena) {
  arena = _upb_Arena_FindRoot(arena);
  size_t memsize = 0;

  _upb_MemBlock* block = arena->freelist;

  while (block) {
    memsize += sizeof(_upb_MemBlock) + block->size;
    block = block->next;
  }

  return memsize;
}

uint32_t upb_Arena_DebugRefCount(upb_Arena* a) {
  // These loads could probably be relaxed, but given that this is debug-only,
  // it's not worth introducing a new variant for it.
  uintptr_t poc = upb_Atomic_LoadAcquire(&a->parent_or_count);
  while (_upb_Arena_IsTaggedPointer(poc)) {
    a = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_LoadAcquire(&a->parent_or_count);
  }
  return _upb_Arena_RefCountFromTagged(poc);
}

static void upb_Arena_addblock(upb_Arena* a, upb_Arena* root, void* ptr,
                               size_t size) {
  _upb_MemBlock* block = ptr;

  /* The block is for arena |a|, but should appear in the freelist of |root|. */
  block->next = root->freelist;
  block->size = (uint32_t)size;
  block->cleanups = 0;
  root->freelist = block;
  a->last_size = block->size;
  if (!root->freelist_tail) root->freelist_tail = block;

  a->head.ptr = UPB_PTR_AT(block, memblock_reserve, char);
  a->head.end = UPB_PTR_AT(block, size, char);
  a->cleanup_metadata = upb_cleanup_metadata(
      &block->cleanups, upb_cleanup_has_initial_block(a->cleanup_metadata));

  UPB_POISON_MEMORY_REGION(a->head.ptr, a->head.end - a->head.ptr);
}

static bool upb_Arena_Allocblock(upb_Arena* a, size_t size) {
  upb_Arena* root = _upb_Arena_FindRoot(a);
  size_t block_size = UPB_MAX(size, a->last_size * 2) + memblock_reserve;
  _upb_MemBlock* block = upb_malloc(root->block_alloc, block_size);

  if (!block) return false;
  upb_Arena_addblock(a, root, block, block_size);
  return true;
}

void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size) {
  if (!upb_Arena_Allocblock(a, size)) return NULL; /* Out of memory. */
  UPB_ASSERT(_upb_ArenaHas(a) >= size);
  return upb_Arena_Malloc(a, size);
}

/* Public Arena API ***********************************************************/

static upb_Arena* arena_initslow(void* mem, size_t n, upb_alloc* alloc) {
  const size_t first_block_overhead = sizeof(upb_Arena) + memblock_reserve;
  upb_Arena* a;

  /* We need to malloc the initial block. */
  n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);
  n -= sizeof(*a);

  a->block_alloc = alloc;
  upb_Atomic_Init(&a->parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  a->freelist = NULL;
  a->freelist_tail = NULL;
  a->cleanup_metadata = upb_cleanup_metadata(NULL, false);

  upb_Arena_addblock(a, a, mem, n);

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
    return arena_initslow(mem, n, alloc);
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);

  a->block_alloc = alloc;
  upb_Atomic_Init(&a->parent_or_count, _upb_Arena_TaggedFromRefcount(1));
  a->last_size = UPB_MAX(128, n);
  a->head.ptr = mem;
  a->head.end = UPB_PTR_AT(mem, n - sizeof(*a), char);
  a->freelist = NULL;
  a->freelist_tail = NULL;
  a->cleanup_metadata = upb_cleanup_metadata(NULL, true);

  return a;
}

static void arena_dofree(upb_Arena* a) {
  _upb_MemBlock* block = a->freelist;
  UPB_ASSERT(_upb_Arena_RefCountFromTagged(a->parent_or_count) == 1);

  while (block) {
    /* Load first since we are deleting block. */
    _upb_MemBlock* next = block->next;

    if (block->cleanups > 0) {
      cleanup_ent* end = UPB_PTR_AT(block, block->size, void);
      cleanup_ent* ptr = end - block->cleanups;

      for (; ptr < end; ptr++) {
        ptr->cleanup(ptr->ud);
      }
    }

    upb_free(a->block_alloc, block);
    block = next;
  }
}

void upb_Arena_Free(upb_Arena* a) {
  uintptr_t poc = upb_Atomic_LoadAcquire(&a->parent_or_count);
retry:
  while (_upb_Arena_IsTaggedPointer(poc)) {
    a = _upb_Arena_PointerFromTagged(poc);
    poc = upb_Atomic_LoadAcquire(&a->parent_or_count);
  }

  // compare_exchange or fetch_sub are RMW operations, which are more
  // expensive then direct loads.  As an optimization, we only do RMW ops
  // when we need to update things for other threads to see.
  if (poc == _upb_Arena_TaggedFromRefcount(1)) {
    arena_dofree(a);
    return;
  }

  if (upb_Atomic_CompareExchangeStrongAcqRel(
          &a->parent_or_count, &poc,
          _upb_Arena_TaggedFromRefcount(_upb_Arena_RefCountFromTagged(poc) -
                                        1))) {
    // We were >1 and we decremented it successfully, so we are done.
    return;
  }

  // We failed our update, so someone has done something, retry the whole
  // process, but the failed exchange reloaded `poc` for us.
  goto retry;
}

bool upb_Arena_AddCleanup(upb_Arena* a, void* ud, upb_CleanupFunc* func) {
  cleanup_ent* ent;
  uint32_t* cleanups = upb_cleanup_pointer(a->cleanup_metadata);

  if (!cleanups || _upb_ArenaHas(a) < sizeof(cleanup_ent)) {
    if (!upb_Arena_Allocblock(a, 128)) return false; /* Out of memory. */
    UPB_ASSERT(_upb_ArenaHas(a) >= sizeof(cleanup_ent));
    cleanups = upb_cleanup_pointer(a->cleanup_metadata);
  }

  a->head.end -= sizeof(cleanup_ent);
  ent = (cleanup_ent*)a->head.end;
  (*cleanups)++;
  UPB_UNPOISON_MEMORY_REGION(ent, sizeof(cleanup_ent));

  ent->cleanup = func;
  ent->ud = ud;

  return true;
}

bool upb_Arena_Fuse(upb_Arena* a1, upb_Arena* a2) {
  // SAFE IN THE PRESENCE OF FUSE/FREE RACES BUT NOT IN THE
  // PRESENCE OF FUSE/FUSE RACES!!!
  //
  // `parent_or_count` has two disctint modes
  // -  parent pointer mode
  // -  refcount mode
  //
  // In parent pointer mode, it may change what pointer it refers to in the
  // tree, but it will always approach a root.  Any operation that walks the
  // tree to the root may collapse levels of the tree concurrently.
  //
  // In refcount mode, any free operation may lower the refcount.
  //
  // Only a fuse operation may increase the refcount.
  // Only a fuse operation may switch `parent_or_count` from parent mode to
  // refcount mode.
  //
  // Given that we do not allow fuse/fuse races, we may rely on the invariant
  // that only refcounts can change once we have found the root.  Because the
  // threads doing the fuse must hold references, we can guarantee that no
  // refcounts will reach zero concurrently.
  upb_Arena* r1 = _upb_Arena_FindRoot(a1);
  upb_Arena* r2 = _upb_Arena_FindRoot(a2);

  if (r1 == r2) return true;  // Already fused.

  // Do not fuse initial blocks since we cannot lifetime extend them.
  if (upb_cleanup_has_initial_block(r1->cleanup_metadata)) return false;
  if (upb_cleanup_has_initial_block(r2->cleanup_metadata)) return false;

  // Only allow fuse with a common allocator
  if (r1->block_alloc != r2->block_alloc) return false;

  uintptr_t r1_poc = upb_Atomic_LoadAcquire(&r1->parent_or_count);
  uintptr_t r2_poc = upb_Atomic_LoadAcquire(&r2->parent_or_count);
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(r1_poc));
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(r2_poc));

  // Keep the tree shallow by joining the smaller tree to the larger.
  if (_upb_Arena_RefCountFromTagged(r1_poc) <
      _upb_Arena_RefCountFromTagged(r2_poc)) {
    upb_Arena* tmp = r1;
    r1 = r2;
    r2 = tmp;

    uintptr_t tmp_poc = r1_poc;
    r1_poc = r2_poc;
    r2_poc = tmp_poc;
  }

  // r1 takes over r2's freelist, this must happen before we update
  // refcounts since the refcount carriers the memory dependencies.
  if (r2->freelist_tail) {
    UPB_ASSERT(r2->freelist_tail->next == NULL);
    r2->freelist_tail->next = r1->freelist;
    r1->freelist = r2->freelist;
  }

  // The moment we install `r1` as the parent for `r2` all racing frees may
  // immediately begin decrementing `r1`'s refcount.  So we must install all the
  // refcounts that we know about first to prevent a premature unref to zero.
  uint32_t r2_refcount = _upb_Arena_RefCountFromTagged(r2_poc);
  upb_Atomic_AddRelease(&r1->parent_or_count, ((uintptr_t)r2_refcount) << 1);

  // When installing `r1` as the parent for `r2` racing frees may have changed
  // the refcount for `r2` so we need to capture the old value to fix up `r1`'s
  // refcount based on the delta from what we saw the first time.
  r2_poc = upb_Atomic_ExchangeAcqRel(&r2->parent_or_count,
                                     _upb_Arena_TaggedFromPointer(r1));
  UPB_ASSERT(_upb_Arena_IsTaggedRefcount(r2_poc));
  uint32_t delta_refcount = r2_refcount - _upb_Arena_RefCountFromTagged(r2_poc);
  if (delta_refcount != 0) {
    upb_Atomic_SubRelease(&r1->parent_or_count, ((uintptr_t)delta_refcount)
                                                    << 1);
  }
  return true;
}
