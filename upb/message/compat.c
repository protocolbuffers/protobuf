// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/compat.h"

#include <stddef.h>
#include <stdint.h>

#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"

// Must be last.
#include "upb/port/def.inc"

bool upb_Message_NextExtension(const upb_Message* msg,
                               const upb_MiniTableExtension** result,
                               uintptr_t* iter) {
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return false;
  uintptr_t i = *iter;
  size_t size = in->size;
  while (i < size) {
    // Iterate backwards, as that's what the previous implementation did
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[size - 1 - i];
    i++;
    if (!upb_TaggedAuxPtr_IsExtension(tagged_ptr)) {
      continue;
    }
    const upb_Extension* ext = upb_TaggedAuxPtr_Extension(tagged_ptr);
    *result = ext->ext;
    *iter = i;
    return true;
  }
  *iter = i;
  return false;
}

const upb_MiniTableExtension* upb_Message_FindExtensionByNumber(
    const upb_Message* msg, uint32_t field_number) {
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  size_t size = in ? in->size : 0;
  for (size_t i = 0; i < size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (!upb_TaggedAuxPtr_IsExtension(tagged_ptr)) {
      continue;
    }
    const upb_Extension* ext = upb_TaggedAuxPtr_Extension(tagged_ptr);
    const upb_MiniTableExtension* e = ext->ext;
    if (upb_MiniTableExtension_Number(e) == field_number) return e;
  }
  return NULL;
}
