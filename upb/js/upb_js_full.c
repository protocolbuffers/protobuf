// Copyright 2023 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// This disables inlining and forces all public functions to be exported.
#define UPB_BUILD_API

#include "upb/mem/arena.h"
#include "upb/mini_descriptor/decode.h"  // IWYU pragma: keep
#include "upb/wire/decode.h"  // IWYU pragma: keep
#include "upb/base/string_view.h"
#include "upb/message/accessors.h"
#include "upb/message/accessors_split64.h"  // IWYU pragma: keep
#include "upb/message/types.h"
#include "upb/mini_table/field.h"

// Must be last.
#include "upb/port/def.inc"

UPB_API void upb_Message_SetString_FromDataAndSize(
    upb_Message* msg, const upb_MiniTableField* field, const char* data,
    size_t size, upb_Arena* arena) {
  upb_Message_SetString(msg, field, upb_StringView_FromDataAndSize(data, size),
                        arena);
}

UPB_API const char* upb_Message_GetString_Data(
    const upb_Message* msg, const upb_MiniTableField* field) {
  const static upb_StringView def_val = {0};
  upb_StringView ret = upb_Message_GetString(msg, field, def_val);
  return ret.data;
}

UPB_API size_t upb_Message_GetString_Size(const upb_Message* msg,
                                          const upb_MiniTableField* field) {
  const static upb_StringView def_val = {0};
  upb_StringView ret = upb_Message_GetString(msg, field, def_val);
  return ret.size;
}

// This is unused when we compile for WASM, but Builder doesn't like it
// if this file cannot compile as a regular cc_binary().
int main(void) {}
