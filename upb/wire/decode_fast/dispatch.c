// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/dispatch.h"

#include <stdint.h>

#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/eps_copy_input_stream.h"

// Must be last.
#include "upb/port/def.inc"

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
      _upb_Decoder_Trace(d, 'r');
      UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);
    case kUpb_IsDoneStatus_NotDone:  // Handled by caller.
    default:
      UPB_UNREACHABLE();
  }
}

const char* _upb_FastDecoder_ErrorJmp2(upb_Decoder* d) {
  UPB_LONGJMP(d->err->buf, 1);
  return NULL;
}
