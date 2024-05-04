// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>

#include "upb/wire/batched.h"
#include "upb/wire/internal/constants.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/accessors.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_EncodeStatus status;
  jmp_buf err;
  upb_Arena* arena;
  char* buf;
  char* end;
  int options;
  int depth;
  _upb_mapsorter sorter;
} upb_BatchEncoder;

typedef struct {
  char* tag;
  char* data;
#ifndef NDEBUG
  char* tag_end;
#endif
} upb_BatchPtrs;

#define DBG(...) // fprintf(stderr, __VA_ARGS__)

typedef upb_BatchPtrs upb_EncodeBatch_Func(upb_BatchEncoder* e,
                                           upb_BatchPtrs ptrs,
                                           const upb_Message* msg,
                                           const upb_MiniTable* m,
                                           const uint16_t* fields, int count);

UPB_NORETURN static void upb_BatchEncoder_Error(upb_BatchEncoder* e,
                                                upb_EncodeStatus s) {
  UPB_ASSERT(s != kUpb_EncodeStatus_Ok);
  e->status = s;
  UPB_LONGJMP(e->err, 1);
}

UPB_NOINLINE
static uintptr_t upb_BatchEncoder_ReserveFallback(upb_BatchEncoder* e, char* ptr,
                                                  size_t size) {
  // DBG("ReserveFallback()\n");
  size_t need = ptr - e->buf + size;
  size_t old_size = e->end - e->buf;
  size_t new_size = UPB_MAX(128, old_size * 2);
  while (new_size < need) need *= 2;
  uintptr_t old_buf = (uintptr_t)e->buf;
  e->buf = upb_Arena_Realloc(e->arena, e->buf, old_size, new_size);
  if (e->buf == NULL) upb_BatchEncoder_Error(e, kUpb_EncodeStatus_OutOfMemory);
  e->end = e->buf + new_size;
  return (uintptr_t)e->buf - old_buf;
}

UPB_FORCEINLINE
char* upb_BatchEncoder_Reserve(upb_BatchEncoder* e, char* ptr, size_t size) {
  if (e->end - ptr >= size) return ptr;
  return ptr + upb_BatchEncoder_ReserveFallback(e, ptr, size);
}

UPB_FORCEINLINE
upb_BatchPtrs upb_BatchEncoder_ReserveData(upb_BatchEncoder* e,
                                           upb_BatchPtrs ptrs, int size) {
  if (e->end - ptrs.data >= size) return ptrs;
  uintptr_t diff = upb_BatchEncoder_ReserveFallback(e, ptrs.data, size);
  return (upb_BatchPtrs){.tag = ptrs.tag + diff,
                         .data = ptrs.data + diff,
#ifndef NDEBUG
                         .tag_end = ptrs.tag_end + diff,
#endif
  };
}

UPB_FORCEINLINE
upb_BatchPtrs upb_BatchEncoder_WriteData(upb_BatchEncoder* e, upb_BatchPtrs ptrs,
                                         const void* data, int size) {
  ptrs = upb_BatchEncoder_ReserveData(e, ptrs, size);
  memcpy(ptrs.data, data, size);
  ptrs.data += size;
  return ptrs;
}

UPB_FORCEINLINE upb_BatchPtrs
upb_BatchEncoder_WriteTagNoAdvance(upb_BatchEncoder* e, upb_BatchPtrs ptrs,
                                   int size, const upb_MiniTableField* f) {
  uint16_t tag;
  uint32_t num = upb_MiniTableField_Number(f);

  // DBG("WriteTagNoAdvance(), field=%p, size=%d, %d tags remaining\n", f, size,
  //     (int)(ptrs.tag_end - ptrs.tag));
  if (UPB_UNLIKELY(num >= kUpb_BigFieldNumber)) {
    tag = kUpb_BigFieldNumber << kUpb_FieldNumberShift;
    ptrs = upb_BatchEncoder_WriteData(e, ptrs, &num, 4);
  } else {
    tag = num << kUpb_FieldNumberShift;
  }

  if (UPB_UNLIKELY(size >= kUpb_LongField)) {
    tag |= kUpb_LongField;
    ptrs = upb_BatchEncoder_WriteData(e, ptrs, &size, 4);
  } else {
    tag |= size;
  }

  memcpy(ptrs.tag, &tag, sizeof(tag));
  ptrs.tag += sizeof(tag);

  return upb_BatchEncoder_ReserveData(e, ptrs, UPB_MAX(size, 8));
}

