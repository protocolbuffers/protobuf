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

#include "upb/message/promote.h"

#include "upb/collections/array.h"
#include "upb/collections/array_internal.h"
#include "upb/collections/map.h"
#include "upb/message/accessors.h"
#include "upb/message/message.h"
#include "upb/mini_table/field_internal.h"
#include "upb/wire/common.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"
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

upb_GetExtension_Status upb_MiniTable_GetOrPromoteExtension(
    upb_Message* msg, const upb_MiniTableExtension* ext_table,
    int decode_options, upb_Arena* arena,
    const upb_Message_Extension** extension) {
  UPB_ASSERT(upb_MiniTableField_CType(&ext_table->field) == kUpb_CType_Message);
  *extension = _upb_Message_Getext(msg, ext_table);
  if (*extension) {
    return kUpb_GetExtension_Ok;
  }

  // Check unknown fields, if available promote.
  int field_number = ext_table->field.number;
  upb_FindUnknownRet result = upb_MiniTable_FindUnknown(
      msg, field_number, kUpb_WireFormat_DefaultDepthLimit);
  if (result.status != kUpb_FindUnknown_Ok) {
    return kUpb_GetExtension_NotPresent;
  }
  size_t len;
  size_t ofs = result.ptr - upb_Message_GetUnknown(msg, &len);
  // Decode and promote from unknown.
  const upb_MiniTable* extension_table = ext_table->sub.submsg;
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
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, ext_table, arena);
  if (!ext) {
    return kUpb_GetExtension_OutOfMemory;
  }
  memcpy(&ext->data, &extension_msg, sizeof(extension_msg));
  *extension = ext;
  const char* delete_ptr = upb_Message_GetUnknown(msg, &len) + ofs;
  upb_Message_DeleteUnknown(msg, delete_ptr, result.len);
  return kUpb_GetExtension_Ok;
}

upb_GetExtensionAsBytes_Status upb_MiniTable_GetExtensionAsBytes(
    const upb_Message* msg, const upb_MiniTableExtension* ext_table,
    int encode_options, upb_Arena* arena, const char** extension_data,
    size_t* len) {
  const upb_Message_Extension* msg_ext = _upb_Message_Getext(msg, ext_table);
  UPB_ASSERT(upb_MiniTableField_CType(&ext_table->field) == kUpb_CType_Message);
  if (msg_ext) {
    upb_EncodeStatus status =
        upb_Encode(msg_ext->data.ptr, msg_ext->ext->sub.submsg, encode_options,
                   arena, (char**)extension_data, len);
    if (status != kUpb_EncodeStatus_Ok) {
      return kUpb_GetExtensionAsBytes_EncodeError;
    }
    return kUpb_GetExtensionAsBytes_Ok;
  }
  int field_number = ext_table->field.number;
  upb_FindUnknownRet result = upb_MiniTable_FindUnknown(
      msg, field_number, upb_DecodeOptions_GetMaxDepth(encode_options));
  if (result.status != kUpb_FindUnknown_Ok) {
    return kUpb_GetExtensionAsBytes_NotPresent;
  }
  const char* data = result.ptr;
  uint32_t tag;
  uint64_t message_len = 0;
  data = upb_WireReader_ReadTag(data, &tag);
  data = upb_WireReader_ReadVarint(data, &message_len);
  *extension_data = data;
  *len = message_len;
  return kUpb_GetExtensionAsBytes_Ok;
}

static upb_FindUnknownRet upb_FindUnknownRet_ParseError(void) {
  return (upb_FindUnknownRet){.status = kUpb_FindUnknown_ParseError};
}

upb_FindUnknownRet upb_MiniTable_FindUnknown(const upb_Message* msg,
                                             uint32_t field_number,
                                             int depth_limit) {
  size_t size;
  upb_FindUnknownRet ret;

  const char* ptr = upb_Message_GetUnknown(msg, &size);
  upb_EpsCopyInputStream stream;
  upb_EpsCopyInputStream_Init(&stream, &ptr, size, true);

  while (!upb_EpsCopyInputStream_IsDone(&stream, &ptr)) {
    uint32_t tag;
    const char* unknown_begin = ptr;
    ptr = upb_WireReader_ReadTag(ptr, &tag);
    if (!ptr) return upb_FindUnknownRet_ParseError();
    if (field_number == upb_WireReader_GetFieldNumber(tag)) {
      ret.status = kUpb_FindUnknown_Ok;
      ret.ptr = upb_EpsCopyInputStream_GetAliasedPtr(&stream, unknown_begin);
      ptr = _upb_WireReader_SkipValue(ptr, tag, depth_limit, &stream);
      // Because we know that the input is a flat buffer, it is safe to perform
      // pointer arithmetic on aliased pointers.
      ret.len = upb_EpsCopyInputStream_GetAliasedPtr(&stream, ptr) - ret.ptr;
      return ret;
    }

    ptr = _upb_WireReader_SkipValue(ptr, tag, depth_limit, &stream);
    if (!ptr) return upb_FindUnknownRet_ParseError();
  }
  ret.status = kUpb_FindUnknown_NotPresent;
  ret.ptr = NULL;
  ret.len = 0;
  return ret;
}

