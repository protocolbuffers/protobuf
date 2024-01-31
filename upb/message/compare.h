// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_COMPARE_H_
#define UPB_MESSAGE_COMPARE_H_

#include "upb/base/descriptor_constants.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Compares two messages by serializing them and calling memcmp().
UPB_API bool upb_Message_IsExactlyEqual(const upb_Message* msg1,
                                        const upb_Message* msg2,
                                        const upb_MiniTable* m);

// Performs a shallow field comparison. Do not use on message types.
UPB_API_INLINE bool upb_MessageValue_IsEqual(upb_MessageValue val1,
                                             upb_MessageValue val2,
                                             upb_CType ctype) {
  switch (ctype) {
    case kUpb_CType_Bool:
      return val1.bool_val == val2.bool_val;

    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return val1.int32_val == val2.int32_val;

    case kUpb_CType_Double:
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

#endif  // UPB_MESSAGE_COMPARE_H_
