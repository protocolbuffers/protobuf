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

#include "upb/base/internal/endian.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/tagged_ptr.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/message.h"

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

UPB_INLINE bool UPB_PRIVATE(_upb_Message_GetHasbit)(
    const struct upb_Message* msg, const upb_MiniTableField* f) {
  const size_t offset = UPB_PRIVATE(_upb_MiniTableField_HasbitOffset)(f);
  const char mask = UPB_PRIVATE(_upb_MiniTableField_HasbitMask)(f);

  return (*UPB_PTR_AT(msg, offset, const char) & mask) != 0;
}

UPB_INLINE void UPB_PRIVATE(_upb_Message_SetHasbit)(
    const struct upb_Message* msg, const upb_MiniTableField* f) {
  const size_t offset = UPB_PRIVATE(_upb_MiniTableField_HasbitOffset)(f);
  const char mask = UPB_PRIVATE(_upb_MiniTableField_HasbitMask)(f);

  (*UPB_PTR_AT(msg, offset, char)) |= mask;
}

UPB_INLINE void UPB_PRIVATE(_upb_Message_ClearHasbit)(
    const struct upb_Message* msg, const upb_MiniTableField* f) {
  const size_t offset = UPB_PRIVATE(_upb_MiniTableField_HasbitOffset)(f);
  const char mask = UPB_PRIVATE(_upb_MiniTableField_HasbitMask)(f);

  (*UPB_PTR_AT(msg, offset, char)) &= ~mask;
}

// Oneof case access ///////////////////////////////////////////////////////////

UPB_INLINE uint32_t* UPB_PRIVATE(_upb_Message_OneofCasePtr)(
    struct upb_Message* msg, const upb_MiniTableField* f) {
  return UPB_PTR_AT(msg, UPB_PRIVATE(_upb_MiniTableField_OneofOffset)(f),
                    uint32_t);
}

UPB_INLINE uint32_t UPB_PRIVATE(_upb_Message_GetOneofCase)(
    const struct upb_Message* msg, const upb_MiniTableField* f) {
  const uint32_t* ptr =
      UPB_PRIVATE(_upb_Message_OneofCasePtr)((struct upb_Message*)msg, f);

  return *ptr;
}

UPB_INLINE void UPB_PRIVATE(_upb_Message_SetOneofCase)(
    struct upb_Message* msg, const upb_MiniTableField* f) {
  uint32_t* ptr = UPB_PRIVATE(_upb_Message_OneofCasePtr)(msg, f);

  *ptr = upb_MiniTableField_Number(f);
}

// Returns true if the given field is the current oneof case.
// Does nothing if it is not the current oneof case.
UPB_INLINE bool UPB_PRIVATE(_upb_Message_ClearOneofCase)(
    struct upb_Message* msg, const upb_MiniTableField* f) {
  uint32_t* ptr = UPB_PRIVATE(_upb_Message_OneofCasePtr)(msg, f);

  if (*ptr != upb_MiniTableField_Number(f)) return false;
  *ptr = 0;
  return true;
}

// LINT.ThenChange(GoogleInternalName2)

// Returns false if the message is missing any of its required fields.
UPB_INLINE bool UPB_PRIVATE(_upb_Message_IsInitializedShallow)(
    const struct upb_Message* msg, const upb_MiniTable* m) {
  uint64_t bits;
  memcpy(&bits, msg + 1, sizeof(bits));
  bits = upb_BigEndian64(bits);
  return (UPB_PRIVATE(_upb_MiniTable_RequiredMask)(m) & ~bits) == 0;
}

UPB_INLINE void* UPB_PRIVATE(_upb_Message_MutableDataPtr)(
    struct upb_Message* msg, const upb_MiniTableField* f) {
  return (char*)msg + f->UPB_ONLYBITS(offset);
}

UPB_INLINE const void* UPB_PRIVATE(_upb_Message_DataPtr)(
    const struct upb_Message* msg, const upb_MiniTableField* f) {
  return (const char*)msg + f->UPB_ONLYBITS(offset);
}

