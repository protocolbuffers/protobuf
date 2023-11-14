// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_FIELD_H_
#define UPB_MINI_TABLE_FIELD_H_

#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_MiniTableField upb_MiniTableField;

UPB_API_INLINE upb_FieldType
upb_MiniTableField_Type(const upb_MiniTableField* field) {
  if (field->mode & kUpb_LabelFlags_IsAlternate) {
    if (field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Int32) {
      return kUpb_FieldType_Enum;
    } else if (field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Bytes) {
      return kUpb_FieldType_String;
    } else {
      UPB_ASSERT(false);
    }
  }
  return (upb_FieldType)field->UPB_PRIVATE(descriptortype);
}

UPB_API_INLINE upb_CType upb_MiniTableField_CType(const upb_MiniTableField* f) {
  return upb_FieldType_CType(upb_MiniTableField_Type(f));
}

UPB_API_INLINE bool upb_MiniTableField_IsExtension(
    const upb_MiniTableField* field) {
  return field->mode & kUpb_LabelFlags_IsExtension;
}

UPB_API_INLINE bool upb_MiniTableField_IsClosedEnum(
    const upb_MiniTableField* field) {
  return field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Enum;
}

UPB_API_INLINE bool upb_MiniTableField_HasPresence(
    const upb_MiniTableField* field) {
  if (upb_MiniTableField_IsExtension(field)) {
    return !upb_IsRepeatedOrMap(field);
  } else {
    return field->presence != 0;
  }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_FIELD_H_ */
