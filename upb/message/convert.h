// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_CONVERT_H_
#define UPB_MESSAGE_CONVERT_H_

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Converts a message between two different `upb_MiniTable` types.
//
// It is equivalent to encoding the source message and then decoding it
// using the destination `upb_MiniTable`, but is more efficient when
// both `upb_MiniTable`s are derived from the same proto definition (e.g., via
// tree-shaking).
//
// If the destination message, or any of its submessages, uses the same
// minitable as the matching part of the source (determined by pointer
// identity), the destination will alias the source's memory (shallow copy)
// instead of performing a deep copy. Strings and unknown fields are aliased
// from the input message.
//
// Fields present in the source but not the destination will be encoded and
// added to the destination's unknown fields (or extensions, if the extension
// registry allows it).
//
// NOTE: The source and destination `upb_MiniTable`s must be compatible; the
// function has undefined behavior otherwise.
//
// Returns a new message on success, or NULL on failure.
const upb_Message* upb_Message_Convert(const upb_Message* src,
                                       const upb_MiniTable* src_mt,
                                       const upb_MiniTable* dst_mt,
                                       const upb_ExtensionRegistry* extreg,
                                       upb_Arena* arena);

#ifdef __cplusplus
}  // extern "C"
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_CONVERT_H_
