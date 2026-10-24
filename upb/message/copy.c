// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/copy.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/hash/common.h"
#include "upb/hash/int_table.h"
#include "upb/hash/str_table.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/map.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/internal/size_log2.h"
#include "upb/mini_table/message.h"
#include "upb/wire/encode.h"
#include "upb/wire/encode_extension.h"

// Must be last.
#include "upb/port/def.inc"

static upb_StringView upb_Clone_StringView(upb_StringView str,
                                           upb_Arena* arena) {
  if (str.size == 0) {
    return upb_StringView_FromDataAndSize(NULL, 0);
  }
  void* cloned_data = upb_Arena_Malloc(arena, str.size);
  if (cloned_data == NULL) {
    return upb_StringView_FromDataAndSize(NULL, 0);
  }
  memcpy(cloned_data, str.data, str.size);
  return upb_StringView_FromDataAndSize(cloned_data, str.size);
}

static bool upb_Clone_MessageValue(void* value, upb_CType value_type,
                                   const upb_MiniTable* sub, upb_Arena* arena) {
  switch (value_type) {
    case kUpb_CType_Bool:
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return true;
    case kUpb_CType_String:
    case kUpb_CType_Bytes: {
      upb_StringView source = *(upb_StringView*)value;
      size_t size = source.size;
      void* cloned_data = upb_Arena_Malloc(arena, size);
      if (cloned_data == NULL) {
        return false;
      }
      *(upb_StringView*)value =
          upb_StringView_FromDataAndSize(cloned_data, size);
      memcpy(cloned_data, source.data, size);
      return true;
    } break;
    case kUpb_CType_Message: {
      UPB_ASSERT(sub);
      const upb_Message* source = *(upb_Message**)value;
      UPB_ASSERT(source);
      upb_Message* clone = upb_Message_DeepClone(source, sub, arena);
      *(upb_Message**)value = clone;
      return clone != NULL;
    } break;
  }
  UPB_UNREACHABLE();
}

static bool upb_Clone_MapValue(upb_value* tabval, size_t map_val_size,
                               upb_CType value_field_type,
                               const upb_MiniTable* value_sub, upb_Arena* arena,
                               upb_value* cloned_tabval) {
  upb_MessageValue val;
  _upb_map_fromvalue(*tabval, &val, map_val_size);
  if (!upb_Clone_MessageValue(&val, value_field_type, value_sub, arena)) {
    return false;
  }
  *cloned_tabval = (upb_value){0};  // Initialize
  if (!_upb_map_tovalue(&val, map_val_size, cloned_tabval, arena)) {
    return false;
  }
  return true;
}

upb_Map* upb_Map_DeepClone(const upb_Map* map, upb_CType key_type,
                           upb_CType value_type,
                           const upb_MiniTable* map_entry_table,
                           upb_Arena* arena) {
  // Read everything needed from `map` and `map_entry_table` before allocating,
  // as the compiler can't tell that the allocation doesn't modify them.
  const char key_size = map->key_size;
  const char val_size = map->val_size;
  const bool is_strtable = map->UPB_PRIVATE(is_strtable);
  const upb_MiniTableField* value_field =
      upb_MiniTable_MapValue(map_entry_table);
  const upb_MiniTable* value_sub = upb_MiniTable_SubMessage(value_field);
  upb_CType value_field_type = upb_MiniTableField_CType(value_field);

  bool is_primitive = value_field_type != kUpb_CType_Message &&
                      value_field_type != kUpb_CType_String &&
                      value_field_type != kUpb_CType_Bytes;

  upb_Map* cloned_map = upb_Arena_Malloc(arena, sizeof(upb_Map));
  if (cloned_map == NULL) {
    return NULL;
  }
  cloned_map->key_size = key_size;
  cloned_map->val_size = val_size;
  cloned_map->UPB_PRIVATE(is_frozen) = false;
  cloned_map->UPB_PRIVATE(is_strtable) = is_strtable;

  if (is_strtable) {
    if (!upb_strtable_copy(&cloned_map->t.strtable, &map->t.strtable, arena)) {
      return NULL;
    }
    if (!is_primitive) {
      intptr_t iter = UPB_STRTABLE_BEGIN;
      upb_StringView key;
      upb_value tabval;
      while (
          upb_strtable_next2(&cloned_map->t.strtable, &key, &tabval, &iter)) {
        upb_value cloned_tabval;
        if (!upb_Clone_MapValue(&tabval, val_size, value_field_type, value_sub,
                                arena, &cloned_tabval)) {
          return NULL;
        }
        upb_strtable_setentryvalue(&cloned_map->t.strtable, iter,
                                   cloned_tabval);
      }
    }
  } else {
    if (!upb_inttable_copy(&cloned_map->t.inttable, &map->t.inttable, arena)) {
      return NULL;
    }
    if (!is_primitive) {
      intptr_t iter = UPB_INTTABLE_BEGIN;
      uintptr_t key;
      upb_value tabval;
      while (upb_inttable_next(&cloned_map->t.inttable, &key, &tabval, &iter)) {
        upb_value cloned_tabval;
        if (!upb_Clone_MapValue(&tabval, val_size, value_field_type, value_sub,
                                arena, &cloned_tabval)) {
          return NULL;
        }
        upb_inttable_setentryvalue(&cloned_map->t.inttable, iter,
                                   cloned_tabval);
      }
    }
  }
  return cloned_map;
}

