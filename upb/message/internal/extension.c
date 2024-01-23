// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/extension.h"

#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/extension.h"

// Must be last.
#include "upb/port/def.inc"

const struct upb_Extension* UPB_PRIVATE(_upb_Message_Getext)(
    const struct upb_Message* msg, const upb_MiniTableExtension* e) {
  size_t n;
  const struct upb_Extension* ext = UPB_PRIVATE(_upb_Message_Getexts)(msg, &n);

  // For now we use linear search exclusively to find extensions.
  // If this becomes an issue due to messages with lots of extensions,
  // we can introduce a table of some sort.
  for (size_t i = 0; i < n; i++) {
    if (ext[i].ext == e) {
      return &ext[i];
    }
  }

  return NULL;
}

const struct upb_Extension* UPB_PRIVATE(_upb_Message_Getexts)(
    const struct upb_Message* msg, size_t* count) {
  upb_Message_Internal* in = msg->internal;
  if (in) {
    *count = (in->size - in->ext_begin) / sizeof(struct upb_Extension);
    return UPB_PTR_AT(in, in->ext_begin, void);
  } else {
    *count = 0;
    return NULL;
  }
}

struct upb_Extension* UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* a) {
  struct upb_Extension* ext =
      (struct upb_Extension*)UPB_PRIVATE(_upb_Message_Getext)(msg, e);
  if (ext) return ext;
  if (!UPB_PRIVATE(_upb_Message_Realloc)(msg, sizeof(struct upb_Extension), a))
    return NULL;
  upb_Message_Internal* in = msg->internal;
  in->ext_begin -= sizeof(struct upb_Extension);
  ext = UPB_PTR_AT(in, in->ext_begin, void);
  memset(ext, 0, sizeof(struct upb_Extension));
  ext->ext = e;
  return ext;
}
