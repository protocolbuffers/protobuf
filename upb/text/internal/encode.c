// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/text/internal/encode.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/lex/round_trip.h"
#include "upb/message/array.h"
#include "upb/message/internal/iterator.h"
#include "upb/message/internal/map_entry.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/unknown_fields.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/text/options.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/reader.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

#define CHK(x)     \
  do {             \
    if (!(x)) {    \
      return NULL; \
    }              \
  } while (0)

static void _upb_FieldDebugString(txtenc* e, upb_MessageValue val,
                                  const upb_MiniTableField* f,
                                  const upb_MiniTable* mt, const char* label,
                                  const upb_MiniTableExtension* ext);

static void _upb_ArrayDebugString(txtenc* e, const upb_Array* arr,
                                  const upb_MiniTableField* f,
                                  const upb_MiniTable* mt,
                                  const upb_MiniTableExtension* ext);

/*
 * Unknown fields are printed by number.
 *
 * 1001: 123
 * 1002: "hello"
 * 1006: 0xdeadbeef
 * 1003: {
 *   1: 111
 * }
 */
const char* UPB_PRIVATE(_upb_TextEncode_Unknown)(txtenc* e, const char* ptr,
                                                 upb_EpsCopyInputStream* stream,
                                                 int groupnum) {
  // We are guaranteed that the unknown data is valid wire format, and will not
  // contain tag zero.
  uint32_t end_group = groupnum > 0
                           ? ((groupnum << kUpb_WireReader_WireTypeBits) |
                              kUpb_WireType_EndGroup)
                           : 0;

  while (!upb_EpsCopyInputStream_IsDone(stream, &ptr)) {
    uint32_t tag;
    CHK(ptr = upb_WireReader_ReadTag(ptr, &tag, stream));
    if (tag == end_group) return ptr;

    UPB_PRIVATE(_upb_TextEncode_Indent)(e);
    UPB_PRIVATE(_upb_TextEncode_Printf)
    (e, "%d: ", (int)upb_WireReader_GetFieldNumber(tag));

    switch (upb_WireReader_GetWireType(tag)) {
      case kUpb_WireType_Varint: {
        uint64_t val;
        CHK(ptr = upb_WireReader_ReadVarint(ptr, &val, stream));
        UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRIu64, val);
        break;
      }
      case kUpb_WireType_32Bit: {
        uint32_t val;
        ptr = upb_WireReader_ReadFixed32(ptr, &val, stream);
        UPB_PRIVATE(_upb_TextEncode_Printf)(e, "0x%08" PRIu32, val);
        break;
      }
      case kUpb_WireType_64Bit: {
        uint64_t val;
        ptr = upb_WireReader_ReadFixed64(ptr, &val, stream);
        UPB_PRIVATE(_upb_TextEncode_Printf)(e, "0x%016" PRIu64, val);
        break;
      }
      case kUpb_WireType_Delimited: {
        int size;
        char* start = e->ptr;
        size_t start_overflow = e->overflow;
        upb_StringView sv;
        CHK(ptr = upb_WireReader_ReadSize(ptr, &size, stream));
        CHK(ptr = upb_EpsCopyInputStream_ReadStringAlwaysAlias(stream, ptr,
                                                               size, &sv));

        // Speculatively try to parse as message.
        UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "{");
        UPB_PRIVATE(_upb_TextEncode_EndField)(e);

        // EpsCopyInputStream can't back up, so create a sub-stream for the
        // speculative parse.
        upb_EpsCopyInputStream sub_stream;
        const char* sub_ptr = sv.data;
        upb_EpsCopyInputStream_Init(&sub_stream, &sub_ptr, size);

        e->indent_depth++;
        if (UPB_PRIVATE(_upb_TextEncode_Unknown)(e, sub_ptr, &sub_stream, -1)) {
          e->indent_depth--;
          UPB_PRIVATE(_upb_TextEncode_Indent)(e);
          UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "}");
        } else {
          // Didn't work out, print as raw bytes.
          e->indent_depth--;
          e->ptr = start;
          e->overflow = start_overflow;
          UPB_PRIVATE(_upb_TextEncode_Bytes)(e, sv);
        }
        break;
      }
      case kUpb_WireType_StartGroup:
        UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "{");
        UPB_PRIVATE(_upb_TextEncode_EndField)(e);
        e->indent_depth++;
        CHK(ptr = UPB_PRIVATE(_upb_TextEncode_Unknown)(
                e, ptr, stream, upb_WireReader_GetFieldNumber(tag)));
        e->indent_depth--;
        UPB_PRIVATE(_upb_TextEncode_Indent)(e);
        UPB_PRIVATE(_upb_TextEncode_PutStr)(e, "}");
        break;
      default:
        return NULL;
    }
    UPB_PRIVATE(_upb_TextEncode_EndField)(e);
  }

  return end_group == 0 && !upb_EpsCopyInputStream_IsError(stream) ? ptr : NULL;
}

