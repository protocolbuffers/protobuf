// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/copy.h"

#include <stdbool.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

static bool upb_MessageField_IsMap(const upb_MiniTableField* field) {
  return upb_FieldMode_Get(field) == kUpb_FieldMode_Map;
}

static upb_StringView upb_Clone_StringView(upb_StringView str,
                                           upb_Arena* arena) {
  if (str.size == 0) {
    return upb_StringView_FromDataAndSize(NULL, 0);
  }
  void* cloned_data = upb_Arena_Malloc(arena, str.size);
  upb_StringView cloned_str =
      upb_StringView_FromDataAndSize(cloned_data, str.size);
  memcpy(cloned_data, str.data, str.size);
  return cloned_str;
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
      int size = source.size;
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
      const upb_TaggedMessagePtr source = *(upb_TaggedMessagePtr*)value;
      bool is_empty = upb_TaggedMessagePtr_IsEmpty(source);
      if (is_empty) sub = &_kUpb_MiniTable_Empty;
      UPB_ASSERT(source);
      upb_Message* clone = upb_Message_DeepClone(
          _upb_TaggedMessagePtr_GetMessage(source), sub, arena);
      *(upb_TaggedMessagePtr*)value =
          _upb_TaggedMessagePtr_Pack(clone, is_empty);
      return clone != NULL;
    } break;
  }
  UPB_UNREACHABLE();
}

upb_Map* upb_Map_DeepClone(const upb_Map* map, upb_CType key_type,
                           upb_CType value_type,
                           const upb_MiniTable* map_entry_table,
                           upb_Arena* arena) {
  upb_Map* cloned_map = _upb_Map_New(arena, map->key_size, map->val_size);
  if (cloned_map == NULL) {
    return NULL;
  }
  upb_MessageValue key, val;
  size_t iter = kUpb_Map_Begin;
  while (upb_Map_Next(map, &key, &val, &iter)) {
    const upb_MiniTableField* value_field = &map_entry_table->fields[1];
    const upb_MiniTable* value_sub =
        (value_field->UPB_PRIVATE(submsg_index) != kUpb_NoSub)
            ? upb_MiniTable_GetSubMessageTable(map_entry_table, value_field)
            : NULL;
    upb_CType value_field_type = upb_MiniTableField_CType(value_field);
    if (!upb_Clone_MessageValue(&val, value_field_type, value_sub, arena)) {
      return NULL;
    }
    if (upb_Map_Insert(cloned_map, key, val, arena) ==
        kUpb_MapInsertStatus_OutOfMemory) {
      return NULL;
    }
  }
  return cloned_map;
}

static upb_Map* upb_Message_Map_DeepClone(const upb_Map* map,
                                          const upb_MiniTable* mini_table,
                                          const upb_MiniTableField* field,
                                          upb_Message* clone,
                                          upb_Arena* arena) {
  const upb_MiniTable* map_entry_table =
      mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSERT(map_entry_table);

  const upb_MiniTableField* key_field = &map_entry_table->fields[0];
  const upb_MiniTableField* value_field = &map_entry_table->fields[1];

  upb_Map* cloned_map = upb_Map_DeepClone(
      map, upb_MiniTableField_CType(key_field),
      upb_MiniTableField_CType(value_field), map_entry_table, arena);
  if (!cloned_map) {
    return NULL;
  }
  _upb_Message_SetNonExtensionField(clone, field, &cloned_map);
  return cloned_map;
}

upb_Array* upb_Array_DeepClone(const upb_Array* array, upb_CType value_type,
                               const upb_MiniTable* sub, upb_Arena* arena) {
  size_t size = array->size;
  upb_Array* cloned_array =
      _upb_Array_New(arena, size, _upb_Array_CTypeSizeLg2(value_type));
  if (!cloned_array) {
    return NULL;
  }
  if (!_upb_Array_ResizeUninitialized(cloned_array, size, arena)) {
    return NULL;
  }
  for (size_t i = 0; i < size; ++i) {
    upb_MessageValue val = upb_Array_Get(array, i);
    if (!upb_Clone_MessageValue(&val, value_type, sub, arena)) {
      return false;
    }
    upb_Array_Set(cloned_array, i, val);
  }
  return cloned_array;
}

static bool upb_Message_Array_DeepClone(const upb_Array* array,
                                        const upb_MiniTable* mini_table,
                                        const upb_MiniTableField* field,
                                        upb_Message* clone, upb_Arena* arena) {
  _upb_MiniTableField_CheckIsArray(field);
  upb_Array* cloned_array = upb_Array_DeepClone(
      array, upb_MiniTableField_CType(field),
      upb_MiniTableField_CType(field) == kUpb_CType_Message &&
              field->UPB_PRIVATE(submsg_index) != kUpb_NoSub
          ? upb_MiniTable_GetSubMessageTable(mini_table, field)
          : NULL,
      arena);

  // Clear out upb_Array* due to parent memcpy.
  _upb_Message_SetNonExtensionField(clone, field, &cloned_array);
  return true;
}

