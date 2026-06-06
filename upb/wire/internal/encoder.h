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
#include <stdint.h>

#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"
#include "upb/wire/internal/back_alloc.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  upb_BackAlloc alloc;
  int options;
  int depth;
  upb_EncodeStatus status;
  _upb_mapsorter sorter;
  jmp_buf* err;
} upb_encstate;

UPB_INLINE char* UPB_PRIVATE(_upb_encstate_init)(upb_encstate* e, jmp_buf* err,
                                                 upb_Arena* arena) {
  e->status = kUpb_EncodeStatus_Ok;
  char* ptr = upb_BackAlloc_Init(&e->alloc, arena);
  e->options = 0;
  e->depth = 0;
  e->err = err;
  _upb_mapsorter_init(&e->sorter);
  return ptr;
}

UPB_INLINE void UPB_PRIVATE(_upb_encstate_destroy)(upb_encstate* e) {
  _upb_mapsorter_destroy(&e->sorter);
}

// Internal version of upb_Encode that encodes a single field.
//
// The caller must clean up the `upb_encstate` by calling
// `_upb_encstate_destroy(e)` when done.
upb_EncodeStatus UPB_PRIVATE(_upb_Encode_Field)(upb_encstate* e,
                                                const upb_Message* msg,
                                                const upb_MiniTableField* field,
                                                char** buf, size_t* size,
                                                int options);

// Internal version of upb_Encode that encodes a single extension.
//
// The caller must clean up the `upb_encstate` by calling
// `_upb_encstate_destroy(e)` when done.
upb_EncodeStatus UPB_PRIVATE(_upb_Encode_Extension)(
    upb_encstate* e, const upb_MiniTableExtension* ext,
    upb_MessageValue ext_val, bool is_message_set, char** buf, size_t* size,
    int options);

char* encode_message(char* ptr, upb_encstate* e, const upb_Message* msg,
                     const upb_MiniTable* m, size_t* size);

#define kUpb_Encoder_EncodeVarint32MaxSize 5
static char* upb_Encoder_EncodeVarint32(uint32_t val, char* ptr) {
  do {
    uint8_t byte = val & 0x7fU;
    val >>= 7;
    if (val) byte |= 0x80U;
    *(ptr++) = byte;
  } while (val);
  return ptr;
}

UPB_INLINE
bool _upb_Encoder_AddEnumValueToUnknown(upb_Message* msg,
                                        const upb_MiniTableField* field,
                                        uint64_t val, upb_Arena* arena) {
  // Unrecognized enum goes into unknown fields.
  // For packed fields the tag could be arbitrarily far in the past,
  // so we just re-encode the tag and value here.
  const uint32_t tag =
      ((uint32_t)field->UPB_PRIVATE(number) << 3) | kUpb_WireType_Varint;
  char buf[2 * kUpb_Encoder_EncodeVarint32MaxSize];
  char* end = buf;
  end = upb_Encoder_EncodeVarint32(tag, end);
  end = upb_Encoder_EncodeVarint32(val, end);

  return UPB_PRIVATE(_upb_Message_AddUnknown)(msg, buf, end - buf, arena,
                                              kUpb_AddUnknown_Copy);
}

bool _upb_Encoder_AddMapEntryUnknown(upb_Message* msg,
                                     const upb_MiniTableField* field,
                                     upb_Message* ent_msg,
                                     const upb_MiniTable* entry,
                                     upb_Arena* arena);

upb_EncodeStatus _upb_Encode(const upb_Message* msg, const upb_MiniTable* l,
                             int options, upb_Arena* arena, char** buf,
                             size_t* size, bool prepend_len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_INTERNAL_ENCODE_H_ */
