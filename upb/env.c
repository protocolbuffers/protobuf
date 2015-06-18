/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2014 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb/env.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct cleanup_ent {
  upb_cleanup_func *cleanup;
  void *ud;
  struct cleanup_ent *next;
} cleanup_ent;

static void *seeded_alloc(void *ud, void *ptr, size_t oldsize, size_t size);

/* Default allocator **********************************************************/

/* Just use realloc, keeping all allocated blocks in a linked list to destroy at
 * the end. */

typedef struct mem_block {
  /* List is doubly-linked, because in cases where realloc() moves an existing
   * block, we need to be able to remove the old pointer from the list
   * efficiently. */
  struct mem_block *prev, *next;
#ifndef NDEBUG
  size_t size;  /* Doesn't include mem_block structure. */
#endif
} mem_block;

typedef struct {
  mem_block *head;
} default_alloc_ud;

static void *default_alloc(void *_ud, void *ptr, size_t oldsize, size_t size) {
  default_alloc_ud *ud = _ud;
  mem_block *from, *block;
  void *ret;
  UPB_UNUSED(oldsize);

  from = ptr ? (void*)((char*)ptr - sizeof(mem_block)) : NULL;

#ifndef NDEBUG
  if (from) {
    assert(oldsize <= from->size);
  }
#endif

  /* TODO(haberman): we probably need to provide even better alignment here,
   * like 16-byte alignment of the returned data pointer. */
  block = realloc(from, size + sizeof(mem_block));
  if (!block) return NULL;
  ret = (char*)block + sizeof(*block);

#ifndef NDEBUG
  block->size = size;
#endif

  if (from) {
    if (block != from) {
      /* The block was moved, so pointers in next and prev blocks must be
       * updated to its new location. */
      if (block->next) block->next->prev = block;
      if (block->prev) block->prev->next = block;
      if (ud->head == from) ud->head = block;
    }
  } else {
    /* Insert at head of linked list. */
    block->prev = NULL;
    block->next = ud->head;
    if (block->next) block->next->prev = block;
    ud->head = block;
  }

  return ret;
}

static void default_alloc_cleanup(void *_ud) {
  default_alloc_ud *ud = _ud;
  mem_block *block = ud->head;

  while (block) {
    void *to_free = block;
    block = block->next;
    free(to_free);
  }
}


/* Standard error functions ***************************************************/

static bool default_err(void *ud, const upb_status *status) {
  UPB_UNUSED(ud);
  UPB_UNUSED(status);
  return false;
}

static bool write_err_to(void *ud, const upb_status *status) {
  upb_status *copy_to = ud;
  upb_status_copy(copy_to, status);
  return false;
}


/* upb_env ********************************************************************/

void upb_env_init(upb_env *e) {
  default_alloc_ud *ud = (default_alloc_ud*)&e->default_alloc_ud;
  e->ok_ = true;
  e->bytes_allocated = 0;
  e->cleanup_head = NULL;

  ud->head = NULL;

  /* Set default functions. */
  upb_env_setallocfunc(e, default_alloc, ud);
  upb_env_seterrorfunc(e, default_err, NULL);
}

void upb_env_uninit(upb_env *e) {
  cleanup_ent *ent = e->cleanup_head;

  while (ent) {
    ent->cleanup(ent->ud);
    ent = ent->next;
  }

  /* Must do this after running cleanup functions, because this will delete
     the memory we store our cleanup entries in! */
  if (e->alloc == default_alloc) {
    default_alloc_cleanup(e->alloc_ud);
  }
}

UPB_FORCEINLINE void upb_env_setallocfunc(upb_env *e, upb_alloc_func *alloc,
                                          void *ud) {
  e->alloc = alloc;
  e->alloc_ud = ud;
}

UPB_FORCEINLINE void upb_env_seterrorfunc(upb_env *e, upb_error_func *func,
                                          void *ud) {
  e->err = func;
  e->err_ud = ud;
}

void upb_env_reporterrorsto(upb_env *e, upb_status *status) {
  e->err = write_err_to;
  e->err_ud = status;
}

