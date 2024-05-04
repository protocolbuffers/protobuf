// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// upb_Encode: parsing from a upb_Message using a upb_MiniTable.

#ifndef UPB_WIRE_BATCHED_ENCODE_H_
#define UPB_WIRE_BATCHED_ENCODE_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_API upb_EncodeStatus upb_BatchedEncode(const upb_Message* msg,
                                           const upb_MiniTable* l, int options,
                                           upb_Arena* arena, char** buf,
                                           size_t* size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_BATCHED_ENCODE_H_ */
