// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_FIELD_H_
#define UPB_MINI_TABLE_INTERNAL_FIELD_H_

#include <stdint.h>

#include "upb/base/descriptor_constants.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MiniTableField {
  uint32_t number;
  uint16_t offset;
  int16_t presence;       // If >0, hasbit_index.  If <0, ~oneof_index

  // Indexes into `upb_MiniTable.subs`
  // Will be set to `kUpb_NoSub` if `descriptortype` != MESSAGE/GROUP/ENUM
  uint16_t UPB_PRIVATE(submsg_index);

  uint8_t UPB_PRIVATE(descriptortype);

  // upb_FieldMode | upb_LabelFlags | (upb_FieldRep << kUpb_FieldRep_Shift)
  uint8_t mode;
};

#define kUpb_NoSub ((uint16_t)-1)

typedef enum {
  kUpb_FieldMode_Map = 0,
  kUpb_FieldMode_Array = 1,
  kUpb_FieldMode_Scalar = 2,
} upb_FieldMode;

// Mask to isolate the upb_FieldMode from field.mode.
#define kUpb_FieldMode_Mask 3

// Extra flags on the mode field.
typedef enum {
  kUpb_LabelFlags_IsPacked = 4,
  kUpb_LabelFlags_IsExtension = 8,
  // Indicates that this descriptor type is an "alternate type":
  //   - for Int32, this indicates that the actual type is Enum (but was
  //     rewritten to Int32 because it is an open enum that requires no check).
  //   - for Bytes, this indicates that the actual type is String (but does
  //     not require any UTF-8 check).
  kUpb_LabelFlags_IsAlternate = 16,
} upb_LabelFlags;

// Note: we sort by this number when calculating layout order.
typedef enum {
  kUpb_FieldRep_1Byte = 0,
  kUpb_FieldRep_4Byte = 1,
  kUpb_FieldRep_StringView = 2,
  kUpb_FieldRep_8Byte = 3,

  kUpb_FieldRep_NativePointer =
      UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte),
  kUpb_FieldRep_Max = kUpb_FieldRep_8Byte,
} upb_FieldRep;

#define kUpb_FieldRep_Shift 6

UPB_INLINE upb_FieldRep
_upb_MiniTableField_GetRep(const struct upb_MiniTableField* field) {
  return (upb_FieldRep)(field->mode >> kUpb_FieldRep_Shift);
}

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE upb_FieldMode
upb_FieldMode_Get(const struct upb_MiniTableField* field) {
  return (upb_FieldMode)(field->mode & 3);
}

UPB_INLINE void _upb_MiniTableField_CheckIsArray(
    const struct upb_MiniTableField* field) {
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_NativePointer);
  UPB_ASSUME(upb_FieldMode_Get(field) == kUpb_FieldMode_Array);
  UPB_ASSUME(field->presence == 0);
}

UPB_INLINE void _upb_MiniTableField_CheckIsMap(
    const struct upb_MiniTableField* field) {
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_NativePointer);
  UPB_ASSUME(upb_FieldMode_Get(field) == kUpb_FieldMode_Map);
  UPB_ASSUME(field->presence == 0);
}

UPB_INLINE bool upb_IsRepeatedOrMap(const struct upb_MiniTableField* field) {
  // This works because upb_FieldMode has no value 3.
  return !(field->mode & kUpb_FieldMode_Scalar);
}

UPB_INLINE bool upb_IsSubMessage(const struct upb_MiniTableField* field) {
  return field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Message ||
         field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Group;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_FIELD_H_ */
