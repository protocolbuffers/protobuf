// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/dispatch.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/error_handler.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"
#include "upb/wire/internal/reader.h"
#include "upb/wire/types.h"

UPB_NOINLINE UPB_PRESERVE_NONE const char* upb_DecodeFast_MessageIsDoneFallback(
    UPB_PARSE_PARAMS) {
  int overrun;
  switch (UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(&d->input, ptr,
                                                           &overrun)) {
    case kUpb_IsDoneStatus_Done: {
      // We've reach end-of-message.  Sync hasbits and maybe check required
      // fields.
      d->message_is_done = true;
      upb_DecodeFast_SetHasbits(msg, hasbits);
      const upb_MiniTable* m = decode_totablep(table);
      return UPB_UNLIKELY(m->UPB_PRIVATE(required_count))
                 ? _upb_Decoder_CheckRequired(d, ptr, msg, m)
                 : ptr;
    }
    case kUpb_IsDoneStatus_NeedFallback:
      // We've reached end-of-buffer.  Refresh the buffer.
      ptr = UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallback)(&d->input, ptr,
                                                               overrun);

      // We successfully refreshed the buffer (otherwise the function above
      // would have thrown an error with longjmp()).  So continue with the
      // fast decoder.
      data = _upb_FastDecoder_LoadTag(ptr);
      UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);
    case kUpb_IsDoneStatus_NotDone:  // Handled by caller.
    default:
      UPB_UNREACHABLE();
  }
}

const char* _upb_FastDecoder_ErrorJmp2(upb_Decoder* d) {
  UPB_LONGJMP(d->err.buf, 1);
  return NULL;
}

UPB_PRESERVE_NONE const char* _upb_FastDecoder_DecodeGeneric(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  (void)data;
  upb_DecodeFast_SetHasbits(msg, hasbits);
  return ptr;
}

UPB_PRESERVE_NONE const char* _upb_FastDecoder_HandleUnknown(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t data) {
  (void)table;
  (void)data;
  upb_DecodeFast_SetHasbits(msg, hasbits);

  const char* tag_start = ptr;

  uint32_t tag;
  const char* next_ptr = upb_WireReader_ReadTag(tag_start, &tag, &d->input);

  if (UPB_UNLIKELY(!next_ptr)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
    return _upb_FastDecoder_ErrorJmp2(d);
  }
  ptr = next_ptr;

  uint32_t field_number = tag >> 3;
  uint32_t wire_type = tag & 7;

  wireval val;
  memset(&val, 0, sizeof(val));

  if (wire_type == kUpb_WireType_Delimited) {
    ptr = upb_Decoder_DecodeSize(d, ptr, &val.size);
    if (!ptr) {
      upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_Malformed);
      return _upb_FastDecoder_ErrorJmp2(d);
    }
  }

  return _upb_Decoder_DecodeUnknownField(d, ptr, msg, field_number, wire_type,
                                         val, tag_start);
}