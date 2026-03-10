// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/text/encode.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/lex/round_trip.h"
#include "upb/message/array.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/map_entry.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/reflection/def.h"
#include "upb/reflection/message.h"
#include "upb/text/internal/encode.h"
#include "upb/wire/eps_copy_input_stream.h"

// Must be last.
#include "upb/port/def.inc"

static void _upb_TextEncode_Msg(txtenc* e, const upb_Message* msg,
                                const upb_MessageDef* m);

static void _upb_TextEncode_Enum(int32_t val, const upb_FieldDef* f,
                                 txtenc* e) {
  const upb_EnumDef* e_def = upb_FieldDef_EnumSubDef(f);
  const upb_EnumValueDef* ev = upb_EnumDef_FindValueByNumber(e_def, val);

  if (ev) {
    UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%s", upb_EnumValueDef_Name(ev));
  } else {
    UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRId32, val);
  }
}

static void _upb_TextEncode_Field(txtenc* e, upb_MessageValue val,
                                  const upb_FieldDef* f) {
  UPB_PRIVATE(_upb_TextEncode_Indent)(e);
  const upb_CType ctype = upb_FieldDef_CType(f);
  const bool is_ext = upb_FieldDef_IsExtension(f);
  const char* full = upb_FieldDef_FullName(f);
  const char* name = upb_FieldDef_Name(f);

  if (ctype == kUpb_CType_Message) {
    if (is_ext) {
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "[%s] {", full);
    } else {
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%s {", name);
    }
    UPB_PRIVATE(_upb_TextEncode_EndField)(e);
    e->indent_depth++;
    _upb_TextEncode_Msg(e, val.msg_val, upb_FieldDef_MessageSubDef(f));
    e->indent_depth--;
    UPB_PRIVATE(_upb_TextEncode_Indent)(e);
    UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "}");
    UPB_PRIVATE(_upb_TextEncode_EndField)(e);
    return;
  }

  if (is_ext) {
    UPB_PRIVATE(_upb_TextEncode_Printf)(e, "[%s]: ", full);
  } else {
    UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%s: ", name);
  }

  if (ctype == kUpb_CType_Enum) {
    _upb_TextEncode_Enum(val.int32_val, f, e);
  } else {
    UPB_PRIVATE(_upb_TextEncode_Scalar)(e, val, ctype);
  }

  UPB_PRIVATE(_upb_TextEncode_EndField)(e);
}

/*
 * Arrays print as simple repeated elements, eg.
 *
 *    foo_field: 1
 *    foo_field: 2
 *    foo_field: 3
 */
static void _upb_TextEncode_Array(txtenc* e, const upb_Array* arr,
                                  const upb_FieldDef* f) {
  size_t i;
  size_t size = upb_Array_Size(arr);

  for (i = 0; i < size; i++) {
    _upb_TextEncode_Field(e, upb_Array_Get(arr, i), f);
  }
}

static void _upb_TextEncode_MapEntry(txtenc* e, upb_MessageValue key,
                                     upb_MessageValue val,
                                     const upb_FieldDef* f) {
  const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* key_f = upb_MessageDef_Field(entry, 0);
  const upb_FieldDef* val_f = upb_MessageDef_Field(entry, 1);
  UPB_PRIVATE(_upb_TextEncode_Indent)(e);
  UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%s {", upb_FieldDef_Name(f));
  UPB_PRIVATE(_upb_TextEncode_EndField)(e);
  e->indent_depth++;

  _upb_TextEncode_Field(e, key, key_f);
  _upb_TextEncode_Field(e, val, val_f);

  e->indent_depth--;
  UPB_PRIVATE(_upb_TextEncode_Indent)(e);
  UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "}");
  UPB_PRIVATE(_upb_TextEncode_EndField)(e);
}

/*
 * Maps print as messages of key/value, etc.
 *
 *    foo_map: {
 *      key: "abc"
 *      value: 123
 *    }
 *    foo_map: {
 *      key: "def"
 *      value: 456
 *    }
 */
static void _upb_TextEncode_Map(txtenc* e, const upb_Map* map,
                                const upb_FieldDef* f) {
  if (e->options & UPB_TXTENC_NOSORT) {
    size_t iter = kUpb_Map_Begin;
    upb_MessageValue key, val;
    while (upb_Map_Next(map, &key, &val, &iter)) {
      _upb_TextEncode_MapEntry(e, key, val, f);
    }
  } else {
    if (upb_Map_Size(map) == 0) return;

    if (!map->UPB_PRIVATE(is_strtable)) {
      // For inttable, first encode the array part, then sort the table entries.
      intptr_t iter = UPB_INTTABLE_BEGIN;
      while ((size_t)++iter < map->t.inttable.array_size) {
        upb_value value = map->t.inttable.array[iter];
        if (upb_inttable_arrhas(&map->t.inttable, iter)) {
          upb_MessageValue key, val;
          memcpy(&key, &iter, sizeof(iter));
          _upb_map_fromvalue(value, &val, map->val_size);
          _upb_TextEncode_MapEntry(e, key, val, f);
        }
      }
    }
    const upb_MessageDef* entry = upb_FieldDef_MessageSubDef(f);
    const upb_FieldDef* key_f = upb_MessageDef_Field(entry, 0);
    _upb_sortedmap sorted;
    upb_MapEntry ent;

    _upb_mapsorter_pushmap(&e->sorter, upb_FieldDef_Type(key_f), map, &sorted);
    while (_upb_sortedmap_next(&e->sorter, map, &sorted, &ent)) {
      upb_MessageValue key, val;
      memcpy(&key, &ent.k, sizeof(key));
      memcpy(&val, &ent.v, sizeof(val));
      _upb_TextEncode_MapEntry(e, key, val, f);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  }
}

static void _upb_TextEncode_Msg(txtenc* e, const upb_Message* msg,
                                const upb_MessageDef* m) {
  size_t iter = kUpb_Message_Begin;
  const upb_FieldDef* f;
  upb_MessageValue val;

  while (upb_Message_Next(msg, m, e->ext_pool, &f, &val, &iter)) {
    if (upb_FieldDef_IsMap(f)) {
      _upb_TextEncode_Map(e, val.map_val, f);
    } else if (upb_FieldDef_IsRepeated(f)) {
      _upb_TextEncode_Array(e, val.array_val, f);
    } else {
      _upb_TextEncode_Field(e, val, f);
    }
  }

  UPB_PRIVATE(_upb_TextEncode_ParseUnknown)(e, msg);
}

size_t upb_TextEncode(const upb_Message* msg, const upb_MessageDef* m,
                      const upb_DefPool* ext_pool, int options, char* buf,
                      size_t size) {
  txtenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = UPB_PTRADD(buf, size);
  e.overflow = 0;
  e.indent_depth = 0;
  e.options = options;
  e.ext_pool = ext_pool;
  _upb_mapsorter_init(&e.sorter);

  _upb_TextEncode_Msg(&e, msg, m);
  _upb_mapsorter_destroy(&e.sorter);
  return UPB_PRIVATE(_upb_TextEncode_Nullz)(&e, size);
}
