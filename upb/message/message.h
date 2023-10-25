// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Public APIs for message operations that do not depend on the schema.
//
// MiniTable-based accessors live in accessors.h.

#ifndef UPB_MESSAGE_MESSAGE_H_
#define UPB_MESSAGE_MESSAGE_H_

#include <stddef.h>

#include "upb/mem/arena.h"
#include "upb/message/types.h"  // IWYU pragma: export
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new message with the given mini_table on the given arena.
UPB_API upb_Message* upb_Message_New(const upb_MiniTable* mini_table,
                                     upb_Arena* arena);

// Returns the number of extensions present in this message.
size_t upb_Message_ExtensionCount(const upb_Message* msg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_MESSAGE_H_ */
