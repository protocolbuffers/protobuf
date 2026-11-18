// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_COMPAT_H_
#define UPB_MESSAGE_COMPAT_H_

#include <stdint.h>

#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"

// Must be last.
#include "upb/port/def.inc"

// upb does not support mixing minitables from different sources but these
// functions are still used by some existing users so for now we make them
// available here. This may or may not change in the future so do not add
// them to new code.

#ifdef __cplusplus
extern "C" {
#endif

// Same as upb_Message_NextExtension but iterates in reverse wire order
//
// Prefer upb_Message_NextExtensionFieldReverse() when the upb_MiniTableField
// that describes each extension is sufficient. Use this function when the
// upb_MiniTableExtension is needed, e.g. for its extendee.
bool upb_Message_NextExtensionReverse(const upb_Message* msg,
                                      const upb_MiniTableExtension** result,
                                      uintptr_t* iter);

// Returns the minitable with the given field number, or NULL on failure.
//
// Prefer upb_Message_FindExtensionFieldByNumber() when the upb_MiniTableField
// that describes the extension is sufficient. Use this function when the
// upb_MiniTableExtension is needed, e.g. for its extendee.
const upb_MiniTableExtension* upb_Message_FindExtensionByNumber(
    const upb_Message* msg, uint32_t field_number);

// Same as upb_Message_NextExtensionField but iterates in reverse wire order.
bool upb_Message_NextExtensionFieldReverse(const upb_Message* msg,
                                           const upb_MiniTableField** result,
                                           uintptr_t* iter);

// Returns the upb_MiniTableField that describes the extension with the given
// field number, or NULL if the message has no such extension.
const upb_MiniTableField* upb_Message_FindExtensionFieldByNumber(
    const upb_Message* msg, uint32_t field_number);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_COMPAT_H_ */
