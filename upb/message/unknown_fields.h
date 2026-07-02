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

// Support iteration over unknown, including non-canonical extensions.
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
  *iter = i;
  return false;
}

// Support deletion of unknown, including non-canonical extensions.
UPB_NODISCARD upb_Message_DeleteUnknownStatus upb_Message_DeleteUnknown2(
    struct upb_Message* msg, struct upb_MessageUnknown* data, uintptr_t* iter,
    struct upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_UNKNOWN_FIELDS_H_ */
