// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_DECODE_FAST_SELECT_H_
#define UPB_WIRE_INTERNAL_DECODE_FAST_SELECT_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // The function that will be called to parse this field.  The function
  // pointer for it must be placed into _upb_FastTable_Entry.field_parser.
  //
  // The function pointer can be either looked up at runtime via
  // upb_DecodeFast_GetFunctionPointer(), or be referenced in generated code via
  // upb_DecodeFast_GetFunctionName().
  uint32_t function_idx;

  // The associated data that will be passed to the function.  This must be
  // placed into _upb_FastTable_Entry.field_data.
  uint64_t function_data;
} upb_DecodeFast_TableEntry;

UPB_NODISCARD
UPB_FORCEINLINE uint32_t UPB_PRIVATE(_upb_DecodeFast_GetWireTypeForField)(
    const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsPacked(field)) return kUpb_WireType_Delimited;
  switch (upb_MiniTableField_Type(field)) {
    case kUpb_FieldType_Double:
    case kUpb_FieldType_Fixed64:
    case kUpb_FieldType_SFixed64:
      return kUpb_WireType_64Bit;
    case kUpb_FieldType_Float:
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
      return kUpb_WireType_32Bit;
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_Bool:
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Enum:
    case kUpb_FieldType_SInt32:
    case kUpb_FieldType_SInt64:
      return kUpb_WireType_Varint;
    case kUpb_FieldType_Group:
      return kUpb_WireType_StartGroup;
    case kUpb_FieldType_Message:
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes:
      return kUpb_WireType_Delimited;
    default:
      UPB_UNREACHABLE();
  }
}

UPB_NODISCARD
UPB_FORCEINLINE bool UPB_PRIVATE(_upb_DecodeFast_GetEncodedTag)(
    const upb_MiniTableField* field, uint16_t* out_tag,
    upb_DecodeFast_TagSize* out_tag_size) {
  uint32_t num = upb_MiniTableField_Number(field);
  uint32_t wire_type = UPB_PRIVATE(_upb_DecodeFast_GetWireTypeForField)(field);
  if (num >= 2048) {
    return false;  // Tag >2 bytes, too large for fast decode.
  } else if (num >= 16) {
    *out_tag = ((num << 4) & 0x7f00) | 0x80 | ((num << 3) & 0x7f) | wire_type;
    *out_tag_size = kUpb_DecodeFast_Tag2Byte;
    return true;
  } else {
    *out_tag = (num << 3) | wire_type;
    *out_tag_size = kUpb_DecodeFast_Tag1Byte;
    return true;
  }
}

UPB_NODISCARD
UPB_FORCEINLINE bool UPB_PRIVATE(_upb_DecodeFast_GetFieldCardinality)(
    const upb_MiniTableField* field,
    upb_DecodeFast_Cardinality* out_cardinality) {
  if (upb_MiniTableField_IsScalar(field)) {
    *out_cardinality = upb_MiniTableField_IsInOneof(field)
                           ? kUpb_DecodeFast_Oneof
                           : kUpb_DecodeFast_Scalar;
    return true;
  }
  if (upb_MiniTableField_IsArray(field)) {
    *out_cardinality = upb_MiniTableField_IsPacked(field)
                           ? kUpb_DecodeFast_Packed
                           : kUpb_DecodeFast_Repeated;
    return true;
  }
  return false;  // Can't parse maps with generic fast decode.
}

