// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/error_handler.h"
#include "upb/base/string_view.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/eps_copy_input_stream.h"
#include "upb/wire/reader.h"
#include "upb/wire/types.h"
#include "utf8_range.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_Map* map;
  const upb_MiniTable* sub_table;
  const upb_MiniTableField* key_field;
  const upb_MiniTableField* val_field;
} upb_DecodeFastMap;

// Resolves and initializes the map field on the target message.
// Traverses the MiniTable metadata once, validates closed enum constraints,
// creates the upb_Map if not yet allocated, and synchronizes hasbits.
UPB_FORCEINLINE
bool upb_DecodeFast_GetMap(upb_Decoder* d, upb_Message* msg,
                           const upb_MiniTable* table, uint64_t data,
                           uint64_t* hasbits, upb_DecodeFastMap* map_ctx,
                           upb_DecodeFastNext* next) {
  // Sync hasbits so we don't have to preserve them across the map entries.
  upb_DecodeFast_SetHasbits(msg, *hasbits);
  *hasbits = 0;

  uint32_t submsg_ofs = upb_DecodeFastData_GetSubofs(data) * 8;
  const upb_MiniTableSubInternal* sub = UPB_PTR_AT(
      table->UPB_ONLYBITS(fields), submsg_ofs, upb_MiniTableSubInternal);
  const upb_MiniTable* entry_table = sub->UPB_PRIVATE(submsg);
  // Map entries are synthetic messages co-generated with the parent message in
  // the same compilation unit, so they are never tree-shaken independently of
  // the parent message, and are strictly validated to have exactly 2 fields
  // (field 1 = key, field 2 = value). Unlinked dynamic map tables are filtered
  // out prior to fast decode dispatch.
  UPB_ASSERT(entry_table);
  UPB_ASSERT(entry_table->UPB_PRIVATE(field_count) == 2);

  const upb_MiniTableField* key_field = &entry_table->UPB_PRIVATE(fields)[0];
  const upb_MiniTableField* val_field = &entry_table->UPB_PRIVATE(fields)[1];
  if (UPB_UNLIKELY(upb_MiniTableField_IsClosedEnum(key_field) ||
                   upb_MiniTableField_IsClosedEnum(val_field))) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  bool value_is_message = upb_MiniTableField_IsSubMessage(val_field);
  const upb_MiniTable* sub_table =
      value_is_message ? upb_MiniTable_GetSubMessageTable(val_field) : NULL;
  if (UPB_UNLIKELY(value_is_message && !sub_table)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  uint16_t offset = upb_DecodeFastData_GetOffset(data);
  upb_Map** map_p = UPB_PTR_AT(msg, offset, upb_Map*);
  upb_Map* map = *map_p;
  if (UPB_UNLIKELY(!map)) {
    map = _upb_Decoder_CreateMap(d, entry_table);
    if (UPB_UNLIKELY(!map)) {
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_OutOfMemory, next);
    }
    *map_p = map;
  }

  map_ctx->map = map;
  map_ctx->sub_table = sub_table;
  map_ctx->key_field = key_field;
  map_ctx->val_field = val_field;
  return true;
}

