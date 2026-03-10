// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_DECODE_FAST_FUNCTION_ARRAY_H_
#define UPB_WIRE_INTERNAL_DECODE_FAST_FUNCTION_ARRAY_H_

#include <stdint.h>

#include "upb/mini_table/internal/message.h"

_upb_FieldParser* upb_DecodeFast_GetFunctionPointer(uint32_t function_idx);

#endif  // UPB_WIRE_INTERNAL_DECODE_FAST_FUNCTION_ARRAY_H_
