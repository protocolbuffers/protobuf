// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/non_canonical_field.h"

#include <stddef.h>

#include "upb/mem/arena.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/field.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_NonCanonicalField* UPB_PRIVATE(_upb_Message_CreateNonCanonicalField)(
    struct upb_Message* msg, const upb_MiniTableField* f, upb_Arena* a) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(!upb_MiniTableField_IsMap(f));
  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, a)) return NULL;
  const size_t size = UPB_PRIVATE(_upb_NonCanonicalField_Size)(f);
  struct upb_NonCanonicalField* ncf =
      (struct upb_NonCanonicalField*)upb_Arena_Malloc(a, size);
  if (UPB_UNLIKELY(!ncf)) return NULL;
  _upb_Message_AlignedMemsetZero(ncf, size);
  ncf->UPB_ONLYBITS(field) = f;
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  in->aux_data[in->size++] = upb_TaggedAuxPtr_MakeNonCanonicalField(ncf);
  return ncf;
}
