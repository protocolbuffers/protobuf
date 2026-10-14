// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_EXTENSION_H_
#define UPB_MESSAGE_INTERNAL_EXTENSION_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/field_data.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/types.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/extension.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/message.h"

// Must be last.
#include "upb/port/def.inc"

// The internal representation of an extension is self-describing: it contains
// enough information that we can serialize it to binary format without needing
// to look it up in a upb_ExtensionRegistry.
//
// The value is stored in the same allocation, at offset
// kUpb_Extension_DataOffset from the start of the struct, and is sized
// according to the extension's field representation (see _upb_Extension_Size).
// It is only accessed through offset calculations, like the fields of a
// upb_Message.
typedef struct upb_Extension {
  const upb_MiniTableExtension* UPB_ONLYBITS(ext);
} upb_Extension;

// The offset of the value from the start of the upb_Extension. Rounded up to 8
// so that 8-byte values are naturally aligned on 32-bit platforms, where
// sizeof(upb_Extension) is 4 (the allocation itself is always 8-byte aligned).
enum {
  UPB_ONLYBITS(kUpb_Extension_DataOffset) =
      UPB_ALIGN_UP(sizeof(upb_Extension), 8),
};

#ifdef __cplusplus
extern "C" {
#endif

// Returns the MiniTableExtension that describes this extension. Never NULL.
UPB_API_INLINE const upb_MiniTableExtension* upb_Extension_MiniTableExtension(
    const upb_Extension* ext) {
  UPB_ASSERT(ext->UPB_ONLYBITS(ext) != NULL);
  return ext->UPB_ONLYBITS(ext);
}

// Returns the MiniTableField that describes this extension. Prefer this over
// upb_Extension_MiniTableExtension() when only field properties (number, type,
// sub-message, etc.) are needed. Never NULL.
UPB_API_INLINE const upb_MiniTableField* upb_Extension_MiniTableField(
    const upb_Extension* ext) {
  return upb_MiniTableExtension_ToField(upb_Extension_MiniTableExtension(ext));
}

// Copies the value of `ext` into `val`, which must point to storage of the
// appropriate type for the extension's field representation.
UPB_API_INLINE void upb_Extension_GetField(const upb_Extension* ext,
                                           void* val) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_IsExtension(f));
  const void* data =
      UPB_PTR_AT(ext, UPB_ONLYBITS(kUpb_Extension_DataOffset), const void);
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f, val, data);
}

// Sets the value of `ext` from `val`, which must point to a value of the
// extension's C type (e.g. `const bool*` for a bool extension, `struct
// upb_Array**` for a repeated extension).
UPB_API_INLINE void upb_Extension_SetField(upb_Extension* ext,
                                           const void* val) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_IsExtension(f));
  void* data = UPB_PTR_AT(ext, UPB_ONLYBITS(kUpb_Extension_DataOffset), void);
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f, data, val);
}

// Returns the value of this extension.
//
// For repeated/map extensions, the resulting struct upb_Array*/upb_Map* can be
// NULL if an struct upb_Array/upb_Map has not been allocated yet.
UPB_API_INLINE upb_MessageValue
upb_Extension_GetValue(const upb_Extension* ext) {
  upb_MessageValue val;
  memset(&val, 0, sizeof(val));
  upb_Extension_GetField(ext, &val);
  return val;
}

// Sets the value of this extension.
UPB_API_INLINE void upb_Extension_SetValue(upb_Extension* ext,
                                           upb_MessageValue val) {
  upb_Extension_SetField(ext, &val);
}

UPB_API_INLINE void upb_Extension_SetBool(upb_Extension* ext, bool value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Bool);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_1Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetDouble(upb_Extension* ext, double value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Double);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_8Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetFloat(upb_Extension* ext, float value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Float);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_4Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetInt32(upb_Extension* ext, int32_t value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Int32 ||
             upb_MiniTableField_CType(f) == kUpb_CType_Enum);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_4Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetInt64(upb_Extension* ext, int64_t value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Int64);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_8Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetUInt32(upb_Extension* ext,
                                            uint32_t value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_UInt32);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_4Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetUInt64(upb_Extension* ext,
                                            uint64_t value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_UInt64);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_8Byte);
  upb_Extension_SetField(ext, &value);
}

