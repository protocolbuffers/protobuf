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
  uint16_t* tag;
  char* data;
} upb_BatchPtrs;

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
  return (upb_BatchPtrs){.tag = ptrs.tag + diff, .data = ptrs.data + diff};
}

UPB_FORCEINLINE
upb_BatchPtrs upb_BatchEncoder_WriteData(upb_BatchEncoder* e, upb_BatchPtrs ptrs,
                                         const void* data, int size) {
  ptrs = upb_BatchEncoder_ReserveData(e, ptrs, size);
  memcpy(ptrs.data, data, size);
  return ptrs;
}

UPB_FORCEINLINE upb_BatchPtrs
upb_BatchEncoder_WriteTagNoAdvance(upb_BatchEncoder* e, upb_BatchPtrs ptrs,
                                   int size, const upb_MiniTableField* f) {
  uint16_t tag;
  uint32_t num = upb_MiniTableField_Number(f);

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

  *ptrs.tag++ = tag;

  return upb_BatchEncoder_ReserveData(e, ptrs, size);
}

UPB_FORCEINLINE void* upb_BatchEncoder_WriteTag(upb_BatchEncoder* e,
                                                upb_BatchPtrs* ptrs, int size,
                                                const upb_MiniTableField* f) {
  *ptrs = upb_BatchEncoder_WriteTagNoAdvance(e, *ptrs, size, f);
  void* ret = ptrs->data;
  ptrs->data += size;
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

    // Over-reserve and overwrite.
    ptrs = upb_BatchEncoder_WriteTagNoAdvance(e, ptrs, 8, f);
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
    int size = upb_BatchedEncoder_PrimitiveFieldSize(f);
    const upb_Array* arr = *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Array*);
    int bytes = upb_Array_Size(arr) * size;
    UPB_ASSERT(bytes > 0);
    const char* data = upb_Array_DataPtr(arr);
    void* dst = upb_BatchEncoder_WriteTag(e, &ptrs, size, f);
    memcpy(dst, data, bytes);  // XXX: big endian
  }
  return ptrs;
}

static upb_BatchPtrs upb_BatchedEncoder_EncodeRepeatedStringBatch(
    upb_BatchEncoder* e, upb_BatchPtrs ptrs, const upb_Message* msg,
    const upb_MiniTable* m, const uint16_t* fields, int count) {
  for (int i = 0; i < count; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, fields[i]);
    UPB_ASSERT(upb_MiniTableField_IsString(f));
    UPB_ASSERT(upb_MiniTableField_IsArray(f));
    const upb_Array* arr = *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Array*);
    const upb_StringView* v = upb_Array_DataPtr(arr);
    const upb_StringView* end = v + upb_Array_Size(arr);
    UPB_ASSERT(v != end);
    for (; v != end; v++) {
      void* dst = upb_BatchEncoder_WriteTag(e, &ptrs, v->size, f);
      memcpy(dst, v->data, v->size);
    }
  }
  return ptrs;
}

static upb_BatchPtrs upb_BatchEncoder_StartBatch(upb_BatchEncoder* e, char* ptr,
                                                 int size) {
  const int tag_bytes = size * sizeof(uint16_t);
  const int data_bytes = size * 8;  // This is an estimate.
  const int bytes = tag_bytes + data_bytes;
  uint16_t* tag_ptr = (uint16_t*)upb_BatchEncoder_Reserve(e, ptr, bytes);
  return (upb_BatchPtrs){.tag = tag_ptr, .data = (char*)(tag_ptr + size)};
}

static char* upb_BatchEncoder_EndBatch(upb_BatchEncoder* e,
                                       upb_BatchPtrs ptrs) {
  return ptrs.data;
}

