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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file contains shared definitions that are widely used across upb.
 */

#ifndef UPB_H_
#define UPB_H_

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

/* upb_status *****************************************************************/

#define UPB_STATUS_MAX_MESSAGE 127

typedef struct {
  bool ok;
  char msg[UPB_STATUS_MAX_MESSAGE];  /* Error message; NULL-terminated. */
} upb_status;

const char *upb_status_errmsg(const upb_status *status);
bool upb_ok(const upb_status *status);

/* These are no-op if |status| is NULL. */
void upb_status_clear(upb_status *status);
void upb_status_seterrmsg(upb_status *status, const char *msg);
void upb_status_seterrf(upb_status *status, const char *fmt, ...)
    UPB_PRINTF(2, 3);
void upb_status_vseterrf(upb_status *status, const char *fmt, va_list args)
    UPB_PRINTF(2, 0);
void upb_status_vappenderrf(upb_status *status, const char *fmt, va_list args)
    UPB_PRINTF(2, 0);

/** upb_strview ************************************************************/

typedef struct {
  const char *data;
  size_t size;
} upb_strview;

UPB_INLINE upb_strview upb_strview_make(const char *data, size_t size) {
  upb_strview ret;
  ret.data = data;
  ret.size = size;
  return ret;
}

UPB_INLINE upb_strview upb_strview_makez(const char *data) {
  return upb_strview_make(data, strlen(data));
}

UPB_INLINE bool upb_strview_eql(upb_strview a, upb_strview b) {
  return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

#define UPB_STRVIEW_INIT(ptr, len) {ptr, len}

#define UPB_STRVIEW_FORMAT "%.*s"
#define UPB_STRVIEW_ARGS(view) (int)(view).size, (view).data

/** upb_alloc *****************************************************************/

/* A upb_alloc is a possibly-stateful allocator object.
 *
 * It could either be an arena allocator (which doesn't require individual
 * free() calls) or a regular malloc() (which does).  The client must therefore
 * free memory unless it knows that the allocator is an arena allocator. */

struct upb_alloc;
typedef struct upb_alloc upb_alloc;

/* A malloc()/free() function.
 * If "size" is 0 then the function acts like free(), otherwise it acts like
 * realloc().  Only "oldsize" bytes from a previous allocation are preserved. */
typedef void *upb_alloc_func(upb_alloc *alloc, void *ptr, size_t oldsize,
                             size_t size);

struct upb_alloc {
  upb_alloc_func *func;
};

UPB_INLINE void *upb_malloc(upb_alloc *alloc, size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, NULL, 0, size);
}

UPB_INLINE void *upb_realloc(upb_alloc *alloc, void *ptr, size_t oldsize,
                             size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, ptr, oldsize, size);
}

UPB_INLINE void upb_free(upb_alloc *alloc, void *ptr) {
  assert(alloc);
  alloc->func(alloc, ptr, 0, 0);
}

/* The global allocator used by upb.  Uses the standard malloc()/free(). */

extern upb_alloc upb_alloc_global;

/* Functions that hard-code the global malloc.
 *
 * We still get benefit because we can put custom logic into our global
 * allocator, like injecting out-of-memory faults in debug/testing builds. */

UPB_INLINE void *upb_gmalloc(size_t size) {
  return upb_malloc(&upb_alloc_global, size);
}

UPB_INLINE void *upb_grealloc(void *ptr, size_t oldsize, size_t size) {
  return upb_realloc(&upb_alloc_global, ptr, oldsize, size);
}

UPB_INLINE void upb_gfree(void *ptr) {
  upb_free(&upb_alloc_global, ptr);
}

/* upb_arena ******************************************************************/

/* upb_arena is a specific allocator implementation that uses arena allocation.
 * The user provides an allocator that will be used to allocate the underlying
 * arena blocks.  Arenas by nature do not require the individual allocations
 * to be freed.  However the Arena does allow users to register cleanup
 * functions that will run when the arena is destroyed.
 *
 * A upb_arena is *not* thread-safe.
 *
 * You could write a thread-safe arena allocator that satisfies the
 * upb_alloc interface, but it would not be as efficient for the
 * single-threaded case. */

