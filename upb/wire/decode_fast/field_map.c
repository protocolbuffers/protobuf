// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>

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
  // Whether string keys/values need UTF-8 validation. Hoisted out of the entry
  // loop: this depends on the field type/mode and on d->options, all of which
  // are fixed for the whole map.
  bool key_validate_utf8;
  bool val_validate_utf8;
} upb_DecodeFastMap;

// Resolves and initializes the map field on the target message.
// Traverses the MiniTable metadata once, creates the upb_Map if not yet
// allocated, and synchronizes hasbits.
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
  map_ctx->key_validate_utf8 =
      _upb_Decoder_FieldRequiresUtf8Validation(d, key_field);
  map_ctx->val_validate_utf8 =
      _upb_Decoder_FieldRequiresUtf8Validation(d, val_field);
  return true;
}

// Zig-zag decodes `v` as sint32 or sint64 depending on the storage `size` of
// the map key/value (the map path only knows the width, not the field type).
UPB_FORCEINLINE
uint64_t upb_DecodeFast_MapZigZagDecode(uint64_t v, size_t size) {
  if (size == 4) return _upb_Decoder_ZigZagDecode32(v);
  UPB_ASSERT(size == 8);
  return _upb_Decoder_ZigZagDecode64(v);
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

  if (UPB_UNLIKELY(
          !upb_EpsCopyInputStream_SizeFitsWithoutSwap(EPS(d), p, size))) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  const char* entry_end = p + size;
  if (UPB_UNLIKELY(p >= entry_end)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  // Parse Key (Field 1)
  UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(EPS(d), 1);
  uint8_t key_tag = *p++;
  uint8_t expected_key_tag = upb_DecodeFastData_GetKeyTag(data);
  if (UPB_UNLIKELY(key_tag != expected_key_tag)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  UPB_PRIVATE(upb_EpsCopyInputStream_BoundsCheckedToEnd)(EPS(d), p, entry_end);

  upb_MessageValue key_val = upb_MessageValue_Zero();
  if (is_str_map) {
    int klen;
    if (UPB_UNLIKELY(!upb_DecodeFast_DecodeSize(d, &p, &klen, next))) {
      return false;
    }
    if (UPB_UNLIKELY(entry_end - p < klen || klen < 0)) {
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    }
    // Note: We do not need to copy string keys here even when aliasing is
    // disabled because upb_strtable_insert() always deep-copies keys into its
    // own arena-allocated upb_SizePrefixString.
    p = upb_EpsCopyInputStream_ReadStringAlwaysAlias(EPS(d), p, klen,
                                                     &key_val.str_val);
    if (UPB_UNLIKELY(p == NULL)) {
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    }
    // Validate the key before looking at the value so that an entry with both
    // a bad key and a bad value reports the same error as the MiniTable
    // decoder, which validates the key as soon as it reads it.
    if (map_ctx->key_validate_utf8 &&
        !utf8_range_IsValid(key_val.str_val.data, key_val.str_val.size)) {
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_BadUtf8, next);
    }
  } else {
    uint64_t k = 0;
    switch (expected_key_tag & 0x7) {
      case kUpb_WireType_Varint:
        p = upb_WireReader_ReadVarint(p, &k, EPS(d));
        if (UPB_UNLIKELY(upb_DecodeFastData_KeyIsZigZag(data))) {
          k = upb_DecodeFast_MapZigZagDecode(
              k, upb_DecodeFastData_GetKeySize(data));
        }
        break;
      case kUpb_WireType_32Bit: {
        if (UPB_UNLIKELY(entry_end - p < 4)) {
          return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                     next);
        }
        uint32_t k32;
        p = upb_WireReader_ReadFixed32(p, &k32, EPS(d));
        k = k32;
        break;
      }
      case kUpb_WireType_64Bit:
        if (UPB_UNLIKELY(entry_end - p < 8)) {
          return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                     next);
        }
        p = upb_WireReader_ReadFixed64(p, &k, EPS(d));
        break;
      default:
        // upb_DecodeFast_TryFillMapEntry() only selects the int map parsers for
        // key types that use one of the wire types above.
        UPB_UNREACHABLE();
    }
    if (upb_DecodeFastData_KeyIs32(data)) {
      key_val.uint32_val = (uint32_t)k;
    } else {
      UPB_ASSERT(upb_DecodeFastData_GetKeySize(data) == 8);
      key_val.uint64_val = k;
    }
  }

  if (UPB_UNLIKELY(p >= entry_end)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  // Parse Value (Field 2)
  UPB_PRIVATE(upb_EpsCopyInputStream_BoundsCheckedToEnd)(EPS(d), p, entry_end);
  UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(EPS(d), 1);
  uint8_t val_tag = *p++;
  upb_MessageValue val = upb_MessageValue_Zero();

  uint8_t expected_val_tag = upb_DecodeFastData_GetValTag(data);
  uint8_t expected_val_wire_type = expected_val_tag & 0x7;
  // Fasttable won't contain entries for anything but these
  UPB_ASSERT(expected_val_wire_type == kUpb_WireType_Varint ||
             expected_val_wire_type == kUpb_WireType_32Bit ||
             expected_val_wire_type == kUpb_WireType_64Bit ||
             expected_val_wire_type == kUpb_WireType_Delimited);

  if (UPB_UNLIKELY(val_tag != expected_val_tag)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  // The value must have at least one byte; an empty value is malformed.
  if (UPB_UNLIKELY(p >= entry_end)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }

  UPB_PRIVATE(upb_EpsCopyInputStream_BoundsCheckedToEnd)(EPS(d), p, entry_end);
  switch (expected_val_wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t v = 0;
      const char* p_next = upb_WireReader_ReadVarint(p, &v, EPS(d));
      if (UPB_UNLIKELY(p_next != entry_end)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      p = p_next;
      if (upb_DecodeFastData_ValIsBool(data)) {
        val.bool_val = (v != 0);
      } else {
        if (UPB_UNLIKELY(upb_DecodeFastData_ValIsZigZag(data))) {
          v = upb_DecodeFast_MapZigZagDecode(
              v, upb_DecodeFastData_GetValSize(data));
        }
        val.uint64_val = v;
      }
      break;
    }
    case kUpb_WireType_64Bit: {
      if (UPB_UNLIKELY(entry_end - p != 8)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      p = upb_WireReader_ReadFixed64(p, &val.uint64_val, EPS(d));
      break;
    }
    case kUpb_WireType_32Bit: {
      if (UPB_UNLIKELY(entry_end - p != 4)) {
        return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable,
                                   next);
      }
      p = upb_WireReader_ReadFixed32(p, &val.uint32_val, EPS(d));
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
          return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_OutOfMemory, next);
        }
        upb_DecodeFast_MessageContext ctx = {map_ctx->sub_table, false,
                                             sub_msg};
        if (upb_EpsCopyInputStream_TryParseDelimitedFast(
                EPS(d), &p, vlen, &upb_DecodeFast_MessageData, &ctx)) {
          if (UPB_UNLIKELY(p == NULL)) {
            return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, next);
          }
        } else {
          ptrdiff_t delta = upb_EpsCopyInputStream_PushLimit(EPS(d), p, vlen);
          p = upb_DecodeFast_MessageData(EPS(d), p, vlen, &ctx);
          if (UPB_UNLIKELY(p == NULL)) {
            return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, next);
          }
          upb_EpsCopyInputStream_PopLimit(EPS(d), p, delta);
        }
        val.msg_val = sub_msg;
      } else {
        if (UPB_UNLIKELY(!_upb_Decoder_ReadString(
                d, &p, vlen, &val.str_val, map_ctx->val_validate_utf8))) {
          return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_OutOfMemory, next);
        }
      }
      break;
    }
    default:
      UPB_UNREACHABLE();
  }

  if (UPB_UNLIKELY(_upb_Map_Insert(map_ctx->map, &key_val,
                                   upb_DecodeFastData_GetKeySize(data), &val,
                                   upb_DecodeFastData_GetValSize(data),
                                   &d->arena) ==
                   kUpb_MapInsertStatus_OutOfMemory)) {
    return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_OutOfMemory, next);
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
  UPB_ASSERT(is_str_map == upb_DecodeFastData_IsStrMap(data));
  UPB_ASSERT((tagsize == kUpb_DecodeFast_Tag2Byte) ==
             upb_DecodeFastData_IsTag2(data));
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

  // The map's table discriminator is derived from the same entry MiniTable that
  // selected this parser, so it always agrees with our compile-time is_str_map:
  // _upb_Decoder_CreateMap() passes key_size = UPB_MAPTYPE_STRING (0) exactly
  // for string/bytes keys, and _upb_Map_New() sets is_strtable from that. Tell
  // the compiler so it can fold away the strtable/inttable branch on the insert
  // path.
  UPB_ASSUME(map_ctx.map->UPB_PRIVATE(is_strtable) == is_str_map);

  // The sizes packed into `data` by upb_DecodeFast_TryFillMapEntry() describe
  // the same map that _upb_Decoder_CreateMap() sized from kSizeInMap[], so they
  // must agree.
  UPB_ASSUME(map_ctx.map->key_size == upb_DecodeFastData_GetKeySize(data));
  UPB_ASSUME(map_ctx.map->val_size == upb_DecodeFastData_GetValSize(data));

  // Lets string-map instantiations fold key_size to the constant
  // UPB_MAPTYPE_STRING rather than extracting it from `data` on the insert
  // path.
  UPB_ASSUME(is_str_map ==
             (upb_DecodeFastData_GetKeySize(data) == UPB_MAPTYPE_STRING));

  int depth_cost = map_ctx.sub_table ? 2 : 1;
  if (d->depth < depth_cost) {
    // This isn't yet an error, because if the map's entries are malformed
    // (value field not actually present or wrong wire type) then it won't
    // actually recurse and thus won't exceed the max depth. But we don't want
    // to repeatedly check the depth limit on every entry, so let the fallback
    // decoder handle this.
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    return;
  }
  d->depth -= depth_cost;

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

  d->depth += depth_cost;
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
