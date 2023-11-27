// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_FIELD_H_
#define UPB_MINI_TABLE_FIELD_H_

#include <stddef.h>

#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct upb_MiniTableField upb_MiniTableField;

#ifdef __cplusplus
extern "C" {
#endif

UPB_API_INLINE upb_FieldType
upb_MiniTableField_Type(const upb_MiniTableField* f) {
  if (f->mode & kUpb_LabelFlags_IsAlternate) {
    if (f->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Int32) {
      return kUpb_FieldType_Enum;
    } else if (f->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Bytes) {
      return kUpb_FieldType_String;
    } else {
      UPB_ASSERT(false);
    }
  }
  return (upb_FieldType)f->UPB_PRIVATE(descriptortype);
}

UPB_API_INLINE upb_CType upb_MiniTableField_CType(const upb_MiniTableField* f) {
  return upb_FieldType_CType(upb_MiniTableField_Type(f));
}

UPB_API_INLINE bool upb_MiniTableField_IsClosedEnum(
    const upb_MiniTableField* field) {
  return field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Enum;
}

UPB_API_INLINE bool upb_MiniTableField_IsExtension(
    const upb_MiniTableField* f) {
  return f->mode & kUpb_LabelFlags_IsExtension;
}

UPB_API_INLINE bool upb_MiniTableField_IsInOneof(const upb_MiniTableField* f) {
  return f->presence < 0;
}

UPB_API_INLINE bool upb_MiniTableField_IsRepeatedOrMap(
    const upb_MiniTableField* f) {
  // This works because upb_FieldMode has no value 3.
  return !(f->mode & kUpb_FieldMode_Scalar);
}

UPB_API_INLINE bool upb_MiniTableField_IsSubMessage(
    const upb_MiniTableField* f) {
  return f->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Message ||
         f->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Group;
}

UPB_API_INLINE bool upb_MiniTableField_HasPresence(
    const upb_MiniTableField* f) {
  if (upb_MiniTableField_IsExtension(f)) {
    return !upb_MiniTableField_IsRepeatedOrMap(f);
  } else {
    return f->presence != 0;
  }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_FIELD_H_ */