typedef void upb_cleanup_func(void *ud);

struct upb_arena;
typedef struct upb_arena upb_arena;

typedef struct {
  /* We implement the allocator interface.
   * This must be the first member of upb_arena!
   * TODO(haberman): remove once handlers are gone. */
  upb_alloc alloc;

  char *ptr, *end;
} _upb_arena_head;

/* Creates an arena from the given initial block (if any -- n may be 0).
 * Additional blocks will be allocated from |alloc|.  If |alloc| is NULL, this
 * is a fixed-size arena and cannot grow. */
upb_arena *upb_arena_init(void *mem, size_t n, upb_alloc *alloc);
void upb_arena_free(upb_arena *a);
bool upb_arena_addcleanup(upb_arena *a, void *ud, upb_cleanup_func *func);
bool upb_arena_fuse(upb_arena *a, upb_arena *b);
void *_upb_arena_slowmalloc(upb_arena *a, size_t size);

UPB_INLINE upb_alloc *upb_arena_alloc(upb_arena *a) { return (upb_alloc*)a; }

UPB_INLINE size_t _upb_arenahas(upb_arena *a) {
  _upb_arena_head *h = (_upb_arena_head*)a;
  return (size_t)(h->end - h->ptr);
}

UPB_INLINE void *upb_arena_malloc(upb_arena *a, size_t size) {
  _upb_arena_head *h = (_upb_arena_head*)a;
  void* ret;
  size = UPB_ALIGN_MALLOC(size);

  if (UPB_UNLIKELY(_upb_arenahas(a) < size)) {
    return _upb_arena_slowmalloc(a, size);
  }

  ret = h->ptr;
  h->ptr += size;
  UPB_UNPOISON_MEMORY_REGION(ret, size);

#if UPB_ASAN
  {
    size_t guard_size = 32;
    if (_upb_arenahas(a) >= guard_size) {
      h->ptr += guard_size;
    } else {
      h->ptr = h->end;
    }
  }
#endif

  return ret;
}

UPB_INLINE void *upb_arena_realloc(upb_arena *a, void *ptr, size_t oldsize,
                                   size_t size) {
  void *ret = upb_arena_malloc(a, size);

  if (ret && oldsize > 0) {
    memcpy(ret, ptr, oldsize);
  }

  return ret;
}

UPB_INLINE upb_arena *upb_arena_new(void) {
  return upb_arena_init(NULL, 0, &upb_alloc_global);
}

/* Constants ******************************************************************/

/* Generic function type. */
typedef void upb_func(void);

/* A list of types as they are encoded on-the-wire. */
typedef enum {
  UPB_WIRE_TYPE_VARINT      = 0,
  UPB_WIRE_TYPE_64BIT       = 1,
  UPB_WIRE_TYPE_DELIMITED   = 2,
  UPB_WIRE_TYPE_START_GROUP = 3,
  UPB_WIRE_TYPE_END_GROUP   = 4,
  UPB_WIRE_TYPE_32BIT       = 5
} upb_wiretype_t;

/* The types a field can have.  Note that this list is not identical to the
 * types defined in descriptor.proto, which gives INT32 and SINT32 separate
 * types (we distinguish the two with the "integer encoding" enum below). */
typedef enum {
  UPB_TYPE_BOOL     = 1,
  UPB_TYPE_FLOAT    = 2,
  UPB_TYPE_INT32    = 3,
  UPB_TYPE_UINT32   = 4,
  UPB_TYPE_ENUM     = 5,  /* Enum values are int32. */
  UPB_TYPE_MESSAGE  = 6,
  UPB_TYPE_DOUBLE   = 7,
  UPB_TYPE_INT64    = 8,
  UPB_TYPE_UINT64   = 9,
  UPB_TYPE_STRING   = 10,
  UPB_TYPE_BYTES    = 11
} upb_fieldtype_t;