static bool upb_Clone_ExtensionValue(
    const upb_MiniTableExtension* mini_table_ext,
    const upb_Message_Extension* source, upb_Message_Extension* dest,
    upb_Arena* arena) {
  dest->data = source->data;
  return upb_Clone_MessageValue(
      &dest->data, upb_MiniTableField_CType(&mini_table_ext->field),
      mini_table_ext->sub.submsg, arena);
}

upb_Message* _upb_Message_Copy(upb_Message* dst, const upb_Message* src,
                               const upb_MiniTable* mini_table,
                               upb_Arena* arena) {
  upb_StringView empty_string = upb_StringView_FromDataAndSize(NULL, 0);
  // Only copy message area skipping upb_Message_Internal.
  memcpy(dst, src, mini_table->size);
  for (size_t i = 0; i < mini_table->field_count; ++i) {
    const upb_MiniTableField* field = &mini_table->fields[i];
    if (!upb_IsRepeatedOrMap(field)) {
      switch (upb_MiniTableField_CType(field)) {
        case kUpb_CType_Message: {
          upb_TaggedMessagePtr tagged =
              upb_Message_GetTaggedMessagePtr(src, field, NULL);
          const upb_Message* sub_message =
              _upb_TaggedMessagePtr_GetMessage(tagged);
          if (sub_message != NULL) {
            // If the message is currently in an unlinked, "empty" state we keep
            // it that way, because we don't want to deal with decode options,
            // decode status, or possible parse failure here.
            bool is_empty = upb_TaggedMessagePtr_IsEmpty(tagged);
            const upb_MiniTable* sub_message_table =
                is_empty ? &_kUpb_MiniTable_Empty
                         : upb_MiniTable_GetSubMessageTable(mini_table, field);
            upb_Message* dst_sub_message =
                upb_Message_DeepClone(sub_message, sub_message_table, arena);
            if (dst_sub_message == NULL) {
              return NULL;
            }
            _upb_Message_SetTaggedMessagePtr(
                dst, mini_table, field,
                _upb_TaggedMessagePtr_Pack(dst_sub_message, is_empty));
          }
        } break;
        case kUpb_CType_String:
        case kUpb_CType_Bytes: {
          upb_StringView str = upb_Message_GetString(src, field, empty_string);
          if (str.size != 0) {
            if (!upb_Message_SetString(
                    dst, field, upb_Clone_StringView(str, arena), arena)) {
              return NULL;
            }
          }
        } break;
        default:
          // Scalar, already copied.
          break;
      }
    } else {
      if (upb_MessageField_IsMap(field)) {
        const upb_Map* map = upb_Message_GetMap(src, field);
        if (map != NULL) {
          if (!upb_Message_Map_DeepClone(map, mini_table, field, dst, arena)) {
            return NULL;
          }
        }
      } else {
        const upb_Array* array = upb_Message_GetArray(src, field);
        if (array != NULL) {
          if (!upb_Message_Array_DeepClone(array, mini_table, field, dst,
                                           arena)) {
            return NULL;
          }
        }
      }
    }
  }
  // Clone extensions.
  size_t ext_count;
  const upb_Message_Extension* ext = _upb_Message_Getexts(src, &ext_count);
  for (size_t i = 0; i < ext_count; ++i) {
    const upb_Message_Extension* msg_ext = &ext[i];
    const upb_MiniTableField* field = &msg_ext->ext->field;
    upb_Message_Extension* dst_ext =
        _upb_Message_GetOrCreateExtension(dst, msg_ext->ext, arena);
    if (!dst_ext) return NULL;
    if (!upb_IsRepeatedOrMap(field)) {
      if (!upb_Clone_ExtensionValue(msg_ext->ext, msg_ext, dst_ext, arena)) {
        return NULL;
      }
    } else {
      upb_Array* msg_array = (upb_Array*)msg_ext->data.ptr;
      UPB_ASSERT(msg_array);
      upb_Array* cloned_array =
          upb_Array_DeepClone(msg_array, upb_MiniTableField_CType(field),
                              msg_ext->ext->sub.submsg, arena);
      if (!cloned_array) {
        return NULL;
      }
      dst_ext->data.ptr = (void*)cloned_array;
    }
  }

  // Clone unknowns.
  size_t unknown_size = 0;
  const char* ptr = upb_Message_GetUnknown(src, &unknown_size);
  if (unknown_size != 0) {
    UPB_ASSERT(ptr);
    // Make a copy into destination arena.
    if (!_upb_Message_AddUnknown(dst, ptr, unknown_size, arena)) {
      return NULL;
    }
  }
  return dst;
}

bool upb_Message_DeepCopy(upb_Message* dst, const upb_Message* src,
                          const upb_MiniTable* mini_table, upb_Arena* arena) {
  upb_Message_Clear(dst, mini_table);
  return _upb_Message_Copy(dst, src, mini_table, arena) != NULL;
}

// Deep clones a message using the provided target arena.
//
// Returns NULL on failure.
upb_Message* upb_Message_DeepClone(const upb_Message* message,
                                   const upb_MiniTable* mini_table,
                                   upb_Arena* arena) {
  upb_Message* clone = upb_Message_New(mini_table, arena);
  return _upb_Message_Copy(clone, message, mini_table, arena);
}
