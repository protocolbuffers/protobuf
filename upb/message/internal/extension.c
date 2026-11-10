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

const upb_AuxField* UPB_PRIVATE(_upb_Message_GetExtension)(
    const struct upb_Message* msg, const upb_MiniTableField* f) {
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return NULL;

  for (size_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
      const upb_AuxField* ext = upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);
      if (upb_Extension_MiniTableField(ext) == f) {
        return ext;
      }
    }
  }

  return NULL;
}

const upb_AuxField* UPB_PRIVATE(_upb_Message_Getext)(
    const struct upb_Message* msg, const upb_MiniTableExtension* e) {
  return UPB_PRIVATE(_upb_Message_GetExtension)(
      msg, upb_MiniTableExtension_ToField(e));
}

// Appends a new zero-initialized entry described by `f` to the aux data of
// `msg`, with the given tag. Returns NULL on allocation failure.
static upb_AuxField* _upb_Message_AppendAuxField(struct upb_Message* msg,
                                                 const upb_MiniTableField* f,
                                                 upb_Arena* a,
                                                 upb_TaggedAuxType tag) {
  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, a)) return NULL;
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  upb_AuxField* ext = UPB_PRIVATE(_upb_Extension_New)(f, a);
  if (!ext) return NULL;
  in->aux_data[in->size++] = upb_TaggedAuxPtr_MakeAuxField(ext, tag);
  return ext;
}

upb_AuxField* UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  const upb_MiniTableField* f = upb_MiniTableExtension_ToField(e);
  UPB_ASSERT(!upb_MiniTableField_IsMap(f));
  // Reuse an existing canonical extension to prevent duplicate entries for the
  // same extension.
  upb_AuxField* ext =
      (upb_AuxField*)UPB_PRIVATE(_upb_Message_GetExtension)(msg, f);
  if (ext) return ext;
  return _upb_Message_AppendAuxField(msg, f, a,
                                     kUpb_TaggedAuxType_CanonicalExtension);
}

upb_AuxField* UPB_PRIVATE(_upb_Message_CreateUnknownExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a) {
  return UPB_PRIVATE(_upb_Message_CreateUnknownField)(
      msg, upb_MiniTableExtension_ToField(e), a);
}

upb_AuxField* UPB_PRIVATE(_upb_Message_CreateUnknownField)(
    struct upb_Message* msg, const upb_MiniTableField* f, upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  // Non-canonical entries are never maps.
  UPB_ASSERT(!upb_MiniTableField_IsMap(f));
  // Like for an extension field, the value lives in the upb_AuxField and the
  // entry is present iff it exists, so the field has no offset or presence.
  UPB_ASSERT(UPB_PRIVATE(_upb_MiniTableField_Offset)(f) == 0);
  UPB_ASSERT(f->presence == 0);
  // Like unknown fields (see `_upb_Message_AddUnknown`), non-canonical entries
  // accumulate; an existing entry for the same field is never reused.
  return _upb_Message_AppendAuxField(msg, f, a,
                                     kUpb_TaggedAuxType_UnknownAuxField);
}
