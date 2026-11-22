// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/cardinality.h"

#include <stddef.h>

#include "upb/base/error_handler.h"
#include "upb/message/array.h"
#include "upb/message/internal/array.h"
#include "upb/port/overflow.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/eps_copy_input_stream.h"

// Must be last.
#include "upb/port/def.inc"

UPB_PRESERVE_MOST
upb_DecodeFast_GrowResult upb_DecodeFast_GrowArray(upb_Decoder* d,
                                                   upb_Array* arr, void* dst,
                                                   upb_DecodeFast_Type type) {
  int valbytes = upb_DecodeFast_ValueBytes(type);
  const char* old_start = upb_Array_MutableDataPtr(arr);
  UPB_ASSERT(old_start != NULL && dst != NULL && (const char*)dst >= old_start);
  size_t size = (size_t)((const char*)dst - old_start) / valbytes;
  UPB_PRIVATE(_upb_Array_SetSize)(arr, size);
  size_t new_size;
  if (UPB_UNLIKELY(
          upb_AddOverflow(size, (size_t)1, &new_size) ||
          !UPB_PRIVATE(_upb_Array_Realloc)(arr, new_size, &d->arena))) {
    upb_ErrorHandler_ThrowError(d->err, kUpb_DecodeStatus_OutOfMemory);
  }
  void* start = upb_Array_MutableDataPtr(arr);
  return (upb_DecodeFast_GrowResult){
      UPB_PTR_AT(start, size * valbytes, char),
      UPB_PTR_AT(start, upb_Array_Capacity(arr) * valbytes, char),
  };
}

UPB_PRESERVE_MOST
const char* upb_DecodeFast_IsDoneFallback(upb_Decoder* d, const char* ptr) {
  int overrun;
  upb_IsDoneStatus status = UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
      &d->input, ptr, &overrun);
  UPB_ASSERT(status == kUpb_IsDoneStatus_NeedFallback);
  return UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallback)(EPS(d), ptr,
                                                            overrun);
}
