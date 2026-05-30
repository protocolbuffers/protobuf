// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_DECODE_FAST_FIELD_HELPERS_H_
#define UPB_WIRE_DECODE_FAST_FIELD_HELPERS_H_

#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/field.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

// Extract a field number from a 2-byte tag.
UPB_INLINE uint32_t _upb_DecodeFast_Tag2FieldNumber(uint64_t data) {
  return ((data & 0x7f) | ((data & 0x7f00) >> 1)) >> 3;
}

// Manually extract the field number, tag length, and wire type from a tag >=
// 2048. Requires `ptr` and a 2-byte `tag` (the first two bytes of the tag
// loaded by
// `_upb_FastDecoder_LoadTag(ptr)`). Returns false if the tag is malformed; the
// wire format and tag number are not validated.
UPB_FORCEINLINE bool _upb_DecodeFast_ParseLongTag(const char* ptr, uint16_t tag,
                                                  uint32_t* out_field_num,
                                                  uint32_t* out_tag_len) {
  uint32_t field_num = (tag & 0x7f00) >> 1 | (tag & 0x7f);
  int count = 3;
  field_num |= (uint32_t)(ptr[2] & 0x7f) << 14;
  if ((ptr[2] & 0x80) != 0) {
    count = 4;
    field_num |= (uint32_t)(ptr[3] & 0x7f) << 21;
    if ((ptr[3] & 0x80) != 0) {
      count = 5;
      field_num |= (uint32_t)(ptr[4] & 0x7f) << 28;
      if (UPB_UNLIKELY((ptr[4] & 0xf0) != 0)) {
        return false;
      }
    }
  }
  *out_tag_len = count;
  *out_field_num = field_num >> 3;
  return true;
}

UPB_FORCEINLINE bool _upb_DecodeFast_ParseTag(const char* ptr, uint16_t tag,
                                              uint32_t* out_field_num,
                                              uint32_t* out_tag_len) {
  // Important: if the branch is correctly predicted, the tag_len assignment
  // is treated as constant and subsequent loads will not have a data
  // dependency on the branch.
  if (UPB_LIKELY((tag & 0x80) == 0)) {
    *out_tag_len = 1;
    *out_field_num = (uint8_t)tag >> 3;
    return true;
  } else if (UPB_LIKELY((tag & 0x8000) == 0)) {
    *out_tag_len = 2;
    *out_field_num = _upb_DecodeFast_Tag2FieldNumber(tag);
    return true;
  } else {
    return _upb_DecodeFast_ParseLongTag(ptr, tag, out_field_num, out_tag_len);
  }
}

UPB_INLINE uint32_t
_upb_MiniTableField_GetWireType(const upb_MiniTableField* field) {
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

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_DECODE_FAST_FIELD_HELPERS_H_
