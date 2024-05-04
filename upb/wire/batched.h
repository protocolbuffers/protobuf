// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// upb_Encode: parsing from a upb_Message using a upb_MiniTable.

#ifndef UPB_WIRE_BATCHED_H_
#define UPB_WIRE_BATCHED_H_

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    kUpb_MaxBatch = 128,

    kUpb_FieldNumberShift = 6,
    kUpb_BigFieldNumber = (1 << 10) - 1,

    kUpb_LongField = 31,
};

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_BATCHED_H_ */