#undef CHK

void UPB_PRIVATE(_upb_TextEncode_ParseUnknown)(txtenc* e,
                                               const upb_Message* msg) {
  if ((e->options & UPB_TXTENC_SKIPUNKNOWN) != 0) return;

  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown unknown;
  while (upb_Message_NextUnknown2(msg, &unknown, &iter)) {
    if (unknown.type == kUpb_MessageUnknownType_StringView) {
      upb_StringView view = unknown.value.bytes;
      char* start = e->ptr;
      upb_EpsCopyInputStream stream;
      upb_EpsCopyInputStream_Init(&stream, &view.data, view.size);
      if (!UPB_PRIVATE(_upb_TextEncode_Unknown)(e, view.data, &stream, -1)) {
        /* Unknown failed to parse, back up and don't print it at all. */
        e->ptr = start;
      }
    } else {
      UPB_ASSERT(unknown.type == kUpb_MessageUnknownType_NonCanonicalExtension);
      const struct upb_Extension* ext_struct = unknown.value.extension;
      const upb_MiniTableExtension* ext = ext_struct->ext;
      upb_MessageValue val_ext = ext_struct->data;
      const upb_MiniTableField* f = upb_MiniTableExtension_ToField(ext);
      const upb_MiniTable* mt = upb_MiniTableExtension_Extendee(ext);
      UPB_ASSERT(!upb_MiniTableField_IsMap(f));
      if (upb_MiniTableField_IsArray(f)) {
        _upb_ArrayDebugString(e, val_ext.array_val, f, mt, ext);
      } else {
        _upb_FieldDebugString(e, val_ext, f, mt, NULL, ext);
      }
    }
  }
}

void UPB_PRIVATE(_upb_TextEncode_Scalar)(txtenc* e, upb_MessageValue val,
                                         upb_CType ctype) {
  switch (ctype) {
    case kUpb_CType_Bool:
      UPB_PRIVATE(_upb_TextEncode_PutStr)(e, val.bool_val ? "true" : "false");
      break;
    case kUpb_CType_Float: {
      char buf[32];
      _upb_EncodeRoundTripFloat(val.float_val, buf, sizeof(buf));
      UPB_PRIVATE(_upb_TextEncode_PutStr)(e, buf);
      break;
    }
    case kUpb_CType_Double: {
      char buf[32];
      _upb_EncodeRoundTripDouble(val.double_val, buf, sizeof(buf));
      UPB_PRIVATE(_upb_TextEncode_PutStr)(e, buf);
      break;
    }
    case kUpb_CType_Int32:
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRId32, val.int32_val);
      break;
    case kUpb_CType_UInt32:
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRIu32, val.uint32_val);
      break;
    case kUpb_CType_Int64:
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRId64, val.int64_val);
      break;
    case kUpb_CType_UInt64:
      UPB_PRIVATE(_upb_TextEncode_Printf)(e, "%" PRIu64, val.uint64_val);
      break;
    case kUpb_CType_String:
      UPB_PRIVATE(_upb_HardenedPrintString)
      (e, val.str_val.data, val.str_val.size);
      break;
    case kUpb_CType_Bytes:
      UPB_PRIVATE(_upb_TextEncode_Bytes)(e, val.str_val);
      break;
    case kUpb_CType_Enum:
      UPB_ASSERT(false);  // handled separately in each encoder
      break;
    default:
      UPB_UNREACHABLE();
  }
}

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
                                    : upb_MiniTable_SubMessage(f);
    UPB_PRIVATE(_upb_MessageDebugString)(e, val.msg_val, subm);
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
  const upb_MiniTable* entry = upb_MiniTable_SubMessage(f);
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

    const upb_MiniTable* entry = upb_MiniTable_SubMessage(f);
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

void UPB_PRIVATE(_upb_MessageDebugString)(txtenc* e, const upb_Message* msg,
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
  iter = kUpb_Message_ExtensionBegin;
  while (upb_Message_NextExtension(msg, &ext, &val_ext, &iter)) {
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

  UPB_PRIVATE(_upb_TextEncode_ParseUnknown)(e, msg);
}
