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

#include "upb/message/copy.h"

#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/message.h"

// Must be last.
#include "upb/mini_table/common.h"
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

// Deep clones a message using the provided target arena.
//
// Returns NULL on failure.
upb_Message* upb_Message_DeepClone(const upb_Message* message,
                                   const upb_MiniTable* mini_table,
                                   upb_Arena* arena) {
  upb_Message* clone = upb_Message_New(mini_table, arena);
  upb_StringView empty_string = upb_StringView_FromDataAndSize(NULL, 0);
  // Only copy message area skipping upb_Message_Internal.
  memcpy(clone, message, mini_table->size);
  for (size_t i = 0; i < mini_table->field_count; ++i) {
    const upb_MiniTableField* field = &mini_table->fields[i];
    if (!upb_IsRepeatedOrMap(field)) {
      switch (upb_MiniTableField_CType(field)) {
        case kUpb_CType_Message: {
          const upb_Message* sub_message =
              upb_Message_GetMessage(message, field, NULL);
          if (sub_message != NULL) {
            const upb_MiniTable* sub_message_table =
                upb_MiniTable_GetSubMessageTable(mini_table, field);
            upb_Message* cloned_sub_message =
                upb_Message_DeepClone(sub_message, sub_message_table, arena);
            if (cloned_sub_message == NULL) {
              return NULL;
            }
            upb_Message_SetMessage(clone, mini_table, field,
                                   cloned_sub_message);
          }
        } break;
        case kUpb_CType_String:
        case kUpb_CType_Bytes: {
          upb_StringView str =
              upb_Message_GetString(message, field, empty_string);
          if (str.size != 0) {
            if (!upb_Message_SetString(
                    clone, field, upb_Clone_StringView(str, arena), arena)) {
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
        const upb_Map* map = upb_Message_GetMap(message, field);
        if (map != NULL) {
          if (!upb_Message_Map_DeepClone(map, mini_table, field, clone,
                                         arena)) {
            return NULL;
          }
        }
      } else {
        const upb_Array* array = upb_Message_GetArray(message, field);
        if (array != NULL) {
          if (!upb_Message_Array_DeepClone(array, mini_table, field, clone,
                                           arena)) {
            return NULL;
          }
        }
      }
    }
  }
  // Clone extensions.
  size_t ext_count;
  const upb_Message_Extension* ext = _upb_Message_Getexts(message, &ext_count);
  for (size_t i = 0; i < ext_count; ++i) {
    const upb_Message_Extension* msg_ext = &ext[i];
    upb_Message_Extension* cloned_ext =
        _upb_Message_GetOrCreateExtension(clone, msg_ext->ext, arena);
    if (!cloned_ext) {
      return NULL;
    }
    if (!upb_Clone_ExtensionValue(msg_ext->ext, msg_ext, cloned_ext, arena)) {
      return NULL;
    }
  }

  // Clone unknowns.
  size_t unknown_size = 0;
  const char* ptr = upb_Message_GetUnknown(message, &unknown_size);
  if (unknown_size != 0) {
    UPB_ASSERT(ptr);
    // Make a copy into destination arena.
    void* cloned_unknowns = upb_Arena_Malloc(arena, unknown_size);
    if (cloned_unknowns == NULL) {
      return NULL;
    }
    memcpy(cloned_unknowns, ptr, unknown_size);
    if (!_upb_Message_AddUnknown(clone, cloned_unknowns, unknown_size, arena)) {
      return NULL;
    }
  }
  return clone;
}
