// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_UNKNOWN_FIELDS_H_
#define UPB_MESSAGE_UNKNOWN_FIELDS_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/base/string_view.h"
#include "upb/mem/internal/arena.h"
#include "upb/message/internal/extension.h"  // IWYU pragma: export
#include "upb/message/internal/message.h"
#include "upb/message/internal/non_canonical_field.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/field.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// A non-canonical field: a parsed field value whose schema is known but is not
// part of the schema of the message holding it. It is semantically an unknown
// field, but is kept in parsed form to lazily defer serialization.
//
// It is described only by a upb_MiniTableField (the field of an extension, or a
// regular field of another message), never by a upb_MiniTableExtension, and has
// no extendee.
//
// NOTE: Non-canonical fields are being introduced to replace non-canonical
// extensions (b/569995025). Not all readers handle them yet, and nothing
// creates them yet outside of tests.
typedef struct upb_NonCanonicalField upb_NonCanonicalField;

// Returns the MiniTableField that describes this non-canonical field. Never
// NULL.
UPB_API_INLINE const upb_MiniTableField* upb_NonCanonicalField_MiniTableField(
    const upb_NonCanonicalField* f);

// Copies the value of `f` into `val`, which must point to storage of the
// appropriate type for the field's representation.
UPB_API_INLINE void upb_NonCanonicalField_GetField(
    const upb_NonCanonicalField* f, void* val);

// Returns the value of this non-canonical field.
//
// For a repeated field, the resulting struct upb_Array* can be NULL if a
// struct upb_Array has not been allocated yet.
UPB_API_INLINE upb_MessageValue
upb_NonCanonicalField_GetValue(const upb_NonCanonicalField* f);

typedef enum {
  kUpb_MessageUnknownType_StringView,
  // TODO: Remove kUpb_MessageUnknownType_NonCanonicalExtension
  // once kUpb_MessageUnknownType_NonCanonicalField is used everywhere.
  kUpb_MessageUnknownType_NonCanonicalExtension,
  kUpb_MessageUnknownType_NonCanonicalField,
} upb_MessageUnknownType;

// Represents an unknown field in a message, whether it's in a serialized
// (upb_StringView), parsed non-canonical extension (upb_Extension*), or parsed
// non-canonical field (upb_NonCanonicalField*) format.
//
// See upb_NonCanonicalField above for non-canonical fields.
typedef struct upb_MessageUnknown {
  uint8_t type;
  union {
    upb_StringView bytes;
    // TODO: Remove extension once
    // kUpb_MessageUnknownType_NonCanonicalField is used everywhere.
    const upb_Extension* extension;
    const upb_NonCanonicalField* field;
  } value;
} upb_MessageUnknown;

// Support iteration over unknown (upb_MessageUnknown*), including unknown
// upb_StringView, non-canonical extensions (upb_Extension*) and non-canonical
// fields (upb_NonCanonicalField*).
UPB_INLINE bool upb_Message_NextUnknown2(const struct upb_Message* msg,
                                         struct upb_MessageUnknown* data,
                                         uintptr_t* iter) {
  const upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  size_t i = *iter;
  if (in) {
    while (i < in->size) {
      upb_TaggedAuxPtr tagged_ptr = in->aux_data[i++];
      if (upb_TaggedAuxPtr_IsUnknownStringView(tagged_ptr)) {
        data->type = kUpb_MessageUnknownType_StringView;
        data->value.bytes = *upb_TaggedPtrAux_StringViewRepr(tagged_ptr);
        *iter = i;
        return true;
      } else if (upb_TaggedAuxPtr_IsNonCanonicalExtension(tagged_ptr)) {
        data->type = kUpb_MessageUnknownType_NonCanonicalExtension;
        data->value.extension =
            upb_TaggedAuxPtr_NonCanonicalExtension(tagged_ptr);
        *iter = i;
        return true;
      } else if (upb_TaggedAuxPtr_IsNonCanonicalField(tagged_ptr)) {
        data->type = kUpb_MessageUnknownType_NonCanonicalField;
        data->value.field = upb_TaggedAuxPtr_NonCanonicalField(tagged_ptr);
        *iter = i;
        return true;
      }
    }
  }
  data->type = kUpb_MessageUnknownType_StringView;
  data->value.bytes.size = 0;
  data->value.bytes.data = NULL;
  *iter = i;
  return false;
}

