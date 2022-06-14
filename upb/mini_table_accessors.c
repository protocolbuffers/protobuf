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

#include "upb/mini_table_accessors.h"

#include "upb/mini_table.h"
#include "upb/msg_internal.h"

// Must be last.
#include "upb/port_def.inc"

size_t upb_MiniTable_Field_GetSize(const upb_MiniTable_Field* f) {
  static unsigned char sizes[] = {
      0,                      /* 0 */
      8,                      /* kUpb_FieldType_Double */
      4,                      /* kUpb_FieldType_Float */
      8,                      /* kUpb_FieldType_Int64 */
      8,                      /* kUpb_FieldType_UInt64 */
      4,                      /* kUpb_FieldType_Int32 */
      8,                      /* kUpb_FieldType_Fixed64 */
      4,                      /* kUpb_FieldType_Fixed32 */
      1,                      /* kUpb_FieldType_Bool */
      sizeof(upb_StringView), /* kUpb_FieldType_String */
      sizeof(void*),          /* kUpb_FieldType_Group */
      sizeof(void*),          /* kUpb_FieldType_Message */
      sizeof(upb_StringView), /* kUpb_FieldType_Bytes */
      4,                      /* kUpb_FieldType_UInt32 */
      4,                      /* kUpb_FieldType_Enum */
      4,                      /* kUpb_FieldType_SFixed32 */
      8,                      /* kUpb_FieldType_SFixed64 */
      4,                      /* kUpb_FieldType_SInt32 */
      8,                      /* kUpb_FieldType_SInt64 */
  };
  return upb_IsRepeatedOrMap(f) ? sizeof(void*) : sizes[f->descriptortype];
}

// Maps descriptor type to elem_size_lg2.
int upb_MiniTable_Field_CTypeLg2Size(const upb_MiniTable_Field* f) {
  static const uint8_t sizes[] = {
      -1,             /* invalid descriptor type */
      3,              /* DOUBLE */
      2,              /* FLOAT */
      3,              /* INT64 */
      3,              /* UINT64 */
      2,              /* INT32 */
      3,              /* FIXED64 */
      2,              /* FIXED32 */
      0,              /* BOOL */
      UPB_SIZE(3, 4), /* STRING */
      UPB_SIZE(2, 3), /* GROUP */
      UPB_SIZE(2, 3), /* MESSAGE */
      UPB_SIZE(3, 4), /* BYTES */
      2,              /* UINT32 */
      2,              /* ENUM */
      2,              /* SFIXED32 */
      3,              /* SFIXED64 */
      2,              /* SINT32 */
      3,              /* SINT64 */
  };
  return sizes[f->descriptortype];
}

bool upb_MiniTable_HasField(const upb_Message* msg,
                            const upb_MiniTable_Field* field) {
  if (_upb_MiniTable_Field_InOneOf(field)) {
    return _upb_getoneofcase_field(msg, field) == field->number;
  } else if (field->presence > 0) {
    return _upb_hasbit_field(msg, field);
  } else {
    UPB_ASSERT(field->descriptortype == kUpb_FieldType_Message ||
               field->descriptortype == kUpb_FieldType_Group);
    return upb_MiniTable_GetMessage(msg, field) != NULL;
  }
}

void upb_MiniTable_ClearField(upb_Message* msg,
                              const upb_MiniTable_Field* field) {
  char* mem = UPB_PTR_AT(msg, field->offset, char);
  if (field->presence > 0) {
    _upb_clearhas_field(msg, field);
  } else if (_upb_MiniTable_Field_InOneOf(field)) {
    uint32_t* oneof_case = _upb_oneofcase_field(msg, field);
    if (*oneof_case != field->number) return;
    *oneof_case = 0;
  }
  memset(mem, 0, upb_MiniTable_Field_GetSize(field));
}

void* upb_MiniTable_ResizeArray(upb_Message* msg,
                                const upb_MiniTable_Field* field, size_t len,
                                upb_Arena* arena) {
  return _upb_Array_Resize_accessor2(
      msg, field->offset, len, upb_MiniTable_Field_CTypeLg2Size(field), arena);
}

typedef struct {
  const char* ptr;
  uint64_t val;
} decode_vret;

UPB_NOINLINE
static decode_vret decode_longvarint64(const char* ptr, uint64_t val) {
  decode_vret ret = {NULL, 0};
  uint64_t byte;
  int i;
  for (i = 1; i < 10; i++) {
    byte = (uint8_t)ptr[i];
    val += (byte - 1) << (i * 7);
    if (!(byte & 0x80)) {
      ret.ptr = ptr + i + 1;
      ret.val = val;
      return ret;
    }
  }
  return ret;
}