// Parses a single map entry (key, value) and inserts it directly into upb_Map.
UPB_FORCEINLINE
bool upb_DecodeFast_ParseMapEntry(upb_Decoder* d, const char** ptr,
                                  const upb_DecodeFastMap* map_ctx,
                                  uint64_t data, bool is_str_map,
                                  upb_DecodeFastNext* next) {
  const char* p = *ptr;
  int size;
  if (UPB_UNLIKELY(!upb_DecodeFast_DecodeSize(d, &p, &size, next))) {
    return false;
  }

  const char* stream_end = d->input.limit_ptr + UPB_MAX(0, d->input.limit);
  if (UPB_UNLIKELY(size < 0 || size > 0xffff || stream_end - p < size)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }
  UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(EPS(d));

  const char* entry_end = p + size;
  if (UPB_UNLIKELY(p >= entry_end)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  // 1. Parse Key (Field 1)
  uint8_t key_tag = *p++;
  upb_StringView raw_key_str;
  upb_MessageValue key_val;
  if (is_str_map) {
    if (UPB_UNLIKELY(key_tag != ((1 << 3) | kUpb_WireType_Delimited))) {
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    }
    int klen;
    if (UPB_UNLIKELY(!upb_DecodeFast_DecodeSize(d, &p, &klen, next))) {
      return false;
    }
    if (UPB_UNLIKELY(entry_end - p < klen || klen < 0)) {
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    }
    p = upb_EpsCopyInputStream_ReadStringAlwaysAlias(EPS(d), p, klen,
                                                     &raw_key_str);
    if (UPB_UNLIKELY(p == NULL)) {
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    }
  } else {
    uint64_t k = 0;
    if (key_tag == ((1 << 3) | kUpb_WireType_Varint)) {
      p = upb_WireReader_ReadVarint(p, &k, EPS(d));
      if (UPB_UNLIKELY(upb_DecodeFastData_KeyIsZigZag(data))) {
        k = (k >> 1) ^ -(int64_t)(k & 1);
      }
    } else if (key_tag == ((1 << 3) | kUpb_WireType_32Bit)) {
      if (UPB_UNLIKELY(entry_end - p < 4)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      uint32_t k32;
      memcpy(&k32, p, sizeof(k32));
      k = k32;
      p += 4;
    } else if (key_tag == ((1 << 3) | kUpb_WireType_64Bit)) {
      if (UPB_UNLIKELY(entry_end - p < 8)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      memcpy(&k, p, sizeof(k));
      p += 8;
    } else {
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    }
    if (map_ctx->map->key_size == 4) {
      key_val.uint32_val = (uint32_t)k;
    } else {
      key_val.uint64_val = k;
    }
  }

  if (UPB_UNLIKELY(p >= entry_end)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  // 2. Parse Value (Field 2)
  uint8_t val_tag = *p++;
  upb_MessageValue val;
  uint8_t val_wire_type = val_tag & 0x7;
  if (UPB_UNLIKELY((val_tag & ~0x7) != (2 << 3))) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  switch (val_wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t v = 0;
      UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(EPS(d));
      const char* p_next = upb_WireReader_ReadVarint(p, &v, EPS(d));
      if (UPB_UNLIKELY(p_next != entry_end)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      p = p_next;
      if (UPB_UNLIKELY(upb_DecodeFastData_ValIsZigZag(data))) {
        v = (v >> 1) ^ -(int64_t)(v & 1);
      }
      if (map_ctx->map->val_size == 1) {
        val.bool_val = (v != 0);
      } else {
        val.uint64_val = v;
      }
      break;
    }
    case kUpb_WireType_64Bit: {
      if (UPB_UNLIKELY(entry_end - p != 8)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      memcpy(&val.uint64_val, p, 8);
      p += 8;
      break;
    }
    case kUpb_WireType_32Bit: {
      if (UPB_UNLIKELY(entry_end - p != 4)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      memcpy(&val.uint32_val, p, 4);
      p += 4;
      break;
    }
    case kUpb_WireType_Delimited: {
      int vlen;
      if (UPB_UNLIKELY(!upb_DecodeFast_DecodeSize(d, &p, &vlen, next))) {
        return false;
      }
      if (UPB_UNLIKELY(entry_end - p != vlen)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      if (map_ctx->sub_table) {
        upb_Message* sub_msg = _upb_Message_New(map_ctx->sub_table, &d->arena);
        if (UPB_UNLIKELY(!sub_msg)) {
          _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
        }
        upb_DecodeFast_MessageContext ctx = {map_ctx->sub_table, false,
                                             sub_msg};
        if (upb_EpsCopyInputStream_TryParseDelimitedFast(
                EPS(d), &p, vlen, &upb_DecodeFast_MessageData, &ctx)) {
          if (UPB_UNLIKELY(p == NULL)) {
            _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
          }
        } else {
          ptrdiff_t delta = upb_EpsCopyInputStream_PushLimit(EPS(d), p, vlen);
          p = upb_DecodeFast_MessageData(EPS(d), p, vlen, &ctx);
          if (UPB_UNLIKELY(p == NULL)) {
            _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
          }
          upb_EpsCopyInputStream_PopLimit(EPS(d), p, delta);
        }
        if (UPB_UNLIKELY(map_ctx->sub_table->UPB_PRIVATE(required_count))) {
          _upb_Decoder_CheckRequired(d, p, sub_msg, map_ctx->sub_table);
        }
        val.msg_val = sub_msg;
      } else {
        bool validate_utf8 = map_ctx->val_field->UPB_PRIVATE(descriptortype) ==
                             kUpb_FieldType_String;
        if (UPB_UNLIKELY(!_upb_Decoder_ReadString(d, &p, vlen, &val.str_val,
                                                  validate_utf8))) {
          return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                     next);
        }
      }
      break;
    }
    default:
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  // 3. Process string key UTF-8 validation if string map.
  // Note: We do not need to copy string keys here even when aliasing is
  // disabled because upb_strtable_insert() always deep-copies keys into its own
  // arena-allocated upb_SizePrefixString.
  if (is_str_map) {
    bool validate_utf8 = map_ctx->key_field->UPB_PRIVATE(descriptortype) ==
                         kUpb_FieldType_String;
    if (validate_utf8 &&
        !utf8_range_IsValid(raw_key_str.data, raw_key_str.size)) {
      upb_ErrorHandler_ThrowError(d->err, kUpb_DecodeStatus_BadUtf8);
      return false;
    }
    key_val.str_val = raw_key_str;
  }

  // 4. Insert into upb_Map
  if (UPB_UNLIKELY(_upb_Map_Insert(map_ctx->map, &key_val,
                                   map_ctx->map->key_size, &val,
                                   map_ctx->map->val_size, &d->arena) ==
                   kUpb_MapInsertStatus_OutOfMemory)) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }

  *ptr = p;
  return true;
}

UPB_FORCEINLINE
void upb_DecodeFast_Map(upb_Decoder* d, const char** ptr, upb_Message* msg,
                        const upb_MiniTable* table, uint64_t* hasbits,
                        uint64_t data, uint64_t data2,
                        upb_DecodeFast_TagSize tagsize, bool is_str_map,
                        upb_DecodeFastNext* next) {
  uint16_t expected = upb_DecodeFastData_GetExpectedTag(data);
  uint16_t actual = upb_DecodeFastData2_GetOriginalTag(data2);
  if (UPB_UNLIKELY(!upb_DecodeFast_TagMatches(expected, actual, tagsize))) {
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackMismatchedSlot, next);
    return;
  }

  upb_DecodeFastMap map_ctx;
  if (UPB_UNLIKELY(!upb_DecodeFast_GetMap(d, msg, table, data, hasbits,
                                          &map_ctx, next))) {
    return;
  }

  if (map_ctx.sub_table) {
    if (UPB_UNLIKELY(--d->depth < 0)) {
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);
    }
  }

  const char* p = *ptr + upb_DecodeFast_TagSizeBytes(tagsize);
  while (1) {
    const char* entry_start = p - upb_DecodeFast_TagSizeBytes(tagsize);
    if (UPB_UNLIKELY(!upb_DecodeFast_ParseMapEntry(d, &p, &map_ctx, data,
                                                   is_str_map, next))) {
      *ptr = entry_start;
      break;
    }
    *ptr = p;
    _upb_Decoder_Trace(d, 'F');
    if (!upb_DecodeFast_TryMatchTag(d, p, expected, next, tagsize)) {
      break;
    }
    p += upb_DecodeFast_TagSizeBytes(tagsize);
  }

  if (map_ctx.sub_table) {
    d->depth++;
  }
}

#define F(key_type, is_str, tagsize)                               \
  UPB_NOINLINE UPB_PRESERVE_NONE upb_FastDecoder_Return            \
  upb_DecodeFast_##key_type##Map_##tagsize(UPB_PARSE_PARAMS) {     \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;        \
    upb_DecodeFast_Map(d, &ptr, msg, table, &hasbits, data, data2, \
                       kUpb_DecodeFast_##tagsize, is_str, &next);  \
    UPB_DECODEFAST_NEXT(next);                                     \
  }

F(Int, false, Tag1Byte)
F(Int, false, Tag2Byte)
F(Str, true, Tag1Byte)
F(Str, true, Tag2Byte)

#undef F
