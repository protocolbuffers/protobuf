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

#if UPB_FASTTABLE
UPB_FORCEINLINE
bool upb_DecodeFast_Delimited_Message(upb_Decoder* d, const char** ptr,
                                      upb_DecodeFast_MessageContext* c,
                                      upb_DecodeFastNext* ret) {
  const char* p = *ptr;
  int size;

  if (!upb_DecodeFast_DecodeSize(d, &p, &size, ret)) return false;

  if ((ptrdiff_t)size <= d->input.limit_ptr - p) {
    // Fast path: inlined upb_EpsCopyInputStream_TryParseDelimitedFast
    const char* saved_limit_ptr = d->input.limit_ptr;
    int saved_limit = d->input.limit;
    d->input.limit_ptr = p + size;
    d->input.limit = d->input.limit_ptr - d->input.end;

    p = _upb_Decoder_DecodeMessage_PreserveNone(d, p, c->msg, c->table);

    d->input.limit_ptr = saved_limit_ptr;
    d->input.limit = saved_limit;
  } else {
    // Slow path: with Push/Pop limit
    ptrdiff_t delta = upb_EpsCopyInputStream_PushLimit(&d->input, p, size);
    if (UPB_UNLIKELY(delta < 0)) {
      *ptr = NULL;
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
    }

    p = _upb_Decoder_DecodeMessage_PreserveNone(d, p, c->msg, c->table);

    if (UPB_UNLIKELY(p == NULL)) goto fail;
    upb_EpsCopyInputStream_PopLimit(&d->input, p, delta);
  }

  if (UPB_UNLIKELY(p == NULL)) goto fail;
  if (UPB_UNLIKELY(d->end_group != DECODE_NOGROUP)) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  }
  *ptr = p;
  return true;

fail:
  UPB_ASSERT(*ret != kUpb_DecodeFastNext_FallbackToMiniTable);
  *ptr = NULL;
  return false;
}
#endif

UPB_FORCEINLINE
bool upb_DecodeFast_SingleMessage(upb_Decoder* d, const char** ptr, void* dst,
                                  upb_DecodeFast_Type type,
                                  upb_DecodeFastNext* next, void* ctx) {
  upb_DecodeFast_MessageContext* c = ctx;
  void** submsg_dst = dst;

  if (c->is_repeated || UPB_LIKELY(*submsg_dst == NULL)) {
    c->msg = *submsg_dst = _upb_Message_New(c->table, &d->arena);
  } else {
    c->msg = *submsg_dst;  // Reusing non-repeated message.
  }

#if UPB_FASTTABLE
  return upb_DecodeFast_Delimited_Message(d, ptr, c, next);
#else
  UPB_UNREACHABLE();
#endif
}

UPB_FORCEINLINE
void upb_DecodeFast_Message(upb_Decoder* d, const char** ptr, upb_Message* msg,
                            const upb_MiniTable* table, uint64_t* hasbits,
                            uint64_t* data, upb_DecodeFastNext* ret,
                            upb_DecodeFast_Type type,
                            upb_DecodeFast_Cardinality card,
                            upb_DecodeFast_TagSize tagsize, uint64_t data2) {
  uint16_t submsg_idx = upb_DecodeFastData_GetFieldIndex(*data);

  // OPT: we could remove an indirection by precomputing the offset directly
  // to the submessage table pointer, instead of doing an extra hop through the
  // field.
  const upb_MiniTableField* field =
      upb_MiniTable_GetFieldByIndex(table, submsg_idx);
  const upb_MiniTable* subtablep = upb_MiniTable_GetSubMessageTable(field);

  upb_DecodeFast_MessageContext ctx = {subtablep,
                                       card == kUpb_DecodeFast_Repeated};

  if (subtablep == NULL) {
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, ret);
    return;
  }

  if (--d->depth < 0) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);
  }

  upb_DecodeFast_Unpacked(d, ptr, msg, data, hasbits, ret, type, card, tagsize,
                          &upb_DecodeFast_SingleMessage, &ctx, data2);

  d->depth++;
}

#define F(type, card, tagsize)                                             \
  upb_FastDecoder_Return UPB_PRESERVE_NONE UPB_DECODEFAST_FUNCNAME(        \
      type, card, tagsize)(UPB_PARSE_PARAMS) {                             \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;                \
    upb_DecodeFast_Message(d, &ptr, msg, table, &hasbits, &data, &next,    \
                           kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                           kUpb_DecodeFast_##tagsize, data2);              \
    UPB_DECODEFAST_NEXT(next);                                             \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Message)

#undef F
#undef FASTDECODE_SUBMSG
