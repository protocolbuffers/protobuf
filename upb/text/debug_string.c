// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/text/debug_string.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/message/array.h"
#include "upb/message/internal/iterator.h"
#include "upb/message/internal/map_entry.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/text/internal/encode.h"
#include "upb/wire/eps_copy_input_stream.h"

// Must be last.
#include "upb/port/def.inc"

static void _upb_MessageDebugString(txtenc* e, const upb_Message* msg,
                                    const upb_MiniTable* mt);

static void _upb_FieldDebugString(txtenc* e, upb_MessageValue val,
                                  const upb_MiniTableField* f,
                                  const upb_MiniTable* mt, const char* label,
                                  const upb_MiniTableExtension* ext) {
  UPB_PRIVATE(_upb_TextEncode_Indent)(e);
  const upb_CType ctype = upb_MiniTableField_CType(f);
  const bool is_ext = upb_MiniTableField_IsExtension(f);
  char number[10];  // A 32-bit integer can hold up to 10 digits.
  snprintf(number, sizeof(number), "%" PRIu32, upb_MiniTableField_Number(f));
  // label is to pass down whether we're dealing with a "key" of a map or
  // a "value" of a map.
  if (!label) label = number;

  if (is_ext) {
    UPB_PRIVATE(_upb_TextEncode_Printf)(e, "[%s]", label);
  } else {
    UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%s", label);
  }

  if (ctype == kUpb_CType_Message) {
    UPB_PRIVATE(_upb_TextEncode_Printf)(e, " {");
    UPB_PRIVATE(_upb_TextEncode_EndField)(e);
    e->indent_depth++;
    const upb_MiniTable* subm = ext ? upb_MiniTableExtension_GetSubMessage(ext)
                                    : upb_MiniTable_SubMessage(mt, f);
    _upb_MessageDebugString(e, val.msg_val, subm);
    e->indent_depth--;
    UPB_PRIVATE(_upb_TextEncode_Indent)(e);
    UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "}");
    UPB_PRIVATE(_upb_TextEncode_EndField)(e);
    return;
  }

  UPB_PRIVATE(_upb_TextEncode_Printf)(e, ": ");

  if (ctype ==
      kUpb_CType_Enum) {  // Enum has to be processed separately because of
                          // divergent behavior between encoders
    UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRId32, val.int32_val);
  } else {
    UPB_PRIVATE(_upb_TextEncode_Scalar)(e, val, ctype);
  }

  UPB_PRIVATE(_upb_TextEncode_EndField)(e);
}

/*
 * Arrays print as simple repeated elements, eg.
 *
 *    5: 1
 *    5: 2
 *    5: 3
 */
static void _upb_ArrayDebugString(txtenc* e, const upb_Array* arr,
                                  const upb_MiniTableField* f,
                                  const upb_MiniTable* mt,
                                  const upb_MiniTableExtension* ext) {
  for (size_t i = 0, n = upb_Array_Size(arr); i < n; i++) {
    _upb_FieldDebugString(e, upb_Array_Get(arr, i), f, mt, NULL, ext);
  }
}

static void _upb_MapEntryDebugString(txtenc* e, upb_MessageValue key,
                                     upb_MessageValue val,
                                     const upb_MiniTableField* f,
                                     const upb_MiniTable* mt) {
  const upb_MiniTable* entry = upb_MiniTable_SubMessage(mt, f);
  const upb_MiniTableField* key_f = upb_MiniTable_MapKey(entry);
  const upb_MiniTableField* val_f = upb_MiniTable_MapValue(entry);

  UPB_PRIVATE(_upb_TextEncode_Indent)(e);
  UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%u {", upb_MiniTableField_Number(f));
  UPB_PRIVATE(_upb_TextEncode_EndField)(e);
  e->indent_depth++;

  _upb_FieldDebugString(e, key, key_f, entry, "key", NULL);
  _upb_FieldDebugString(e, val, val_f, entry, "value", NULL);

  e->indent_depth--;
  UPB_PRIVATE(_upb_TextEncode_Indent)(e);
  UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "}");
  UPB_PRIVATE(_upb_TextEncode_EndField)(e);
}

/*
 * Maps print as messages of key/value, etc.
 *
 *    1 {
 *      key: "abc"
 *      value: 123
 *    }
 *    2 {
 *      key: "def"
 *      value: 456
 *    }
 */
