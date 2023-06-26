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
** Our memory representation for parsing tables and messages themselves.
** Functions in this file are used by generated code and possibly reflection.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MESSAGE_INTERNAL_H_
#define UPB_MESSAGE_INTERNAL_H_

#include <stdlib.h>
#include <string.h>

#include "upb/hash/common.h"
#include "upb/message/extension_internal.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

extern const float kUpb_FltInfinity;
extern const double kUpb_Infinity;
extern const double kUpb_NaN;

/* Internal members of a upb_Message that track unknown fields and/or
 * extensions. We can change this without breaking binary compatibility.  We put
 * these before the user's data.  The user's upb_Message* points after the
 * upb_Message_Internal. */

typedef struct {
  /* Total size of this structure, including the data that follows.
   * Must be aligned to 8, which is alignof(upb_Message_Extension) */
  uint32_t size;

  /* Offsets relative to the beginning of this structure.
   *
   * Unknown data grows forward from the beginning to unknown_end.
   * Extension data grows backward from size to ext_begin.
   * When the two meet, we're out of data and have to realloc.
   *
   * If we imagine that the final member of this struct is:
   *   char data[size - overhead];  // overhead =
   * sizeof(upb_Message_InternalData)
   *
   * Then we have:
   *   unknown data: data[0 .. (unknown_end - overhead)]
   *   extensions data: data[(ext_begin - overhead) .. (size - overhead)] */
  uint32_t unknown_end;
  uint32_t ext_begin;
  /* Data follows, as if there were an array:
   *   char data[size - sizeof(upb_Message_InternalData)]; */
} upb_Message_InternalData;

typedef struct {
  upb_Message_InternalData* internal;
  /* Message data follows. */
} upb_Message_Internal;

/* Maps upb_CType -> memory size. */
extern char _upb_CTypeo_size[12];

UPB_INLINE size_t upb_msg_sizeof(const upb_MiniTable* t) {
  return t->size + sizeof(upb_Message_Internal);
}

// Inline version upb_Message_New(), for internal use.
UPB_INLINE upb_Message* _upb_Message_New(const upb_MiniTable* mini_table,
                                         upb_Arena* arena) {
  size_t size = upb_msg_sizeof(mini_table);
  void* mem = upb_Arena_Malloc(arena, size + sizeof(upb_Message_Internal));
  if (UPB_UNLIKELY(!mem)) return NULL;
  upb_Message* msg = UPB_PTR_AT(mem, sizeof(upb_Message_Internal), upb_Message);
  memset(mem, 0, size);
  return msg;
}

UPB_INLINE upb_Message_Internal* upb_Message_Getinternal(
    const upb_Message* msg) {
  ptrdiff_t size = sizeof(upb_Message_Internal);
  return (upb_Message_Internal*)((char*)msg - size);
}

// Discards the unknown fields for this message only.
void _upb_Message_DiscardUnknown_shallow(upb_Message* msg);

// Adds unknown data (serialized protobuf data) to the given message.
// The data is copied into the message instance.
bool _upb_Message_AddUnknown(upb_Message* msg, const char* data, size_t len,
                             upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_H_ */