UPB_FORCEINLINE
static const char* decode_varint64(const char* ptr, uint64_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = byte;
    return ptr + 1;
  } else {
    decode_vret res = decode_longvarint64(ptr, byte);
    if (!res.ptr) return NULL;
    *val = res.val;
    return res.ptr;
  }
}

UPB_FORCEINLINE
static const char* decode_tag(const char* ptr, uint32_t* val) {
  uint64_t byte = (uint8_t)*ptr;
  if (UPB_LIKELY((byte & 0x80) == 0)) {
    *val = (uint32_t)byte;
    return ptr + 1;
  } else {
    const char* start = ptr;
    decode_vret res = decode_longvarint64(ptr, byte);
    if (!res.ptr || res.ptr - start > 5 || res.val > UINT32_MAX) {
      return NULL;  // Malformed.
    }
    *val = (uint32_t)res.val;
    return res.ptr;
  }
}

typedef enum {
  kUpb_FindUnknown_Ok,
  kUpb_FindUnknown_NotPresent,
  kUpb_FindUnknown_ParseError,
} upb_FindUnknown_Status;

typedef struct {
  upb_FindUnknown_Status status;
  const char* ptr;
  size_t len;
} find_unknown_ret;

static find_unknown_ret UnknownFieldSet_FindField(const upb_Message* msg,
                                                  int field_number);

upb_GetExtension_Status upb_MiniTable_GetOrPromoteExtension(
    upb_Message* msg, const upb_MiniTable_Extension* ext_table,
    int decode_options, upb_Arena* arena,
    const upb_Message_Extension** extension) {
  UPB_ASSERT(ext_table->field.descriptortype == kUpb_FieldType_Message);
  *extension = _upb_Message_Getext(msg, ext_table);
  if (*extension) {
    return kUpb_GetExtension_Ok;
  }

  // Check unknown fields, if available promote.
  int field_number = ext_table->field.number;
  find_unknown_ret result = UnknownFieldSet_FindField(msg, field_number);
  if (result.status != kUpb_FindUnknown_Ok) {
    UPB_ASSERT(result.status != kUpb_GetExtension_ParseError);
    return kUpb_GetExtension_NotPresent;
  }
  // Decode and promote from unknown.
  const upb_MiniTable* extension_table = ext_table->sub.submsg;
  upb_Message* extension_msg = _upb_Message_New(extension_table, arena);
  if (!extension_msg) {
    return kUpb_GetExtension_OutOfMemory;
  }
  const char* data = result.ptr;
  uint32_t tag;
  uint64_t message_len;
  data = decode_tag(data, &tag);
  data = decode_varint64(data, &message_len);
  upb_DecodeStatus status =
      upb_Decode(data, message_len, extension_msg, extension_table, NULL,
                 decode_options, arena);
  if (status == kUpb_DecodeStatus_OutOfMemory) {
    return kUpb_GetExtension_OutOfMemory;
  }
  if (status != kUpb_DecodeStatus_Ok) return kUpb_GetExtension_ParseError;
  // Add to extensions.
  upb_Message_Extension* ext =
      _upb_Message_GetOrCreateExtension(msg, ext_table, arena);
  if (!ext) {
    return kUpb_GetExtension_OutOfMemory;
  }
  memcpy(&ext->data, &extension_msg, sizeof(extension_msg));
  *extension = ext;
  // Remove unknown field.
  upb_Message_Internal* in = upb_Message_Getinternal(msg);
  const char* internal_unknown_end =
      UPB_PTR_AT(in->internal, in->internal->unknown_end, char);
  if ((result.ptr + result.len) != internal_unknown_end) {
    memmove((char*)result.ptr, result.ptr + result.len,
            internal_unknown_end - result.ptr - result.len);
  }
  in->internal->unknown_end -= result.len;
  return kUpb_GetExtension_Ok;
}

upb_GetExtensionAsBytes_Status upb_MiniTable_GetExtensionAsBytes(
    const upb_Message* msg, const upb_MiniTable_Extension* ext_table,
    int encode_options, upb_Arena* arena, const char** extension_data,
    size_t* len) {
  const upb_Message_Extension* msg_ext = _upb_Message_Getext(msg, ext_table);
  UPB_ASSERT(ext_table->field.descriptortype == kUpb_FieldType_Message);
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
  find_unknown_ret result = UnknownFieldSet_FindField(msg, field_number);
  if (result.status != kUpb_FindUnknown_Ok) {
    UPB_ASSERT(result.status != kUpb_GetExtension_ParseError);
    return kUpb_GetExtensionAsBytes_NotPresent;
  }
  const char* data = result.ptr;
  uint32_t tag;
  uint64_t message_len;
  data = decode_tag(data, &tag);
  data = decode_varint64(data, &message_len);
  *extension_data = data;
  *len = message_len;
  return kUpb_GetExtensionAsBytes_Ok;
}

