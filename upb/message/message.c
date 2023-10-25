// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/message.h"

#include <math.h>
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/message/realloc.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

const float kUpb_FltInfinity = INFINITY;
const double kUpb_Infinity = INFINITY;
const double kUpb_NaN = NAN;

upb_Message* upb_Message_New(const upb_MiniTable* mini_table,
                             upb_Arena* arena) {
  return _upb_Message_New(mini_table, arena);
}

const upb_Message_Extension* _upb_Message_Getexts(const upb_Message* msg,
                                                  size_t* count) {
  const upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (in->internal) {
    *count = (in->internal->size - in->internal->ext_begin) /
             sizeof(upb_Message_Extension);
    return UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  } else {
    *count = 0;
    return NULL;
  }
}

const upb_Message_Extension* _upb_Message_Getext(
    const upb_Message* msg, const upb_MiniTableExtension* e) {
  size_t n;
  const upb_Message_Extension* ext = _upb_Message_Getexts(msg, &n);

  /* For now we use linear search exclusively to find extensions. If this
   * becomes an issue due to messages with lots of extensions, we can introduce
   * a table of some sort. */
  for (size_t i = 0; i < n; i++) {
    if (ext[i].ext == e) {
      return &ext[i];
    }
  }

  return NULL;
}

upb_Message_Extension* _upb_Message_GetOrCreateExtension(
    upb_Message* msg, const upb_MiniTableExtension* e, upb_Arena* arena) {
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, e);
  if (ext) return ext;
  if (!_upb_Message_Realloc(msg, sizeof(upb_Message_Extension), arena))
    return NULL;
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  in->internal->ext_begin -= sizeof(upb_Message_Extension);
  ext = UPB_PTR_AT(in->internal, in->internal->ext_begin, void);
  memset(ext, 0, sizeof(upb_Message_Extension));
  ext->ext = e;
  return ext;
}

size_t upb_Message_ExtensionCount(const upb_Message* msg) {
  size_t count;
  _upb_Message_Getexts(msg, &count);
  return count;
}