static void _upb_MapDebugString(txtenc* e, const upb_Map* map,
                                const upb_MiniTableField* f,
                                const upb_MiniTable* mt) {
  if (e->options & UPB_TXTENC_NOSORT) {
    size_t iter = kUpb_Map_Begin;
    upb_MessageValue key, val;
    while (upb_Map_Next(map, &key, &val, &iter)) {
      _upb_MapEntryDebugString(e, key, val, f, mt);
    }
  } else {
    if (upb_Map_Size(map) == 0) return;

    const upb_MiniTable* entry = upb_MiniTable_SubMessage(mt, f);
    const upb_MiniTableField* key_f = upb_MiniTable_GetFieldByIndex(entry, 0);
    _upb_sortedmap sorted;
    upb_MapEntry ent;

    _upb_mapsorter_pushmap(&e->sorter, upb_MiniTableField_Type(key_f), map,
                           &sorted);
    while (_upb_sortedmap_next(&e->sorter, map, &sorted, &ent)) {
      upb_MessageValue key, val;
      memcpy(&key, &ent.k, sizeof(key));
      memcpy(&val, &ent.v, sizeof(val));
      _upb_MapEntryDebugString(e, key, val, f, mt);
    }
    _upb_mapsorter_popmap(&e->sorter, &sorted);
  }
}

static void _upb_MessageDebugString(txtenc* e, const upb_Message* msg,
                                    const upb_MiniTable* mt) {
  size_t iter = kUpb_BaseField_Begin;
  const upb_MiniTableField* f;
  upb_MessageValue val;

  // Base fields will be printed out first, followed by extension fields, and
  // finally unknown fields.

  while (UPB_PRIVATE(_upb_Message_NextBaseField)(msg, mt, &f, &val, &iter)) {
    if (upb_MiniTableField_IsMap(f)) {
      _upb_MapDebugString(e, val.map_val, f, mt);
    } else if (upb_MiniTableField_IsArray(f)) {
      // ext set to NULL as we're not dealing with extensions yet
      _upb_ArrayDebugString(e, val.array_val, f, mt, NULL);
    } else {
      // ext set to NULL as we're not dealing with extensions yet
      // label set to NULL as we're not currently working with a MapEntry
      _upb_FieldDebugString(e, val, f, mt, NULL, NULL);
    }
  }

  const upb_MiniTableExtension* ext;
  upb_MessageValue val_ext;
  iter = kUpb_Extension_Begin;
  while (
      UPB_PRIVATE(_upb_Message_NextExtension)(msg, mt, &ext, &val_ext, &iter)) {
    const upb_MiniTableField* f = &ext->UPB_PRIVATE(field);
    // It is not sufficient to only pass |f| as we lose valuable information
    // about sub-messages. It is required that we pass |ext|.
    if (upb_MiniTableField_IsMap(f)) {
      UPB_UNREACHABLE();  // Maps cannot be extensions.
      break;
    } else if (upb_MiniTableField_IsArray(f)) {
      _upb_ArrayDebugString(e, val_ext.array_val, f, mt, ext);
    } else {
      // label set to NULL as we're not currently working with a MapEntry
      _upb_FieldDebugString(e, val_ext, f, mt, NULL, ext);
    }
  }

  if ((e->options & UPB_TXTENC_SKIPUNKNOWN) == 0) {
    size_t size;
    const char* ptr = upb_Message_GetUnknown(msg, &size);
    if (size != 0) {
      char* start = e->ptr;
      upb_EpsCopyInputStream stream;
      upb_EpsCopyInputStream_Init(&stream, &ptr, size, true);
      if (!UPB_PRIVATE(_upb_TextEncode_Unknown)(e, ptr, &stream, -1)) {
        /* Unknown failed to parse, back up and don't print it at all. */
        e->ptr = start;
      }
    }
  }
}

size_t upb_DebugString(const upb_Message* msg, const upb_MiniTable* mt,
                       int options, char* buf, size_t size) {
  txtenc e;

  e.buf = buf;
  e.ptr = buf;
  e.end = UPB_PTRADD(buf, size);
  e.overflow = 0;
  e.indent_depth = 0;
  e.options = options;
  e.ext_pool = NULL;
  _upb_mapsorter_init(&e.sorter);

  _upb_MessageDebugString(&e, msg, mt);
  _upb_mapsorter_destroy(&e.sorter);
  return UPB_PRIVATE(_upb_TextEncode_Nullz)(&e, size);
}
