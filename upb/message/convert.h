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

// Converts a message between two different `upb_MiniTable` types. The source
// and destination `upb_MiniTable`s must be compatible, e.g. they are both
// derived from the same proto definition (e.g., via tree-shaking) or subsets of
// some message proto.
//
// It is equivalent to encoding the source message and then decoding it
// using the destination `upb_MiniTable`, but is generally faster and uses less
// memory.
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
// Returns a new message on success, or NULL on failure. This function may
// return NULL even for valid inputs, if it encounters a case it does not
// support. If the caller wishes to handle all cases, they should detect NULL
// and fallback to serializing the source message and then decoding it using
// the destination `upb_MiniTable`.
//
// `decode_options` and `encode_options` are passed to the underlying decode and
// encode operations. Note that `kUpb_DecodeOption_AliasString` is set
// unconditionally, so strings in decoded unknown fields are always aliased
// regardless of `decode_options`.
const upb_Message* upb_Message_Convert(const upb_Message* src,
                                       const upb_MiniTable* src_mt,
                                       const upb_MiniTable* dst_mt,
                                       const upb_ExtensionRegistry* extreg,
                                       int decode_options, int encode_options,
                                       upb_Arena* arena);

#ifdef __cplusplus
}  // extern "C"
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_CONVERT_H_
