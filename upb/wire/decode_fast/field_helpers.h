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
