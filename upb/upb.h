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

/* upb_Status *****************************************************************/

#define _kUpb_Status_MaxMessage 127

typedef struct {
  bool ok;
  char msg[_kUpb_Status_MaxMessage]; /* Error message; NULL-terminated. */
} upb_Status;

const char* upb_Status_ErrorMessage(const upb_Status* status);
bool upb_Status_IsOk(const upb_Status* status);

/* These are no-op if |status| is NULL. */
void upb_Status_Clear(upb_Status* status);
void upb_Status_SetErrorMessage(upb_Status* status, const char* msg);
void upb_Status_SetErrorFormat(upb_Status* status, const char* fmt, ...)
    UPB_PRINTF(2, 3);
void upb_Status_VSetErrorFormat(upb_Status* status, const char* fmt,
                                va_list args) UPB_PRINTF(2, 0);
void upb_Status_VAppendErrorFormat(upb_Status* status, const char* fmt,
                                   va_list args) UPB_PRINTF(2, 0);

/** upb_StringView ************************************************************/

typedef struct {
  const char* data;
  size_t size;
} upb_StringView;

UPB_INLINE upb_StringView upb_StringView_FromDataAndSize(const char* data,
                                                         size_t size) {
  upb_StringView ret;
  ret.data = data;
  ret.size = size;
  return ret;
}

UPB_INLINE upb_StringView upb_StringView_FromString(const char* data) {
  return upb_StringView_FromDataAndSize(data, strlen(data));
}

UPB_INLINE bool upb_StringView_IsEqual(upb_StringView a, upb_StringView b) {
  return a.size == b.size && memcmp(a.data, b.data, a.size) == 0;
}

#define UPB_STRINGVIEW_INIT(ptr, len) \
  { ptr, len }

#define UPB_STRINGVIEW_FORMAT "%.*s"
#define UPB_STRINGVIEW_ARGS(view) (int)(view).size, (view).data

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
typedef void* upb_alloc_func(upb_alloc* alloc, void* ptr, size_t oldsize,
                             size_t size);

struct upb_alloc {
  upb_alloc_func* func;
};

UPB_INLINE void* upb_malloc(upb_alloc* alloc, size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, NULL, 0, size);
}

UPB_INLINE void* upb_realloc(upb_alloc* alloc, void* ptr, size_t oldsize,
                             size_t size) {
  UPB_ASSERT(alloc);
  return alloc->func(alloc, ptr, oldsize, size);
}

UPB_INLINE void upb_free(upb_alloc* alloc, void* ptr) {
  assert(alloc);
  alloc->func(alloc, ptr, 0, 0);
}

/* The global allocator used by upb.  Uses the standard malloc()/free(). */

extern upb_alloc upb_alloc_global;

/* Functions that hard-code the global malloc.
 *
 * We still get benefit because we can put custom logic into our global
 * allocator, like injecting out-of-memory faults in debug/testing builds. */

UPB_INLINE void* upb_gmalloc(size_t size) {
  return upb_malloc(&upb_alloc_global, size);
}

UPB_INLINE void* upb_grealloc(void* ptr, size_t oldsize, size_t size) {
  return upb_realloc(&upb_alloc_global, ptr, oldsize, size);
}

UPB_INLINE void upb_gfree(void* ptr) { upb_free(&upb_alloc_global, ptr); }

/* upb_Arena ******************************************************************/

/* upb_Arena is a specific allocator implementation that uses arena allocation.
 * The user provides an allocator that will be used to allocate the underlying
 * arena blocks.  Arenas by nature do not require the individual allocations
 * to be freed.  However the Arena does allow users to register cleanup
 * functions that will run when the arena is destroyed.
 *
 * A upb_Arena is *not* thread-safe.
 *
 * You could write a thread-safe arena allocator that satisfies the
 * upb_alloc interface, but it would not be as efficient for the
 * single-threaded case. */

typedef void upb_CleanupFunc(void* ud);

struct upb_Arena;
typedef struct upb_Arena upb_Arena;

typedef struct {
  /* We implement the allocator interface.
   * This must be the first member of upb_Arena!
   * TODO(haberman): remove once handlers are gone. */
  upb_alloc alloc;

  char *ptr, *end;
} _upb_ArenaHead;

/* Creates an arena from the given initial block (if any -- n may be 0).
 * Additional blocks will be allocated from |alloc|.  If |alloc| is NULL, this
 * is a fixed-size arena and cannot grow. */
upb_Arena* upb_Arena_Init(void* mem, size_t n, upb_alloc* alloc);
void upb_Arena_Free(upb_Arena* a);
bool upb_Arena_AddCleanup(upb_Arena* a, void* ud, upb_CleanupFunc* func);
bool upb_Arena_Fuse(upb_Arena* a, upb_Arena* b);
void* _upb_Arena_SlowMalloc(upb_Arena* a, size_t size);

UPB_INLINE upb_alloc* upb_Arena_Alloc(upb_Arena* a) { return (upb_alloc*)a; }