UPB_FORCEINLINE void* upb_BatchEncoder_WriteTag(upb_BatchEncoder* e,
                                                upb_BatchPtrs* ptrs, int size,
                                                const upb_MiniTableField* f) {
  *ptrs = upb_BatchEncoder_WriteTagNoAdvance(e, *ptrs, size, f);
  void* ret = ptrs->data;
  ptrs->data += size;
  return ret;
}

static upb_BatchPtrs upb_BatchEncoder_StartBatch(upb_BatchEncoder* e, char* ptr,
                                                 int size) {
  UPB_ASSERT(size > 0);
  UPB_ASSERT(size < kUpb_MaxBatch);
  const int tag_bytes = size * sizeof(uint16_t);
  const int data_bytes = size * 8;  // This is an estimate.
  const int bytes = tag_bytes + data_bytes + 1;
  ptr = upb_BatchEncoder_Reserve(e, ptr, bytes);
  *ptr++ = size;
  DBG("StartBatch(), size=%d\n", size);
  return (upb_BatchPtrs){
      .tag = ptr,
      .data = ptr + size * 2,
#ifndef NDEBUG
      .tag_end = ptr + size * 2,
#endif
  };
}

static char* upb_BatchEncoder_EndBatch(upb_BatchEncoder* e,
                                       upb_BatchPtrs ptrs) {
  DBG("EndBatch()\n");
#ifndef NDEBUG
  if (ptrs.tag != ptrs.tag_end) {
    DBG("tag_end=%p, tag=%p\n", ptrs.tag_end, ptrs.tag);
  }
  UPB_ASSERT(ptrs.tag == ptrs.tag_end);
#endif
  return ptrs.data;
}

static char* upb_BatchEncoder_StartSubMessageBatch(upb_BatchEncoder* e,
                                                   char* ptr,
                                                   const upb_MiniTableField* f,
                                                   int size) {
  UPB_ASSERT(size > 0);
  UPB_ASSERT(size < kUpb_MaxBatch);
  ptr = upb_BatchEncoder_Reserve(e, ptr, 5);
  uint8_t batch_size = size | 0x80;
  DBG("StartSubMessageBatch(), size=%d, batch_size=%d\n", size, (int)batch_size);
  UPB_ASSERT(batch_size != 0);
  *ptr++ = batch_size;
  uint32_t field_number = upb_MiniTableField_Number(f);
  memcpy(ptr, &field_number, 4);
  return ptr + 4;
}

static char* upb_BatchEncoder_EncodeMessage(upb_BatchEncoder* e, char* ptr,
                                            const upb_Message* msg,
                                            const upb_MiniTable* m);

static char* upb_BatchEncoder_EncodeSubMessage(upb_BatchEncoder* e, char* ptr,
                                               const upb_Message* msg,
                                               const upb_MiniTable* m) {
  ptr = upb_BatchEncoder_EncodeMessage(e, ptr, msg, m);
  ptr = upb_BatchEncoder_Reserve(e, ptr, 1);
  *ptr = 0;
  return ptr + 1;
}

static uint64_t upb_BatchedEncoder_PrimitiveFieldMask(
    const upb_MiniTableField* f) {
  return (1ULL << upb_BatchEncoder_PrimitiveFieldSize(f) * 8) - 1;
}

static int upb_BatchEncoder_PrimitiveDataSize(const upb_MiniTableField* f,
                                                uint64_t val) {
  uint64_t masked = val & upb_BatchedEncoder_PrimitiveFieldMask(f);
  int ret = masked == 0 ? 0 : (64 - __builtin_clzg(masked, 0) + 7) / 8;
  // DBG("For value %llu (unmasked=%llu, field size=%d, clzg=%d), returned "
  //     "size=%d\n",
  //     (unsigned long long)masked, (unsigned long long)val,
  //     (int)upb_BatchEncoder_PrimitiveFieldSize(f), __builtin_clzg(masked, 0),
  //     ret);
  return ret;
}