UPB_NODISCARD
UPB_FORCEINLINE bool UPB_PRIVATE(_upb_DecodeFast_GetFieldType)(
    const upb_MiniTableField* field, upb_DecodeFast_Type* out_type) {
  // We use descriptortype directly instead of upb_MiniTableField_Type because
  // we want the munging of field->descriptortype:
  //  - kUpb_FieldType_String -> kUpb_FieldType_Bytes if no UTF-8 validation is
  //    required.
  //  - kUpb_FieldType_Enum -> kUpb_FieldType_Int32 if the enum is open.
  upb_FieldType type = (upb_FieldType)field->UPB_PRIVATE(descriptortype);

  if (type == kUpb_FieldType_Group) {
    return false;  // Currently not supported.
  }

  if (upb_MiniTableField_IsClosedEnum(field)) {
    *out_type = kUpb_DecodeFast_ClosedEnum;
    return true;
  }

  static const int8_t types[] = {
      -1,                        // 0 (unused)
      kUpb_DecodeFast_Fixed64,   // kUpb_FieldType_Double = 1
      kUpb_DecodeFast_Fixed32,   // kUpb_FieldType_Float = 2
      kUpb_DecodeFast_Varint64,  // kUpb_FieldType_Int64 = 3
      kUpb_DecodeFast_Varint64,  // kUpb_FieldType_UInt64 = 4
      kUpb_DecodeFast_Varint32,  // kUpb_FieldType_Int32 = 5
      kUpb_DecodeFast_Fixed64,   // kUpb_FieldType_Fixed64 = 6
      kUpb_DecodeFast_Fixed32,   // kUpb_FieldType_Fixed32 = 7
      kUpb_DecodeFast_Bool,      // kUpb_FieldType_Bool = 8
      kUpb_DecodeFast_String,    // kUpb_FieldType_String = 9
      -1,                        // kUpb_FieldType_Group = 10
      kUpb_DecodeFast_Message,   // kUpb_FieldType_Message = 11
      kUpb_DecodeFast_Bytes,     // kUpb_FieldType_Bytes = 12
      kUpb_DecodeFast_Varint32,  // kUpb_FieldType_UInt32 = 13
      kUpb_DecodeFast_Varint32,  // kUpb_FieldType_Enum = 14
      kUpb_DecodeFast_Fixed32,   // kUpb_FieldType_SFixed32 = 15
      kUpb_DecodeFast_Fixed64,   // kUpb_FieldType_SFixed64 = 16
      kUpb_DecodeFast_ZigZag32,  // kUpb_FieldType_SInt32 = 17
      kUpb_DecodeFast_ZigZag64,  // kUpb_FieldType_SInt64 = 18
  };

  UPB_ASSERT(type < UPB_ARRAY_SIZE(types));
  *out_type = (upb_DecodeFast_Type)types[type];
  return true;
}

UPB_NODISCARD
UPB_FORCEINLINE bool UPB_PRIVATE(_upb_DecodeFast_GetFunctionIndex)(
    const upb_MiniTableField* field, upb_DecodeFast_TagSize tag_size,
    uint32_t* out_index) {
  upb_DecodeFast_Cardinality cardinality;
  upb_DecodeFast_Type type;
  if (!UPB_PRIVATE(_upb_DecodeFast_GetFieldCardinality)(field, &cardinality)) {
    return false;
  }
  if (!UPB_PRIVATE(_upb_DecodeFast_GetFieldType)(field, &type)) return false;

  *out_index =
      (uint32_t)type << 3 | (uint32_t)cardinality << 1 | (uint32_t)tag_size;
  return true;
}

UPB_NODISCARD
UPB_FORCEINLINE bool UPB_PRIVATE(_upb_DecodeFast_GetPresence)(
    const upb_MiniTableField* field, uint64_t* out_data) {
  if (upb_MiniTableField_IsInOneof(field)) {
    *out_data = upb_MiniTableField_Number(field);
    return true;
  } else if (UPB_PRIVATE(_upb_MiniTableField_HasHasbit)(field)) {
    *out_data = field->presence - 64;
    return *out_data < 32;  // We use uint32_t for hasbits currently.
  } else {
    // We only store 32 bits of hasbits back to the message, so for fields that
    // don't have a hasbit, we just set the high bit which won't be stored.
    *out_data = 63;
    return true;
  }
}

// Returns the offset of the upb_MiniTableSubInternal entry for `field`,
// expressed in 8-byte units relative to the start of the fields array.
UPB_NODISCARD
UPB_FORCEINLINE uint64_t UPB_PRIVATE(_upb_DecodeFast_GetSubofs)(
    const upb_MiniTable* m, const upb_MiniTableField* field) {
  uint64_t idx = (uint64_t)(field - m->UPB_PRIVATE(fields));
  // Here we rely on the fact that sizeof(upb_MiniTableField) is the same on
  // both 32 and 64 bit; if it wasn't, we could generate a bad offset if we
  // compiled on a 32 bit machine targetting a 64 bit one.
  UPB_STATIC_ASSERT(sizeof(upb_MiniTableField) % kUpb_SubmsgOffsetBytes == 0,
                    "upb_MiniTableField must be a multiple of the offset");
  uint64_t ofs_4byte =
      idx * (sizeof(upb_MiniTableField) / kUpb_SubmsgOffsetBytes) +
      field->UPB_PRIVATE(submsg_ofs);
  // Fasttable is only supported on 64-bit platforms where pointers and the
  // submessage entries (upb_MiniTableSubInternal) are 8 bytes, requiring
  // 8-byte alignment. Since the base fields array is aligned to pointer size
  // (at least 8 bytes), and each submessage entry must be 8-byte aligned,
  // the total byte offset (4 * ofs_4byte) from the start of the fields array
  // to the submessage entry is guaranteed to be a multiple of 8.
  // Consequently, ofs_4byte is guaranteed to be even, and thus this offset
  // can be scaled by 8 when loading.
  UPB_ASSERT(ofs_4byte % 2 == 0);
  return ofs_4byte / 2;
}

