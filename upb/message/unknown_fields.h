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

#include "upb/mem/internal/arena.h"
#include "upb/message/internal/message.h"

// Must be last.
#include "upb/message/internal/types.h"
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Support iteration over unknown (upb_MessageUnknown*), including unknown
// upb_StringView and non-canonical extensions (upb_Extension*).
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
      }
    }
  }
  data->type = kUpb_MessageUnknownType_StringView;
  data->value.bytes.size = 0;
  data->value.bytes.data = NULL;
  *iter = i;
  return false;
}

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
//         case kUpb_Message_DeleteUnknown_DeletedLast: return ok;
//         case kUpb_Message_DeleteUnknown_IterUpdated: break;
//         // If DeleteUnknown returned kUpb_Message_DeleteUnknown_IterUpdated,
//         // then data now points to the remaining unknown fields after the
//         // region that was just deleted.
//         case kUpb_Message_DeleteUnknown_AllocFail: return err;
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