static upb_BatchPtrs upb_BatchedEncoder_EncodeScalarPrimitiveBatch(
    upb_BatchEncoder* e, upb_BatchPtrs ptrs, const upb_Message* msg,
    const upb_MiniTable* m, const uint16_t* fields, int count) {
  for (int i = 0; i < count; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, fields[i]);
    UPB_ASSERT(upb_MiniTableField_IsPrimitive(f));
    UPB_ASSERT(!upb_MiniTableField_IsArray(f));
    const void* field_data = UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), void);
    uint64_t val;
    memcpy(&val, field_data, 8);
    int size = upb_BatchEncoder_PrimitiveDataSize(f, val);
    DBG("- Encoding scalar primitive number=%d, value=%llu, size=%d\n",
        (int)upb_MiniTableField_Number(f),
        (unsigned long long)(val & ((1 << size * 8) - 1)), size);

    // Over-reserve and overwrite.
    ptrs = upb_BatchEncoder_WriteTagNoAdvance(e, ptrs, size, f);
    memcpy(ptrs.data, &val, 8);

    ptrs.data += size;  // Advance by correct amount.
  }
  return ptrs;
}

static upb_BatchPtrs upb_BatchedEncoder_EncodeScalarStringBatch(
    upb_BatchEncoder* e, upb_BatchPtrs ptrs, const upb_Message* msg,
    const upb_MiniTable* m, const uint16_t* fields, int count) {
  for (int i = 0; i < count; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, fields[i]);
    UPB_ASSERT(upb_MiniTableField_IsString(f));
    UPB_ASSERT(!upb_MiniTableField_IsArray(f));
    const upb_StringView v =
        *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_StringView);
    DBG("- Encoding scalar string number=%d, value=%.*s\n",
        (int)upb_MiniTableField_Number(f),
        (int)v.size, v.data);
    void* dst = upb_BatchEncoder_WriteTag(e, &ptrs, v.size, f);
    memcpy(dst, v.data, v.size);
  }
  return ptrs;
}

static upb_BatchPtrs upb_BatchedEncoder_EncodeRepeatedPrimitiveBatch(
    upb_BatchEncoder* e, upb_BatchPtrs ptrs, const upb_Message* msg,
    const upb_MiniTable* m, const uint16_t* fields, int count) {
  for (int i = 0; i < count; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, fields[i]);
    UPB_ASSERT(upb_MiniTableField_IsPrimitive(f));
    UPB_ASSERT(upb_MiniTableField_IsArray(f));
    int size = upb_BatchEncoder_PrimitiveFieldSize(f);
    const upb_Array* arr = *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Array*);
    int bytes = upb_Array_Size(arr) * size;
    UPB_ASSERT(bytes > 0);
    const char* data = upb_Array_DataPtr(arr);
    void* dst = upb_BatchEncoder_WriteTag(e, &ptrs, size, f);
    memcpy(dst, data, bytes);  // XXX: big endian
  }
  return ptrs;
}

static upb_BatchPtrs upb_BatchedEncoder_EncodeRepeatedStringBatchFrom(
    upb_BatchEncoder* e, upb_BatchPtrs ptrs, const upb_Message* msg,
    const upb_MiniTable* m, const uint16_t* fields, int count, int batch_idx,
    int remaining_batch_size) {
  for (int i = 0; i < count; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, fields[i]);
    UPB_ASSERT(upb_MiniTableField_IsString(f));
    UPB_ASSERT(upb_MiniTableField_IsArray(f));
    const upb_Array* arr = *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Array*);
    const upb_StringView* v = upb_Array_DataPtr(arr);
    const upb_StringView* end = v + upb_Array_Size(arr);
    UPB_ASSERT(v != end);
    for (; v != end; v++) {
      if (++batch_idx == kUpb_MaxBatch) {
        remaining_batch_size -= kUpb_MaxBatch;
        batch_idx -= kUpb_MaxBatch;
        char* ptr = upb_BatchEncoder_EndBatch(e, ptrs);
        ptrs = upb_BatchEncoder_StartBatch(
            e, ptr, UPB_MIN(remaining_batch_size, kUpb_MaxBatch));
      }
      void* dst = upb_BatchEncoder_WriteTag(e, &ptrs, v->size, f);
      memcpy(dst, v->data, v->size);
    }
  }
  return ptrs;
}

static upb_BatchPtrs upb_BatchedEncoder_EncodeRepeatedStringBatch(
    upb_BatchEncoder* e, upb_BatchPtrs ptrs, const upb_Message* msg,
    const upb_MiniTable* m, const uint16_t* fields, int count) {
  return upb_BatchedEncoder_EncodeRepeatedStringBatchFrom(
      e, ptrs, msg, m, fields, count, 0, kUpb_MaxBatch);
}