// Warning: See TODO(b/267655898)
upb_UnknownToMessageRet upb_MiniTable_PromoteUnknownToMessage(
    upb_Message* msg, const upb_MiniTable* mini_table,
    const upb_MiniTableField* field, const upb_MiniTable* sub_mini_table,
    int decode_options, upb_Arena* arena) {
  upb_FindUnknownRet unknown;
  // We need to loop and merge unknowns that have matching tag field->number.
  upb_Message* message = NULL;
  // Callers should check that message is not set first before calling
  // PromotoUnknownToMessage.
  UPB_ASSERT(upb_MiniTable_GetSubMessageTable(mini_table, field) ==
             sub_mini_table);
  bool is_oneof = _upb_MiniTableField_InOneOf(field);
  if (!is_oneof || _upb_getoneofcase_field(msg, field) == field->number) {
    UPB_ASSERT(upb_Message_GetMessage(msg, field, NULL) == NULL);
  }
  upb_UnknownToMessageRet ret;
  ret.status = kUpb_UnknownToMessage_Ok;
  do {
    unknown = upb_MiniTable_FindUnknown(
        msg, field->number, upb_DecodeOptions_GetMaxDepth(decode_options));
    switch (unknown.status) {
      case kUpb_FindUnknown_Ok: {
        const char* unknown_data = unknown.ptr;
        size_t unknown_size = unknown.len;
        ret = upb_MiniTable_ParseUnknownMessage(unknown_data, unknown_size,
                                                sub_mini_table, message,
                                                decode_options, arena);
        if (ret.status == kUpb_UnknownToMessage_Ok) {
          message = ret.message;
          upb_Message_DeleteUnknown(msg, unknown_data, unknown_size);
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
      *_upb_oneofcase_field(msg, field) = field->number;
    }
    upb_Message_SetMessage(msg, mini_table, field, message);
    ret.message = message;
  }
  return ret;
}

// Moves repeated messages in unknowns to a upb_Array.
//
// Since the repeated field is not a scalar type we don't check for
// kUpb_LabelFlags_IsPacked.
// TODO(b/251007554): Optimize. Instead of converting messages one at a time,
// scan all unknown data once and compact.
upb_UnknownToMessage_Status upb_MiniTable_PromoteUnknownToMessageArray(
    upb_Message* msg, const upb_MiniTableField* field,
    const upb_MiniTable* mini_table, int decode_options, upb_Arena* arena) {
  upb_Array* repeated_messages = upb_Message_GetMutableArray(msg, field);
  // Find all unknowns with given field number and parse.
  upb_FindUnknownRet unknown;
  do {
    unknown = upb_MiniTable_FindUnknown(
        msg, field->number, upb_DecodeOptions_GetMaxDepth(decode_options));
    if (unknown.status == kUpb_FindUnknown_Ok) {
      upb_UnknownToMessageRet ret = upb_MiniTable_ParseUnknownMessage(
          unknown.ptr, unknown.len, mini_table,
          /* base_message= */ NULL, decode_options, arena);
      if (ret.status == kUpb_UnknownToMessage_Ok) {
        upb_MessageValue value;
        value.msg_val = ret.message;
        // Allocate array on demand before append.
        if (!repeated_messages) {
          upb_Message_ResizeArray(msg, field, 0, arena);
          repeated_messages = upb_Message_GetMutableArray(msg, field);
        }
        if (!upb_Array_Append(repeated_messages, value, arena)) {
          return kUpb_UnknownToMessage_OutOfMemory;
        }
        upb_Message_DeleteUnknown(msg, unknown.ptr, unknown.len);
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
  const upb_MiniTable* map_entry_mini_table =
      mini_table->subs[field->UPB_PRIVATE(submsg_index)].submsg;
  UPB_ASSERT(map_entry_mini_table);
  UPB_ASSERT(map_entry_mini_table);
  UPB_ASSERT(map_entry_mini_table->field_count == 2);
  UPB_ASSERT(upb_FieldMode_Get(field) == kUpb_FieldMode_Map);
  // Find all unknowns with given field number and parse.
  upb_FindUnknownRet unknown;
  while (1) {
    unknown = upb_MiniTable_FindUnknown(
        msg, field->number, upb_DecodeOptions_GetMaxDepth(decode_options));
    if (unknown.status != kUpb_FindUnknown_Ok) break;
    upb_UnknownToMessageRet ret = upb_MiniTable_ParseUnknownMessage(
        unknown.ptr, unknown.len, map_entry_mini_table,
        /* base_message= */ NULL, decode_options, arena);
    if (ret.status != kUpb_UnknownToMessage_Ok) return ret.status;
    // Allocate map on demand before append.
    upb_Map* map = upb_Message_GetOrCreateMutableMap(msg, map_entry_mini_table,
                                                     field, arena);
    upb_Message* map_entry_message = ret.message;
    upb_MapInsertStatus insert_status = upb_Message_InsertMapEntry(
        map, mini_table, field, map_entry_message, arena);
    if (insert_status == kUpb_MapInsertStatus_OutOfMemory) {
      return kUpb_UnknownToMessage_OutOfMemory;
    }
    UPB_ASSUME(insert_status == kUpb_MapInsertStatus_Inserted ||
               insert_status == kUpb_MapInsertStatus_Replaced);
    upb_Message_DeleteUnknown(msg, unknown.ptr, unknown.len);
  }
  return kUpb_UnknownToMessage_Ok;
}
