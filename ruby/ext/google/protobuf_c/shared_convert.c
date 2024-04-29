// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// -----------------------------------------------------------------------------
// Ruby <-> upb data conversion functions. Strictly free of dependencies on
// Ruby interpreter internals.

#include "shared_convert.h"

bool shared_Msgval_IsEqual(upb_MessageValue val1, upb_MessageValue val2,
                           upb_CType type, upb_MessageDef* msgdef,
                           upb_Status* status) {
  switch (type) {
    case kUpb_CType_Bool:
      return memcmp(&val1, &val2, 1) == 0;
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return memcmp(&val1, &val2, 4) == 0;
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return memcmp(&val1, &val2, 8) == 0;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return val1.str_val.size == val2.str_val.size &&
             memcmp(val1.str_val.data, val2.str_val.data, val1.str_val.size) ==
                 0;
    case kUpb_CType_Message:
      return shared_Message_Equal(val1.msg_val, val2.msg_val, msgdef, status);
    default:
      upb_Status_SetErrorMessage(status, "Internal error, unexpected type");
  }
}

uint64_t shared_Msgval_GetHash(upb_MessageValue val, upb_CType type,
                               upb_MessageDef* msgdef, uint64_t seed,
                               upb_Status* status) {
  switch (type) {
    case kUpb_CType_Bool:
      return _upb_Hash(&val, 1, seed);
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return _upb_Hash(&val, 4, seed);
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return _upb_Hash(&val, 8, seed);
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return _upb_Hash(val.str_val.data, val.str_val.size, seed);
    case kUpb_CType_Message:
      return shared_Message_Hash(val.msg_val, msgdef, seed, status);
    default:
      upb_Status_SetErrorMessage(status, "Internal error, unexpected type");
  }
}