static char* upb_BatchEncoder_EncodeBatchIfFull(upb_BatchEncoder* e, char* ptr,
                                                const upb_Message* msg,
                                                const upb_MiniTable* m,
                                                const uint16_t* batch,
                                                int* batch_size,
                                                upb_EncodeBatch_Func* encode) {
  int size = *batch_size;
  if (UPB_LIKELY(size != kUpb_MaxBatch)) return ptr;

  upb_BatchPtrs ptrs = upb_BatchEncoder_StartBatch(e, ptr, size);
  ptrs = encode(e, ptrs, msg, m, batch, size);
  ptr = upb_BatchEncoder_EndBatch(e, ptrs);
  *batch_size = 0;
  return ptr;
}

static bool encode_IsPresentFieldBranchless(const upb_Message* msg,
                                             const upb_MiniTableField* f) {
  int16_t presence = f->presence;
  uint16_t hasbit_word_offset = (uint16_t)f->presence / 32;
  uint16_t oneof_case_offset = ~presence;
  bool is_oneof = presence < 0;
  uint16_t read_from = is_oneof ? oneof_case_offset : hasbit_word_offset;
  uint32_t val;
  memcpy(&val, UPB_PTR_AT(msg, read_from, char*), sizeof(val));
  bool hasbit_present = val & ((uint16_t)presence % 32);
  return presence == 0
             ? false
             : (is_oneof ? val == f->UPB_PRIVATE(number) : hasbit_present);
}

static bool upb_IsPresentHasbitFieldBranchless(
    const upb_Message* msg, const upb_MiniTableField* f) {
  // This is like _upb_Message_GetHasbit() except it tolerates the
  //   f->presence == 0 case (no hasbit)
  // and returns false in that case.
  // DBG("upb_IsPresentHasbitFieldBranchless(): f->presence: %d, type: %d, "
  //     "field_num: %d\n",
  //     (int)f->presence, upb_MiniTableField_Type(f),
  //     upb_MiniTableField_Number(f));
  uint16_t hasbit_byte_offset = (uint16_t)f->presence / 8;
  char val;
  memcpy(&val, UPB_PTR_AT(msg, hasbit_byte_offset, char), sizeof(val));
  return f->presence <= 0 ? false : val & (1 << ((uint16_t)f->presence % 8));
}

/*
bool upb_IsNonZeroImplicitPresenceBranchless(const upb_Message* msg,
                                             const upb_MiniTableField* f) {
  bool is_implicit_presence =
      f->presence == 0 &&
      UPB_PRIVATE(_upb_MiniTableField_Mode)(f) == kUpb_FieldMode_Scalar;
  uint64_t val;
  memcpy(&val, UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), char), 8);  // Overread.
  const widths = (8 << 24) | (
  uint64_t mask = ;
  return (val & mask) != 0;
}
*/

static int upb_NonEmptyArraySizeBranchless(const upb_Message* msg,
                                           const upb_MiniTableField* f) {
  bool is_array =
      UPB_PRIVATE(_upb_MiniTableField_Mode)(f) == kUpb_FieldMode_Array;
  uintptr_t msg_arr_ptr;
  const void* from = UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), void);
  // DBG("upb_NonEmptyArraySizeBranchless(): is_array: %d, offset: %d, from=%p, to=%p, size=%d, field_num: %d\n",
          // (int)is_array, f->UPB_PRIVATE(offset), from, &msg_arr_ptr, (int)sizeof(msg_arr_ptr), upb_MiniTableField_Number(f));
  memcpy(&msg_arr_ptr, from, sizeof(msg_arr_ptr));
  //DBG("ok!\n");
  static const upb_Array empty_array = {0, 0, 0};
  const upb_Array* arr_ptr =
      is_array && msg_arr_ptr ? (const upb_Array*)msg_arr_ptr : &empty_array;
  return upb_Array_Size(arr_ptr);
}

