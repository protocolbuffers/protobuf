// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/decode_fast/select.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/internal/log2.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

int upb_DecodeFast_BuildTable(const upb_MiniTable* m,
                              upb_DecodeFast_TableEntry table[32]) {
  if (m->UPB_PRIVATE(ext) & kUpb_ExtMode_IsMapEntry) return 0;

  for (size_t i = 0; i < 32; i++) {
    table[i].function_idx = UINT32_MAX;
    table[i].function_data = 0;
  }

  // Fasttable only handles fields with tag size of 1 or 2 bytes. If all known
  // fields with such tag sizes have supported field types, we can short circuit
  // slot misses to unknown field handling
  bool all_supported_tag_size_fields_compatible_with_fast_decode = true;

  // If, in addition, all handled fields are assigned unique slots, then we can
  // short circuit slot collision to unknown field handling as well.
  bool all_fields_assigned_unique_slots = true;

  int max = 0;
  for (size_t i = 0, n = upb_MiniTable_FieldCount(m); i < n; i++) {
    const upb_MiniTableField* field = upb_MiniTable_GetFieldByIndex(m, i);
    bool supported_tag_size;
    upb_DecodeFast_TableEntry entry;
    if (!upb_DecodeFast_TryFillEntry(m, field, &supported_tag_size, &entry)) {
      if (supported_tag_size) {
        // Check if this tag collides
        all_supported_tag_size_fields_compatible_with_fast_decode = false;
      }
      continue;
    }
    int slot = upb_DecodeFastData_GetTableSlot(entry.function_data);
    if (table[slot].function_idx == UINT32_MAX) {
      table[slot] = entry;
      max = UPB_MAX(max, slot);
    } else {
      all_fields_assigned_unique_slots = false;
      if (!upb_MiniTableField_IsScalar(field) &&
          !UPB_PRIVATE(_upb_MiniTableField_IsFastTableCollision)(field)) {
        UPB_PRIVATE(_upb_MiniTableField_SetIsFastTableCollision)(
            (upb_MiniTableField*)field);
      }
    }
  }

  int table_size = max == 0 ? 0 : upb_RoundUpToPowerOfTwo(max + 1);

  // If the message is not extendable, we can swap the generic handler for a
  // fast unknown field handler in remaining open slots.
  // The fast unknown handler only covers 1/2 byte tags and falls back for >2
  // bytes; thus, we do not need to check for total exhaustiveness in field
  // coverage, only for 1/2 byte tags.
  const bool non_extendable =
      UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(m) == kUpb_ExtMode_NonExtendable;
  if (all_supported_tag_size_fields_compatible_with_fast_decode &&
      (non_extendable ||
       UPB_PRIVATE(_upb_MiniTable_ExtModeBase)(m) == kUpb_ExtMode_Extendable)) {
    uint32_t fast_handler_idx = non_extendable
                                    ? kUpb_DecodeFast_Unknown
                                    : kUpb_DecodeFast_ExtensionOrUnknown;
    for (int i = 0; i < table_size; i++) {
      if (table[i].function_idx == UINT32_MAX) {
        table[i].function_idx = fast_handler_idx;
        table[i].function_data = 0;
      }
    }
    // Also override generic fallback if all fields are assigned unique slots.
    if (all_fields_assigned_unique_slots) {
      ((upb_MiniTable*)m)->UPB_PRIVATE(ext) |=
          kUpb_ExtMode_AllFastFieldsAssigned;
    }
  }

  return table_size;
}

uint8_t upb_DecodeFast_GetTableMask(int table_size) {
  return table_size > 0 ? (table_size - 1) << 3 : 255;
}

const char* upb_DecodeFast_GetFunctionName(uint32_t function_idx) {
#define STRINGIFY1(x) #x
#define STRINGIFY2(x) STRINGIFY1(x)
#define FUNCSTR(...) STRINGIFY2(UPB_DECODEFAST_FUNCNAME(__VA_ARGS__)),
  // Constructing all combinations of strings at compile time wastes ~2k of
  // binary size and RAM compared with using eg. snprintf() at runtime.  But
  // this function is only used from the compiler, where 2k is inconsequential.
  static const char* names[] = {UPB_DECODEFAST_FUNCTIONS(FUNCSTR)};
#undef STRINGIFY1
#undef STRINGIFY2
#undef FUNCSTR

  if (function_idx == UINT32_MAX) return "_upb_FastDecoder_DecodeGeneric";
  if (function_idx == kUpb_DecodeFast_Unknown) {
    return "_upb_FastDecoder_DecodeUnknown";
  }
  if (function_idx == kUpb_DecodeFast_ExtensionOrUnknown) {
    return "_upb_FastDecoder_DecodeExtensionOrUnknown";
  }
  if (function_idx == kUpb_DecodeFast_IntMap_Tag1Byte) {
    return "upb_DecodeFast_IntMap_Tag1Byte";
  }
  if (function_idx == kUpb_DecodeFast_IntMap_Tag2Byte) {
    return "upb_DecodeFast_IntMap_Tag2Byte";
  }
  if (function_idx == kUpb_DecodeFast_StrMap_Tag1Byte) {
    return "upb_DecodeFast_StrMap_Tag1Byte";
  }
  if (function_idx == kUpb_DecodeFast_StrMap_Tag2Byte) {
    return "upb_DecodeFast_StrMap_Tag2Byte";
  }
  UPB_ASSERT(function_idx < UPB_ARRAY_SIZE(names));
  return names[function_idx];
}

#include "upb/port/undef.inc"