upb_Array* upb_Array_DeepClone(const upb_Array* array, upb_CType value_type,
                               const upb_MiniTable* sub, upb_Arena* arena) {
  const size_t size = upb_Array_Size(array);
  const int lg2 = (int)UPB_PRIVATE(_upb_Array_ElemSizeLg2)(array);
  UPB_ASSERT(size == 0 || lg2 == UPB_PRIVATE(_upb_CType_SizeLg2)(value_type));
  // The elements are read and written in place, rather than through
  // upb_Array_Get() and upb_Array_Set(), which aren't inline.
  const char* src = (const char*)upb_Array_DataPtr(array);
  upb_Array* cloned_array = UPB_PRIVATE(_upb_Array_New)(arena, size, lg2);
  if (!cloned_array) {
    return NULL;
  }
  if (!UPB_PRIVATE(_upb_Array_ResizeUninitialized)(cloned_array, size, arena)) {
    return NULL;
  }
  char* dst = (char*)upb_Array_MutableDataPtr(cloned_array);
  switch (value_type) {
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      for (size_t i = 0; i < size; ++i) {
        upb_StringView str;
        UPB_MEMCPY_INLINE(&str, src + i * sizeof(str), sizeof(str));
        void* cloned_data = upb_Arena_Malloc(arena, str.size);
        if (cloned_data == NULL) return NULL;
        memcpy(cloned_data, str.data, str.size);
        str.data = (const char*)cloned_data;
        UPB_MEMCPY_INLINE(dst + i * sizeof(str), &str, sizeof(str));
      }
      break;
    case kUpb_CType_Message:
      UPB_ASSERT(sub);
      for (size_t i = 0; i < size; ++i) {
        const upb_Message* msg;
        UPB_MEMCPY_INLINE(&msg, src + i * sizeof(msg), sizeof(msg));
        UPB_ASSERT(msg);
        upb_Message* cloned_msg = upb_Message_DeepClone(msg, sub, arena);
        if (cloned_msg == NULL) return NULL;
        UPB_MEMCPY_INLINE(dst + i * sizeof(cloned_msg), &cloned_msg,
                          sizeof(cloned_msg));
      }
      break;
    default:
      // Scalars have nothing to clone, so they are copied all at once.
      if (size != 0) memcpy(dst, src, size << lg2);
      break;
  }
  return cloned_array;
}

static bool upb_Clone_ExtensionValue(const upb_MiniTableField* field,
                                     const upb_Extension* source,
                                     upb_Extension* dest, upb_Arena* arena) {
  upb_MessageValue val = upb_Extension_GetValue(source);
  if (!upb_Clone_MessageValue(&val, upb_MiniTableField_CType(field),
                              upb_MiniTable_SubMessage(field), arena)) {
    return false;
  }
  upb_Extension_SetValue(dest, val);
  return true;
}

// Returns true if `f` is not a oneof member, or is the active member of its
// oneof in `msg`. If this returns false, the field's storage in `msg` holds the
// value of some other member of the oneof and must not be interpreted as `f`.
UPB_FORCEINLINE
bool upb_Copy_IsActiveOneofMember(const upb_Message* msg,
                                  const upb_MiniTableField* f) {
  return !upb_MiniTableField_IsInOneof(f) ||
         UPB_PRIVATE(_upb_Message_GetOneofCase)(msg, f) ==
             upb_MiniTableField_Number(f);
}