// Iterates over unknown fields in wire format (upb_StringView).
// If an unknown field is a non-canonical extension, it is automatically
// encoded into wire format into `*arena` using default encode options (0).
//
// `arena` is a pointer to `upb_Arena*`. If `*arena` is NULL when a
// non-canonical extension is encountered, an arena will be lazily created via
// `upb_Arena_New()`. The caller is responsible for freeing `*arena` (if
// non-NULL) using `upb_Arena_Free(*arena)` after iteration completes.
//
// NOTE: Automatically encoding non-canonical extensions into wire format may
// incur a performance penalty if non-canonical extensions are present, as
// encoding requires allocating temporary buffers in `*arena`. Use
// `upb_Message_NextUnknown2` if you want to inspect non-canonical extensions
// directly without encoding them.
UPB_NODISCARD bool upb_Message_NextWireFormatUnknown(
    const struct upb_Message* msg, struct upb_Arena** arena,
    upb_StringView* data, uintptr_t* iter);

typedef enum {
  kUpb_FindUnknown_Ok,
  kUpb_FindUnknown_NotPresent,
  kUpb_FindUnknown_ParseError,
} upb_FindUnknown_Status;

typedef struct {
  upb_FindUnknown_Status status;
  struct upb_MessageUnknown unknown;
  uintptr_t iter;
} upb_FindUnknownRet2;

// Finds first occurrence of unknown data (upb_MessageUnknown) by tag id in
// message, including unknown upb_StringView and non-canonical extensions
// (upb_Extension*).
//
// If multiple matching entries exist for the same field number (e.g. both a
// raw unknown upb_StringView and a non-canonical extension), this function
// returns the one encountered first in internal iteration order (which follows
// the order they were added or parsed).
//
// A depth_limit of zero means to just use the upb default depth limit.
upb_FindUnknownRet2 upb_Message_FindUnknown2(const struct upb_Message* msg,
                                             uint32_t field_number,
                                             int depth_limit);

typedef enum {
  kUpb_DeleteUnknown_DeletedLast,
  kUpb_DeleteUnknown_IterUpdated,
  kUpb_DeleteUnknown_AllocFail,
} upb_Message_DeleteUnknownStatus;

// Removes a segment of unknown data from the message, advancing to the next
// segment.  Returns false if the removed segment was at the end of the last
// chunk.
//
// This must be done while iterating:
//
//   uintptr_t iter = kUpb_Message_UnknownBegin;
//   upb_StringView data;
//   // Iterate chunks
//   while (upb_Message_NextUnknown(msg, &data, &iter)) {
//     // Iterate within a chunk, deleting ranges
//     while (ShouldDeleteSubSegment(&data)) {
//       // Data now points to the region to be deleted
//       switch (upb_Message_DeleteUnknown(msg, &data, &iter)) {
//         case kUpb_DeleteUnknown_DeletedLast: return ok;
//         case kUpb_DeleteUnknown_IterUpdated: break;
//         // If DeleteUnknown returned kUpb_DeleteUnknown_IterUpdated,
//         // then data now points to the remaining unknown fields after the
//         // region that was just deleted.
//         case kUpb_DeleteUnknown_AllocFail: return err;
//       }
//     }
//   }
//
// The range given in `data` must be contained inside the most recently
// returned region.
// TODO: b/510055656 - Legacy API that works with messages that only have
// unknown data in upb_StringView format. Use `upb_Message_DeleteUnknown2` for
// messages that may have non-canonical extensions.
UPB_NODISCARD upb_Message_DeleteUnknownStatus
upb_Message_DeleteUnknown(struct upb_Message* msg, upb_StringView* data,
                          uintptr_t* iter, struct upb_Arena* arena);

// Removes a segment of unknown data from the message, advancing to the next
// segment.  Returns false if the removed segment was at the end of the last
// chunk.
//
// This must be done while iterating:
//
//   uintptr_t iter = kUpb_Message_UnknownBegin;
//   upb_MessageUnknown data;
//   // Iterate chunks
//   while (upb_Message_NextUnknown2(msg, &data, &iter)) {
//     // Iterate within a chunk, deleting ranges
//     while (ShouldDeleteSubSegment(&data)) {
//       // Data now points to the region to be deleted
//       switch (upb_Message_DeleteUnknown2(msg, &data, &iter)) {
//         case kUpb_DeleteUnknown_DeletedLast: return ok;
//         case kUpb_DeleteUnknown_IterUpdated: break;
//         // If DeleteUnknown returned kUpb_DeleteUnknown_IterUpdated,
//         // then data now points to the remaining unknown fields after the
//         // region that was just deleted.
//         case kUpb_DeleteUnknown_AllocFail: return err;
//       }
//     }
//   }
//
// The range given in `data` must be contained inside the most recently
// returned region.
//
// Support deletion of unknown (upb_MessageUnknown*), including unknown
// upb_StringView and non-canonical extensions (upb_Extension*).
UPB_NODISCARD upb_Message_DeleteUnknownStatus upb_Message_DeleteUnknown2(
    struct upb_Message* msg, struct upb_MessageUnknown* data, uintptr_t* iter,
    struct upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_UNKNOWN_FIELDS_H_ */
