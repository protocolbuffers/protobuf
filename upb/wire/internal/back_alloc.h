// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_WIRE_INTERNAL_BACK_ALLOC_H__
#define GOOGLE_UPB_UPB_WIRE_INTERNAL_BACK_ALLOC_H__

#include <stddef.h>

#include "upb/mem/arena.h"
#include "upb/mem/internal/arena.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Allocates memory from the back of the arena.
typedef struct {
  upb_Arena* arena;
  char *buf, *limit;
  bool standalone;
} upb_BackAlloc;

// Needed because C doesn't allow NULL - NULL.
extern char upb_BackAlloc_sentinel;

char* upb_BackAlloc_Grow(upb_BackAlloc* a, char* ptr, size_t need);

UPB_INLINE char* upb_BackAlloc_Init(upb_BackAlloc* a, upb_Arena* arena) {
  a->arena = arena;
  // This could eagerly steal whatever's in the arena, since stealing with a
  // minimum of 0 can't fail.
  a->buf = &upb_BackAlloc_sentinel;
  a->limit = &upb_BackAlloc_sentinel;
  a->standalone = false;
  return a->limit;
}

UPB_INLINE void upb_BackAlloc_Abort(upb_BackAlloc* a) {
  if (a->standalone) {
    UPB_PRIVATE(_upb_Arena_FreeBlock)(a->arena, a->buf);
  } else if (a->limit != a->buf) {
    UPB_PRIVATE(_upb_Arena_UseBlock)(a->arena, a->buf, a->limit - a->buf);
  }
}

UPB_INLINE size_t upb_BackAlloc_Finish(upb_BackAlloc* a, const char* ptr) {
  if (a->standalone) {
    UPB_PRIVATE(_upb_Arena_AddBlock)(a->arena, a->buf);
  }
  if (ptr != a->buf) {
    UPB_PRIVATE(_upb_Arena_UseBlock)(a->arena, a->buf, ptr - a->buf);
  }
  return a->limit - ptr;
}

UPB_FORCEINLINE bool upb_BackAlloc_HasBytes(const upb_BackAlloc* a,
                                            const char* ptr, size_t need) {
  size_t have = ptr - a->buf;
  return have >= need;
}

UPB_FORCEINLINE char* upb_BackAlloc_Reserve(upb_BackAlloc* a, char* ptr,
                                            size_t need) {
  return upb_BackAlloc_HasBytes(a, ptr, need)
             ? ptr - need
             : upb_BackAlloc_Grow(a, ptr, need);
}

UPB_INLINE size_t upb_BackAlloc_Size(const upb_BackAlloc* a, const char* ptr) {
  return a->limit - ptr;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // GOOGLE_UPB_UPB_WIRE_INTERNAL_BACK_ALLOC_H__