static const char* UnknownFieldSet_SkipGroup(const char* ptr, const char* end,
                                             int group_number);

static const char* UnknownFieldSet_SkipField(const char* ptr, const char* end,
                                             uint32_t tag) {
  int field_number = tag >> 3;
  int wire_type = tag & 7;
  switch (wire_type) {
    case kUpb_WireType_Varint: {
      uint64_t val;
      return decode_varint64(ptr, &val);
    }
    case kUpb_WireType_64Bit:
      if (end - ptr < 8) return NULL;
      return ptr + 8;
    case kUpb_WireType_32Bit:
      if (end - ptr < 4) return NULL;
      return ptr + 4;
    case kUpb_WireType_Delimited: {
      uint64_t size;
      ptr = decode_varint64(ptr, &size);
      if (!ptr || end - ptr < size) return NULL;
      return ptr + size;
    }
    case kUpb_WireType_StartGroup:
      return UnknownFieldSet_SkipGroup(ptr, end, field_number);
    case kUpb_WireType_EndGroup:
      return NULL;
    default:
      assert(0);
      return NULL;
  }
}

static const char* UnknownFieldSet_SkipGroup(const char* ptr, const char* end,
                                             int group_number) {
  uint32_t end_tag = (group_number << 3) | kUpb_WireType_EndGroup;
  while (true) {
    if (ptr == end) return NULL;
    uint64_t tag;
    ptr = decode_varint64(ptr, &tag);
    if (!ptr) return NULL;
    if (tag == end_tag) return ptr;
    ptr = UnknownFieldSet_SkipField(ptr, end, (uint32_t)tag);
    if (!ptr) return NULL;
  }
  return ptr;
}

enum {
  kUpb_MessageSet_StartItemTag = (1 << 3) | kUpb_WireType_StartGroup,
  kUpb_MessageSet_EndItemTag = (1 << 3) | kUpb_WireType_EndGroup,
  kUpb_MessageSet_TypeIdTag = (2 << 3) | kUpb_WireType_Varint,
  kUpb_MessageSet_MessageTag = (3 << 3) | kUpb_WireType_Delimited,
};

static find_unknown_ret UnknownFieldSet_FindField(const upb_Message* msg,
                                                  int field_number) {
  size_t size;
  find_unknown_ret ret;

  const char* ptr = upb_Message_GetUnknown(msg, &size);
  if (size == 0) {
    ret.ptr = NULL;
    return ret;
  }
  const char* end = ptr + size;
  uint64_t uint64_val;

  while (ptr < end) {
    uint32_t tag;
    int field;
    int wire_type;
    const char* unknown_begin = ptr;
    ptr = decode_tag(ptr, &tag);
    field = tag >> 3;
    wire_type = tag & 7;
    switch (wire_type) {
      case kUpb_WireType_EndGroup:
        ret.status = kUpb_FindUnknown_ParseError;
        return ret;
      case kUpb_WireType_Varint:
        ptr = decode_varint64(ptr, &uint64_val);
        if (!ptr) {
          ret.status = kUpb_FindUnknown_ParseError;
          return ret;
        }
        break;
      case kUpb_WireType_32Bit:
        ptr += 4;
        break;
      case kUpb_WireType_64Bit:
        ptr += 8;
        break;
      case kUpb_WireType_Delimited:
        // Read size.
        ptr = decode_varint64(ptr, &uint64_val);
        if (uint64_val >= INT32_MAX || !ptr) {
          ret.status = kUpb_FindUnknown_ParseError;
          return ret;
        }
        ptr += uint64_val;
        break;
      case kUpb_WireType_StartGroup:
        // tag >> 3 specifies the group number, recurse and skip
        // until we see group end tag.
        ptr = UnknownFieldSet_SkipGroup(ptr, end, field_number);
        break;
      default:
        ret.status = kUpb_FindUnknown_ParseError;
        return ret;
    }
    if (field_number == field) {
      ret.status = kUpb_FindUnknown_Ok;
      ret.ptr = unknown_begin;
      ret.len = ptr - unknown_begin;
      return ret;
    }
  }
  ret.status = kUpb_FindUnknown_NotPresent;
  ret.ptr = NULL;
  ret.len = 0;
  return ret;
}
