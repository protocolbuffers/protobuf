// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_INTERNAL_MESSAGE_H_
#define UPB_MINI_TABLE_INTERNAL_MESSAGE_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/sub.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_Decoder;
struct upb_Message;

typedef UPB_PRESERVE_NONE const char* _upb_FieldParser(
    struct upb_Decoder* d, const char* ptr, struct upb_Message* msg,
    intptr_t table, uint64_t hasbits, uint64_t data);

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

enum {
  kUpb_Message_Align = 8,
};

// upb_MiniTable represents the memory layout of a given upb_MessageDef.
// The members are public so generated code can initialize them,
// but users MUST NOT directly read or write any of its members.

// LINT.IfChange(minitable_struct_definition)
struct upb_MiniTable {
  const struct upb_MiniTableField* UPB_ONLYBITS(fields);

  // Must be aligned to kUpb_Message_Align. Doesn't include internal members
  // like unknown fields, extension dict, pointer to msglayout, etc.
  uint16_t UPB_PRIVATE(size);

  uint16_t UPB_ONLYBITS(field_count);

  uint8_t UPB_PRIVATE(ext);  // upb_ExtMode, uint8_t here so sizeof(ext) == 1
  uint8_t UPB_PRIVATE(dense_below);
  uint8_t UPB_PRIVATE(table_mask);
  uint8_t UPB_PRIVATE(required_count);  // Required fields have the low hasbits.

#ifdef UPB_TRACING_ENABLED
  const char* UPB_PRIVATE(full_name);
#endif

#if UPB_FASTTABLE || !defined(__cplusplus)
  // Flexible array member is not supported in C++, but it is an extension in
  // every compiler that supports UPB_FASTTABLE.
  _upb_FastTable_Entry UPB_PRIVATE(fasttable)[];
#endif
};
// LINT.ThenChange(//depot/google3/third_party/upb/bits/typescript/mini_table.ts)

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE void UPB_PRIVATE(upb_MiniTable_CheckInvariants)(
    const struct upb_MiniTable* mt) {
  UPB_STATIC_ASSERT(UPB_MALLOC_ALIGN >= kUpb_Message_Align, "Under aligned");
  UPB_STATIC_ASSERT(kUpb_Message_Align >= UPB_ALIGN_OF(void*), "Under aligned");
  UPB_ASSERT(mt->UPB_PRIVATE(size) % kUpb_Message_Align == 0);
}

UPB_INLINE const struct upb_MiniTable* UPB_PRIVATE(
    _upb_MiniTable_StrongReference)(const struct upb_MiniTable* mt) {
#if defined(__GNUC__)
  __asm__("" : : "r"(mt));
#else
  const struct upb_MiniTable* volatile unused = mt;
  (void)&unused;  // Use address to avoid an extra load of "unused".
#endif
  return mt;
}

UPB_API_INLINE int upb_MiniTable_FieldCount(const struct upb_MiniTable* m) {
  return m->UPB_ONLYBITS(field_count);
}

UPB_API_INLINE bool upb_MiniTable_IsMessageSet(const struct upb_MiniTable* m) {
  return m->UPB_PRIVATE(ext) == kUpb_ExtMode_IsMessageSet;
}

UPB_FORCEINLINE
const struct upb_MiniTableField* UPB_PRIVATE(upb_MiniTable_LowerBound)(
    const struct upb_MiniTable* m, uint32_t lo, uint32_t search_len,
    uint32_t number) {
  const struct upb_MiniTableField* search_base = &m->UPB_ONLYBITS(fields)[lo];
  while (search_len > 1) {
    size_t mid_offset = search_len >> 1;
    if (UPB_UNPREDICTABLE(search_base[mid_offset].UPB_ONLYBITS(number) <=
                          number)) {
      search_base = &search_base[mid_offset];
    }
    search_len -= mid_offset;
  }

  return search_base;
}

