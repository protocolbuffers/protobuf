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

#ifndef UPB_MINI_TABLE_FIELD_INTERNAL_H_
#define UPB_MINI_TABLE_FIELD_INTERNAL_H_

#include "upb/base/descriptor_constants.h"
#include "upb/mini_table/types.h"

// Must be last.
#include "upb/port/def.inc"

// LINT.IfChange(mini_table_field_layout)

struct upb_MiniTableField {
  uint32_t number;
  uint16_t offset;
  int16_t presence;       // If >0, hasbit_index.  If <0, ~oneof_index

  // Indexes into `upb_MiniTable.subs`
  // Will be set to `kUpb_NoSub` if `descriptortype` != MESSAGE/GROUP/ENUM
  uint16_t UPB_PRIVATE(submsg_index);

  uint8_t UPB_PRIVATE(descriptortype);

  // upb_FieldMode | upb_LabelFlags | (upb_FieldRep << kUpb_FieldRep_Shift)
  uint8_t mode;
};

#define kUpb_NoSub ((uint16_t)-1)

typedef enum {
  kUpb_FieldMode_Map = 0,
  kUpb_FieldMode_Array = 1,
  kUpb_FieldMode_Scalar = 2,
} upb_FieldMode;

// Mask to isolate the upb_FieldMode from field.mode.
#define kUpb_FieldMode_Mask 3

// Extra flags on the mode field.
typedef enum {
  kUpb_LabelFlags_IsPacked = 4,
  kUpb_LabelFlags_IsExtension = 8,
  // Indicates that this descriptor type is an "alternate type":
  //   - for Int32, this indicates that the actual type is Enum (but was
  //     rewritten to Int32 because it is an open enum that requires no check).
  //   - for Bytes, this indicates that the actual type is String (but does
  //     not require any UTF-8 check).
  kUpb_LabelFlags_IsAlternate = 16,
} upb_LabelFlags;

// Note: we sort by this number when calculating layout order.
typedef enum {
  kUpb_FieldRep_1Byte = 0,
  kUpb_FieldRep_4Byte = 1,
  kUpb_FieldRep_StringView = 2,
  kUpb_FieldRep_8Byte = 3,

  kUpb_FieldRep_NativePointer =
      UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte),
  kUpb_FieldRep_Max = kUpb_FieldRep_8Byte,
} upb_FieldRep;

#define kUpb_FieldRep_Shift 6

// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/mini_table_field.ts:mini_table_field_layout)

UPB_INLINE upb_FieldRep
_upb_MiniTableField_GetRep(const upb_MiniTableField* field) {
  return (upb_FieldRep)(field->mode >> kUpb_FieldRep_Shift);
}

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE upb_FieldMode upb_FieldMode_Get(const upb_MiniTableField* field) {
  return (upb_FieldMode)(field->mode & 3);
}

UPB_INLINE void _upb_MiniTableField_CheckIsArray(
    const upb_MiniTableField* field) {
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_NativePointer);
  UPB_ASSUME(upb_FieldMode_Get(field) == kUpb_FieldMode_Array);
  UPB_ASSUME(field->presence == 0);
}

UPB_INLINE void _upb_MiniTableField_CheckIsMap(
    const upb_MiniTableField* field) {
  UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_NativePointer);
  UPB_ASSUME(upb_FieldMode_Get(field) == kUpb_FieldMode_Map);
  UPB_ASSUME(field->presence == 0);
}

UPB_INLINE bool upb_IsRepeatedOrMap(const upb_MiniTableField* field) {
  // This works because upb_FieldMode has no value 3.
  return !(field->mode & kUpb_FieldMode_Scalar);
}

UPB_INLINE bool upb_IsSubMessage(const upb_MiniTableField* field) {
  return field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Message ||
         field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Group;
}

// LINT.IfChange(presence_logic)

// Hasbit access ///////////////////////////////////////////////////////////////

UPB_INLINE size_t _upb_hasbit_ofs(size_t idx) { return idx / 8; }

UPB_INLINE char _upb_hasbit_mask(size_t idx) { return 1 << (idx % 8); }

UPB_INLINE bool _upb_hasbit(const upb_Message* msg, size_t idx) {
  return (*UPB_PTR_AT(msg, _upb_hasbit_ofs(idx), const char) &
          _upb_hasbit_mask(idx)) != 0;
}

UPB_INLINE void _upb_sethas(const upb_Message* msg, size_t idx) {
  (*UPB_PTR_AT(msg, _upb_hasbit_ofs(idx), char)) |= _upb_hasbit_mask(idx);
}

UPB_INLINE void _upb_clearhas(const upb_Message* msg, size_t idx) {
  (*UPB_PTR_AT(msg, _upb_hasbit_ofs(idx), char)) &= ~_upb_hasbit_mask(idx);
}

UPB_INLINE size_t _upb_Message_Hasidx(const upb_MiniTableField* f) {
  UPB_ASSERT(f->presence > 0);
  return f->presence;
}

UPB_INLINE bool _upb_hasbit_field(const upb_Message* msg,
                                  const upb_MiniTableField* f) {
  return _upb_hasbit(msg, _upb_Message_Hasidx(f));
}

UPB_INLINE void _upb_sethas_field(const upb_Message* msg,
                                  const upb_MiniTableField* f) {
  _upb_sethas(msg, _upb_Message_Hasidx(f));
}

// Oneof case access ///////////////////////////////////////////////////////////

UPB_INLINE size_t _upb_oneofcase_ofs(const upb_MiniTableField* f) {
  UPB_ASSERT(f->presence < 0);
  return ~(ptrdiff_t)f->presence;
}

UPB_INLINE uint32_t* _upb_oneofcase_field(upb_Message* msg,
                                          const upb_MiniTableField* f) {
  return UPB_PTR_AT(msg, _upb_oneofcase_ofs(f), uint32_t);
}

UPB_INLINE uint32_t _upb_getoneofcase_field(const upb_Message* msg,
                                            const upb_MiniTableField* f) {
  return *_upb_oneofcase_field((upb_Message*)msg, f);
}

// LINT.ThenChange(GoogleInternalName2)
// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/presence.ts:presence_logic)

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_FIELD_INTERNAL_H_ */
