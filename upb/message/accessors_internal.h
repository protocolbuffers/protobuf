/*
 * Copyright (c) 2009-2023, Google LLC
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

#ifndef UPB_MESSAGE_ACCESSORS_INTERNAL_H_
#define UPB_MESSAGE_ACCESSORS_INTERNAL_H_

#include "upb/collections/map_internal.h"
#include "upb/message/extension_internal.h"
#include "upb/message/internal.h"
#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

#if defined(__GNUC__) && !defined(__clang__)
// GCC raises incorrect warnings in these functions.  It thinks that we are
// overrunning buffers, but we carefully write the functions in this file to
// guarantee that this is impossible.  GCC gets this wrong due it its failure
// to perform constant propagation as we expect:
//   - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108217
//   - https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108226
//
// Unfortunately this also indicates that GCC is not optimizing away the
// switch() in cases where it should be, compromising the performance.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#if __GNUC__ >= 11
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

UPB_INLINE bool _upb_MiniTableField_InOneOf(const upb_MiniTableField* field) {
  return field->presence < 0;
}

UPB_INLINE void* _upb_MiniTableField_GetPtr(upb_Message* msg,
                                            const upb_MiniTableField* field) {
  return (char*)msg + field->offset;
}

UPB_INLINE const void* _upb_MiniTableField_GetConstPtr(
    const upb_Message* msg, const upb_MiniTableField* field) {
  return (char*)msg + field->offset;
}

UPB_INLINE void _upb_Message_SetPresence(upb_Message* msg,
                                         const upb_MiniTableField* field) {
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (_upb_MiniTableField_InOneOf(field)) {
    *_upb_oneofcase_field(msg, field) = field->number;
  }
}

// LINT.IfChange(message_raw_fields)

UPB_INLINE bool _upb_MiniTable_ValueIsNonZero(const void* default_val,
                                              const upb_MiniTableField* field) {
  char zero[16] = {0};
  switch (_upb_MiniTableField_GetRep(field)) {
    case kUpb_FieldRep_1Byte:
      return memcmp(&zero, default_val, 1) != 0;
    case kUpb_FieldRep_4Byte:
      return memcmp(&zero, default_val, 4) != 0;
    case kUpb_FieldRep_8Byte:
      return memcmp(&zero, default_val, 8) != 0;
    case kUpb_FieldRep_StringView: {
      const upb_StringView* sv = (const upb_StringView*)default_val;
      return sv->size != 0;
    }
  }
  UPB_UNREACHABLE();
}

UPB_INLINE void _upb_MiniTable_CopyFieldData(void* to, const void* from,
                                             const upb_MiniTableField* field) {
  switch (_upb_MiniTableField_GetRep(field)) {
    case kUpb_FieldRep_1Byte:
      memcpy(to, from, 1);
      return;
    case kUpb_FieldRep_4Byte:
      memcpy(to, from, 4);
      return;
    case kUpb_FieldRep_8Byte:
      memcpy(to, from, 8);
      return;
    case kUpb_FieldRep_StringView: {
      memcpy(to, from, sizeof(upb_StringView));
      return;
    }
  }
  UPB_UNREACHABLE();
}

// LINT.ThenChange(//depot/google3/third_party/upb/js/impl/upb_bits/message.ts:message_raw_fields)

UPB_INLINE size_t
_upb_MiniTable_ElementSizeLg2(const upb_MiniTableField* field) {
  const unsigned char table[] = {
      0,
      3,               // kUpb_FieldType_Double = 1,
      2,               // kUpb_FieldType_Float = 2,
      3,               // kUpb_FieldType_Int64 = 3,
      3,               // kUpb_FieldType_UInt64 = 4,
      2,               // kUpb_FieldType_Int32 = 5,
      3,               // kUpb_FieldType_Fixed64 = 6,
      2,               // kUpb_FieldType_Fixed32 = 7,
      0,               // kUpb_FieldType_Bool = 8,
      UPB_SIZE(3, 4),  // kUpb_FieldType_String = 9,
      UPB_SIZE(2, 3),  // kUpb_FieldType_Group = 10,
      UPB_SIZE(2, 3),  // kUpb_FieldType_Message = 11,
      UPB_SIZE(3, 4),  // kUpb_FieldType_Bytes = 12,
      2,               // kUpb_FieldType_UInt32 = 13,
      2,               // kUpb_FieldType_Enum = 14,
      2,               // kUpb_FieldType_SFixed32 = 15,
      3,               // kUpb_FieldType_SFixed64 = 16,
      2,               // kUpb_FieldType_SInt32 = 17,
      3,               // kUpb_FieldType_SInt64 = 18,
  };
  return table[field->UPB_PRIVATE(descriptortype)];
}

// Here we define universal getter/setter functions for message fields.
// These look very branchy and inefficient, but as long as the MiniTableField
// values are known at compile time, all the branches are optimized away and
// we are left with ideal code.  This can happen either through through
// literals or UPB_ASSUME():
//
//   // Via struct literals.
//   bool FooMessage_set_bool_field(const upb_Message* msg, bool val) {
//     const upb_MiniTableField field = {1, 0, 0, /* etc... */};
//     // All value in "field" are compile-time known.
//     _upb_Message_SetNonExtensionField(msg, &field, &value);
//   }
//
//   // Via UPB_ASSUME().
//   UPB_INLINE bool upb_Message_SetBool(upb_Message* msg,
//                                       const upb_MiniTableField* field,
//                                       bool value, upb_Arena* a) {
//     UPB_ASSUME(field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Bool);
//     UPB_ASSUME(!upb_IsRepeatedOrMap(field));
//     UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
//     _upb_Message_SetField(msg, field, &value, a);
//   }
//
// As a result, we can use these universal getters/setters for *all* message
// accessors: generated code, MiniTable accessors, and reflection.  The only
// exception is the binary encoder/decoder, which need to be a bit more clever
// about how they read/write the message data, for efficiency.
//
// These functions work on both extensions and non-extensions. If the field
// of a setter is known to be a non-extension, the arena may be NULL and the
// returned bool value may be ignored since it will always succeed.