UPB_INLINE size_t _upb_ArenaHas(upb_Arena* a) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  return (size_t)(h->end - h->ptr);
}

UPB_INLINE void* upb_Arena_Malloc(upb_Arena* a, size_t size) {
  _upb_ArenaHead* h = (_upb_ArenaHead*)a;
  void* ret;
  size = UPB_ALIGN_MALLOC(size);

  if (UPB_UNLIKELY(_upb_ArenaHas(a) < size)) {
    return _upb_Arena_SlowMalloc(a, size);
  }

  ret = h->ptr;
  h->ptr += size;
  UPB_UNPOISON_MEMORY_REGION(ret, size);

#if UPB_ASAN
  {
    size_t guard_size = 32;
    if (_upb_ArenaHas(a) >= guard_size) {
      h->ptr += guard_size;
    } else {
      h->ptr = h->end;
    }
  }
#endif

  return ret;
}

UPB_INLINE void* upb_Arena_Realloc(upb_Arena* a, void* ptr, size_t oldsize,
                                   size_t size) {
  void* ret = upb_Arena_Malloc(a, size);

  if (ret && oldsize > 0) {
    memcpy(ret, ptr, oldsize);
  }

  return ret;
}

UPB_INLINE upb_Arena* upb_Arena_New(void) {
  return upb_Arena_Init(NULL, 0, &upb_alloc_global);
}

/* Constants ******************************************************************/

/* A list of types as they are encoded on-the-wire. */
typedef enum {
  kUpb_WireType_Varint = 0,
  kUpb_WireType_64Bit = 1,
  kUpb_WireType_Delimited = 2,
  kUpb_WireType_StartGroup = 3,
  kUpb_WireType_EndGroup = 4,
  kUpb_WireType_32Bit = 5
} upb_WireType;

/* The types a field can have.  Note that this list is not identical to the
 * types defined in descriptor.proto, which gives INT32 and SINT32 separate
 * types (we distinguish the two with the "integer encoding" enum below). */
typedef enum {
  kUpb_CType_Bool = 1,
  kUpb_CType_Float = 2,
  kUpb_CType_Int32 = 3,
  kUpb_CType_UInt32 = 4,
  kUpb_CType_Enum = 5, /* Enum values are int32. */
  kUpb_CType_Message = 6,
  kUpb_CType_Double = 7,
  kUpb_CType_Int64 = 8,
  kUpb_CType_UInt64 = 9,
  kUpb_CType_String = 10,
  kUpb_CType_Bytes = 11
} upb_CType;

/* The repeated-ness of each field; this matches descriptor.proto. */
typedef enum {
  kUpb_Label_Optional = 1,
  kUpb_Label_Required = 2,
  kUpb_Label_Repeated = 3
} upb_Label;

/* Descriptor types, as defined in descriptor.proto. */
typedef enum {
  kUpb_FieldType_Double = 1,
  kUpb_FieldType_Float = 2,
  kUpb_FieldType_Int64 = 3,
  kUpb_FieldType_UInt64 = 4,
  kUpb_FieldType_Int32 = 5,
  kUpb_FieldType_Fixed64 = 6,
  kUpb_FieldType_Fixed32 = 7,
  kUpb_FieldType_Bool = 8,
  kUpb_FieldType_String = 9,
  kUpb_FieldType_Group = 10,
  kUpb_FieldType_Message = 11,
  kUpb_FieldType_Bytes = 12,
  kUpb_FieldType_UInt32 = 13,
  kUpb_FieldType_Enum = 14,
  kUpb_FieldType_SFixed32 = 15,
  kUpb_FieldType_SFixed64 = 16,
  kUpb_FieldType_SInt32 = 17,
  kUpb_FieldType_SInt64 = 18
} upb_FieldType;

#define kUpb_Map_Begin ((size_t)-1)

UPB_INLINE bool _upb_IsLittleEndian(void) {
  int x = 1;
  return *(char*)&x == 1;
}

UPB_INLINE uint32_t _upb_BigEndian_Swap32(uint32_t val) {
  if (_upb_IsLittleEndian()) {
    return val;
  } else {
    return ((val & 0xff) << 24) | ((val & 0xff00) << 8) |
           ((val & 0xff0000) >> 8) | ((val & 0xff000000) >> 24);
  }
}

UPB_INLINE uint64_t _upb_BigEndian_Swap64(uint64_t val) {
  if (_upb_IsLittleEndian()) {
    return val;
  } else {
    return ((uint64_t)_upb_BigEndian_Swap32(val) << 32) |
           _upb_BigEndian_Swap32(val >> 32);
  }
}

UPB_INLINE int _upb_Log2Ceiling(int x) {
  if (x <= 1) return 0;
#ifdef __GNUC__
  return 32 - __builtin_clz(x - 1);
#else
  int lg2 = 0;
  while (1 << lg2 < x) lg2++;
  return lg2;
#endif
}

UPB_INLINE int _upb_Log2Ceilingsize(int x) { return 1 << _upb_Log2Ceiling(x); }

#include "upb/port_undef.inc"

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UPB_H_ */
