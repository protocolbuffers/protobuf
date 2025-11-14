// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/eps_copy_input_stream.h"

// Must be last.
#include "upb/port/def.inc"

static const char* _upb_EpsCopyInputStream_NoOpCallback(
    upb_EpsCopyInputStream* e, const char* old_end, const char* new_start) {
  return new_start;
}

const char* UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallbackNoCallback)(
    upb_EpsCopyInputStream* e, const char* ptr, int overrun) {
  return UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallbackInline)(
      e, ptr, overrun, _upb_EpsCopyInputStream_NoOpCallback);
}
