// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>

#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  const upb_MiniTable* table;
  bool is_repeated;
  upb_Message* msg;
} upb_DecodeFast_MessageContext;

static const char* upb_DecodeFast_MessageData(upb_EpsCopyInputStream* st,
                                              const char* ptr, int size,
                                              void* ctx) {
  upb_Decoder* d = (upb_Decoder*)st;
  upb_DecodeFast_MessageContext* c = ctx;
  ptr = _upb_Decoder_DecodeMessage((upb_Decoder*)st, ptr, c->msg, c->table);
  if (d->end_group != DECODE_NOGROUP) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  return ptr;
}

static bool upb_DecodeFast_SingleMessage(upb_Decoder* d, const char** ptr,
                                         void* dst, upb_DecodeFast_Type type,
                                         upb_DecodeFastNext* next, void* ctx) {
  upb_DecodeFast_MessageContext* c = ctx;
  void** submsg_dst = dst;

  if (c->is_repeated || UPB_LIKELY(*submsg_dst == NULL)) {
    c->msg = *submsg_dst = _upb_Message_New(c->table, &d->arena);
  } else {
    c->msg = *submsg_dst;  // Reusing non-repeated message.
  }

  return upb_DecodeFast_Delimited(d, ptr, &upb_DecodeFast_MessageData, next, c);
}

void upb_DecodeFast_Message(upb_Decoder* d, const char** ptr, upb_Message* msg,
                            intptr_t table, uint64_t* hasbits, uint64_t* data,
                            upb_DecodeFastNext* ret, upb_DecodeFast_Type type,
                            upb_DecodeFast_Cardinality card,
                            upb_DecodeFast_TagSize tagsize) {
  uint16_t submsg_idx = upb_DecodeFastData_GetFieldIndex(*data);
  const upb_MiniTable* tablep = decode_totablep(table);

  // OPT: we could remove an indirection by precomputing the offset directly
  // to the submessage table pointer, instead of doing an extra hop through the
  // field.
  const upb_MiniTableField* field =
      upb_MiniTable_GetFieldByIndex(tablep, submsg_idx);
  const upb_MiniTable* subtablep = upb_MiniTable_GetSubMessageTable(field);

  upb_DecodeFast_MessageContext ctx = {subtablep,
                                       card == kUpb_DecodeFast_Repeated};

  if (subtablep == NULL) {
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, ret);
    return;
  }

  // This check is technically redundant because upb_DecodeFast_Unpacked will
  // check the tag again. But we perform this check early to avoid erroring out
  // for "bounds exceeded" if this is not actually a sub-message field.
  //
  // The compiler should be able to see that the tag was already checked, so
  // the later check should be optimized away.
  if (UPB_UNLIKELY(!upb_DecodeFast_MaskedTagIsZero(*data, tagsize))) {
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, ret);
    return;
  }

  if (--d->depth == 0) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);
  }

  upb_DecodeFast_Unpacked(d, ptr, msg, data, hasbits, ret, type, card, tagsize,
                          &upb_DecodeFast_SingleMessage, &ctx);

  d->depth++;
}

#define F(type, card, tagsize)                                             \
  const char* UPB_PRESERVE_NONE UPB_DECODEFAST_FUNCNAME(                   \
      type, card, tagsize)(UPB_PARSE_PARAMS) {                             \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;                \
    upb_DecodeFast_Message(d, &ptr, msg, table, &hasbits, &data, &next,    \
                           kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                           kUpb_DecodeFast_##tagsize);                     \
    UPB_DECODEFAST_NEXT(next);                                             \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Message)

#undef F
#undef FASTDECODE_SUBMSG