static char* upb_BatchEncoder_EncodeMessage(upb_BatchEncoder* e, char* ptr,
                                            const upb_Message* msg,
                                            const upb_MiniTable* m) {
  if (e->options & kUpb_EncodeOption_CheckRequired) {
    if (m->UPB_PRIVATE(required_count)) {
      if (!UPB_PRIVATE(_upb_Message_IsInitializedShallow)(msg, m)) {
        upb_BatchEncoder_Error(e, kUpb_EncodeStatus_MissingRequired);
      }
    }
  }

  /* NYI: Unknown Fields
  if ((e->options & kUpb_EncodeOption_SkipUnknown) == 0) {
    size_t unknown_size;
    const char* unknown = upb_Message_GetUnknown(msg, &unknown_size);

    if (unknown) {
      encode_bytes(e, unknown, unknown_size);
    }
  }
  */
  DBG("Encode primitives for message msg=%p, m=%p, size=%d\n", msg, m,
      m->UPB_PRIVATE(size));

  uint16_t primitive_batch[kUpb_MaxBatch/4];
  uint16_t string_batch[kUpb_MaxBatch/4];
  int primitive_batch_size = 0;
  int string_batch_size = 0;

  for (int i = 0, n = upb_MiniTable_FieldCount(m); i < n; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, i);
    bool is_present;
    if (upb_MiniTableField_IsInOneof(f)) {
      // Unusual: oneof
      is_present = upb_Message_HasBaseField(msg, f);
    } else {
      is_present = upb_IsPresentHasbitFieldBranchless(msg, f);
    }
    primitive_batch[primitive_batch_size] = (uint16_t)i;
    string_batch[string_batch_size] = (uint16_t)i;
    primitive_batch_size += is_present && upb_MiniTableField_IsPrimitive(f);
    string_batch_size += is_present && upb_MiniTableField_IsString(f);
    ptr = upb_BatchEncoder_EncodeBatchIfFull(
        e, ptr, msg, m, primitive_batch, &primitive_batch_size,
        upb_BatchedEncoder_EncodeScalarPrimitiveBatch);
    ptr = upb_BatchEncoder_EncodeBatchIfFull(
        e, ptr, msg, m, string_batch, &string_batch_size,
        upb_BatchedEncoder_EncodeScalarStringBatch);
  }

  //uint16_t implicit_batch[kUpb_MaxBatch];
  //int implicit_batch_size = 0;
  uint16_t primitive_array_batch[kUpb_MaxBatch/4];
  uint16_t string_array_batch[kUpb_MaxBatch/4];
  int primitive_array_batch_size = 0;
  int string_array_batch_size = 0;
  int string_array_count_size = 0;

  for (int i = 0, n = upb_MiniTable_FieldCount(m); i < n; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, i);
    if (UPB_PRIVATE(_upb_MiniTableField_Mode)(f) == kUpb_FieldMode_Map) {
      // Unusual: map.
      //
      // TODO: unify map/array representations so that both have size at the
      // same offset, so we can test whether they are empty or not without a
      // branch.
      continue;
    }
    primitive_array_batch[primitive_array_batch_size] = (uint16_t)i;
    string_array_batch[string_array_batch_size] = (uint16_t)i;
    int size = upb_NonEmptyArraySizeBranchless(msg, f);
    primitive_array_batch_size += size > 0 && upb_MiniTableField_IsPrimitive(f);
    string_array_batch_size += size > 0 && upb_MiniTableField_IsString(f);
    string_array_count_size += upb_MiniTableField_IsString(f) ? size : 0;
    ptr = upb_BatchEncoder_EncodeBatchIfFull(
        e, ptr, msg, m, primitive_array_batch, &primitive_array_batch_size,
        upb_BatchedEncoder_EncodeRepeatedPrimitiveBatch);
    ptr = upb_BatchEncoder_EncodeBatchIfFull(
        e, ptr, msg, m, string_array_batch, &string_array_batch_size,
        upb_BatchedEncoder_EncodeRepeatedStringBatch);
  }

  // The number of present fields that have exactly one batch element.
  int fixed_batch_size =
      primitive_batch_size + string_batch_size + primitive_array_batch_size;

  // The total number of batch elements, including string arrays which have more
  // than one element per field.
  int total_batch_size = fixed_batch_size + string_array_count_size;

  int physical_batch_size = UPB_MIN(total_batch_size, kUpb_MaxBatch);

  if (physical_batch_size > 0) {
    upb_BatchPtrs ptrs =
        upb_BatchEncoder_StartBatch(e, ptr, physical_batch_size);
    ptrs = upb_BatchedEncoder_EncodeScalarPrimitiveBatch(
        e, ptrs, msg, m, primitive_batch, primitive_batch_size);
    ptrs = upb_BatchedEncoder_EncodeScalarStringBatch(
        e, ptrs, msg, m, string_batch, string_batch_size);
    ptrs = upb_BatchedEncoder_EncodeRepeatedPrimitiveBatch(
        e, ptrs, msg, m, primitive_array_batch, primitive_array_batch_size);
    ptrs = upb_BatchedEncoder_EncodeRepeatedStringBatchFrom(
        e, ptrs, msg, m, string_array_batch, string_array_batch_size,
        fixed_batch_size, total_batch_size - physical_batch_size);
    ptr = upb_BatchEncoder_EndBatch(e, ptrs);
  }

  DBG("Done encoding primitives for message %p, size=%d\n", m,
      m->UPB_PRIVATE(size));

  for (int i = 0, n = upb_MiniTable_FieldCount(m); i < n; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, i);
    const upb_MiniTable* submsg_m = upb_MiniTable_SubMessage(m, f);
    if (!submsg_m) continue;
    if (upb_MiniTableField_IsArray(f)) {
      const upb_Array* arr = *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Array*);
      if (!arr) continue;
      int size = upb_Array_Size(arr);
      if (size == 0) continue;
      ptr = upb_BatchEncoder_StartSubMessageBatch(e, ptr, f, size);
      const upb_Message** elem = (const upb_Message**)upb_Array_DataPtr(arr);
      const upb_Message** end = elem + size;
      for (; elem != end; elem++) {
        UPB_ASSERT(*elem != NULL);
        ptr = upb_BatchEncoder_EncodeSubMessage(e, ptr, *elem, submsg_m);
      }
    } else {
      if (!upb_Message_HasBaseField(msg, f)) continue;
      const upb_Message* submsg =
          *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Message*);
      UPB_ASSERT(submsg != NULL);
      ptr = upb_BatchEncoder_StartSubMessageBatch(e, ptr, f, 1);
      ptr = upb_BatchEncoder_EncodeSubMessage(e, ptr, submsg, submsg_m);
    }
  }

