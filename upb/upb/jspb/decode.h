// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_JSPB_DECODE_H_
#define UPB_JSPB_DECODE_H_

#include <stddef.h>

#include "upb/upb/base/status.h"
#include "upb/upb/mem/arena.h"
#include "upb/upb/message/message.h"
#include "upb/upb/mini_table/extension_registry.h"
#include "upb/upb/mini_table/message.h"

// Must be last.
#include "upb/upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum { upb_JspbDecode_IgnoreUnknown = 1 };

UPB_API bool upb_JspbDecode(const char* buf, size_t size, upb_Message* msg,
                            const upb_MiniTable* m,
                            const upb_ExtensionRegistry* extreg, int options,
                            upb_Arena* arena, upb_Status* status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/upb/port/undef.inc"

#endif /* UPB_JSPB_DECODE_H_ */
