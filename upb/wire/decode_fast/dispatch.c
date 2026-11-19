// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/dispatch.h"

#include <stdint.h>

#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/eps_copy_input_stream.h"

// Must be last.
#include "upb/port/def.inc"

// The calls in these two functions would otherwise force
// upb_DecodeFast_MessageIsDoneFallback() to set up a stack frame (to save the
// link register) even on its common paths, which make no calls.
UPB_NOINLINE UPB_PRESERVE_NONE static upb_FastDecoder_Return
upb_DecodeFast_MessageIsDoneCheckRequired(UPB_PARSE_PARAMS) {
  ptr = _upb_Decoder_CheckRequired(d, ptr, msg, table);
  if (upb_DecodeFastData2_InRepeatedMessageRun(data2)) {
    upb_DecodeFast_Func* next = d->fast_repeated_message_next;
    UPB_MUSTTAIL return next(UPB_PARSE_ARGS);
  }
  d->message_is_done = true;
  return (upb_FastDecoder_Return){.ptr = ptr};
}

UPB_NOINLINE UPB_PRESERVE_NONE static upb_FastDecoder_Return
upb_DecodeFast_RefreshBuffer(UPB_PARSE_PARAMS) {
  int overrun;
  upb_IsDoneStatus status = UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
      &d->input, ptr, &overrun);
  UPB_ASSERT(status == kUpb_IsDoneStatus_NeedFallback);
  (void)status;
  ptr =
      UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallback)(EPS(d), ptr, overrun);

  // We successfully refreshed the buffer (otherwise the function above
  // would have thrown an error with longjmp()).  So continue with the
  // fast decoder.
  uint16_t tag = _upb_FastDecoder_LoadTag(ptr);
  _upb_Decoder_Trace(d, 'r');
  data2 = upb_DecodeFastData2_PackOriginalTag(data2, tag);
  UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);
}

UPB_NOINLINE UPB_PRESERVE_NONE upb_FastDecoder_Return
upb_DecodeFast_MessageIsDoneFallback(UPB_PARSE_PARAMS) {
  int overrun;
  switch (UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(&d->input, ptr,
                                                           &overrun)) {
    case kUpb_IsDoneStatus_Done: {
      // We've reach end-of-message.  Sync hasbits and maybe check required
      // fields.
      upb_DecodeFast_SetHasbits(msg, hasbits);
      if (UPB_UNLIKELY(table->UPB_PRIVATE(required_count))) {
        UPB_MUSTTAIL return upb_DecodeFast_MessageIsDoneCheckRequired(
            d, ptr, msg, table, 0, data, data2);
      }
      if (upb_DecodeFastData2_InRepeatedMessageRun(data2)) {
        // This message is an element of a repeated field; move on to the next
        // element without unwinding the stack.
        upb_DecodeFast_Func* next = d->fast_repeated_message_next;
        UPB_MUSTTAIL return next(d, ptr, msg, table, 0, data, data2);
      }
      d->message_is_done = true;
      return (upb_FastDecoder_Return){.ptr = ptr};
    }
    case kUpb_IsDoneStatus_NeedFallback:
      // We've reached end-of-buffer.  Refresh the buffer.
      UPB_MUSTTAIL return upb_DecodeFast_RefreshBuffer(UPB_PARSE_ARGS);
    case kUpb_IsDoneStatus_NotDone:  // Handled by caller.
    default:
      UPB_UNREACHABLE();
  }
}

upb_FastDecoder_Return _upb_FastDecoder_ErrorJmp2(upb_Decoder* d) {
  UPB_LONGJMP(d->err->buf, 1);
  return (upb_FastDecoder_Return){.ptr = NULL};
}