upb_Message* _upb_Message_Copy(upb_Message* dst, const upb_Message* src,
                               const upb_MiniTable* mini_table,
                               upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(dst));
  // Only copy message area skipping upb_Message_Internal.
  memcpy(dst + 1, src + 1, _upb_Message_Size(mini_table) - sizeof(upb_Message));

  // After the memcpy above, `dst` holds a shallow copy of every field of `src`,
  // including hasbits and oneof cases. All that remains is to replace pointers
  // to data owned by `src` (strings, submessages, arrays, maps) with deep
  // copies. Field storage is read and written in place in `dst` rather than
  // through the generic accessors, since the presence bits are already correct.
  const upb_MiniTableField* f = mini_table->UPB_PRIVATE(fields);
  const upb_MiniTableField* const end =
      f + upb_MiniTable_FieldCount(mini_table);
  for (; f != end; f++) {
    void* data = UPB_PRIVATE(_upb_Message_MutableDataPtr)(dst, f);
    if (upb_MiniTableField_IsScalar(f)) {
      switch (f->UPB_PRIVATE(descriptortype)) {
        case kUpb_FieldType_Message:
        case kUpb_FieldType_Group: {
          if (!upb_Copy_IsActiveOneofMember(dst, f)) break;
          const upb_Message* sub_message;
          UPB_MEMCPY_INLINE(&sub_message, data, sizeof(sub_message));
          if (sub_message == NULL) break;
          upb_Message* dst_sub_message = upb_Message_DeepClone(
              sub_message, upb_MiniTable_GetSubMessageTable(f), arena);
          if (dst_sub_message == NULL) goto err;
          UPB_MEMCPY_INLINE(data, &dst_sub_message, sizeof(dst_sub_message));
          UPB_PRIVATE(_upb_Message_SetPresence)(dst, f);
        } break;
        case kUpb_FieldType_String:
        case kUpb_FieldType_Bytes: {
          if (!upb_Copy_IsActiveOneofMember(dst, f)) break;
          upb_StringView str;
          UPB_MEMCPY_INLINE(&str, data, sizeof(str));
          if (str.size == 0) break;
          upb_StringView cloned_str = upb_Clone_StringView(str, arena);
          if (cloned_str.data == NULL) goto err;
          UPB_MEMCPY_INLINE(data, &cloned_str, sizeof(cloned_str));
          UPB_PRIVATE(_upb_Message_SetPresence)(dst, f);
        } break;
        default:
          // Scalar, already copied.
          break;
      }
    } else if (upb_MiniTableField_IsMap(f)) {
      const upb_Map* map;
      UPB_MEMCPY_INLINE(&map, data, sizeof(map));
      if (map == NULL) continue;
      const upb_MiniTable* map_entry_table =
          upb_MiniTable_MapEntrySubMessage(f);
      UPB_ASSERT(map_entry_table);
      upb_Map* cloned_map = upb_Map_DeepClone(
          map, upb_MiniTableField_CType(upb_MiniTable_MapKey(map_entry_table)),
          upb_MiniTableField_CType(upb_MiniTable_MapValue(map_entry_table)),
          map_entry_table, arena);
      if (cloned_map == NULL) goto err;
      UPB_MEMCPY_INLINE(data, &cloned_map, sizeof(cloned_map));
    } else {
      const upb_Array* array;
      UPB_MEMCPY_INLINE(&array, data, sizeof(array));
      if (array == NULL) continue;
      upb_CType ctype = upb_MiniTableField_CType(f);
      upb_Array* cloned_array = upb_Array_DeepClone(
          array, ctype,
          ctype == kUpb_CType_Message ? upb_MiniTable_GetSubMessageTable(f)
                                      : NULL,
          arena);
      if (cloned_array == NULL) goto err;
      UPB_MEMCPY_INLINE(data, &cloned_array, sizeof(cloned_array));
    }
  }
  // Clone extensions.
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(src);
  if (!in) return dst;

  for (size_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (upb_TaggedAuxPtr_IsNonCanonicalExtension(tagged_ptr)) {
      // A non-canonical extension references a `upb_MiniTableExtension` (and
      // possibly its extendee and sub-message/enum tables) that belongs to a
      // foreign schema, e.g. the source schema of a `upb_Message_Convert()`.
      // That schema is not guaranteed to outlive the clone, so we must not
      // copy the pointer. Instead, serialize the extension into the
      // destination arena and store it as unknown bytes, preserving order.
      const upb_Extension* msg_ext =
          upb_TaggedAuxPtr_NonCanonicalExtension(tagged_ptr);
      upb_StringView encoded;
      if (upb_EncodeExtension(msg_ext, arena, &encoded, /*encode_options=*/0) !=
          kUpb_EncodeStatus_Ok) {
        goto err;
      }
      if (encoded.size != 0 &&
          !UPB_PRIVATE(_upb_Message_AddUnknown)(dst, encoded.data, encoded.size,
                                                arena, kUpb_AddUnknown_Alias)) {
        goto err;
      }
    } else if (upb_TaggedAuxPtr_IsCanonicalExtension(tagged_ptr)) {
      const upb_Extension* msg_ext =
          upb_TaggedAuxPtr_CanonicalExtension(tagged_ptr);
      const upb_MiniTableField* field = upb_Extension_MiniTableField(msg_ext);
      upb_Extension* dst_ext = UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(
          dst, upb_Extension_MiniTableExtension(msg_ext), arena);
      if (!dst_ext) goto err;

      if (upb_MiniTableField_IsScalar(field)) {
        if (!upb_Clone_ExtensionValue(field, msg_ext, dst_ext, arena)) {
          goto err;
        }
      } else {
        // Canonical extensions cannot be maps.
        UPB_ASSERT(upb_MiniTableField_IsArray(field));
        const upb_Array* msg_array = upb_Extension_GetArray(msg_ext);
        UPB_ASSERT(msg_array);
        upb_Array* cloned_array =
            upb_Array_DeepClone(msg_array, upb_MiniTableField_CType(field),
                                upb_MiniTable_SubMessage(field), arena);
        if (!cloned_array) goto err;

        upb_Extension_SetArray(dst_ext, cloned_array);
      }
    } else if (upb_TaggedAuxPtr_IsUnknownStringView(tagged_ptr)) {
      // Clone an aliased or non-aliased unknown upb_StringView.
      upb_StringView* unknown = upb_TaggedPtrAux_StringViewRepr(tagged_ptr);
      // Make a copy into destination arena.
      if (!UPB_PRIVATE(_upb_Message_AddUnknown)(
              dst, unknown->data, unknown->size, arena, kUpb_AddUnknown_Copy)) {
        goto err;
      }
    }
  }
  return dst;

