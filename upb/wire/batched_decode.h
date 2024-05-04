// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// upb_decode: parsing into a upb_Message using a upb_MiniTable.

#ifndef UPB_WIRE_BATCHED_DECODE_H_
#define UPB_WIRE_BATCHED_DECODE_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_API upb_DecodeStatus upb_BatchedDecode(const char* buf, size_t size,
                                           upb_Message* msg,
                                           const upb_MiniTable* mt,
                                           const upb_ExtensionRegistry* extreg,
                                           int options, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_BATCHED_DECODE_H_ */
