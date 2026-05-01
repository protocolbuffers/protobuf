// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_ENCODE_H_
#define UPB_WIRE_INTERNAL_ENCODE_H_

#include <setjmp.h>
#include <stddef.h>

#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  upb_EncodeStatus status;
  upb_Arena* arena;
  // These should only be used for arithmetic and reallocation to allow full
  // aliasing analysis on the ptr argument.
  const char UPB_NODEREF *buf, *limit;
  int options;
  int depth;
  _upb_mapsorter sorter;
  jmp_buf* err;
} upb_encstate;

UPB_INLINE void UPB_PRIVATE(_upb_encstate_init)(upb_encstate* e, jmp_buf* err,
                                                upb_Arena* arena) {
  e->status = kUpb_EncodeStatus_Ok;
  e->arena = arena;
  e->buf = NULL;
  e->limit = NULL;
  e->options = 0;
  e->depth = 0;
  e->err = err;
  _upb_mapsorter_init(&e->sorter);
}

// Internal version of upb_Encode that encodes a single field.
//
// The caller must clean up the `upb_encstate` by calling
// `_upb_mapsorter_destroy(&e->sorter)` when done.
upb_EncodeStatus UPB_PRIVATE(_upb_Encode_Field)(upb_encstate* e,
                                                const upb_Message* msg,
                                                const upb_MiniTableField* field,
                                                char** buf, size_t* size,
                                                int options);

// Internal version of upb_Encode that encodes a single extension.
//
// The caller must clean up the `upb_encstate` by calling
// `_upb_mapsorter_destroy(&e->sorter)` when done.
upb_EncodeStatus UPB_PRIVATE(_upb_Encode_Extension)(
    upb_encstate* e, const upb_MiniTableExtension* ext,
    upb_MessageValue ext_val, bool is_message_set, char** buf, size_t* size,
    int options);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_INTERNAL_ENCODE_H_ */