bool upb_env_ok(const upb_env *e) {
  return e->ok_;
}

bool upb_env_reporterror(upb_env *e, const upb_status *status) {
  e->ok_ = false;
  return e->err(e->err_ud, status);
}

bool upb_env_addcleanup(upb_env *e, upb_cleanup_func *func, void *ud) {
  cleanup_ent *ent = upb_env_malloc(e, sizeof(cleanup_ent));
  if (!ent) return false;

  ent->cleanup = func;
  ent->ud = ud;
  ent->next = e->cleanup_head;
  e->cleanup_head = ent;

  return true;
}

void *upb_env_malloc(upb_env *e, size_t size) {
  e->bytes_allocated += size;
  if (e->alloc == seeded_alloc) {
    /* This is equivalent to the next branch, but allows inlining for a
     * measurable perf benefit. */
    return seeded_alloc(e->alloc_ud, NULL, 0, size);
  } else {
    return e->alloc(e->alloc_ud, NULL, 0, size);
  }
}

void *upb_env_realloc(upb_env *e, void *ptr, size_t oldsize, size_t size) {
  char *ret;
  assert(oldsize <= size);
  ret = e->alloc(e->alloc_ud, ptr, oldsize, size);

#ifndef NDEBUG
  /* Overwrite non-preserved memory to ensure callers are passing the oldsize
   * that they truly require. */
  memset(ret + oldsize, 0xff, size - oldsize);
#endif

  return ret;
}

size_t upb_env_bytesallocated(const upb_env *e) {
  return e->bytes_allocated;
}


/* upb_seededalloc ************************************************************/

/* Be conservative and choose 16 in case anyone is using SSE. */
static const size_t maxalign = 16;

static size_t align_up(size_t size) {
  return ((size + maxalign - 1) / maxalign) * maxalign;
}

UPB_FORCEINLINE static void *seeded_alloc(void *ud, void *ptr, size_t oldsize,
                                          size_t size) {
  upb_seededalloc *a = ud;

  size = align_up(size);

  assert(a->mem_limit >= a->mem_ptr);

  if (oldsize == 0 && size <= (size_t)(a->mem_limit - a->mem_ptr)) {
    /* Fast path: we can satisfy from the initial allocation. */
    void *ret = a->mem_ptr;
    a->mem_ptr += size;
    return ret;
  } else {
    char *chptr = ptr;
    /* Slow path: fallback to other allocator. */
    a->need_cleanup = true;
    /* Is `ptr` part of the user-provided initial block? Don't pass it to the
     * default allocator if so; otherwise, it may try to realloc() the block. */
    if (chptr >= a->mem_base && chptr < a->mem_limit) {
      void *ret;
      assert(chptr + oldsize <= a->mem_limit);
      ret = a->alloc(a->alloc_ud, NULL, 0, size);
      if (ret) memcpy(ret, ptr, oldsize);
      return ret;
    } else {
      return a->alloc(a->alloc_ud, ptr, oldsize, size);
    }
  }
}

void upb_seededalloc_init(upb_seededalloc *a, void *mem, size_t len) {
  default_alloc_ud *ud = (default_alloc_ud*)&a->default_alloc_ud;
  a->mem_base = mem;
  a->mem_ptr = mem;
  a->mem_limit = (char*)mem + len;
  a->need_cleanup = false;
  a->returned_allocfunc = false;

  ud->head = NULL;

  upb_seededalloc_setfallbackalloc(a, default_alloc, ud);
}

void upb_seededalloc_uninit(upb_seededalloc *a) {
  if (a->alloc == default_alloc && a->need_cleanup) {
    default_alloc_cleanup(a->alloc_ud);
  }
}

UPB_FORCEINLINE void upb_seededalloc_setfallbackalloc(upb_seededalloc *a,
                                                      upb_alloc_func *alloc,
                                                      void *ud) {
  assert(!a->returned_allocfunc);
  a->alloc = alloc;
  a->alloc_ud = ud;
}

upb_alloc_func *upb_seededalloc_getallocfunc(upb_seededalloc *a) {
  a->returned_allocfunc = true;
  return seeded_alloc;
}
