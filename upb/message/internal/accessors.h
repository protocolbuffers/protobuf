// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_ACCESSORS_H_
#define UPB_MESSAGE_INTERNAL_ACCESSORS_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/tagged_ptr.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
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

UPB_INLINE size_t _upb_Hasbit_Offset(size_t index) { return index / 8; }

UPB_INLINE char _upb_Hasbit_Mask(size_t index) { return 1 << (index % 8); }

UPB_INLINE size_t _upb_Hasbit_Index(const upb_MiniTableField* f) {
  UPB_ASSERT(f->presence > 0);
  return f->presence;
}

UPB_INLINE bool _upb_Message_GetHasbitByIndex(const upb_Message* msg,
                                              size_t index) {
  const size_t offset = _upb_Hasbit_Offset(index);
  const char mask = _upb_Hasbit_Mask(index);
  return (*UPB_PTR_AT(msg, offset, const char) & mask) != 0;
}

UPB_INLINE void _upb_Message_SetHasbitByIndex(const upb_Message* msg,
                                              size_t index) {
  const size_t offset = _upb_Hasbit_Offset(index);
  const char mask = _upb_Hasbit_Mask(index);
  (*UPB_PTR_AT(msg, offset, char)) |= mask;
}

UPB_INLINE void _upb_Message_ClearHasbitByIndex(const upb_Message* msg,
                                                size_t index) {
  const size_t offset = _upb_Hasbit_Offset(index);
  const char mask = _upb_Hasbit_Mask(index);
  (*UPB_PTR_AT(msg, offset, char)) &= ~mask;
}

UPB_INLINE bool _upb_Message_GetHasbitByField(const upb_Message* msg,
                                              const upb_MiniTableField* f) {
  return _upb_Message_GetHasbitByIndex(msg, _upb_Hasbit_Index(f));
}

UPB_INLINE void _upb_Message_SetHasbitByField(const upb_Message* msg,
                                              const upb_MiniTableField* f) {
  _upb_Message_SetHasbitByIndex(msg, _upb_Hasbit_Index(f));
}

UPB_INLINE void _upb_Message_ClearHasbitByField(const upb_Message* msg,
                                                const upb_MiniTableField* f) {
  _upb_Message_ClearHasbitByIndex(msg, _upb_Hasbit_Index(f));
}

// Oneof case access ///////////////////////////////////////////////////////////

UPB_INLINE size_t _upb_MiniTableField_OneofOffset(const upb_MiniTableField* f) {
  UPB_ASSERT(f->presence < 0);
  return ~(ptrdiff_t)f->presence;
}

UPB_INLINE uint32_t* _upb_Message_OneofCasePtr(upb_Message* msg,
                                               const upb_MiniTableField* f) {
  return UPB_PTR_AT(msg, _upb_MiniTableField_OneofOffset(f), uint32_t);
}

UPB_INLINE uint32_t _upb_Message_GetOneofCase(const upb_Message* msg,
                                              const upb_MiniTableField* f) {
  return *_upb_Message_OneofCasePtr((upb_Message*)msg, f);
}

UPB_INLINE void _upb_Message_SetOneofCase(upb_Message* msg,
                                          const upb_MiniTableField* f) {
  *_upb_Message_OneofCasePtr(msg, f) = f->number;
}

// TODO: implement _upb_Message_ClearOneofCase()

// LINT.ThenChange(GoogleInternalName2)

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
    _upb_Message_SetHasbitByField(msg, field);
  } else if (upb_MiniTableField_IsInOneof(field)) {
    _upb_Message_SetOneofCase(msg, field);
  }
}

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
//     UPB_ASSUME(!upb_MiniTableField_IsRepeatedOrMap(field));
//     UPB_ASSUME(_upb_MiniTableField_GetRep(field) == kUpb_FieldRep_1Byte);
//     upb_Message_SetField(msg, field, &value, a);
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
  if (upb_MiniTableField_IsInOneof(field)) {
    return _upb_Message_GetOneofCase(msg, field) == field->number;
  } else {
    return _upb_Message_GetHasbitByField(msg, field);
  }
}

static UPB_FORCEINLINE void _upb_Message_GetNonExtensionField(
    const upb_Message* msg, const upb_MiniTableField* field,
    const void* default_val, void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if ((upb_MiniTableField_IsInOneof(field) ||
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

// Gets a mutable Array, Map or Message field.
// NOTE: For repeated/map fields, the resulting upb_Array*/upb_Map* can
// be NULL if a upb_Array/upb_Map has not been allocated yet. Array/map
// fields do not have presence, so this is semantically identical to a
// pointer to an empty array/map, and must be treated the same for all
// semantic purposes.
//
// For message fields, the pointer is guaranteed to be NULL iff the field
// is unset (as message fields do have presence).
UPB_INLINE upb_MutableMessageValue _upb_Message_GetMutableField(
    const upb_Message* msg, const upb_MiniTableField* field) {
  UPB_ASSUME(upb_MiniTableField_IsRepeatedOrMap(field) ||
             upb_MiniTableField_IsSubMessage(field));

  upb_MutableMessageValue default_val;
  default_val.msg = NULL;

  upb_MutableMessageValue ret;
  if (upb_MiniTableField_IsExtension(field)) {
    _upb_Message_GetExtensionField(msg, (upb_MiniTableExtension*)field,
                                   &default_val, &ret);
  } else {
    _upb_Message_GetNonExtensionField(msg, field, &default_val, &ret);
  }
  return ret;
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
    _upb_Message_ClearHasbitByField(msg, field);
  } else if (upb_MiniTableField_IsInOneof(field)) {
    uint32_t* ptr = _upb_Message_OneofCasePtr(msg, field);
    if (*ptr != field->number) return;
    *ptr = 0;
  }
  const char zeros[16] = {0};
  _upb_MiniTable_CopyFieldData(_upb_MiniTableField_GetPtr(msg, field), zeros,
                               field);
}

UPB_INLINE void _upb_Message_AssertMapIsUntagged(
    const upb_Message* msg, const upb_MiniTableField* field) {
  UPB_UNUSED(msg);
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

#endif  // UPB_MESSAGE_INTERNAL_ACCESSORS_H_
