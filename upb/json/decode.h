// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_JSON_DECODE_H_
#define UPB_JSON_DECODE_H_

#include "upb/reflection/def.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum { upb_JsonDecode_IgnoreUnknown = 1 };

UPB_API bool upb_JsonDecode(const char* buf, size_t size, upb_Message* msg,
                            const upb_MessageDef* m, const upb_DefPool* symtab,
                            int options, upb_Arena* arena, upb_Status* status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_JSONDECODE_H_ */
