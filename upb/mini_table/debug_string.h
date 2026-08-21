// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_DEBUG_STRING_H_
#define UPB_MINI_TABLE_DEBUG_STRING_H_

#include <stddef.h>

#include "upb/mini_table/message.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t upb_MiniTable_DebugString(const upb_MiniTable* mini_table, char* buf,
                                 size_t size);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // UPB_MINI_TABLE_DEBUG_STRING_H_
