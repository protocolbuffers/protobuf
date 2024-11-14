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
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"

// Must be last.
#include "upb/port/def.inc"

bool upb_Message_NextExtension(const upb_Message* msg,
                               const upb_MiniTableExtension** result,
                               uintptr_t* iter) {
  size_t count;
  const upb_Extension* ext = UPB_PRIVATE(_upb_Message_Getexts)(msg, &count);
  size_t i = *iter;
  if (i >= count) {
    return false;
    *result = NULL;
  }
  *result = ext[i].ext;
  *iter = i + 1;
  return true;
}

const upb_MiniTableExtension* upb_Message_FindExtensionByNumber(
    const upb_Message* msg, uint32_t field_number) {
  size_t count;
  const upb_Extension* ext = UPB_PRIVATE(_upb_Message_Getexts)(msg, &count);

  for (; count--; ext++) {
    const upb_MiniTableExtension* e = ext->ext;
    if (upb_MiniTableExtension_Number(e) == field_number) return e;
  }
  return NULL;
}
