// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/extension.h"

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"

// Must be last.
#include "upb/port/def.inc"

const upb_Extension* UPB_PRIVATE(_upb_Message_GetextByField)(
    const struct upb_Message* msg, const upb_MiniTableField* f) {
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return NULL;

  for (size_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
      const upb_Extension* ext =
          upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);
      if (upb_Extension_MiniTableField(ext) == f) {
        return ext;
      }
    }
  }

  return NULL;
}

const upb_Extension* UPB_PRIVATE(_upb_Message_Getext)(
    const struct upb_Message* msg, const upb_MiniTableExtension* e) {
  return UPB_PRIVATE(_upb_Message_GetextByField)(
      msg, upb_MiniTableExtension_ToField(e));
}

upb_Extension* UPB_PRIVATE(_upb_Message_GetOrCreateExtensionWithTag)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a,
    upb_TaggedAuxType tag) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(!upb_MiniTableField_IsMap(upb_MiniTableExtension_ToField(e)));
  // For Canonical Extensions, we check whether the extension has already been
  // set. If we find an extension with the same pointer and tag, we reuse it to
  // prevent duplicate entries for the same extension.
  //
  // For Non-Canonical Extensions, we do NOT reuse them, matching the behavior
  // of adding a unknown StringView (through `_upb_Message_AddUnknown`) which
  // accumulates.
  if (tag == kUpb_TaggedAuxType_CanonicalExtension) {
    upb_Extension* ext =
        (upb_Extension*)UPB_PRIVATE(_upb_Message_Getext)(msg, e);
    if (ext) return ext;
  }
  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, a)) return NULL;
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  upb_Extension* ext = UPB_PRIVATE(_upb_Extension_New)(e, a);
  if (!ext) return NULL;
  in->aux_data[in->size++] = upb_TaggedAuxPtr_MakeExtension(ext, tag);
  return ext;
}

upb_Extension* UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a) {
  return UPB_PRIVATE(_upb_Message_GetOrCreateExtensionWithTag)(
      msg, e, a, kUpb_TaggedAuxType_CanonicalExtension);
}

upb_Extension* UPB_PRIVATE(_upb_Message_CreateNonCanonicalExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a) {
  return UPB_PRIVATE(_upb_Message_GetOrCreateExtensionWithTag)(
      msg, e, a, kUpb_TaggedAuxType_NonCanonicalExtension);
}

upb_Extension* UPB_PRIVATE(_upb_Message_CreateNonCanonicalField)(
    struct upb_Message* msg, const upb_MiniTableField* f, upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  // Non-canonical entries are never maps.
  UPB_ASSERT(!upb_MiniTableField_IsMap(f));
  // Like for an extension field, the value lives in the upb_Extension and the
  // entry is present iff it exists, so the field has no offset or presence.
  UPB_ASSERT(UPB_PRIVATE(_upb_MiniTableField_Offset)(f) == 0);
  UPB_ASSERT(f->presence == 0);
  // Like unknown fields, non-canonical entries accumulate; an existing entry
  // for the same field is never reused.
  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, a)) return NULL;
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  upb_Extension* ext = UPB_PRIVATE(_upb_Extension_NewFromField)(f, a);
  if (!ext) return NULL;
  in->aux_data[in->size++] = upb_TaggedAuxPtr_MakeExtension(
      ext, kUpb_TaggedAuxType_NonCanonicalExtension);
  return ext;
}
