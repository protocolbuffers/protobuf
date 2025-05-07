// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_DECODE_FAST_SELECT_H_
#define UPB_WIRE_INTERNAL_DECODE_FAST_SELECT_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // The function that will be called to parse this field.  The function
  // pointer for it must be placed into _upb_FastTable_Entry.field_parser.
  //
  // The function pointer can be either looked up at runtime via
  // upb_DecodeFast_GetFunctionPointer(), or be referenced in generated code via
  // upb_DecodeFast_GetFunctionName().
  uint32_t function_idx;

  // The associated data that will be passed to the function.  This must be
  // placed into _upb_FastTable_Entry.field_data.
  uint64_t function_data;
} upb_DecodeFast_TableEntry;

// Builds the fasttable for the given message.  The table will be written into
// the given array.  Returns the number of entries in the table that were
// actually used, and should be written to the MiniTable (this number may be
// less than 32).
//
// This function will assume that the lower a field number, the hotter the field
// is.  If at some point we get access to more information about field usage,
// we should consider using that instead.
int upb_DecodeFast_BuildTable(const upb_MiniTable* m,
                              upb_DecodeFast_TableEntry table[32]);

// Returns the mask that should be placed into the table_mask field of the
// mini table for the given table size.
uint8_t upb_DecodeFast_GetTableMask(int table_size);

// Translates a function index into the canonical function name which can be
// emitted into generated code.
const char* upb_DecodeFast_GetFunctionName(uint32_t function_idx);

// Returns a function pointer for the given function index.
_upb_FieldParser* upb_DecodeFast_GetFunctionPointer(uint32_t function_idx);

#ifdef __cplusplus
}  // extern "C"
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_INTERNAL_DECODE_FAST_SELECT_H_
