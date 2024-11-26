// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/promote.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/tagged_ptr.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/tagged_ptr.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"
#include "upb/mini_table/sub.h"
#include "upb/wire/decode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/reader.h"

// Must be last.
#include "upb/port/def.inc"

// Parses unknown data by merging into existing base_message or creating a
// new message usingg mini_table.
static upb_UnknownToMessageRet upb_MiniTable_ParseUnknownMessage(
    const char* unknown_data, size_t unknown_size,
    const upb_MiniTable* mini_table, upb_Message* base_message,
    int decode_options, upb_Arena* arena) {
  upb_UnknownToMessageRet ret;
  ret.message =
      base_message ? base_message : _upb_Message_New(mini_table, arena);
  if (!ret.message) {
    ret.status = kUpb_UnknownToMessage_OutOfMemory;
    return ret;
  }
  // Decode sub message using unknown field contents.
  const char* data = unknown_data;
  uint32_t tag;
  uint64_t message_len = 0;
  data = upb_WireReader_ReadTag(data, &tag);
  data = upb_WireReader_ReadVarint(data, &message_len);
  upb_DecodeStatus status = upb_Decode(data, message_len, ret.message,
                                       mini_table, NULL, decode_options, arena);
  if (status == kUpb_DecodeStatus_OutOfMemory) {
    ret.status = kUpb_UnknownToMessage_OutOfMemory;
  } else if (status == kUpb_DecodeStatus_Ok) {
    ret.status = kUpb_UnknownToMessage_Ok;
  } else {
    ret.status = kUpb_UnknownToMessage_ParseError;
  }
  return ret;
}

upb_GetExtension_Status upb_Message_GetOrPromoteExtension(
    upb_Message* msg, const upb_MiniTableExtension* ext_table,
    int decode_options, upb_Arena* arena, upb_MessageValue* value) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(upb_MiniTableExtension_CType(ext_table) == kUpb_CType_Message);
  const upb_Extension* extension =
      UPB_PRIVATE(_upb_Message_Getext)(msg, ext_table);
  if (extension) {
    memcpy(value, &extension->data, sizeof(upb_MessageValue));
    return kUpb_GetExtension_Ok;
  }

  // Check unknown fields, if available promote.
  int field_number = upb_MiniTableExtension_Number(ext_table);
  upb_FindUnknownRet result = upb_Message_FindUnknown(msg, field_number, 0);
  if (result.status != kUpb_FindUnknown_Ok) {
    return kUpb_GetExtension_NotPresent;
  }
  size_t len;
  size_t ofs = result.ptr - upb_Message_GetUnknown(msg, &len);
  // Decode and promote from unknown.
  const upb_MiniTable* extension_table =
      upb_MiniTableExtension_GetSubMessage(ext_table);
  upb_UnknownToMessageRet parse_result = upb_MiniTable_ParseUnknownMessage(
      result.ptr, result.len, extension_table,
      /* base_message= */ NULL, decode_options, arena);
  switch (parse_result.status) {
    case kUpb_UnknownToMessage_OutOfMemory:
      return kUpb_GetExtension_OutOfMemory;
    case kUpb_UnknownToMessage_ParseError:
      return kUpb_GetExtension_ParseError;
    case kUpb_UnknownToMessage_NotFound:
      return kUpb_GetExtension_NotPresent;
    case kUpb_UnknownToMessage_Ok:
      break;
  }
  upb_Message* extension_msg = parse_result.message;
  // Add to extensions.
  upb_Extension* ext =
      UPB_PRIVATE(_upb_Message_GetOrCreateExtension)(msg, ext_table, arena);
  if (!ext) {
    return kUpb_GetExtension_OutOfMemory;
  }
  ext->data.msg_val = extension_msg;
  value->msg_val = extension_msg;
  // Adding to extensions may have invalidated our previous pointers to unknowns
  // This second search won't be necessary once unknown iteration is pointer
  // stable, because it'll be separate storage from extensions and won't realloc
  uintptr_t iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  while (upb_Message_NextUnknown(msg, &data, &iter)) {
    if (data.size < ofs) {
      ofs -= data.size;
    } else {
      data.data += ofs;
      data.size = result.len;
      break;
    }
  }
  upb_Message_DeleteUnknown(msg, &data, &iter);
  return kUpb_GetExtension_Ok;
}