UPB_INLINE bool _upb_Message_HasExtensionField(
    const upb_Message* msg, const upb_MiniTableExtension* ext) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(&ext->field));
  return _upb_Message_Getext(msg, ext) != NULL;
}

UPB_INLINE bool _upb_Message_HasNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(field));
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if (_upb_MiniTableField_InOneOf(field)) {
    return _upb_getoneofcase_field(msg, field) == field->number;
  } else {
    return _upb_hasbit_field(msg, field);
  }
}

static UPB_FORCEINLINE void _upb_Message_GetNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field,
    const void* default_val, void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if ((_upb_MiniTableField_InOneOf(field) ||
       _upb_MiniTable_ValueIsNonZero(default_val, field)) &&
      !_upb_Message_HasNonExtensionField(msg, field)) {
    _upb_MiniTable_CopyFieldData(val, default_val, field);
    return;
  }
  _upb_MiniTable_CopyFieldData(val, _upb_MiniTableField_GetConstPtr(msg, field),
                               field);
}

UPB_INLINE void _upb_Message_GetExtensionField(
    const upb_Message* msg, const upb_MiniTableExtension* mt_ext,
    const void* default_val, void* val) {
  UPB_ASSUME(upb_MiniTableField_IsExtension(&mt_ext->field));
  const upb_Message_Extension* ext = _upb_Message_Getext(msg, mt_ext);
  if (ext) {
    _upb_MiniTable_CopyFieldData(val, &ext->data, &mt_ext->field);
  } else {
    _upb_MiniTable_CopyFieldData(val, default_val, &mt_ext->field);
  }
}

