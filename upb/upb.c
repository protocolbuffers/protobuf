
#include "upb/upb.int.h"

#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "upb/port_def.inc"

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
  strncpy(status->msg, msg, UPB_STATUS_MAX_MESSAGE - 1);
  status->msg[UPB_STATUS_MAX_MESSAGE - 1] = '\0';
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
  vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  status->msg[UPB_STATUS_MAX_MESSAGE - 1] = '\0';
}

void upb_status_vappenderrf(upb_status *status, const char *fmt, va_list args) {
  size_t len;
  if (!status) return;
  status->ok = false;
  len = strlen(status->msg);
  vsnprintf(status->msg + len, sizeof(status->msg) - len, fmt, args);
  status->msg[UPB_STATUS_MAX_MESSAGE - 1] = '\0';
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

struct mem_block {
  struct mem_block *next;
  uint32_t size;
  uint32_t cleanups;
  /* Data follows. */
};

typedef struct cleanup_ent {
  upb_cleanup_func *cleanup;
  void *ud;
} cleanup_ent;

static const size_t memblock_reserve = UPB_ALIGN_UP(sizeof(mem_block), 16);

static upb_arena *arena_findroot(upb_arena *a) {
  /* Path splitting keeps time complexity down, see:
   *   https://en.wikipedia.org/wiki/Disjoint-set_data_structure */
  while (a->parent != a) {
    upb_arena *next = a->parent;
    a->parent = next->parent;
    a = next;
  }
  return a;
}

static void upb_arena_addblock(upb_arena *a, upb_arena *root, void *ptr,
                               size_t size) {
  mem_block *block = ptr;

  /* The block is for arena |a|, but should appear in the freelist of |root|. */
  block->next = root->freelist;
  block->size = (uint32_t)size;
  block->cleanups = 0;
  root->freelist = block;
  a->last_size = block->size;
  if (!root->freelist_tail) root->freelist_tail = block;

  a->head.ptr = UPB_PTR_AT(block, memblock_reserve, char);
  a->head.end = UPB_PTR_AT(block, size, char);
  a->cleanups = &block->cleanups;

  UPB_POISON_MEMORY_REGION(a->head.ptr, a->head.end - a->head.ptr);
}

static bool upb_arena_allocblock(upb_arena *a, size_t size) {
  upb_arena *root = arena_findroot(a);
  size_t block_size = UPB_MAX(size, a->last_size * 2) + memblock_reserve;
  mem_block *block = upb_malloc(root->block_alloc, block_size);

  if (!block) return false;
  upb_arena_addblock(a, root, block, block_size);
  return true;
}

void *_upb_arena_slowmalloc(upb_arena *a, size_t size) {
  if (!upb_arena_allocblock(a, size)) return NULL;  /* Out of memory. */
  UPB_ASSERT(_upb_arenahas(a) >= size);
  return upb_arena_malloc(a, size);
}

static void *upb_arena_doalloc(upb_alloc *alloc, void *ptr, size_t oldsize,
                               size_t size) {
  upb_arena *a = (upb_arena*)alloc;  /* upb_alloc is initial member. */
  return upb_arena_realloc(a, ptr, oldsize, size);
}

/* Public Arena API ***********************************************************/

upb_arena *arena_initslow(void *mem, size_t n, upb_alloc *alloc) {
  const size_t first_block_overhead = sizeof(upb_arena) + memblock_reserve;
  upb_arena *a;

  /* We need to malloc the initial block. */
  n = first_block_overhead + 256;
  if (!alloc || !(mem = upb_malloc(alloc, n))) {
    return NULL;
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_arena);
  n -= sizeof(*a);

  a->head.alloc.func = &upb_arena_doalloc;
  a->block_alloc = alloc;
  a->parent = a;
  a->refcount = 1;
  a->freelist = NULL;
  a->freelist_tail = NULL;

  upb_arena_addblock(a, a, mem, n);

  return a;
}

upb_arena *upb_arena_init(void *mem, size_t n, upb_alloc *alloc) {
  upb_arena *a;

  /* Round block size down to alignof(*a) since we will allocate the arena
   * itself at the end. */
  n = UPB_ALIGN_DOWN(n, UPB_ALIGN_OF(upb_arena));

  if (UPB_UNLIKELY(n < sizeof(upb_arena))) {
    return arena_initslow(mem, n, alloc);
  }

  a = UPB_PTR_AT(mem, n - sizeof(*a), upb_arena);

  a->head.alloc.func = &upb_arena_doalloc;
  a->block_alloc = alloc;
  a->parent = a;
  a->refcount = 1;
  a->last_size = UPB_MAX(128, n);
  a->head.ptr = mem;
  a->head.end = UPB_PTR_AT(mem, n - sizeof(*a), char);
  a->freelist = NULL;
  a->cleanups = NULL;

  return a;
}

static void arena_dofree(upb_arena *a) {
  mem_block *block = a->freelist;
  UPB_ASSERT(a->parent == a);
  UPB_ASSERT(a->refcount == 0);

  while (block) {
    /* Load first since we are deleting block. */
    mem_block *next = block->next;

    if (block->cleanups > 0) {
      cleanup_ent *end = UPB_PTR_AT(block, block->size, void);
      cleanup_ent *ptr = end - block->cleanups;

      for (; ptr < end; ptr++) {
        ptr->cleanup(ptr->ud);
      }
    }

    upb_free(a->block_alloc, block);
    block = next;
  }
}

void upb_arena_free(upb_arena *a) {
  a = arena_findroot(a);
  if (--a->refcount == 0) arena_dofree(a);
}

bool upb_arena_addcleanup(upb_arena *a, void *ud, upb_cleanup_func *func) {
  cleanup_ent *ent;

  if (!a->cleanups || _upb_arenahas(a) < sizeof(cleanup_ent)) {
    if (!upb_arena_allocblock(a, 128)) return false;  /* Out of memory. */
    UPB_ASSERT(_upb_arenahas(a) >= sizeof(cleanup_ent));
  }

  a->head.end -= sizeof(cleanup_ent);
  ent = (cleanup_ent*)a->head.end;
  (*a->cleanups)++;
  UPB_UNPOISON_MEMORY_REGION(ent, sizeof(cleanup_ent));

  ent->cleanup = func;
  ent->ud = ud;

  return true;
}

void upb_arena_fuse(upb_arena *a1, upb_arena *a2) {
  upb_arena *r1 = arena_findroot(a1);
  upb_arena *r2 = arena_findroot(a2);

  if (r1 == r2) return;  /* Already fused. */

  /* We want to join the smaller tree to the larger tree.
   * So swap first if they are backwards. */
  if (r1->refcount < r2->refcount) {
    upb_arena *tmp = r1;
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
}