UPB_INLINE void UPB_PRIVATE(_upb_Message_SetPresence)(
    struct upb_Message* msg, const upb_MiniTableField* f) {
  if (UPB_PRIVATE(_upb_MiniTableField_HasHasbit)(f)) {
    UPB_PRIVATE(_upb_Message_SetHasbit)(msg, f);
  } else if (upb_MiniTableField_IsInOneof(f)) {
    UPB_PRIVATE(_upb_Message_SetOneofCase)(msg, f);
  }
}

UPB_INLINE void UPB_PRIVATE(_upb_MiniTableField_DataCopy)(
    const upb_MiniTableField* f, void* to, const void* from) {
  switch (UPB_PRIVATE(_upb_MiniTableField_GetRep)(f)) {
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

UPB_INLINE bool UPB_PRIVATE(_upb_MiniTableField_DataEquals)(
    const upb_MiniTableField* f, const void* a, const void* b) {
  switch (UPB_PRIVATE(_upb_MiniTableField_GetRep)(f)) {
    case kUpb_FieldRep_1Byte:
      return memcmp(a, b, 1) == 0;
    case kUpb_FieldRep_4Byte:
      return memcmp(a, b, 4) == 0;
    case kUpb_FieldRep_8Byte:
      return memcmp(a, b, 8) == 0;
    case kUpb_FieldRep_StringView: {
      const upb_StringView sa = *(const upb_StringView*)a;
      const upb_StringView sb = *(const upb_StringView*)b;
      return upb_StringView_IsEqual(sa, sb);
    }
  }
  UPB_UNREACHABLE();
}

UPB_INLINE void UPB_PRIVATE(_upb_MiniTableField_DataClear)(
    const upb_MiniTableField* f, void* val) {
  const char zero[16] = {0};
  return UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f, val, zero);
}

UPB_INLINE bool UPB_PRIVATE(_upb_MiniTableField_DataIsZero)(
    const upb_MiniTableField* f, const void* val) {
  const char zero[16] = {0};
  return UPB_PRIVATE(_upb_MiniTableField_DataEquals)(f, val, zero);
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
//     UPB_ASSUME(upb_MiniTableField_IsScalar(field));
//     UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(field) ==
//                kUpb_FieldRep_1Byte);
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

UPB_INLINE bool UPB_PRIVATE(_upb_Message_HasBaseField)(
    const struct upb_Message* msg, const upb_MiniTableField* field) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(field));
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if (upb_MiniTableField_IsInOneof(field)) {
    return UPB_PRIVATE(_upb_Message_GetOneofCase)(msg, field) ==
           upb_MiniTableField_Number(field);
  } else {
    return UPB_PRIVATE(_upb_Message_GetHasbit)(msg, field);
  }
}

UPB_INLINE bool UPB_PRIVATE(_upb_Message_HasExtension)(
    const struct upb_Message* msg, const upb_MiniTableExtension* ext) {
  UPB_ASSERT(upb_MiniTableField_HasPresence(&ext->UPB_PRIVATE(field)));
  return UPB_PRIVATE(_upb_Message_Getext)(msg, ext) != NULL;
}

static UPB_FORCEINLINE void _upb_Message_GetNonExtensionField(
    const struct upb_Message* msg, const upb_MiniTableField* field,
    const void* default_val, void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  if ((upb_MiniTableField_IsInOneof(field) ||
       !UPB_PRIVATE(_upb_MiniTableField_DataIsZero)(field, default_val)) &&
      !UPB_PRIVATE(_upb_Message_HasBaseField)(msg, field)) {
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(field, val, default_val);
    return;
  }
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)
  (field, val, UPB_PRIVATE(_upb_Message_DataPtr)(msg, field));
}

UPB_INLINE void _upb_Message_GetExtensionField(
    const struct upb_Message* msg, const upb_MiniTableExtension* mt_ext,
    const void* default_val, void* val) {
  const struct upb_Extension* ext =
      UPB_PRIVATE(_upb_Message_Getext)(msg, mt_ext);
  const upb_MiniTableField* f = &mt_ext->UPB_PRIVATE(field);
  UPB_ASSUME(upb_MiniTableField_IsExtension(f));

  if (ext) {
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f, val, &ext->data);
  } else {
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f, val, default_val);
  }
}

