// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Users should include array.h or map.h instead.
// IWYU pragma: private, include "upb/message/array.h"

#ifndef UPB_MESSAGE_VALUE_H_
#define UPB_MESSAGE_VALUE_H_

#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"

// Must be last.
#include "upb/port/def.inc"

typedef union {
  bool bool_val;
  float float_val;
  double double_val;
  int32_t int32_val;
  int64_t int64_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  const struct upb_Array* array_val;
  const struct upb_Map* map_val;
  const struct upb_Message* msg_val;
  upb_StringView str_val;

  // EXPERIMENTAL: A tagged upb_Message*.  Users must use this instead of
  // msg_val if unlinked sub-messages may possibly be in use.  See the
  // documentation in kUpb_DecodeOption_ExperimentalAllowUnlinked for more
  // information.
  uintptr_t tagged_msg_val;  // upb_TaggedMessagePtr
} upb_MessageValue;

typedef union {
  struct upb_Array* array;
  struct upb_Map* map;
  struct upb_Message* msg;
} upb_MutableMessageValue;

#ifdef __cplusplus
extern "C" {
#endif

UPB_API_INLINE bool upb_MessageValue_IsEqual(upb_MessageValue val1,
                                             upb_MessageValue val2,
                                             upb_CType ctype) {
  switch (ctype) {
    case kUpb_CType_Bool:
      return val1.bool_val == val2.bool_val;

    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return val1.int32_val == val2.int32_val;

    case kUpb_CType_Float:
      return val1.float_val == val2.float_val;

    case kUpb_CType_Double:
      return val1.double_val == val2.double_val;

    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return val1.int64_val == val2.int64_val;

    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return upb_StringView_IsEqual(val1.str_val, val2.str_val);

    default:  // Note: This includes kUpb_CType_Message
      UPB_ASSERT(0);
      return false;
  }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_VALUE_H_ */