// This implements the same algorithm as upb_MiniTable_LowerBound but contorts
// itself to select specific arm instructions, which show significant effects on
// little cores.
UPB_FORCEINLINE const struct upb_MiniTableField* UPB_PRIVATE(
    upb_MiniTable_ArmOptimizedLowerBound)(const struct upb_MiniTable* m,
                                          uint32_t lo, uint32_t search_len,
                                          uint32_t number) {
  const uint32_t* search_base =
      &m->UPB_ONLYBITS(fields)[lo].UPB_ONLYBITS(number);
  UPB_STATIC_ASSERT(sizeof(struct upb_MiniTableField) == sizeof(uint32_t) * 3,
                    "Need to update multiplication");
  // Address generation units can't multiply by 12, but they can by 4. So we
  // split it into multiplying by 3 (add and shift) and multiplying by 4 (shift)
  // This code is carefully tuned to produce an optimal assembly sequence on
  // arm64, which takes advantage of dual issue on in-order CPUs to maximize
  // what little instruction level parallelism they have.
  /*
  and     w9, w1, #0xfffffffe
  add     w9, w9, w1, lsr #1
  ldr     w10, [x0, w9, uxtw #2]
  sub     w1, w1, w1, lsr #1
  add     x9, x0, w9, uxtw #2
  cmp     w10, w2
  csel    x0, x0, x9, hi
  cmp     w1, #1
  b.hi    .LBB3_1
   */
  // Doing this requires inhibiting the natural instincts of the compiler to
  // eliminate duplicate work, so we introduce an optimization barrier with
  // asm blocks to defeat common subexpression elimination.
  UPB_STATIC_ASSERT(
      offsetof(struct upb_MiniTableField, UPB_ONLYBITS(number)) == 0,
      "Tag number must be first element of minitable field struct");
  while (search_len > 1) {
#if UPB_ARM64_ASM
#define UPB_OPT_LAUNDER(val) __asm__("" : "+r"(val))
#define UPB_OPT_LAUNDER2(val1, val2) __asm__("" : "+r"(val1), "+r"(val2))
#else
#define UPB_OPT_LAUNDER(val)
#define UPB_OPT_LAUNDER2(val1, val2)
#endif
    // (search_len & ~1) is exactly (half_len * 2). Adding half_len yields
    // (half_len * 3).
    //
    // and mid_offset_words, search_len, #0xfffffffe
    uint32_t mid_offset_words = search_len & 0xfffffffe;

    // add mid_offset_words, mid_offset_words, search_len, lsr #1
    mid_offset_words = mid_offset_words + (search_len >> 1);

    UPB_OPT_LAUNDER(search_len);
    UPB_OPT_LAUNDER(mid_offset_words);

    // Arm processors, even little cores, have Address Generation Units capable
    // of performing these extensions, so we achieve more instruction level
    // parallelism by doing this shift by 2 redundantly with the mid pointer
    // calculation below.
    //
    // ldr mid_num, [search_base, mid_offset_words, uxtw #2]
    uint32_t mid_num = search_base[mid_offset_words];

    // Shrink the search window by half
    // sub search_len, search_len, search_len, lsr #1
    search_len = search_len - (search_len >> 1);
    UPB_OPT_LAUNDER(search_len);
    UPB_OPT_LAUNDER(mid_offset_words);

    // Calculate the mid pointer for the next iteration
    // add mid_ptr, search_base, mid_offset_words, uxtw #2
    const uint32_t* mid_ptr = search_base + mid_offset_words;

    // Forbids LLVM's CSE pass from attempting to merge mid_ptr and mid_num's
    // math. It sees that it can do a select before adding, rather than after;
    // but if it orders it that way it creates a longer dependency chain. We
    // need both as input/output to the same asm block to force them to be
    // present in different registers at the same time; two separate LAUNDER
    // usages could get reordered.
    UPB_OPT_LAUNDER2(mid_ptr, mid_num);

    // cmp + csel
    search_base = UPB_UNPREDICTABLE(mid_num <= number) ? mid_ptr : search_base;
  }
#undef UPB_OPT_LAUNDER
#undef UPB_OPT_LAUNDER2
  return (const struct upb_MiniTableField*)search_base;
}

UPB_API_INLINE
const struct upb_MiniTableField* upb_MiniTable_FindFieldByNumber(
    const struct upb_MiniTable* m, uint32_t number) {
  const uint32_t i = number - 1;  // 0 wraps to UINT32_MAX

  // Ideal case: index into dense fields
  if (i < m->UPB_PRIVATE(dense_below)) {
    UPB_ASSERT(m->UPB_ONLYBITS(fields)[i].UPB_ONLYBITS(number) == number);
    return &m->UPB_ONLYBITS(fields)[i];
  }

  // Early exit if the field number is out of range.
  uint32_t hi = m->UPB_ONLYBITS(field_count);
  uint32_t lo = m->UPB_PRIVATE(dense_below);
  UPB_ASSERT(hi >= lo);
  uint32_t search_len = hi - lo;
  if (search_len == 0 ||
      number > m->UPB_ONLYBITS(fields)[hi - 1].UPB_ONLYBITS(number)) {
    return NULL;
  }

  // Slow case: binary search
  const struct upb_MiniTableField* candidate;
#ifndef NDEBUG
  candidate = UPB_PRIVATE(upb_MiniTable_ArmOptimizedLowerBound)(
      m, lo, search_len, number);
  UPB_ASSERT(candidate ==
             UPB_PRIVATE(upb_MiniTable_LowerBound)(m, lo, search_len, number));
#elif UPB_ARM64_ASM
  candidate = UPB_PRIVATE(upb_MiniTable_ArmOptimizedLowerBound)(
      m, lo, search_len, number);
#else
  candidate = UPB_PRIVATE(upb_MiniTable_LowerBound)(m, lo, search_len, number);
#endif

  return candidate->UPB_ONLYBITS(number) == number ? candidate : NULL;
}

