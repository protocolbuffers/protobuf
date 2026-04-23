// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_DECODE_FAST_FIELD_DISPATCH_H_
#define UPB_WIRE_DECODE_FAST_FIELD_DISPATCH_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "upb/message/message.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

// The standard set of arguments passed to each parsing function.
// Thanks to x86-64 calling conventions, these will stay in registers.
#define UPB_PARSE_PARAMS                                             \
  upb_Decoder *d, const char *ptr, upb_Message *msg, intptr_t table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

UPB_INLINE uint32_t _upb_FastDecoder_LoadTag(const char* ptr) {
  uint16_t tag;
  memcpy(&tag, ptr, 2);
  return tag;
}

// We have to disable HWASAN for this function because we steal the high byte
// of the `table` pointer for our own purposes (the table mask). This overwrites
// the tag that HWASAN depends on for its own checks.
__attribute__((no_sanitize("hwaddress"))) UPB_INLINE
    UPB_PRESERVE_NONE const char*
    _upb_FastDecoder_TagDispatch(struct upb_Decoder* d, const char* ptr,
                                 upb_Message* msg, intptr_t table,
                                 uint64_t hasbits, uint64_t tag) {
  const upb_MiniTable* table_p = decode_totablep(table);
  uint8_t mask = table;
  size_t ofs = tag & mask;
  UPB_ASSUME((ofs & 0xf8) == ofs);

#ifdef __cplusplus
  // Unreachable, since this header is only used from C, but when the header
  // module is compiled for C++ we need to avoid a compilation error.
  UPB_UNREACHABLE();
  UPB_UNUSED(table_p);
  _upb_FastTable_Entry* ent = NULL;
#else
  const _upb_FastTable_Entry* ent = &table_p->UPB_PRIVATE(fasttable)[ofs >> 3];
#endif

  UPB_MUSTTAIL return ent->field_parser(d, ptr, msg, table, hasbits,
                                        ent->field_data ^ tag);
}

UPB_NOINLINE UPB_PRESERVE_NONE const char* upb_DecodeFast_MessageIsDoneFallback(
    UPB_PARSE_PARAMS);

UPB_FORCEINLINE UPB_PRESERVE_NONE const char* upb_DecodeFast_Dispatch(
    UPB_PARSE_PARAMS) {
  int overrun;
  upb_IsDoneStatus status = UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
      &d->input, ptr, &overrun);

  if (UPB_UNLIKELY(status != kUpb_IsDoneStatus_NotDone)) {
    // End-of-message or end-of-buffer.
    UPB_MUSTTAIL return upb_DecodeFast_MessageIsDoneFallback(UPB_PARSE_ARGS);
  }

  // Read two bytes of tag data (for a one-byte tag, the high byte is junk).
  data = _upb_FastDecoder_LoadTag(ptr);
  _upb_Decoder_Trace(d, 'D');
  UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
void upb_DecodeFast_SetHasbits(upb_Message* msg, uint64_t hasbits) {
  // TODO: Can we use `=` instead of` |=`?
  *(uint32_t*)&msg[1] |= hasbits;
}

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_DECODE_FAST_FIELD_DISPATCH_H_
