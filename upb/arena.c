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

#include "upb/arena.h"

#include <stdlib.h>

#include "upb/internal/upb.h"

// Must be last.
#include "upb/port_def.inc"

/* upb_alloc ******************************************************************/

static void* upb_global_allocfunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                                  size_t size) {
  UPB_UNUSED(alloc);
  UPB_UNUSED(oldsize);
  if (size == 0) {
    free(ptr);
    return NULL;
  } else {
    return realloc(ptr, size);
  }
}

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

upb_alloc upb_alloc_global = {&upb_global_allocfunc};

/* upb_Arena ******************************************************************/

struct mem_block {
  struct mem_block* next;
  uint32_t size;
  uint32_t cleanups;
  /* Data follows. */
};

typedef struct cleanup_ent {
  upb_CleanupFunc* cleanup;
  void* ud;
} cleanup_ent;

static const size_t memblock_reserve =
    UPB_ALIGN_UP(sizeof(mem_block), UPB_MALLOC_ALIGN);

static upb_Arena* arena_findroot(upb_Arena* a) {
  /* Path splitting keeps time complexity down, see:
   *   https://en.wikipedia.org/wiki/Disjoint-set_data_structure */
  while (a->parent != a) {
    upb_Arena* next = a->parent;
    a->parent = next->parent;
    a = next;
  }
  return a;
}

static void upb_Arena_addblock(upb_Arena* a, upb_Arena* root, void* ptr,
                               size_t size) {
  mem_block* block = ptr;

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
  upb_Arena* root = arena_findroot(a);
  size_t block_size = UPB_MAX(size, a->last_size * 2) + memblock_reserve;
  mem_block* block = upb_malloc(root->block_alloc, block_size);

  if (!block) return false;
  upb_Arena_addblock(a, root, block, block_size);
  return true;
}

void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size) {
  if (!upb_Arena_Allocblock(a, size)) return NULL; /* Out of memory. */
  UPB_ASSERT(_upb_ArenaHas(a) >= size);
  return upb_Arena_Malloc(a, size);
}

static void* upb_Arena_doalloc(upb_alloc* alloc, void* ptr, size_t oldsize,
                               size_t size) {
  upb_Arena* a = (upb_Arena*)alloc; /* upb_alloc is initial member. */
  return upb_Arena_Realloc(a, ptr, oldsize, size);
}

/* Public Arena API ***********************************************************/

upb_Arena* arena_initslow(void* mem, size_t n, upb_alloc* alloc) {
  const size_t first_block_overhead = sizeof(upb_Arena) + memblock_reserve;
  upb_Arena* a;

  /* We need to malloc the initial block. */
  n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_Arena);
  n -= sizeof(*a);

  a->head.alloc.func = &upb_Arena_doalloc;
  a->block_alloc = alloc;
  a->parent = a;
  a->refcount = 1;
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

  a->head.alloc.func = &upb_Arena_doalloc;
  a->block_alloc = alloc;
  a->parent = a;
  a->refcount = 1;
  a->last_size = UPB_MAX(128, n);
  a->head.ptr = mem;
  a->head.end = UPB_PTR_AT(mem, n - sizeof(*a), char);
  a->freelist = NULL;
  a->cleanup_metadata = upb_cleanup_metadata(NULL, true);

  return a;
}

static void arena_dofree(upb_Arena* a) {
  mem_block* block = a->freelist;
  UPB_ASSERT(a->parent == a);
  UPB_ASSERT(a->refcount == 0);

  while (block) {
    /* Load first since we are deleting block. */
    mem_block* next = block->next;

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
  a = arena_findroot(a);
  if (--a->refcount == 0) arena_dofree(a);
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
  upb_Arena* r1 = arena_findroot(a1);
  upb_Arena* r2 = arena_findroot(a2);

  if (r1 == r2) return true; /* Already fused. */

  /* Do not fuse initial blocks since we cannot lifetime extend them. */
  if (upb_cleanup_has_initial_block(r1->cleanup_metadata)) return false;
  if (upb_cleanup_has_initial_block(r2->cleanup_metadata)) return false;

  /* Only allow fuse with a common allocator */
  if (r1->block_alloc != r2->block_alloc) return false;

  /* We want to join the smaller tree to the larger tree.
   * So swap first if they are backwards. */
  if (r1->refcount < r2->refcount) {
    upb_Arena* tmp = r1;
    r1 = r2;
    r2 = tmp;
  }

  /* r1 takes over r2's freelist and refcount. */
  r1->refcount += r2->refcount;
  if (r2->freelist_tail) {
    UPB_ASSERT(r2->freelist_tail->next == NULL);
    r2->freelist_tail->next = r1->freelist;
    r1->freelist = r2->freelist;
  }
  r2->parent = r1;
  return true;
}