static upb_FindUnknownRet upb_FindUnknownRet_ParseError(void) {
  return (upb_FindUnknownRet){.status = kUpb_FindUnknown_ParseError};
}

upb_FindUnknownRet upb_Message_FindUnknown(const upb_Message* msg,
                                           uint32_t field_number,
                                           int depth_limit) {
  depth_limit = depth_limit ? depth_limit : 100;
  upb_FindUnknownRet ret;
  ret.iter = kUpb_Message_UnknownBegin;
  upb_StringView data;
  while (upb_Message_NextUnknown(msg, &data, &ret.iter)) {
    upb_EpsCopyInputStream stream;
    const char* ptr = data.data;
    upb_EpsCopyInputStream_Init(&stream, &ptr, data.size, true);

    while (!upb_EpsCopyInputStream_IsDone(&stream, &ptr)) {
      uint32_t tag;
      const char* unknown_begin = ptr;
      ptr = upb_WireReader_ReadTag(ptr, &tag);
      if (!ptr) return upb_FindUnknownRet_ParseError();
      if (field_number == upb_WireReader_GetFieldNumber(tag)) {
        ret.status = kUpb_FindUnknown_Ok;
        ret.ptr = upb_EpsCopyInputStream_GetAliasedPtr(&stream, unknown_begin);
        ptr = _upb_WireReader_SkipValue(ptr, tag, depth_limit, &stream);
        // Because we know that the input is a flat buffer, it is safe to
        // perform pointer arithmetic on aliased pointers.
        ret.len = upb_EpsCopyInputStream_GetAliasedPtr(&stream, ptr) - ret.ptr;
        return ret;
      }

      ptr = _upb_WireReader_SkipValue(ptr, tag, depth_limit, &stream);
      if (!ptr) return upb_FindUnknownRet_ParseError();
    }
  }
  ret.status = kUpb_FindUnknown_NotPresent;
  ret.ptr = NULL;
  ret.len = 0;
  ret.iter = kUpb_Message_UnknownBegin;
  return ret;
}

static upb_DecodeStatus upb_Message_PromoteOne(upb_TaggedMessagePtr* tagged,
                                               const upb_MiniTable* mini_table,
                                               int decode_options,
                                               upb_Arena* arena) {
  upb_Message* empty =
      UPB_PRIVATE(_upb_TaggedMessagePtr_GetEmptyMessage)(*tagged);
  size_t unknown_size;
  const char* unknown_data = upb_Message_GetUnknown(empty, &unknown_size);
  upb_Message* promoted = upb_Message_New(mini_table, arena);
  if (!promoted) return kUpb_DecodeStatus_OutOfMemory;
  upb_DecodeStatus status = upb_Decode(unknown_data, unknown_size, promoted,
                                       mini_table, NULL, decode_options, arena);
  if (status == kUpb_DecodeStatus_Ok) {
    *tagged = UPB_PRIVATE(_upb_TaggedMessagePtr_Pack)(promoted, false);
  }
  return status;
}

upb_DecodeStatus upb_Message_PromoteMessage(upb_Message* parent,
                                            const upb_MiniTable* mini_table,
                                            const upb_MiniTableField* field,
                                            int decode_options,
                                            upb_Arena* arena,
                                            upb_Message** promoted) {
  UPB_ASSERT(!upb_Message_IsFrozen(parent));
  const upb_MiniTable* sub_table =
      upb_MiniTable_GetSubMessageTable(mini_table, field);
  UPB_ASSERT(sub_table);
  upb_TaggedMessagePtr tagged =
      upb_Message_GetTaggedMessagePtr(parent, field, NULL);
  upb_DecodeStatus ret =
      upb_Message_PromoteOne(&tagged, sub_table, decode_options, arena);
  if (ret == kUpb_DecodeStatus_Ok) {
    *promoted = upb_TaggedMessagePtr_GetNonEmptyMessage(tagged);
    upb_Message_SetMessage(parent, field, *promoted);
  }
  return ret;
}