/* The repeated-ness of each field; this matches descriptor.proto. */
typedef enum {
  UPB_LABEL_OPTIONAL = 1,
  UPB_LABEL_REQUIRED = 2,
  UPB_LABEL_REPEATED = 3
} upb_label_t;

/* Descriptor types, as defined in descriptor.proto. */
typedef enum {
  /* Old (long) names.  TODO(haberman): remove */
  UPB_DESCRIPTOR_TYPE_DOUBLE   = 1,
  UPB_DESCRIPTOR_TYPE_FLOAT    = 2,
  UPB_DESCRIPTOR_TYPE_INT64    = 3,
  UPB_DESCRIPTOR_TYPE_UINT64   = 4,
  UPB_DESCRIPTOR_TYPE_INT32    = 5,
  UPB_DESCRIPTOR_TYPE_FIXED64  = 6,
  UPB_DESCRIPTOR_TYPE_FIXED32  = 7,
  UPB_DESCRIPTOR_TYPE_BOOL     = 8,
  UPB_DESCRIPTOR_TYPE_STRING   = 9,
  UPB_DESCRIPTOR_TYPE_GROUP    = 10,
  UPB_DESCRIPTOR_TYPE_MESSAGE  = 11,
  UPB_DESCRIPTOR_TYPE_BYTES    = 12,
  UPB_DESCRIPTOR_TYPE_UINT32   = 13,
  UPB_DESCRIPTOR_TYPE_ENUM     = 14,
  UPB_DESCRIPTOR_TYPE_SFIXED32 = 15,
  UPB_DESCRIPTOR_TYPE_SFIXED64 = 16,
  UPB_DESCRIPTOR_TYPE_SINT32   = 17,
  UPB_DESCRIPTOR_TYPE_SINT64   = 18,

  UPB_DTYPE_DOUBLE   = 1,
  UPB_DTYPE_FLOAT    = 2,
  UPB_DTYPE_INT64    = 3,
  UPB_DTYPE_UINT64   = 4,
  UPB_DTYPE_INT32    = 5,
  UPB_DTYPE_FIXED64  = 6,
  UPB_DTYPE_FIXED32  = 7,
  UPB_DTYPE_BOOL     = 8,
  UPB_DTYPE_STRING   = 9,
  UPB_DTYPE_GROUP    = 10,
  UPB_DTYPE_MESSAGE  = 11,
  UPB_DTYPE_BYTES    = 12,
  UPB_DTYPE_UINT32   = 13,
  UPB_DTYPE_ENUM     = 14,
  UPB_DTYPE_SFIXED32 = 15,
  UPB_DTYPE_SFIXED64 = 16,
  UPB_DTYPE_SINT32   = 17,
  UPB_DTYPE_SINT64   = 18
} upb_descriptortype_t;

#define UPB_MAP_BEGIN ((size_t)-1)

UPB_INLINE bool _upb_isle(void) {
  int x = 1;
  return *(char*)&x == 1;
}

UPB_INLINE uint32_t _upb_be_swap32(uint32_t val) {
  if (_upb_isle()) {
    return val;
  } else {
    return ((val & 0xff) << 24) | ((val & 0xff00) << 8) |
           ((val & 0xff0000) >> 8) | ((val & 0xff000000) >> 24);
  }
}

UPB_INLINE uint64_t _upb_be_swap64(uint64_t val) {
  if (_upb_isle()) {
    return val;
  } else {
    return ((uint64_t)_upb_be_swap32(val) << 32) | _upb_be_swap32(val >> 32);
  }
}

UPB_INLINE int _upb_lg2ceil(int x) {
  if (x <= 1) return 0;
#ifdef __GNUC__
  return 32 - __builtin_clz(x - 1);
#else
  int lg2 = 0;
  while (1 << lg2 < x) lg2++;
  return lg2;
#endif
}

UPB_INLINE int _upb_lg2ceilsize(int x) {
  return 1 << _upb_lg2ceil(x);
}

#include "upb/port_undef.inc"

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_H_ */