UPB_API_INLINE const struct upb_MiniTableField* upb_MiniTable_GetFieldByIndex(
    const struct upb_MiniTable* m, uint32_t i) {
  UPB_ASSERT(i < m->UPB_ONLYBITS(field_count));
  return &m->UPB_ONLYBITS(fields)[i];
}

UPB_API_INLINE const struct upb_MiniTable* upb_MiniTable_GetSubMessageTable(
    const struct upb_MiniTableField* f) {
  UPB_ASSERT(upb_MiniTableField_CType(f) == kUpb_CType_Message);
  upb_MiniTableSubInternal* sub =
      UPB_PTR_AT(f, f->UPB_PRIVATE(submsg_ofs) * kUpb_SubmsgOffsetBytes,
                 upb_MiniTableSubInternal);
  return sub->UPB_PRIVATE(submsg);
}

UPB_API_INLINE const struct upb_MiniTable* upb_MiniTable_SubMessage(
    const struct upb_MiniTableField* f) {
  if (upb_MiniTableField_CType(f) != kUpb_CType_Message) {
    return NULL;
  }
  return upb_MiniTable_GetSubMessageTable(f);
}

UPB_API_INLINE bool upb_MiniTable_FieldIsLinked(
    const struct upb_MiniTableField* f) {
  return upb_MiniTable_GetSubMessageTable(f) != NULL;
}

UPB_API_INLINE const struct upb_MiniTable* upb_MiniTable_MapEntrySubMessage(
    const struct upb_MiniTableField* f) {
  UPB_ASSERT(upb_MiniTable_FieldIsLinked(f));  // Map entries must be linked.
  UPB_ASSERT(upb_MiniTableField_IsMap(f));     // Function precondition.
  return upb_MiniTable_GetSubMessageTable(f);
}

UPB_API_INLINE const struct upb_MiniTableEnum* upb_MiniTable_GetSubEnumTable(
    const struct upb_MiniTableField* f) {
  UPB_ASSERT(upb_MiniTableField_CType(f) == kUpb_CType_Enum);
  upb_MiniTableSubInternal* sub =
      UPB_PTR_AT(f, f->UPB_PRIVATE(submsg_ofs) * kUpb_SubmsgOffsetBytes,
                 upb_MiniTableSubInternal);
  return sub->UPB_PRIVATE(subenum);
}

UPB_API_INLINE const struct upb_MiniTableField* upb_MiniTable_MapKey(
    const struct upb_MiniTable* m) {
  UPB_ASSERT(upb_MiniTable_FieldCount(m) == 2);
  const struct upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, 0);
  UPB_ASSERT(upb_MiniTableField_Number(f) == 1);
  return f;
}

UPB_API_INLINE const struct upb_MiniTableField* upb_MiniTable_MapValue(
    const struct upb_MiniTable* m) {
  UPB_ASSERT(upb_MiniTable_FieldCount(m) == 2);
  const struct upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, 1);
  UPB_ASSERT(upb_MiniTableField_Number(f) == 2);
  return f;
}

// Computes a bitmask in which the |m->required_count| lowest bits are set.
//
// Sample output:
//    RequiredMask(1) => 0b1 (0x1)
//    RequiredMask(5) => 0b11111 (0x1f)
UPB_INLINE uint64_t
UPB_PRIVATE(_upb_MiniTable_RequiredMask)(const struct upb_MiniTable* m) {
  int n = m->UPB_PRIVATE(required_count);
  UPB_ASSERT(0 < n && n <= 64);
  return (1ULL << n) - 1;
}

#ifdef UPB_TRACING_ENABLED
UPB_INLINE const char* upb_MiniTable_FullName(
    const struct upb_MiniTable* mini_table) {
  return mini_table->UPB_PRIVATE(full_name);
}
// Initializes tracing proto name from language runtimes that construct
// mini tables dynamically at runtime. The runtime is responsible for passing
// controlling lifetime of name such as storing in same arena as mini_table.
UPB_INLINE void upb_MiniTable_SetFullName(struct upb_MiniTable* mini_table,
                                          const char* full_name) {
  mini_table->UPB_PRIVATE(full_name) = full_name;
}
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_INTERNAL_MESSAGE_H_ */
