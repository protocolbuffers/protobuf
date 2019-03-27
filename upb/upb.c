
#include "upb/upb.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "upb/port_def.inc"

/* Guarantee null-termination and provide ellipsis truncation.
 * It may be tempting to "optimize" this by initializing these final
 * four bytes up-front and then being careful never to overwrite them,
 * this is safer and simpler. */
static void nullz(upb_status *status) {
  const char *ellipsis = "...";
  size_t len = strlen(ellipsis);
  UPB_ASSERT(sizeof(status->msg) > len);
  memcpy(status->msg + sizeof(status->msg) - len, ellipsis, len);
}

/* upb_status *****************************************************************/

void upb_status_clear(upb_status *status) {
  if (!status) return;
  status->ok = true;
  status->msg[0] = '\0';
}

bool upb_ok(const upb_status *status) { return status->ok; }

const char *upb_status_errmsg(const upb_status *status) { return status->msg; }

void upb_status_seterrmsg(upb_status *status, const char *msg) {
  if (!status) return;
  status->ok = false;
  strncpy(status->msg, msg, sizeof(status->msg));
  nullz(status);
}

void upb_status_seterrf(upb_status *status, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  upb_status_vseterrf(status, fmt, args);
  va_end(args);
}

void upb_status_vseterrf(upb_status *status, const char *fmt, va_list args) {
  if (!status) return;
  status->ok = false;
  _upb_vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  nullz(status);
}

/* upb_alloc ******************************************************************/

static void *upb_global_allocfunc(upb_alloc *alloc, void *ptr, size_t oldsize,
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

upb_alloc upb_alloc_global = {&upb_global_allocfunc};

/* upb_arena ******************************************************************/

/* Be conservative and choose 16 in case anyone is using SSE. */
static const size_t maxalign = 16;

static size_t align_up_max(size_t size) {
  return ((size + maxalign - 1) / maxalign) * maxalign;
}

struct upb_arena {
  /* We implement the allocator interface.
   * This must be the first member of upb_arena! */
  upb_alloc alloc;

  /* Allocator to allocate arena blocks.  We are responsible for freeing these
   * when we are destroyed. */
  upb_alloc *block_alloc;

  size_t bytes_allocated;
  size_t next_block_size;
  size_t max_block_size;

  /* Linked list of blocks.  Points to an arena_block, defined in env.c */
  void *block_head;

  /* Cleanup entries.  Pointer to a cleanup_ent, defined in env.c */
  void *cleanup_head;
};

typedef struct mem_block {
  struct mem_block *next;
  size_t size;
  size_t used;
  bool owned;
  /* Data follows. */
} mem_block;

typedef struct cleanup_ent {
  struct cleanup_ent *next;
  upb_cleanup_func *cleanup;
  void *ud;
} cleanup_ent;

static void upb_arena_addblock(upb_arena *a, void *ptr, size_t size,
                               bool owned) {
  mem_block *block = ptr;

  block->next = a->block_head;
  block->size = size;
  block->used = align_up_max(sizeof(mem_block));
  block->owned = owned;

  a->block_head = block;

  /* TODO(haberman): ASAN poison. */
}

static mem_block *upb_arena_allocblock(upb_arena *a, size_t size) {
  size_t block_size = UPB_MAX(size, a->next_block_size) + sizeof(mem_block);
  mem_block *block = upb_malloc(a->block_alloc, block_size);

  if (!block) {
    return NULL;
  }

  upb_arena_addblock(a, block, block_size, true);
  a->next_block_size = UPB_MIN(block_size * 2, a->max_block_size);

  return block;
}

static void *upb_arena_doalloc(upb_alloc *alloc, void *ptr, size_t oldsize,
                               size_t size) {
  upb_arena *a = (upb_arena*)alloc;  /* upb_alloc is initial member. */
  mem_block *block = a->block_head;
  void *ret;

  if (size == 0) {
    return NULL;  /* We are an arena, don't need individual frees. */
  }

  size = align_up_max(size);

  /* TODO(haberman): special-case if this is a realloc of the last alloc? */

  if (!block || block->size - block->used < size) {
    /* Slow path: have to allocate a new block. */
    block = upb_arena_allocblock(a, size);

    if (!block) {
      return NULL;  /* Out of memory. */
    }
  }

  ret = (char*)block + block->used;
  block->used += size;

  if (oldsize > 0) {
    memcpy(ret, ptr, oldsize);  /* Preserve existing data. */
  }

  /* TODO(haberman): ASAN unpoison. */

  a->bytes_allocated += size;
  return ret;
}

/* Public Arena API ***********************************************************/

#define upb_alignof(type) offsetof (struct { char c; type member; }, member)

upb_arena *upb_arena_init(void *mem, size_t n, upb_alloc *alloc) {
  const size_t first_block_overhead = sizeof(upb_arena) + sizeof(mem_block);
  upb_arena *a;
  bool owned = false;

  /* Round block size down to alignof(*a) since we will allocate the arena
   * itself at the end. */
  n &= ~(upb_alignof(upb_arena) - 1);

  if (n < first_block_overhead) {
    /* We need to malloc the initial block. */
    n = first_block_overhead + 256;
    owned = true;
    if (!alloc || !(mem = upb_malloc(alloc, n))) {
      return NULL;
    }
  }

  a = (void*)((char*)mem + n - sizeof(*a));
  n -= sizeof(*a);

  a->alloc.func = &upb_arena_doalloc;
  a->block_alloc = &upb_alloc_global;
  a->bytes_allocated = 0;
  a->next_block_size = 256;
  a->max_block_size = 16384;
  a->cleanup_head = NULL;
  a->block_head = NULL;
  a->block_alloc = alloc;

  upb_arena_addblock(a, mem, n, owned);

  return a;
}

#undef upb_alignof

void upb_arena_free(upb_arena *a) {
  cleanup_ent *ent = a->cleanup_head;
  mem_block *block = a->block_head;

  while (ent) {
    ent->cleanup(ent->ud);
    ent = ent->next;
  }

  /* Must do this after running cleanup functions, because this will delete
   * the memory we store our cleanup entries in! */
  while (block) {
    /* Load first since we are deleting block. */
    mem_block *next = block->next;

    if (block->owned) {
      upb_free(a->block_alloc, block);
    }

    block = next;
  }
}

bool upb_arena_addcleanup(upb_arena *a, void *ud, upb_cleanup_func *func) {
  cleanup_ent *ent = upb_malloc(&a->alloc, sizeof(cleanup_ent));
  if (!ent) {
    return false;  /* Out of memory. */
  }

  ent->cleanup = func;
  ent->ud = ud;
  ent->next = a->cleanup_head;
  a->cleanup_head = ent;

  return true;
}

size_t upb_arena_bytesallocated(const upb_arena *a) {
  return a->bytes_allocated;
}
