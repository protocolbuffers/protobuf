// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_JSPB_ENCODE_H_
#define UPB_JSPB_ENCODE_H_

#include <stddef.h>

#include "upb/upb/base/status.h"
#include "upb/upb/message/types.h"
#include "upb/upb/mini_table/extension_registry.h"
#include "upb/upb/mini_table/message.h"

// Must be last.
#include "upb/upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Output is placed in the given buffer, and always NULL-terminated.  The output
 * size (excluding NULL) is returned.  This means that a return value >= |size|
 * implies that the output was truncated.  (These are the same semantics as
 * snprintf()). */
UPB_API size_t upb_JspbEncode(const upb_Message* msg, const upb_MiniTable* m,
                              const upb_ExtensionRegistry* extreg,
                              int options_unused, char* buf, size_t size,
                              upb_Status* status);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/upb/port/undef.inc"

#endif /* UPB_JSPBENCODE_H_ */