static char* upb_BatchEncoder_EncodeBatchIfFull(upb_BatchEncoder* e, char* ptr,
                                                const upb_Message* msg,
                                                const upb_MiniTable* m,
                                                const uint16_t* batch,
                                                int* batch_size,
                                               upb_EncodeBatch_Func* encode) {
  int size = *batch_size;
  UPB_ASSERT(size <= kUpb_MaxBatch);
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
  uint16_t hasbit_byte_offset = (uint16_t)f->presence / 8;
  uint32_t val;
  memcpy(&val, UPB_PTR_AT(msg, hasbit_byte_offset, char*), sizeof(val));
  return f->presence <= 0 ? false : val & ((uint16_t)f->presence % 8);
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

static bool upb_IsNonEmptyArrayBranchless(const upb_Message* msg,
                                          const upb_MiniTableField* f) {
  bool is_array =
      UPB_PRIVATE(_upb_MiniTableField_Mode)(f) == kUpb_FieldMode_Array;
  const upb_Array* msg_arr_ptr;
  memcpy(&msg_arr_ptr, UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Array*),
          sizeof(msg_arr_ptr));
  static const upb_Array empty_array = {0, 0, 0};
  const upb_Array* arr_ptr = is_array ? msg_arr_ptr : &empty_array;
  return upb_Array_Size(arr_ptr) > 0;
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

  uint16_t primitive_batch[kUpb_MaxBatch/4];
  uint16_t string_batch[kUpb_MaxBatch/4];
  uint16_t submessage_batch[kUpb_MaxBatch/4];
  int primitive_batch_size = 0;
  int string_batch_size = 0;
  int submessage_batch_size = 0;

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
    submessage_batch[submessage_batch_size] = (uint16_t)i;
    primitive_batch_size += is_present && upb_MiniTableField_IsPrimitive(f);
    string_batch_size += is_present && upb_MiniTableField_IsString(f);
    submessage_batch_size += is_present && upb_MiniTableField_IsSubMessage(f);
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
  uint16_t submessage_array_batch[kUpb_MaxBatch/4];
  int primitive_array_batch_size = 0;
  int string_array_batch_size = 0;
  int submessage_array_batch_size = 0;

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
    submessage_array_batch[submessage_array_batch_size] = (uint16_t)i;
    bool array_present = upb_IsNonEmptyArrayBranchless(msg, f);
    primitive_array_batch_size += array_present && upb_MiniTableField_IsPrimitive(f);
    string_array_batch_size += array_present && upb_MiniTableField_IsString(f);
    submessage_array_batch_size += array_present &&
                               !upb_MiniTableField_IsSubMessage(f);
    ptr = upb_BatchEncoder_EncodeBatchIfFull(
        e, ptr, msg, m, primitive_array_batch, &primitive_array_batch_size,
        upb_BatchedEncoder_EncodeRepeatedPrimitiveBatch);
    ptr = upb_BatchEncoder_EncodeBatchIfFull(
        e, ptr, msg, m, string_array_batch, &string_array_batch_size,
        upb_BatchedEncoder_EncodeRepeatedStringBatch);
  }

  int batch_size = primitive_batch_size + string_batch_size +
                   primitive_array_batch_size + string_array_batch_size;

  upb_BatchPtrs ptrs = upb_BatchEncoder_StartBatch(e, ptr, batch_size);
  ptrs = upb_BatchedEncoder_EncodeScalarPrimitiveBatch(
      e, ptrs, msg, m, primitive_batch, primitive_batch_size);
  ptrs = upb_BatchedEncoder_EncodeScalarStringBatch(
      e, ptrs, msg, m, string_batch, string_batch_size);
  ptrs = upb_BatchedEncoder_EncodeRepeatedPrimitiveBatch(
      e, ptrs, msg, m, primitive_array_batch, primitive_array_batch_size);
  ptrs = upb_BatchedEncoder_EncodeRepeatedStringBatch(
      e, ptrs, msg, m, string_array_batch, string_array_batch_size);
  ptr = upb_BatchEncoder_EndBatch(e, ptrs);

  for (int i = 0; i < submessage_batch_size; i++) {
    const upb_MiniTableField* f =
        upb_MiniTable_GetFieldByIndex(m, submessage_batch[i]);
    const upb_Message* submsg =
        *UPB_PTR_AT(msg, f->UPB_PRIVATE(offset), upb_Message*);
    ptr = upb_BatchEncoder_EncodeMessage(e, ptr, submsg, f->submsg);
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
