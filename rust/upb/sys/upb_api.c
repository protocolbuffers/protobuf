// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
//

#include <stddef.h>

#define UPB_BUILD_API

// go/keep-sorted start
#include "upb/mem/arena.h"               // IWYU pragma: keep
#include "upb/message/accessors.h"       // IWYU pragma: keep
#include "upb/message/array.h"           // IWYU pragma: keep
#include "upb/message/compare.h"         // IWYU pragma: keep
#include "upb/message/copy.h"            // IWYU pragma: keep
#include "upb/message/map.h"             // IWYU pragma: keep
#include "upb/message/merge.h"           // IWYU pragma: keep
#include "upb/mini_descriptor/decode.h"  // IWYU pragma: keep
#include "upb/mini_table/message.h"      // IWYU pragma: keep
#include "upb/text/debug_string.h"       // IWYU pragma: keep
// go/keep-sorted end

const char* upb_rust_Message_GetUnknown(const upb_Message* msg, size_t* len) {
  upb_StringView data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  if (upb_Message_NextUnknown(msg, &data, &iter)) {
    *len = data.size;
    return data.data;
  }
  *len = 0;
  return NULL;
}

bool upb_rust_Message_NextUnknown(const upb_Message* msg, const char** data,
                                  size_t* len, uintptr_t* iter) {
  upb_StringView view;
  if (upb_Message_NextUnknown(msg, &view, iter)) {
    *data = view.data;
    *len = view.size;
    return true;
  }
  *data = NULL;
  *len = 0;
  return false;
}

void upb_rust_Message_ClearExtension(upb_Message* msg,
                                     const upb_MiniTableField* f) {
  upb_Message_ClearExtension(msg, (const upb_MiniTableExtension*)f);
}

bool upb_rust_MiniTableField_IsExtension(const upb_MiniTableField* f) {
  return upb_MiniTableField_IsExtension(f);
}