UPB_NODISCARD
UPB_FORCEINLINE bool UPB_PRIVATE(_upb_DecodeFast_GetFunctionData)(
    const upb_MiniTable* m, const upb_MiniTableField* field, uint16_t tag,
    uint64_t* out_data) {
  uint64_t offset = UPB_PRIVATE(_upb_MiniTableField_Offset)(field);
  uint64_t case_offset = 0;
  uint64_t presence = 63;
  if (upb_MiniTableField_IsScalar(field)) {
    if (upb_MiniTableField_IsInOneof(field)) {
      case_offset = UPB_PRIVATE(_upb_MiniTableField_OneofOffset)(field);
    }
    if (!UPB_PRIVATE(_upb_DecodeFast_GetPresence)(field, &presence)) {
      return false;
    }
  }
  uint64_t subofs = 0;
  if (UPB_PRIVATE(_upb_MiniTableField_HasSub)(field)) {
    subofs = UPB_PRIVATE(_upb_DecodeFast_GetSubofs)(m, field);
  }

  return upb_DecodeFast_MakeData(offset, case_offset, presence, subofs, tag,
                                 out_data);
}

// Provides the upb_Map storage size for a map key/value of the given type.
// Returns false for types that cannot appear as a map key or value.
UPB_NODISCARD
UPB_FORCEINLINE bool UPB_PRIVATE(_upb_DecodeFast_MapTypeSize)(
    upb_FieldType type, size_t* out_size) {
  switch (type) {
    case kUpb_FieldType_Bool:
      *out_size = 1;
      return true;
    case kUpb_FieldType_Float:
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_SInt32:
    case kUpb_FieldType_Enum:
      *out_size = 4;
      return true;
    case kUpb_FieldType_Double:
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_Fixed64:
    case kUpb_FieldType_SFixed64:
    case kUpb_FieldType_SInt64:
      *out_size = 8;
      return true;
    case kUpb_FieldType_Message:
    case kUpb_FieldType_Group:
      *out_size = sizeof(void*);
      return true;
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes:
      *out_size = UPB_MAPTYPE_STRING;
      return true;
  }
  return false;
}

// Attempts to fill a fasttable entry for a map field. Returns false if the map
// is incompatible with fast decoding.
UPB_FORCEINLINE bool upb_DecodeFast_TryFillMapEntry(
    const upb_MiniTableField* field, const upb_MiniTable* map_sub, uint16_t tag,
    uint64_t subofs, upb_DecodeFast_TableEntry* entry) {
  if (!UPB_DECODEFAST_MAP_ISENABLED()) return false;
  if (!map_sub || map_sub->UPB_PRIVATE(field_count) != 2) {
    return false;
  }

  // Check entry minitable: closed enums (on values) and boolean keys are
  // excluded from fasttable and handled by generic decode fallback.
  const upb_MiniTableField* kfield = &map_sub->UPB_PRIVATE(fields)[0];
  const upb_MiniTableField* vfield = &map_sub->UPB_PRIVATE(fields)[1];
  upb_FieldType ktype = (upb_FieldType)kfield->UPB_PRIVATE(descriptortype);
  upb_FieldType vtype = (upb_FieldType)vfield->UPB_PRIVATE(descriptortype);

  if (upb_MiniTableField_IsClosedEnum(vfield)) {
    return false;
  }

  if (vtype == kUpb_FieldType_Group) {
    return false;
  }

  bool is_string_key;
  switch (ktype) {
    case kUpb_FieldType_Int32:
    case kUpb_FieldType_Int64:
    case kUpb_FieldType_UInt32:
    case kUpb_FieldType_UInt64:
    case kUpb_FieldType_SInt32:
    case kUpb_FieldType_SInt64:
    case kUpb_FieldType_Fixed32:
    case kUpb_FieldType_Fixed64:
    case kUpb_FieldType_SFixed32:
    case kUpb_FieldType_SFixed64:
      is_string_key = false;
      break;
    case kUpb_FieldType_String:
    case kUpb_FieldType_Bytes:
      is_string_key = true;
      break;
    default:
      return false;
  }

  bool key_is_zigzag =
      (ktype == kUpb_FieldType_SInt32 || ktype == kUpb_FieldType_SInt64);
  bool val_is_zigzag =
      (vtype == kUpb_FieldType_SInt32 || vtype == kUpb_FieldType_SInt64);

  UPB_ASSUME(!upb_MiniTableField_IsPacked(kfield) &&
             !upb_MiniTableField_IsPacked(vfield));
  uint32_t key_wire_type =
      UPB_PRIVATE(_upb_DecodeFast_GetWireTypeForField)(kfield);
  uint32_t val_wire_type =
      UPB_PRIVATE(_upb_DecodeFast_GetWireTypeForField)(vfield);

  // Must agree with kSizeInMap[] in decode.c, which is what actually sizes the
  // upb_Map. The decoder UPB_ASSUME()s the two agree, so a divergence trips an
  // assert in debug builds rather than silently corrupting the map.
  size_t key_size, val_size;
  if (!UPB_PRIVATE(_upb_DecodeFast_MapTypeSize)(ktype, &key_size) ||
      !UPB_PRIVATE(_upb_DecodeFast_MapTypeSize)(vtype, &val_size)) {
    return false;
  }

  bool is_tag2 = (tag > 0x7f);
  if (is_string_key) {
    entry->function_idx = is_tag2 ? kUpb_DecodeFast_StrMap_Tag2Byte
                                  : kUpb_DecodeFast_StrMap_Tag1Byte;
  } else {
    entry->function_idx = is_tag2 ? kUpb_DecodeFast_IntMap_Tag2Byte
                                  : kUpb_DecodeFast_IntMap_Tag1Byte;
  }

  uint64_t offset = UPB_PRIVATE(_upb_MiniTableField_Offset)(field);
  bool key_validate_utf8 = (ktype == kUpb_FieldType_String);
  bool val_validate_utf8 = (vtype == kUpb_FieldType_String);
  bool val_is_message = (vtype == kUpb_FieldType_Message);
  return upb_DecodeFast_MakeMapData(
      offset, key_is_zigzag, val_is_zigzag, is_tag2, is_string_key,
      key_validate_utf8, val_validate_utf8, val_is_message, key_wire_type,
      val_wire_type, key_size, val_size, subofs, tag, &entry->function_data);
}

