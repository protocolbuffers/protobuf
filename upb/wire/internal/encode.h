// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_ENCODE_H_
#define UPB_WIRE_INTERNAL_ENCODE_H_

#include <setjmp.h>
#include <stddef.h>

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Internal version of upb_Encode that encodes a single field.
// Returns kUpb_EncodeStatus_Ok if the field was not present (buf/size will be
// 0)
upb_EncodeStatus UPB_PRIVATE(_upb_Encode_Field)(jmp_buf* err,
                                                const upb_Message* msg,
                                                const upb_MiniTableField* field,
                                                int options, upb_Arena* arena,
                                                char** buf, size_t* size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_INTERNAL_ENCODE_H_ */
