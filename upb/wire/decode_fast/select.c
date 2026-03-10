// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/select.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/internal/log2.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

uint32_t GetWireTypeForField(const upb_MiniTableField* field) {
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

static bool upb_DecodeFast_GetEncodedTag(const upb_MiniTableField* field,
                                         uint16_t* out_tag,
                                         upb_DecodeFast_TagSize* out_tag_size) {
  uint32_t num = upb_MiniTableField_Number(field);
  uint32_t wire_type = GetWireTypeForField(field);
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

static bool upb_DecodeFast_GetFieldCardinality(
    const upb_MiniTableField* field,
    upb_DecodeFast_Cardinality* out_cardinality) {
  switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(field)) {
    case kUpb_FieldMode_Map:
      return false;  // Can't parse maps with fast decode.
    case kUpb_FieldMode_Array:
      *out_cardinality = upb_MiniTableField_IsPacked(field)
                             ? kUpb_DecodeFast_Packed
                             : kUpb_DecodeFast_Repeated;
      return true;
    case kUpb_FieldMode_Scalar:
      *out_cardinality = upb_MiniTableField_IsInOneof(field)
                             ? kUpb_DecodeFast_Oneof
                             : kUpb_DecodeFast_Scalar;
      return true;
  }
  UPB_UNREACHABLE();
}

static bool upb_DecodeFast_GetFieldType(const upb_MiniTable* m,
                                        const upb_MiniTableField* field,
                                        upb_DecodeFast_Type* out_type) {
  // We use descriptortype directly instead of upb_MiniTableField_Type because
  // we want the munging of field->descriptortype:
  //  - kUpb_FieldType_String -> kUpb_FieldType_Bytes if no UTF-8 validation is
  //    required.
  //  - kUpb_FieldType_Enum -> kUpb_FieldType_Int32 if the enum is open.
  upb_FieldType type = field->UPB_PRIVATE(descriptortype);

  if (type == kUpb_FieldType_Group || upb_MiniTableField_IsClosedEnum(field)) {
    return false;  // Currently not supported.
  }

  static const int8_t types[] = {
      [kUpb_FieldType_Bool] = kUpb_DecodeFast_Bool,
      [kUpb_FieldType_Enum] = kUpb_DecodeFast_Varint32,
      [kUpb_FieldType_Int32] = kUpb_DecodeFast_Varint32,
      [kUpb_FieldType_UInt32] = kUpb_DecodeFast_Varint32,
      [kUpb_FieldType_Int64] = kUpb_DecodeFast_Varint64,
      [kUpb_FieldType_UInt64] = kUpb_DecodeFast_Varint64,
      [kUpb_FieldType_Fixed32] = kUpb_DecodeFast_Fixed32,
      [kUpb_FieldType_SFixed32] = kUpb_DecodeFast_Fixed32,
      [kUpb_FieldType_Float] = kUpb_DecodeFast_Fixed32,
      [kUpb_FieldType_Fixed64] = kUpb_DecodeFast_Fixed64,
      [kUpb_FieldType_SFixed64] = kUpb_DecodeFast_Fixed64,
      [kUpb_FieldType_Double] = kUpb_DecodeFast_Fixed64,
      [kUpb_FieldType_SInt32] = kUpb_DecodeFast_ZigZag32,
      [kUpb_FieldType_SInt64] = kUpb_DecodeFast_ZigZag64,
      [kUpb_FieldType_String] = kUpb_DecodeFast_String,
      [kUpb_FieldType_Bytes] = kUpb_DecodeFast_Bytes,
      [kUpb_FieldType_Message] = kUpb_DecodeFast_Message,
  };

  UPB_ASSERT(type < UPB_ARRAY_SIZE(types));
  *out_type = types[type];
  return true;
}

static bool upb_DecodeFast_GetFunctionIndex(const upb_MiniTable* m,
                                            const upb_MiniTableField* field,
                                            upb_DecodeFast_TagSize tag_size,
                                            uint32_t* out_index) {
  upb_DecodeFast_Cardinality cardinality;
  upb_DecodeFast_Type type;
  if (!upb_DecodeFast_GetFieldCardinality(field, &cardinality)) return false;
  if (!upb_DecodeFast_GetFieldType(m, field, &type)) return false;

  *out_index = type << 3 | cardinality << 1 | tag_size;
  return true;
}

static uint64_t upb_DecodeFast_GetPresence(const upb_MiniTableField* field,
                                           uint64_t* out_data) {
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

static bool upb_DecodeFast_GetFunctionData(const upb_MiniTableField* field,
                                           uint16_t tag, uint64_t* out_data) {
  uint64_t offset = UPB_PRIVATE(_upb_MiniTableField_Offset)(field);
  uint64_t case_offset =
      upb_MiniTableField_IsInOneof(field)
          ? UPB_PRIVATE(_upb_MiniTableField_OneofOffset)(field)
          : 0;
  uint64_t submsg_index = upb_MiniTableField_IsSubMessage(field)
                              ? field->UPB_PRIVATE(submsg_ofs)
                              : 0;

  uint64_t presence;

  return upb_DecodeFast_GetPresence(field, &presence) &&
         upb_DecodeFast_MakeData(offset, case_offset, presence, submsg_index,
                                 tag, out_data);
}

static bool upb_DecodeFast_TryFillEntry(const upb_MiniTable* m,
                                        const upb_MiniTableField* field,
                                        upb_DecodeFast_TableEntry* entry) {
  UPB_ASSERT(!upb_MiniTableField_IsExtension(field));
  uint16_t tag;
  upb_DecodeFast_TagSize tag_size;
  return upb_DecodeFast_GetEncodedTag(field, &tag, &tag_size) &&
         upb_DecodeFast_GetFunctionIndex(m, field, tag_size,
                                         &entry->function_idx) &&
         UPB_DECODEFAST_ISENABLED(
             upb_DecodeFast_GetType(entry->function_idx),
             upb_DecodeFast_GetCardinality(entry->function_idx),
             upb_DecodeFast_GetTagSize(entry->function_idx)) &&
         upb_DecodeFast_GetFunctionData(field, tag, &entry->function_data);
}

int upb_DecodeFast_BuildTable(const upb_MiniTable* m,
                              upb_DecodeFast_TableEntry table[32]) {
  for (size_t i = 0; i < 32; i++) {
    table[i].function_idx = UINT32_MAX;
    table[i].function_data = 0;
  }

  int max = 0;
  for (size_t i = 0, n = upb_MiniTable_FieldCount(m); i < n; i++) {
    const upb_MiniTableField* field = upb_MiniTable_GetFieldByIndex(m, i);
    upb_DecodeFast_TableEntry entry;
    if (!upb_DecodeFast_TryFillEntry(m, field, &entry)) continue;
    int slot = upb_DecodeFastData_GetTableSlot(entry.function_data);
    if (table[slot].function_idx == UINT32_MAX) {
      table[slot] = entry;
      max = UPB_MAX(max, slot);
    }
  }

  return max == 0 ? 0 : upb_RoundUpToPowerOfTwo(max + 1);
}

uint8_t upb_DecodeFast_GetTableMask(int table_size) {
  return table_size > 0 ? (table_size - 1) << 3 : 255;
}

const char* upb_DecodeFast_GetFunctionName(uint32_t function_idx) {
#define STRINGIFY1(x) #x
#define STRINGIFY2(x) STRINGIFY1(x)
#define FUNCSTR(...) STRINGIFY2(UPB_DECODEFAST_FUNCNAME(__VA_ARGS__)),
  // Constructing all combinations of strings at compile time wastes ~2k of
  // binary size and RAM compared with using eg. snprintf() at runtime.  But
  // this function is only used from the compiler, where 2k is inconsequential.
  static const char* names[] = {UPB_DECODEFAST_FUNCTIONS(FUNCSTR)};
#undef STRINGIFY1
#undef STRINGIFY2
#undef FUNCSTR

  if (function_idx == UINT32_MAX) return "_upb_FastDecoder_DecodeGeneric";
  UPB_ASSERT(function_idx < UPB_ARRAY_SIZE(names));
  return names[function_idx];
}

#include "upb/port/undef.inc"