// Attempts to fill a fasttable entry for a field. Returns false if the field
// is incompatible with fast decoding.
UPB_NODISCARD
UPB_FORCEINLINE bool upb_DecodeFast_TryFillEntry(
    const upb_MiniTable* m, const upb_MiniTableField* field,
    bool* out_supported_tag_size, upb_DecodeFast_TableEntry* entry) {
  UPB_ASSERT(!upb_MiniTableField_IsExtension(field));
  uint16_t tag;
  upb_DecodeFast_TagSize tag_size;
  *out_supported_tag_size =
      UPB_PRIVATE(_upb_DecodeFast_GetEncodedTag)(field, &tag, &tag_size);
  if (!*out_supported_tag_size) return false;

  if (upb_MiniTableField_IsMap(field)) {
    // Map fields use specialized fast decoder functions:
    // 4 specialized routines for integer and string keys with 1-byte and 2-byte
    // tags, decoding full map entries and handling consecutive entries inline.
    uint64_t subofs = UPB_PRIVATE(_upb_DecodeFast_GetSubofs)(m, field);
    const upb_MiniTableSubInternal* sub =
        (const upb_MiniTableSubInternal*)((const char*)m->UPB_ONLYBITS(fields) +
                                          subofs * 8);
    const upb_MiniTable* map_sub = sub->UPB_PRIVATE(submsg);
    return upb_DecodeFast_TryFillMapEntry(field, map_sub, tag, subofs, entry);
  }

  return UPB_PRIVATE(_upb_DecodeFast_GetFunctionIndex)(field, tag_size,
                                                       &entry->function_idx) &&
         UPB_DECODEFAST_ISENABLED(
             upb_DecodeFast_GetType(entry->function_idx),
             upb_DecodeFast_GetCardinality(entry->function_idx),
             upb_DecodeFast_GetTagSize(entry->function_idx)) &&
         UPB_PRIVATE(_upb_DecodeFast_GetFunctionData)(m, field, tag,
                                                      &entry->function_data);
}

// Builds the fasttable for the given message.  The table will be written into
// the given array.  Returns the number of entries in the table that were
// actually used, and should be written to the MiniTable (this number may be
// less than 32).
//
// This function will assume that the lower a field number, the hotter the field
// is.  If at some point we get access to more information about field usage,
// we should consider using that instead.
int upb_DecodeFast_BuildTable(const upb_MiniTable* m,
                              upb_DecodeFast_TableEntry table[32]);

// Returns the mask that should be placed into the table_mask field of the
// mini table for the given table size.
uint8_t upb_DecodeFast_GetTableMask(int table_size);

// Translates a function index into the canonical function name which can be
// emitted into generated code.
const char* upb_DecodeFast_GetFunctionName(uint32_t function_idx);

#ifdef __cplusplus
}  // extern "C"
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_INTERNAL_DECODE_FAST_SELECT_H_
