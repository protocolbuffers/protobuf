// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_COMPARE_H_
#define UPB_MESSAGE_COMPARE_H_

#include <stddef.h>

#include "upb/base/descriptor_constants.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Returns true if no known fields or extensions are set in the message.
UPB_API bool upb_Message_IsEmpty(const upb_Message* msg,
                                 const upb_MiniTable* m);

typedef enum {
  kUpb_UnknownCompareResult_Equal = 0,
  kUpb_UnknownCompareResult_NotEqual = 1,
  kUpb_UnknownCompareResult_OutOfMemory = 2,
  kUpb_UnknownCompareResult_MaxDepthExceeded = 3,
} upb_UnknownCompareResult;

// Returns true if unknown fields from the two messages are equal when sorted
// and varints are made canonical.
//
// This function is discouraged, as the comparison is inherently lossy without
// schema data:
//
//  1. We don't know whether delimited fields are sub-messages. Unknown
//     sub-messages will therefore not have their fields sorted and varints
//     canonicalized.
//  2. We don't know about oneof/non-repeated fields, which should semantically
//     discard every value except the last.
UPB_API upb_UnknownCompareResult
upb_Message_IsUnknownEqual(const upb_Message* msg1, const upb_Message* msg2,
                           upb_alloc* alloc, int max_depth);

// Tests whether two messages are considered equal. This does not take into
// account unknown fields.
UPB_API bool upb_Message_IsEqual(const upb_Message* msg1,
                                 const upb_Message* msg2,
                                 const upb_MiniTable* m, int options);

// If |ctype| is a message then |m| must point to its minitable.
UPB_API_INLINE bool upb_MessageValue_IsEqual(upb_MessageValue val1,
                                             upb_MessageValue val2,
                                             upb_CType ctype,
                                             const upb_MiniTable* m,
                                             int options) {
  switch (ctype) {
    case kUpb_CType_Bool:
      return val1.bool_val == val2.bool_val;

    case kUpb_CType_Float:
      return val1.float_val == val2.float_val;

    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return val1.int32_val == val2.int32_val;

    case kUpb_CType_Double:
      return val1.double_val == val2.double_val;

    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return val1.int64_val == val2.int64_val;

    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return upb_StringView_IsEqual(val1.str_val, val2.str_val);

    case kUpb_CType_Message:
      return upb_Message_IsEqual(val1.msg_val, val2.msg_val, m, options);

    default:
      UPB_UNREACHABLE();
      return false;
  }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_COMPARE_H_
