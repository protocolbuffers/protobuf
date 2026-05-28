// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_JSON_ENUM_H__
#define GOOGLE_UPB_UPB_JSON_ENUM_H__

#include <string.h>

#include "upb/base/string_view.h"
#include "upb/message/array.h"
#include "upb/message/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

/* Get custom enum name from extension options. Fall back to the standard
CAPS_ENUM_NAME if not found. */
UPB_INLINE upb_StringView json_enum_name(const upb_EnumValueDef* ev,
                                         const upb_DefPool* symtab) {
  const char* name = upb_EnumValueDef_Name(ev);
  upb_StringView res = {name, strlen(name)};

  if (!upb_EnumValueDef_HasOptions(ev)) {
    return res;
  }
  const upb_Message* opts = (const upb_Message*)upb_EnumValueDef_Options(ev);
  if (!opts) {
    return res;
  }
  if (!symtab) {
    return res;
  }
  const upb_FieldDef* ext_field =
      upb_DefPool_FindExtensionByName(symtab, "pb.enumvalue.json");
  if (!ext_field) {
    return res;
  }
  if (!upb_Message_HasFieldByDef(opts, ext_field)) {
    return res;
  }
  upb_MessageValue ext_val = upb_Message_GetFieldByDef(opts, ext_field);
  const upb_Message* json_opts_msg = ext_val.msg_val;
  if (!json_opts_msg) {
    return res;
  }
  const upb_MessageDef* json_opts_m = upb_FieldDef_MessageSubDef(ext_field);
  const upb_FieldDef* custom_string_field =
      upb_MessageDef_FindFieldByNumber(json_opts_m, 1);
  if (!custom_string_field) {
    return res;
  }
  if (!upb_Message_HasFieldByDef(json_opts_msg, custom_string_field)) {
    return res;
  }
  upb_MessageValue string_val =
      upb_Message_GetFieldByDef(json_opts_msg, custom_string_field);
  return string_val.str_val;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // GOOGLE_UPB_UPB_JSON_ENUM_H__
