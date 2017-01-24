
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "upb/upb.h"

bool upb_dumptostderr(void *closure, const upb_status* status) {
  UPB_UNUSED(closure);
  fprintf(stderr, "%s\n", upb_status_errmsg(status));
  return false;
}

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


/* upb_upberr *****************************************************************/

upb_errorspace upb_upberr = {"upb error"};

void upb_upberr_setoom(upb_status *status) {
  status->error_space_ = &upb_upberr;
  upb_status_seterrmsg(status, "Out of memory");
}


/* upb_status *****************************************************************/

void upb_status_clear(upb_status *status) {
  if (!status) return;
  status->ok_ = true;
  status->code_ = 0;
  status->msg[0] = '\0';
}

bool upb_ok(const upb_status *status) { return status->ok_; }

upb_errorspace *upb_status_errspace(const upb_status *status) {
  return status->error_space_;
}

int upb_status_errcode(const upb_status *status) { return status->code_; }

const char *upb_status_errmsg(const upb_status *status) { return status->msg; }

void upb_status_seterrmsg(upb_status *status, const char *msg) {
  if (!status) return;
  status->ok_ = false;
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
  status->ok_ = false;
  _upb_vsnprintf(status->msg, sizeof(status->msg), fmt, args);
  nullz(status);
}

void upb_status_copy(upb_status *to, const upb_status *from) {
  if (!to) return;
  *to = *from;
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

void upb_arena_init(upb_arena *a) {
  a->alloc.func = &upb_arena_doalloc;
  a->block_alloc = &upb_alloc_global;
  a->bytes_allocated = 0;
  a->next_block_size = 256;
  a->max_block_size = 16384;
  a->cleanup_head = NULL;
  a->block_head = NULL;
}

void upb_arena_init2(upb_arena *a, void *mem, size_t size, upb_alloc *alloc) {
  upb_arena_init(a);

  if (size > sizeof(mem_block)) {
    upb_arena_addblock(a, mem, size, false);
  }

  if (alloc) {
    a->block_alloc = alloc;
  }
}

void upb_arena_uninit(upb_arena *a) {
  cleanup_ent *ent = a->cleanup_head;
  mem_block *block = a->block_head;

  while (ent) {
    ent->cleanup(ent->ud);
    ent = ent->next;
  }

  /* Must do this after running cleanup functions, because this will delete
   * the memory we store our cleanup entries in! */
  while (block) {
    mem_block *next = block->next;

    if (block->owned) {
      upb_free(a->block_alloc, block);
    }

    block = next;
  }

  /* Protect against multiple-uninit. */
  a->cleanup_head = NULL;
  a->block_head = NULL;
}

bool upb_arena_addcleanup(upb_arena *a, upb_cleanup_func *func, void *ud) {
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

void upb_env_initonly(upb_env *e) {
  e->ok_ = true;
  e->error_func_ = &default_err;
  e->error_ud_ = NULL;
}

void upb_env_init(upb_env *e) {
  upb_arena_init(&e->arena_);
  upb_env_initonly(e);
}

void upb_env_init2(upb_env *e, void *mem, size_t n, upb_alloc *alloc) {
  upb_arena_init2(&e->arena_, mem, n, alloc);
  upb_env_initonly(e);
}

void upb_env_uninit(upb_env *e) {
  upb_arena_uninit(&e->arena_);
}

void upb_env_seterrorfunc(upb_env *e, upb_error_func *func, void *ud) {
  e->error_func_ = func;
  e->error_ud_ = ud;
}

void upb_env_reporterrorsto(upb_env *e, upb_status *s) {
  e->error_func_ = &write_err_to;
  e->error_ud_ = s;
}

bool upb_env_reporterror(upb_env *e, const upb_status *status) {
  e->ok_ = false;
  return e->error_func_(e->error_ud_, status);
}

void *upb_env_malloc(upb_env *e, size_t size) {
  return upb_malloc(&e->arena_.alloc, size);
}

void *upb_env_realloc(upb_env *e, void *ptr, size_t oldsize, size_t size) {
  return upb_realloc(&e->arena_.alloc, ptr, oldsize, size);
}

void upb_env_free(upb_env *e, void *ptr) {
  upb_free(&e->arena_.alloc, ptr);
}

bool upb_env_addcleanup(upb_env *e, upb_cleanup_func *func, void *ud) {
  return upb_arena_addcleanup(&e->arena_, func, ud);
}

size_t upb_env_bytesallocated(const upb_env *e) {
  return upb_arena_bytesallocated(&e->arena_);
}