UPB_INLINE void _upb_Message_GetField(const upb_Message* msg,
                                      const upb_MiniTableField* field,
                                      const void* default_val, void* val) {
  if (upb_MiniTableField_IsExtension(field)) {
    _upb_Message_GetExtensionField(msg, (upb_MiniTableExtension*)field,
                                   default_val, val);
  } else {
    _upb_Message_GetNonExtensionField(msg, field, default_val, val);
  }
}

UPB_INLINE void _upb_Message_SetNonExtensionField(
    upb_Message* msg, const upb_MiniTableField* field, const void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  _upb_Message_SetPresence(msg, field);
  _upb_MiniTable_CopyFieldData(_upb_MiniTableField_GetPtr(msg, field), val,
                               field);
}

UPB_INLINE bool _upb_Message_SetExtensionField(
    upb_Message* msg, const upb_MiniTableExtension* mt_ext, const void* val,
    upb_Arena* a) {
  UPB_ASSERT(a);
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, mt_ext, a);
  if (!ext) return false;
  _upb_MiniTable_CopyFieldData(&ext->data, val, &mt_ext->field);
  return true;
}

UPB_INLINE bool _upb_Message_SetField(upb_Message* msg,
                                      const upb_MiniTableField* field,
                                      const void* val, upb_Arena* a) {
  if (upb_MiniTableField_IsExtension(field)) {
    const upb_MiniTableExtension* ext = (const upb_MiniTableExtension*)field;
    return _upb_Message_SetExtensionField(msg, ext, val, a);
  } else {
    _upb_Message_SetNonExtensionField(msg, field, val);
    return true;
  }
}

UPB_INLINE void _upb_Message_ClearExtensionField(
    upb_Message* msg, const upb_MiniTableExtension* ext_l) {
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  if (!in->internal) return;
  const upb_Message_Extension* base =
      UPB_PTR_AT(in->internal, in->internal->ext_begin, upb_Message_Extension);
  upb_Message_Extension* ext =
      (upb_Message_Extension*)_upb_Message_Getext(msg, ext_l);
  if (ext) {
    *ext = *base;
    in->internal->ext_begin += sizeof(upb_Message_Extension);
  }
}

UPB_INLINE void _upb_Message_ClearNonExtensionField(
    upb_Message* msg, const upb_MiniTableField* field) {
  if (field->presence > 0) {
    _upb_clearhas(msg, _upb_Message_Hasidx(field));
  } else if (_upb_MiniTableField_InOneOf(field)) {
    uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
    if (*oneof_case != field->number) return;
    *oneof_case = 0;
  }
  const char zeros[16] = {0};
  _upb_MiniTable_CopyFieldData(_upb_MiniTableField_GetPtr(msg, field), zeros,
                               field);
}

UPB_INLINE void _upb_Message_AssertMapIsUntagged(
    const upb_Message* msg, const upb_MiniTableField* field) {
  _upb_MiniTableField_CheckIsMap(field);
#ifndef NDEBUG
  upb_TaggedMessagePtr default_val = 0;
  upb_TaggedMessagePtr tagged;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &tagged);
  UPB_ASSERT(!upb_TaggedMessagePtr_IsEmpty(tagged));
#endif
}

UPB_INLINE upb_Map* _upb_Message_GetOrCreateMutableMap(
    upb_Message* msg, const upb_MiniTableField* field, size_t key_size,
    size_t val_size, upb_Arena* arena) {
  _upb_MiniTableField_CheckIsMap(field);
  _upb_Message_AssertMapIsUntagged(msg, field);
  upb_Map* map = NULL;
  upb_Map* default_map_value = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_map_value, &map);
  if (!map) {
    map = _upb_Map_New(arena, key_size, val_size);
    // Check again due to: https://godbolt.org/z/7WfaoKG1r
    _upb_MiniTableField_CheckIsMap(field);
    _upb_Message_SetNonExtensionField(msg, field, &map);
  }
  return map;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_ACCESSORS_INTERNAL_H_