// Sets the value of a `string` or `bytes` extension. The bytes of the value
// are not copied, so it is the caller's responsibility to ensure that they
// remain valid for the lifetime of the extension.
UPB_API_INLINE void upb_Extension_SetString(upb_Extension* ext,
                                            upb_StringView value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_String ||
             upb_MiniTableField_CType(f) == kUpb_CType_Bytes);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             kUpb_FieldRep_StringView);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetMessage(upb_Extension* ext,
                                             struct upb_Message* value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Message);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetArray(upb_Extension* ext,
                                           struct upb_Array* value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_IsArray(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             kUpb_FieldRep_NativePointer);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetMap(upb_Extension* ext,
                                         struct upb_Map* value) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_IsMap(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             kUpb_FieldRep_NativePointer);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE bool upb_Extension_GetBool(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Bool);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_1Byte);
  return upb_Extension_GetValue(ext).bool_val;
}

UPB_API_INLINE double upb_Extension_GetDouble(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Double);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_8Byte);
  return upb_Extension_GetValue(ext).double_val;
}

UPB_API_INLINE float upb_Extension_GetFloat(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Float);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_4Byte);
  return upb_Extension_GetValue(ext).float_val;
}

UPB_API_INLINE int32_t upb_Extension_GetInt32(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Int32 ||
             upb_MiniTableField_CType(f) == kUpb_CType_Enum);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_4Byte);
  return upb_Extension_GetValue(ext).int32_val;
}

UPB_API_INLINE int64_t upb_Extension_GetInt64(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Int64);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_8Byte);
  return upb_Extension_GetValue(ext).int64_val;
}

UPB_API_INLINE uint32_t upb_Extension_GetUInt32(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_UInt32);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_4Byte);
  return upb_Extension_GetValue(ext).uint32_val;
}

UPB_API_INLINE uint64_t upb_Extension_GetUInt64(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_UInt64);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) == kUpb_FieldRep_8Byte);
  return upb_Extension_GetValue(ext).uint64_val;
}

// Sets the value of a `string` or `bytes` extension. The bytes of the value
// are not copied, so it is the caller's responsibility to ensure that they
// remain valid for the lifetime of the extension.
UPB_API_INLINE upb_StringView
upb_Extension_GetString(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_String ||
             upb_MiniTableField_CType(f) == kUpb_CType_Bytes);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             kUpb_FieldRep_StringView);
  return upb_Extension_GetValue(ext).str_val;
}

UPB_API_INLINE const struct upb_Message* upb_Extension_GetMessage(
    const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_CType(f) == kUpb_CType_Message);
  UPB_ASSUME(upb_MiniTableField_IsScalar(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  return upb_Extension_GetValue(ext).msg_val;
}

UPB_API_INLINE struct upb_Message* upb_Extension_GetMutableMessage(
    upb_Extension* ext) {
  return (struct upb_Message*)upb_Extension_GetMessage(ext);
}

UPB_API_INLINE const struct upb_Array* upb_Extension_GetArray(
    const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_IsArray(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             kUpb_FieldRep_NativePointer);
  return upb_Extension_GetValue(ext).array_val;
}

UPB_API_INLINE struct upb_Array* upb_Extension_GetMutableArray(
    upb_Extension* ext) {
  return (struct upb_Array*)upb_Extension_GetArray(ext);
}

UPB_API_INLINE const struct upb_Map* upb_Extension_GetMap(
    const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  UPB_ASSUME(upb_MiniTableField_IsMap(f));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableField_GetRep)(f) ==
             kUpb_FieldRep_NativePointer);
  return upb_Extension_GetValue(ext).map_val;
}

UPB_API_INLINE struct upb_Map* upb_Extension_GetMutableMap(upb_Extension* ext) {
  return (struct upb_Map*)upb_Extension_GetMap(ext);
}

UPB_NODISCARD upb_Extension* UPB_PRIVATE(
    _upb_Message_GetOrCreateExtensionWithTag)(struct upb_Message* msg,
                                              const upb_MiniTableExtension* ext,
                                              upb_Arena* arena,
                                              upb_TaggedAuxType tag);

