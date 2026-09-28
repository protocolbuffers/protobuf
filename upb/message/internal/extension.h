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
#include "upb/message/internal/map.h"
#include "upb/message/internal/types.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/extension.h"
#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

// The internal representation of an extension is self-describing: it contains
// enough information that we can serialize it to binary format without needing
// to look it up in a upb_ExtensionRegistry.
//
// This representation allocates 16 bytes to data on 64-bit platforms.
// This is rather wasteful for scalars (in the extreme case of bool,
// it wastes 15 bytes). We accept this because we expect messages to be
// the most common extension type.
typedef struct upb_Extension {
  const upb_MiniTableExtension* ext;
  upb_MessageValue data;
} upb_Extension;

#ifdef __cplusplus
extern "C" {
#endif

// Returns the MiniTableExtension that describes this extension.
UPB_API_INLINE const upb_MiniTableExtension* upb_Extension_MiniTableExtension(
    const upb_Extension* ext) {
  return ext->ext;
}

// Returns a pointer to the raw storage for this extension's value.  The
// storage can be treated as a message with a single field at offset 0.
UPB_INLINE void* UPB_PRIVATE(_upb_Extension_MutableDataPtr)(
    upb_Extension* ext) {
  return &ext->data;
}

UPB_INLINE const void* UPB_PRIVATE(_upb_Extension_DataPtr)(
    const upb_Extension* ext) {
  return &ext->data;
}

// Returns the value of this extension.
//
// For repeated/map extensions, the resulting struct upb_Array*/upb_Map* can be
// NULL if an struct upb_Array/upb_Map has not been allocated yet.
UPB_API_INLINE upb_MessageValue
upb_Extension_GetValue(const upb_Extension* ext) {
  return ext->data;
}

// Sets the value of this extension.
UPB_API_INLINE void upb_Extension_SetValue(upb_Extension* ext,
                                           upb_MessageValue val) {
  ext->data = val;
}

// Sets the value of `ext` from `val`, which must point to a value of the
// extension's C type (e.g. `const bool*` for a bool extension, `struct
// upb_Array**` for a repeated extension).
UPB_API_INLINE void upb_Extension_SetField(upb_Extension* ext,
                                           const void* val) {
  const upb_MiniTableField* f =
      upb_MiniTableExtension_ToField(upb_Extension_MiniTableExtension(ext));
  UPB_ASSUME(upb_MiniTableField_IsExtension(f));
  switch (UPB_PRIVATE(_upb_MiniTableField_GetRep)(f)) {
    case kUpb_FieldRep_1Byte:
      memcpy(UPB_PRIVATE(_upb_Extension_MutableDataPtr)(ext), val, 1);
      return;
    case kUpb_FieldRep_4Byte:
      memcpy(UPB_PRIVATE(_upb_Extension_MutableDataPtr)(ext), val, 4);
      return;
    case kUpb_FieldRep_8Byte:
      memcpy(UPB_PRIVATE(_upb_Extension_MutableDataPtr)(ext), val, 8);
      return;
    case kUpb_FieldRep_StringView:
      memcpy(UPB_PRIVATE(_upb_Extension_MutableDataPtr)(ext), val,
             sizeof(upb_StringView));
      return;
  }
  UPB_UNREACHABLE();
}

UPB_API_INLINE void upb_Extension_SetBool(upb_Extension* ext, bool value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Bool);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_1Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetDouble(upb_Extension* ext, double value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Double);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_8Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetFloat(upb_Extension* ext, float value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Float);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_4Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetInt32(upb_Extension* ext, int32_t value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Int32 ||
             upb_MiniTableExtension_CType(e) == kUpb_CType_Enum);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_4Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetInt64(upb_Extension* ext, int64_t value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Int64);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_8Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetUInt32(upb_Extension* ext,
                                            uint32_t value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_UInt32);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_4Byte);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetUInt64(upb_Extension* ext,
                                            uint64_t value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_UInt64);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_8Byte);
  upb_Extension_SetField(ext, &value);
}

