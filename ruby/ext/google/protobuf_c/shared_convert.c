// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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