upb_DecodeStatus upb_Array_PromoteMessages(upb_Array* arr,
                                           const upb_MiniTable* mini_table,
                                           int decode_options,
                                           upb_Arena* arena) {
  void** data = upb_Array_MutableDataPtr(arr);
  size_t size = upb_Array_Size(arr);
  for (size_t i = 0; i < size; i++) {
    upb_TaggedMessagePtr tagged;
    memcpy(&tagged, &data[i], sizeof(tagged));
    if (!upb_TaggedMessagePtr_IsEmpty(tagged)) continue;
    upb_DecodeStatus status =
        upb_Message_PromoteOne(&tagged, mini_table, decode_options, arena);
    if (status != kUpb_DecodeStatus_Ok) return status;
    memcpy(&data[i], &tagged, sizeof(tagged));
  }
  return kUpb_DecodeStatus_Ok;
}

upb_DecodeStatus upb_Map_PromoteMessages(upb_Map* map,
                                         const upb_MiniTable* mini_table,
                                         int decode_options, upb_Arena* arena) {
  size_t iter = kUpb_Map_Begin;
  upb_MessageValue key, val;
  while (upb_Map_Next(map, &key, &val, &iter)) {
    if (!upb_TaggedMessagePtr_IsEmpty(val.tagged_msg_val)) continue;
    upb_DecodeStatus status = upb_Message_PromoteOne(
        &val.tagged_msg_val, mini_table, decode_options, arena);
    if (status != kUpb_DecodeStatus_Ok) return status;
    upb_Map_SetEntryValue(map, iter, val);
  }
  return kUpb_DecodeStatus_Ok;
}

////////////////////////////////////////////////////////////////////////////////
// OLD promotion functions, will be removed!
////////////////////////////////////////////////////////////////////////////////

// Warning: See TODO
upb_UnknownToMessageRet upb_MiniTable_PromoteUnknownToMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, const upb_MiniTable* sub_mini_table,
    int decode_options, upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_FindUnknownRet unknown;
  // We need to loop and merge unknowns that have matching tag field->number.
  upb_Message* message = NULL;
  // Callers should check that message is not set first before calling
  // PromotoUnknownToMessage.
  UPB_ASSERT(upb_MiniTable_GetSubMessageTable(mini_table, field) ==
             sub_mini_table);
  bool is_oneof = upb_MiniTableField_IsInOneof(field);
  if (!is_oneof || UPB_PRIVATE(_upb_Message_GetOneofCase)(msg, field) ==
                       upb_MiniTableField_Number(field)) {
    UPB_ASSERT(upb_Message_GetMessage(msg, field) == NULL);
  }
  upb_UnknownToMessageRet ret;
  ret.status = kUpb_UnknownToMessage_Ok;
  do {
    unknown =
        upb_Message_FindUnknown(msg, upb_MiniTableField_Number(field),
                                upb_DecodeOptions_GetMaxDepth(decode_options));
    switch (unknown.status) {
      case kUpb_FindUnknown_Ok: {
        const char* unknown_data = unknown.ptr;
        size_t unknown_size = unknown.len;
        ret = upb_MiniTable_ParseUnknownMessage(unknown_data, unknown_size,
                                                sub_mini_table, message,
                                                decode_options, arena);
        if (ret.status == kUpb_UnknownToMessage_Ok) {
          message = ret.message;
          upb_StringView del =
              upb_StringView_FromDataAndSize(unknown_data, unknown_size);
          upb_Message_DeleteUnknown(msg, &del, &(unknown.iter));
        }
      } break;
      case kUpb_FindUnknown_ParseError:
        ret.status = kUpb_UnknownToMessage_ParseError;
        break;
      case kUpb_FindUnknown_NotPresent:
        // If we parsed at least one unknown, we are done.
        ret.status =
            message ? kUpb_UnknownToMessage_Ok : kUpb_UnknownToMessage_NotFound;
        break;
    }
  } while (unknown.status == kUpb_FindUnknown_Ok);
  if (message) {
    if (is_oneof) {
      UPB_PRIVATE(_upb_Message_SetOneofCase)(msg, field);
    }
    upb_Message_SetMessage(msg, field, message);
    ret.message = message;
  }
  return ret;
}

