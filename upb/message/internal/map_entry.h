/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_MINI_TABLE_INTERNAL_MAP_ENTRY_DATA_H_
#define UPB_MINI_TABLE_INTERNAL_MAP_ENTRY_DATA_H_

#include <stdint.h>

#include "upb/base/string_view.h"
#include "upb/hash/common.h"

// Map entries aren't actually stored for map fields, they are only used during
// parsing. For parsing, it helps a lot if all map entry messages have the same
// layout. The layout code in mini_table/decode.c will ensure that all map
// entries have this layout.
//
// Note that users can and do create map entries directly, which will also use
// this layout.
//
// NOTE: sync with mini_table/decode.c.
typedef struct {
  // We only need 2 hasbits max, but due to alignment we'll use 8 bytes here,
  // and the uint64_t helps make this clear.
  uint64_t hasbits;
  union {
    upb_StringView str;  // For str/bytes.
    upb_value val;       // For all other types.
  } k;
  union {
    upb_StringView str;  // For str/bytes.
    upb_value val;       // For all other types.
  } v;
} upb_MapEntryData;

typedef struct {
  void* internal_data;
  upb_MapEntryData data;
} upb_MapEntry;

#endif  // UPB_MINI_TABLE_INTERNAL_MAP_ENTRY_DATA_H_