#if 0
  // NYI: extensions
  if (m->UPB_PRIVATE(ext) != kUpb_ExtMode_NonExtendable) {
    /* Encode all extensions together. Unlike C++, we do not attempt to keep
     * these in field number order relative to normal fields or even to each
     * other. */
    size_t ext_count;
    const upb_Extension* ext =
        UPB_PRIVATE(_upb_Message_Getexts)(msg, &ext_count);
    if (ext_count) {
      if (e->options & kUpb_EncodeOption_Deterministic) {
        _upb_sortedmap sorted;
        _upb_mapsorter_pushexts(&e->sorter, ext, ext_count, &sorted);
        while (_upb_sortedmap_nextext(&e->sorter, &sorted, &ext)) {
          encode_ext(e, ext, m->UPB_PRIVATE(ext) == kUpb_ExtMode_IsMessageSet);
        }
        _upb_mapsorter_popmap(&e->sorter, &sorted);
      } else {
        const upb_Extension* end = ext + ext_count;
        for (; ext != end; ext++) {
          encode_ext(e, ext, m->UPB_PRIVATE(ext) == kUpb_ExtMode_IsMessageSet);
        }
      }
    }
  }
#endif
  return ptr;
}

static upb_EncodeStatus upb_Encoder_Encode(upb_BatchEncoder* const encoder,
                                           const upb_Message* const msg,
                                           const upb_MiniTable* const l,
                                           char** const buf, size_t* const size,
                                           bool prepend_len) {
  if (UPB_SETJMP(encoder->err) == 0) {
    char* ptr = upb_BatchEncoder_EncodeMessage(encoder, NULL, msg, l);
    *size = ptr - encoder->buf;
    *buf = encoder->buf;
  } else {
    UPB_ASSERT(encoder->status != kUpb_EncodeStatus_Ok);
    *buf = NULL;
    *size = 0;
  }

  _upb_mapsorter_destroy(&encoder->sorter);
  return encoder->status;
}

static upb_EncodeStatus _upb_Encode(const upb_Message* msg,
                                    const upb_MiniTable* l, int options,
                                    upb_Arena* arena, char** buf, size_t* size,
                                    bool prepend_len) {
  upb_BatchEncoder e;
  unsigned depth = (unsigned)options >> 16;

  e.status = kUpb_EncodeStatus_Ok;
  e.arena = arena;
  e.buf = NULL;
  e.end = NULL;
  e.depth = depth ? depth : kUpb_WireFormat_DefaultDepthLimit;
  e.options = options;
  _upb_mapsorter_init(&e.sorter);

  return upb_Encoder_Encode(&e, msg, l, buf, size, prepend_len);
}

upb_EncodeStatus upb_BatchedEncode(const upb_Message* msg,
                                   const upb_MiniTable* l, int options,
                                   upb_Arena* arena, char** buf, size_t* size) {
  return _upb_Encode(msg, l, options, arena, buf, size, false);
}
