// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_MINITABLE_REGISTRY_H_
#define UPB_MINI_TABLE_MINITABLE_REGISTRY_H_

#include <stdint.h>

#include "upb/mini_table/internal/generated_registry.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_MiniTableEntry {
  uint64_t hash;
  const struct upb_MiniTable* minitable;
} upb_MiniTableEntry;

UPB_API const struct upb_MiniTable* upb_MinitableRegistry_Lookup(uint64_t hash);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MINI_TABLE_MINITABLE_REGISTRY_H_
