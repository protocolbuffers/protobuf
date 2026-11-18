// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/cardinality.h"

#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/eps_copy_input_stream.h"

UPB_PRESERVE_MOST
const char* upb_DecodeFast_IsDoneFallback(upb_Decoder* d, const char* ptr) {
  int overrun;
  upb_IsDoneStatus status = UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
      &d->input, ptr, &overrun);
  UPB_ASSERT(status == kUpb_IsDoneStatus_NeedFallback);
  return UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallback)(EPS(d), ptr,
                                                            overrun);
}
