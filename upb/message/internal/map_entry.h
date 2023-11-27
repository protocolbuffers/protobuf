// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_COLLECTIONS_INTERNAL_MAP_ENTRY_H_
#define UPB_COLLECTIONS_INTERNAL_MAP_ENTRY_H_

#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/message/internal/types.h"

// Map entries aren't actually stored for map fields, they are only used during
// parsing. For parsing, it helps a lot if all map entry messages have the same
// layout. The layout code in mini_table/decode.c will ensure that all map
// entries have this layout.
//
// Note that users can and do create map entries directly, which will also use
// this layout.
//
// NOTE: sync with wire/decode.c.
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
  upb_Message_Internal internal;
  upb_MapEntryData data;
} upb_MapEntry;

#endif  // UPB_COLLECTIONS_INTERNAL_MAP_ENTRY_H_