// Sets the value of a `string` or `bytes` extension. The bytes of the value
// are not copied, so it is the caller's responsibility to ensure that they
// remain valid for the lifetime of the extension.
UPB_API_INLINE void upb_Extension_SetString(upb_Extension* ext,
                                            upb_StringView value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_String ||
             upb_MiniTableExtension_CType(e) == kUpb_CType_Bytes);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_StringView);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetMessage(upb_Extension* ext,
                                             struct upb_Message* value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Message);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetArray(upb_Extension* ext,
                                           struct upb_Array* value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableField_IsArray(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_NativePointer);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE void upb_Extension_SetMap(upb_Extension* ext,
                                         struct upb_Map* value) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableField_IsMap(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_NativePointer);
  upb_Extension_SetField(ext, &value);
}

UPB_API_INLINE bool upb_Extension_GetBool(const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Bool);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_1Byte);
  return upb_Extension_GetValue(ext).bool_val;
}

UPB_API_INLINE double upb_Extension_GetDouble(const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Double);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_8Byte);
  return upb_Extension_GetValue(ext).double_val;
}

UPB_API_INLINE float upb_Extension_GetFloat(const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Float);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_4Byte);
  return upb_Extension_GetValue(ext).float_val;
}

UPB_API_INLINE int32_t upb_Extension_GetInt32(const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Int32 ||
             upb_MiniTableExtension_CType(e) == kUpb_CType_Enum);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_4Byte);
  return upb_Extension_GetValue(ext).int32_val;
}

UPB_API_INLINE int64_t upb_Extension_GetInt64(const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Int64);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_8Byte);
  return upb_Extension_GetValue(ext).int64_val;
}

UPB_API_INLINE uint32_t upb_Extension_GetUInt32(const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_UInt32);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_4Byte);
  return upb_Extension_GetValue(ext).uint32_val;
}

UPB_API_INLINE uint64_t upb_Extension_GetUInt64(const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_UInt64);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_8Byte);
  return upb_Extension_GetValue(ext).uint64_val;
}

// Sets the value of a `string` or `bytes` extension. The bytes of the value
// are not copied, so it is the caller's responsibility to ensure that they
// remain valid for the lifetime of the extension.
UPB_API_INLINE upb_StringView
upb_Extension_GetString(const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_String ||
             upb_MiniTableExtension_CType(e) == kUpb_CType_Bytes);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_StringView);
  return upb_Extension_GetValue(ext).str_val;
}

UPB_API_INLINE const struct upb_Message* upb_Extension_GetMessage(
    const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableExtension_CType(e) == kUpb_CType_Message);
  UPB_ASSUME(upb_MiniTableField_IsScalar(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             UPB_SIZE(kUpb_FieldRep_4Byte, kUpb_FieldRep_8Byte));
  return upb_Extension_GetValue(ext).msg_val;
}

UPB_API_INLINE struct upb_Message* upb_Extension_GetMutableMessage(
    upb_Extension* ext) {
  return (struct upb_Message*)upb_Extension_GetMessage(ext);
}

UPB_API_INLINE const struct upb_Array* upb_Extension_GetArray(
    const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableField_IsArray(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
             kUpb_FieldRep_NativePointer);
  return upb_Extension_GetValue(ext).array_val;
}

UPB_API_INLINE struct upb_Array* upb_Extension_GetMutableArray(
    upb_Extension* ext) {
  return (struct upb_Array*)upb_Extension_GetArray(ext);
}

UPB_API_INLINE const struct upb_Map* upb_Extension_GetMap(
    const upb_Extension* ext) {
  const upb_MiniTableExtension* e = upb_Extension_MiniTableExtension(ext);
  UPB_ASSUME(upb_MiniTableField_IsMap(upb_MiniTableExtension_ToField(e)));
  UPB_ASSUME(UPB_PRIVATE(_upb_MiniTableExtension_GetRep)(e) ==
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
  switch (
      UPB_PRIVATE(_upb_MiniTableField_Mode)(&ext->ext->UPB_PRIVATE(field))) {
    case kUpb_FieldMode_Scalar:
      return false;
    case kUpb_FieldMode_Array:
      return upb_Array_Size(ext->data.array_val) == 0;
    case kUpb_FieldMode_Map:
      return _upb_Map_Size(ext->data.map_val) == 0;
  }
  UPB_UNREACHABLE();
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_EXTENSION_H_ */