// Moves repeated messages in unknowns to a upb_Array.
//
// Since the repeated field is not a scalar type we don't check for
// kUpb_LabelFlags_IsPacked.
// TODO: Optimize. Instead of converting messages one at a time,
// scan all unknown data once and compact.
upb_UnknownToMessage_Status upb_MiniTable_PromoteUnknownToMessageArray(
    upb_Message* msg, const upb_MiniTableField* field,
    const upb_MiniTable* mini_table, int decode_options, upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));

  upb_Array* repeated_messages = upb_Message_GetMutableArray(msg, field);
  // Find all unknowns with given field number and parse.
  upb_FindUnknownRet unknown;
  do {
    unknown =
        upb_Message_FindUnknown(msg, upb_MiniTableField_Number(field),
                                upb_DecodeOptions_GetMaxDepth(decode_options));
    if (unknown.status == kUpb_FindUnknown_Ok) {
      upb_UnknownToMessageRet ret = upb_MiniTable_ParseUnknownMessage(
          unknown.ptr, unknown.len, mini_table,
          /* base_message= */ NULL, decode_options, arena);
      if (ret.status == kUpb_UnknownToMessage_Ok) {
        upb_MessageValue value;
        value.msg_val = ret.message;
        // Allocate array on demand before append.
        if (!repeated_messages) {
          upb_Message_ResizeArrayUninitialized(msg, field, 0, arena);
          repeated_messages = upb_Message_GetMutableArray(msg, field);
        }
        if (!upb_Array_Append(repeated_messages, value, arena)) {
          return kUpb_UnknownToMessage_OutOfMemory;
        }
        upb_StringView del =
            upb_StringView_FromDataAndSize(unknown.ptr, unknown.len);
        upb_Message_DeleteUnknown(msg, &del, &unknown.iter);
      } else {
        return ret.status;
      }
    }
  } while (unknown.status == kUpb_FindUnknown_Ok);
  return kUpb_UnknownToMessage_Ok;
}

// Moves repeated messages in unknowns to a upb_Map.
upb_UnknownToMessage_Status upb_MiniTable_PromoteUnknownToMap(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, int decode_options, upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));

  const upb_MiniTable* map_entry_mini_table =
      upb_MiniTable_MapEntrySubMessage(mini_table, field);
  UPB_ASSERT(upb_MiniTable_FieldCount(map_entry_mini_table) == 2);
  // Find all unknowns with given field number and parse.
  upb_FindUnknownRet unknown;
  while (1) {
    unknown =
        upb_Message_FindUnknown(msg, upb_MiniTableField_Number(field),
                                upb_DecodeOptions_GetMaxDepth(decode_options));
    if (unknown.status != kUpb_FindUnknown_Ok) break;
    upb_UnknownToMessageRet ret = upb_MiniTable_ParseUnknownMessage(
        unknown.ptr, unknown.len, map_entry_mini_table,
        /* base_message= */ NULL, decode_options, arena);
    if (ret.status != kUpb_UnknownToMessage_Ok) return ret.status;
    // Allocate map on demand before append.
    upb_Map* map = upb_Message_GetOrCreateMutableMap(msg, map_entry_mini_table,
                                                     field, arena);
    upb_Message* map_entry_message = ret.message;
    bool insert_success = upb_Message_SetMapEntry(map, mini_table, field,
                                                  map_entry_message, arena);
    if (!insert_success) return kUpb_UnknownToMessage_OutOfMemory;
    upb_StringView del =
        upb_StringView_FromDataAndSize(unknown.ptr, unknown.len);
    upb_Message_DeleteUnknown(msg, &del, &unknown.iter);
  }
  return kUpb_UnknownToMessage_Ok;
}
