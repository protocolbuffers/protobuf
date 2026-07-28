// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/extension.h"

#include <stdint.h>
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/extension.h"

// Must be last.
#include "upb/port/def.inc"

const upb_Extension* UPB_PRIVATE(_upb_Message_Getext)(
    const struct upb_Message* msg, const upb_MiniTableExtension* e) {
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return NULL;

  for (size_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
      const upb_Extension* ext =
          upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);
      if (ext->ext == e) {
        return ext;
      }
    }
  }

  return NULL;
}

upb_Extension* UPB_PRIVATE(_upb_Message_GetOrCreateExtensionWithTag)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a,
    upb_TaggedAuxType tag) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));

  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (in) {
    for (size_t i = 0; i < in->size; i++) {
      upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
      upb_Extension* ext = upb_TaggedAuxPtr_TryGetExtension(tagged_ptr);
      // We check both the extension pointer and the tag. If we find an
      // extension with the *same* tag, we reuse it to prevent duplicate
      // entries of the same semantic type.
      //
      // This also means we allow Canonical and Non-Canonical extensions for the
      // same field number to coexist in aux_data. It aligns with standard
      // Protobuf and UPB semantics, where unknown fields (whether stored as raw
      // bytes or parsed Non-Canonical extensions) and known fields (Canonical
      // extensions) can coexist for the same field number.
      if (ext && ext->ext == e && upb_TaggedAuxPtr_Type(tagged_ptr) == tag) {
        return ext;
      }
    }
  }

  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, a)) return NULL;
  in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  upb_Extension* ext =
      (upb_Extension*)upb_Arena_Malloc(a, sizeof(upb_Extension));
  if (!ext) return NULL;
  memset(ext, 0, sizeof(upb_Extension));
  ext->ext = e;
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