err:
  upb_Message_Clear(dst, mini_table);
  return NULL;
}

bool upb_Message_DeepCopy(upb_Message* dst, const upb_Message* src,
                          const upb_MiniTable* mini_table, upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(dst));
  upb_Message_Clear(dst, mini_table);
  return _upb_Message_Copy(dst, src, mini_table, arena) != NULL;
}

// Deep clones a message using the provided target arena.
//
// Returns NULL on failure.
upb_Message* upb_Message_DeepClone(const upb_Message* msg,
                                   const upb_MiniTable* m, upb_Arena* arena) {
  // _upb_Message_Copy() overwrites everything past the upb_Message header.
  upb_Message* clone = _upb_Message_NewUninit(m, arena);
  if (UPB_UNLIKELY(!clone)) return NULL;
  memset(clone, 0, sizeof(upb_Message));
  return _upb_Message_Copy(clone, msg, m, arena);
}

// Overwrites all of `dst`, which may be uninitialized, with a shallow copy of
// `src`.
static bool _upb_Message_ShallowCopy(upb_Message* dst, const upb_Message* src,
                                     const upb_MiniTable* m, upb_Arena* arena) {
  memcpy(dst, src, _upb_Message_Size(m));
  return UPB_PRIVATE(_upb_Message_CopyInternal)(dst, src, arena);
}

// Performs a shallow copy.
bool upb_Message_ShallowCopy(upb_Message* dst, const upb_Message* src,
                             const upb_MiniTable* m, upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(dst));
  return _upb_Message_ShallowCopy(dst, src, m, arena);
}

// Performs a shallow clone.
upb_Message* upb_Message_ShallowClone(const upb_Message* msg,
                                      const upb_MiniTable* m,
                                      upb_Arena* arena) {
  upb_Message* clone = _upb_Message_NewUninit(m, arena);
  if (!clone) return NULL;
  if (!_upb_Message_ShallowCopy(clone, msg, m, arena)) return NULL;
  return clone;
}
