// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_EXTENSION_H_
#define UPB_MESSAGE_INTERNAL_EXTENSION_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/arena.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/types.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

// The internal representation of an extension is self-describing: it contains
// enough information that we can serialize it to binary format without needing
// to look it up in a upb_ExtensionRegistry.
//
// This representation allocates 16 bytes to data on 64-bit platforms.
// This is rather wasteful for scalars (in the extreme case of bool,
// it wastes 15 bytes). We accept this because we expect messages to be
// the most common extension type.
typedef struct upb_Extension {
  const upb_MiniTableExtension* ext;
  upb_MessageValue data;
} upb_Extension;

#ifdef __cplusplus
extern "C" {
#endif

UPB_NODISCARD upb_Extension* UPB_PRIVATE(
    _upb_Message_GetOrCreateExtensionWithTag)(struct upb_Message* msg,
                                              const upb_MiniTableExtension* ext,
                                              upb_Arena* arena,
                                              upb_TaggedAuxType tag);

// Adds the given extension data to the given message.
// |ext| is copied into the message instance.
// This logically replaces any previously-added extension with this number.
//
// If the message holds a lazy (not yet promoted) payload for this extension,
// the payload is discarded and replaced by a fresh, empty extension. Callers
// that need the parsed value must call upb_Message_PromoteLazyExtension()
// first.
UPB_NODISCARD upb_Extension* UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* ext,
    upb_Arena* arena);

// Adds the given non-canonical extension data to the given message.
// |ext| is copied into the message instance.
// This logically replaces any previously-added extension with this number.
UPB_NODISCARD upb_Extension* UPB_PRIVATE(
    _upb_Message_CreateNonCanonicalExtension)(struct upb_Message* msg,
                                              const upb_MiniTableExtension* ext,
                                              upb_Arena* arena);

// Returns an extension for a message with a given mini table,
// or NULL if no extension exists with this mini table. Lazy extensions that
// have not been promoted yet are not returned.
const upb_Extension* UPB_PRIVATE(_upb_Message_Getext)(
    const struct upb_Message* msg, const upb_MiniTableExtension* ext);

struct upb_TaggedAuxPtr;
struct upb_ExtensionRegistry;

// Finds the aux_data entry holding the canonical, promoted or lazy extension
// `e`, if any. Returns true and sets `*index` and `*ptr` (either of which may
// be NULL) on success. Non-canonical extensions are never matched.
bool UPB_PRIVATE(_upb_Message_FindExtensionEntry)(
    const struct upb_Message* msg, const upb_MiniTableExtension* e,
    size_t* index, struct upb_TaggedAuxPtr* ptr);

typedef enum {
  // The payload was stored (or appended to an existing lazy payload).
  kUpb_AddLazyExtension_Ok,
  // The message already holds a parsed value for this extension; the caller
  // must parse the payload into `*out_ext` instead.
  kUpb_AddLazyExtension_ParseEagerly,
  kUpb_AddLazyExtension_OutOfMemory,
} upb_AddLazyExtensionStatus;

// Stores the serialized payload of one occurrence of the lazy extension `e` in
// the message. `payload` is the length-delimited value without its tag and
// length prefix. If `alias` is true the message will reference `payload`
// directly, otherwise it is copied into `arena`. When the extension already
// has a lazy payload, the two are coalesced into a single contiguous copy.
//
// `registry` and `options` are retained so that the deferred parse can be
// performed later; `registry` must outlive the message.
UPB_NODISCARD upb_AddLazyExtensionStatus UPB_PRIVATE(
    _upb_Message_AddLazyExtension)(struct upb_Message* msg,
                                   const upb_MiniTableExtension* e,
                                   const struct upb_ExtensionRegistry* registry,
                                   int options, upb_StringView payload,
                                   bool alias, upb_Arena* arena,
                                   upb_Extension** out_ext);

UPB_INLINE bool UPB_PRIVATE(_upb_Extension_IsEmpty)(const upb_Extension* ext) {
  switch (
      UPB_PRIVATE(_upb_MiniTableField_Mode)(&ext->ext->UPB_PRIVATE(field))) {
    case kUpb_FieldMode_Scalar:
      return false;
    case kUpb_FieldMode_Array:
      return upb_Array_Size(ext->data.array_val) == 0;
    case kUpb_FieldMode_Map:
      return _upb_Map_Size(ext->data.map_val) == 0;
  }
  UPB_UNREACHABLE();
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_EXTENSION_H_ */