// Adds the given extension data to the given message.
// |ext| is copied into the message instance.
// This logically replaces any previously-added extension with this number.
UPB_NODISCARD upb_Extension* UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
    struct upb_Message* msg, const upb_MiniTableExtension* ext,
    upb_Arena* arena);

// Adds the given non-canonical extension data to the given message.
// |ext| is copied into the message instance.
// This logically replaces any previously-added extension with this number.
UPB_NODISCARD upb_Extension* UPB_PRIVATE(
    _upb_Message_CreateNonCanonicalExtension)(struct upb_Message* msg,
                                              const upb_MiniTableExtension* ext,
                                              upb_Arena* arena);

// Returns an extension for a message with a given mini table,
// or NULL if no extension exists with this mini table.
const upb_Extension* UPB_PRIVATE(_upb_Message_Getext)(
    const struct upb_Message* msg, const upb_MiniTableExtension* ext);

UPB_INLINE bool UPB_PRIVATE(_upb_Extension_IsEmpty)(const upb_Extension* ext) {
  const upb_MiniTableField* f = upb_Extension_MiniTableField(ext);
  switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(f)) {
    case kUpb_FieldMode_Scalar:
      return false;
    case kUpb_FieldMode_Array:
      return upb_Array_Size(upb_Extension_GetValue(ext).array_val) == 0;
    case kUpb_FieldMode_Map:
      return _upb_Map_Size(upb_Extension_GetValue(ext).map_val) == 0;
  }
  UPB_UNREACHABLE();
}

// Returns the allocation size of an upb_Extension described by `f`, including
// the value. Rounded up to kUpb_Message_Align (which the arena would do anyway)
// so that the extension can be zeroed with _upb_Message_AlignedMemsetZero().
UPB_NODISCARD UPB_INLINE size_t
UPB_PRIVATE(_upb_Extension_Size)(const upb_MiniTableField* f) {
  UPB_ASSERT(upb_MiniTableField_IsExtension(f));
  static const uint8_t rep_sizes[] = {
      1,                       // kUpb_FieldRep_1Byte
      4,                       // kUpb_FieldRep_4Byte
      sizeof(upb_StringView),  // kUpb_FieldRep_StringView
      8,                       // kUpb_FieldRep_8Byte
  };
  return UPB_ALIGN_UP(UPB_ONLYBITS(kUpb_Extension_DataOffset) +
                          rep_sizes[UPB_PRIVATE(_upb_MiniTableField_GetRep)(f)],
                      kUpb_Message_Align);
}

// Allocates a shallow copy of `src` on `a`, or returns NULL on allocation
// failure.
UPB_NODISCARD UPB_INLINE upb_Extension* UPB_PRIVATE(_upb_Extension_Clone)(
    const upb_Extension* src, upb_Arena* a) {
  const size_t ext_size =
      UPB_PRIVATE(_upb_Extension_Size)(upb_Extension_MiniTableField(src));
  enum {
    kSmallExtSize = UPB_ALIGN_UP(UPB_ONLYBITS(kUpb_Extension_DataOffset) + 1,
                                 kUpb_Message_Align),
    kBigExtSize = UPB_ALIGN_UP(
        UPB_ONLYBITS(kUpb_Extension_DataOffset) + sizeof(upb_StringView),
        kUpb_Message_Align),
  };
  UPB_ASSUME(ext_size == kSmallExtSize || ext_size == kBigExtSize);
  char* dst = (char*)upb_Arena_Malloc(a, ext_size);
  if (UPB_UNLIKELY(!dst)) return NULL;
  // Avoid out-of-line variable length memcpy for short length
  memcpy(dst, src, kSmallExtSize);
  if (ext_size != kSmallExtSize) {
    UPB_ASSERT(ext_size == kBigExtSize);
    memcpy(dst + kSmallExtSize, UPB_PTR_AT(src, kSmallExtSize, const char),
           kBigExtSize - kSmallExtSize);
  }
  return (upb_Extension*)dst;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_EXTENSION_H_ */