UPB_INLINE void _upb_Message_SetNonExtensionField(
    struct upb_Message* msg, const upb_MiniTableField* field, const void* val) {
  UPB_ASSUME(!upb_MiniTableField_IsExtension(field));
  UPB_PRIVATE(_upb_Message_SetPresence)(msg, field);
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)
  (field, UPB_PRIVATE(_upb_Message_MutableDataPtr)(msg, field), val);
}

UPB_INLINE bool _upb_Message_SetExtensionField(
    struct upb_Message* msg, const upb_MiniTableExtension* mt_ext,
    const void* val, upb_Arena* a) {
  UPB_ASSERT(a);
  struct upb_Extension* ext =
      UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(msg, mt_ext, a);
  if (!ext) return false;
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)
  (&mt_ext->UPB_PRIVATE(field), &ext->data, val);
  return true;
}

UPB_INLINE void UPB_PRIVATE(_upb_Message_Clear)(struct upb_Message* msg,
                                                const upb_MiniTable* m) {
  memset(msg, 0, m->UPB_PRIVATE(size));
}

UPB_INLINE void UPB_PRIVATE(_upb_Message_ClearBaseField)(
    struct upb_Message* msg, const upb_MiniTableField* f) {
  if (UPB_PRIVATE(_upb_MiniTableField_HasHasbit)(f)) {
    UPB_PRIVATE(_upb_Message_ClearHasbit)(msg, f);
  } else if (upb_MiniTableField_IsInOneof(f)) {
    uint32_t* ptr = UPB_PRIVATE(_upb_Message_OneofCasePtr)(msg, f);
    if (*ptr != upb_MiniTableField_Number(f)) return;
    *ptr = 0;
  }
  const char zeros[16] = {0};
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)
  (f, UPB_PRIVATE(_upb_Message_MutableDataPtr)(msg, f), zeros);
}

UPB_INLINE void UPB_PRIVATE(_upb_Message_ClearExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* e) {
  upb_Message_Internal* in = msg->internal;
  if (!in) return;
  const struct upb_Extension* base =
      UPB_PTR_AT(in, in->ext_begin, struct upb_Extension);
  struct upb_Extension* ext =
      (struct upb_Extension*)UPB_PRIVATE(_upb_Message_Getext)(msg, e);
  if (ext) {
    *ext = *base;
    in->ext_begin += sizeof(struct upb_Extension);
  }
}

UPB_INLINE void _upb_Message_AssertMapIsUntagged(
    const struct upb_Message* msg, const upb_MiniTableField* field) {
  UPB_UNUSED(msg);
  UPB_PRIVATE(_upb_MiniTableField_CheckIsMap)(field);
#ifndef NDEBUG
  uintptr_t default_val = 0;
  uintptr_t tagged;
  _upb_Message_GetNonExtensionField(msg, field, &default_val, &tagged);
  UPB_ASSERT(!UPB_PRIVATE(_upb_TaggedMessagePtr_IsEmpty)(tagged));
#endif
}

UPB_INLINE struct upb_Map* _upb_Message_GetOrCreateMutableMap(
    struct upb_Message* msg, const upb_MiniTableField* field, size_t key_size,
    size_t val_size, upb_Arena* arena) {
  UPB_PRIVATE(_upb_MiniTableField_CheckIsMap)(field);
  _upb_Message_AssertMapIsUntagged(msg, field);
  struct upb_Map* map = NULL;
  struct upb_Map* default_map_value = NULL;
  _upb_Message_GetNonExtensionField(msg, field, &default_map_value, &map);
  if (!map) {
    map = _upb_Map_New(arena, key_size, val_size);
    // Check again due to: https://godbolt.org/z/7WfaoKG1r
    UPB_PRIVATE(_upb_MiniTableField_CheckIsMap)(field);
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
