// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_MESSAGE_H_
#define UPB_MINI_TABLE_INTERNAL_MESSAGE_H_

#include "upb/message/types.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/sub.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_Decoder;
typedef const char* _upb_FieldParser(struct upb_Decoder* d, const char* ptr,
                                     upb_Message* msg, intptr_t table,
                                     uint64_t hasbits, uint64_t data);
typedef struct {
  uint64_t field_data;
  _upb_FieldParser* field_parser;
} _upb_FastTable_Entry;

typedef enum {
  kUpb_ExtMode_NonExtendable = 0,  // Non-extendable message.
  kUpb_ExtMode_Extendable = 1,     // Normal extendable message.
  kUpb_ExtMode_IsMessageSet = 2,   // MessageSet message.
  kUpb_ExtMode_IsMessageSet_ITEM =
      3,  // MessageSet item (temporary only, see decode.c)

  // During table building we steal a bit to indicate that the message is a map
  // entry.  *Only* used during table building!
  kUpb_ExtMode_IsMapEntry = 4,
} upb_ExtMode;

// upb_MiniTable represents the memory layout of a given upb_MessageDef.
// The members are public so generated code can initialize them,
// but users MUST NOT directly read or write any of its members.
// LINT.IfChange(minitable_struct_definition)
struct upb_MiniTable {
  const union upb_MiniTableSub* UPB_PRIVATE(subs);
  const struct upb_MiniTableField* UPB_ONLYBITS(fields);

  // Must be aligned to sizeof(void*). Doesn't include internal members like
  // unknown fields, extension dict, pointer to msglayout, etc.
  uint16_t UPB_PRIVATE(size);

  uint16_t UPB_ONLYBITS(field_count);

  uint8_t UPB_PRIVATE(ext);  // upb_ExtMode, uint8_t here so sizeof(ext) == 1
  uint8_t UPB_PRIVATE(dense_below);
  uint8_t UPB_PRIVATE(table_mask);
  uint8_t UPB_PRIVATE(required_count);  // Required fields have the low hasbits.

  // To statically initialize the tables of variable length, we need a flexible
  // array member, and we need to compile in gnu99 mode (constant initialization
  // of flexible array members is a GNU extension, not in C99 unfortunately.
  _upb_FastTable_Entry UPB_PRIVATE(fasttable)[];
};
// LINT.ThenChange(//depot/google3/third_party/upb/bits/typescript/mini_table.ts)

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE const struct upb_MiniTable* UPB_PRIVATE(_upb_MiniTable_Empty)(void) {
  extern const struct upb_MiniTable UPB_PRIVATE(_kUpb_MiniTable_Empty);

  return &UPB_PRIVATE(_kUpb_MiniTable_Empty);
}

UPB_INLINE int UPB_PRIVATE(_upb_MiniTable_FieldCount)(
    const struct upb_MiniTable* m) {
  return m->UPB_ONLYBITS(field_count);
}

UPB_INLINE bool UPB_PRIVATE(_upb_MiniTable_IsEmpty)(
    const struct upb_MiniTable* m) {
  extern const struct upb_MiniTable UPB_PRIVATE(_kUpb_MiniTable_Empty);

  return m == &UPB_PRIVATE(_kUpb_MiniTable_Empty);
}

UPB_INLINE const struct upb_MiniTableField* UPB_PRIVATE(
    _upb_MiniTable_GetFieldByIndex)(const struct upb_MiniTable* m, uint32_t i) {
  return &m->UPB_ONLYBITS(fields)[i];
}

UPB_INLINE const union upb_MiniTableSub* UPB_PRIVATE(
    _upb_MiniTable_GetSubByIndex)(const struct upb_MiniTable* m, uint32_t i) {
  return &m->UPB_PRIVATE(subs)[i];
}

UPB_INLINE const struct upb_MiniTable* UPB_PRIVATE(
    _upb_MiniTable_GetSubMessageTable)(const struct upb_MiniTable* m,
                                       const struct upb_MiniTableField* f) {
  UPB_ASSERT(UPB_PRIVATE(_upb_MiniTableField_CType)(f) == kUpb_CType_Message);
  const struct upb_MiniTable* ret = UPB_PRIVATE(_upb_MiniTableSub_Message)(
      m->UPB_PRIVATE(subs)[f->UPB_PRIVATE(submsg_index)]);
  UPB_ASSUME(ret);
  return UPB_PRIVATE(_upb_MiniTable_IsEmpty)(ret) ? NULL : ret;
}

UPB_INLINE const struct upb_MiniTableEnum* UPB_PRIVATE(
    _upb_MiniTable_GetSubEnumTable)(const struct upb_MiniTable* m,
                                    const struct upb_MiniTableField* f) {
  UPB_ASSERT(UPB_PRIVATE(_upb_MiniTableField_CType)(f) == kUpb_CType_Enum);
  return UPB_PRIVATE(_upb_MiniTableSub_Enum)(
      m->UPB_PRIVATE(subs)[f->UPB_PRIVATE(submsg_index)]);
}

UPB_INLINE const struct upb_MiniTableField* UPB_PRIVATE(_upb_MiniTable_MapKey)(
    const struct upb_MiniTable* m) {
  UPB_ASSERT(UPB_PRIVATE(_upb_MiniTable_FieldCount)(m) == 2);
  const struct upb_MiniTableField* f =
      UPB_PRIVATE(_upb_MiniTable_GetFieldByIndex)(m, 0);
  UPB_ASSERT(UPB_PRIVATE(_upb_MiniTableField_Number)(f) == 1);
  return f;
}

UPB_INLINE const struct upb_MiniTableField* UPB_PRIVATE(
    _upb_MiniTable_MapValue)(const struct upb_MiniTable* m) {
  UPB_ASSERT(UPB_PRIVATE(_upb_MiniTable_FieldCount)(m) == 2);
  const struct upb_MiniTableField* f =
      UPB_PRIVATE(_upb_MiniTable_GetFieldByIndex)(m, 1);
  UPB_ASSERT(UPB_PRIVATE(_upb_MiniTableField_Number)(f) == 2);
  return f;
}

UPB_INLINE bool UPB_PRIVATE(_upb_MiniTable_MessageFieldIsLinked)(
    const struct upb_MiniTable* m, const struct upb_MiniTableField* f) {
  return UPB_PRIVATE(_upb_MiniTable_GetSubMessageTable)(m, f) != NULL;
}

// Computes a bitmask in which the |m->required_count| lowest bits are set,
// except that we skip the lowest bit (because upb never uses hasbit 0).
//
// Sample output:
//    RequiredMask(1) => 0b10 (0x2)
//    RequiredMask(5) => 0b111110 (0x3e)
UPB_INLINE uint64_t
UPB_PRIVATE(_upb_MiniTable_RequiredMask)(const struct upb_MiniTable* m) {
  int n = m->UPB_PRIVATE(required_count);
  UPB_ASSERT(0 < n && n <= 63);
  return ((1ULL << n) - 1) << 1;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_MESSAGE_H_ */
