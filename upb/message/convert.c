// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/internal/convert.h"

#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/error_handler.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/copy.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/map_sorter.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/encoder.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_Decoder decoder;
  upb_encstate encoder;
  upb_Arena* arena;
  upb_ErrorHandler err;
  int options;
} upb_Converter;

static int getAddUnknownOptions(int convert_options) {
  return (convert_options & kUpb_ConvertOption_Alias) ? kUpb_AddUnknown_Alias
                                                      : kUpb_AddUnknown_Copy;
}

static int getDecodeOptions(int convert_options) {
  return (convert_options & kUpb_ConvertOption_Alias)
             ? kUpb_DecodeOption_AliasString
             : 0;
}

static void upb_Message_SetFieldOrExtension(upb_Message* msg,
                                            const upb_MiniTableField* f,
                                            const upb_MiniTableExtension* ext,
                                            const upb_MessageValue* val,
                                            upb_Arena* arena) {
  if (ext != NULL) {
    upb_Message_SetExtension(msg, ext, val, arena);
  } else {
    upb_Message_SetBaseField(msg, f, val);
  }
}

static void upb_Message_EncodeExtensionAsUnknown(
    upb_encstate* e, upb_Message* dst, const upb_MiniTable* dst_mt,
    const upb_MiniTableExtension* ext, upb_MessageValue val, int depth,
    int add_unknown_option, upb_ErrorHandler* err) {
  char* buf;
  size_t size;
  int encode_options = upb_Encode_LimitDepth(0, depth);
  bool is_message_set = upb_MiniTable_IsMessageSet(dst_mt);
  UPB_PRIVATE(_upb_Encode_Extension)(e, ext, val, is_message_set, &buf, &size,
                                     encode_options);
  if (size > 0) {
    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, buf, size, e->arena,
                                              add_unknown_option)) {
      if (e->err) {
        upb_ErrorHandler_ThrowError(err, kUpb_ErrorCode_OutOfMemory);
      }
    }
  }
}

static upb_StringView upb_StringView_Clone(upb_StringView src, upb_Arena* arena,
                                           upb_ErrorHandler* err) {
  char* new_str = upb_Arena_Malloc(arena, src.size);
  if (src.size > 0 && !new_str) {
    upb_ErrorHandler_ThrowError(err, kUpb_ErrorCode_OutOfMemory);
  }
  if (src.size > 0) {
    memcpy(new_str, src.data, src.size);
  }
  return upb_StringView_FromDataAndSize(new_str, src.size);
}

static upb_Array* upb_Array_Clone(const upb_Array* src, upb_CType type,
                                  const upb_MiniTable* sub_mt, upb_Arena* arena,
                                  upb_ErrorHandler* err) {
  upb_Array* dst = upb_Array_DeepClone(src, type, sub_mt, arena);
  if (!dst) {
    upb_ErrorHandler_ThrowError(err, kUpb_ErrorCode_OutOfMemory);
  }
  return dst;
}

static upb_Map* upb_Map_Clone(const upb_Map* src, upb_CType key_type,
                              upb_CType val_type, const upb_MiniTable* sub_mt,
                              upb_Arena* arena, upb_ErrorHandler* err) {
  upb_Map* dst = upb_Map_DeepClone(src, key_type, val_type, sub_mt, arena);
  if (!dst) {
    upb_ErrorHandler_ThrowError(err, kUpb_ErrorCode_OutOfMemory);
  }
  return dst;
}

static upb_Message* upb_Message_Clone(const upb_Message* src,
                                      const upb_MiniTable* mt, upb_Arena* arena,
                                      upb_ErrorHandler* err) {
  upb_Message* dst = upb_Message_DeepClone(src, mt, arena);
  if (!dst) {
    upb_ErrorHandler_ThrowError(err, kUpb_ErrorCode_OutOfMemory);
  }
  return dst;
}

// Sets the message field in the destination message. This requires that the
// destination and source fields are both scalar messages and that the
// minitables of the source and destination fields are identical.
//
// If aliasing is enabled, the source message is used directly. Otherwise, the
// source message is deep cloned into the destination message.
static void upb_Message_SetMessageOrClone(upb_Converter* c, upb_Message* dst,
                                          const upb_MiniTableField* dst_f,
                                          const upb_Message* src_msg,
                                          const upb_MiniTable* mt) {
  if (c->options & kUpb_ConvertOption_Alias) {
    upb_Message_SetMessage(dst, dst_f, (upb_Message*)src_msg);
  } else {
    upb_Message* dst_msg = upb_Message_Clone(src_msg, mt, c->arena, &c->err);
    upb_Message_SetMessage(dst, dst_f, dst_msg);
  }
}

static void upb_Message_SetFieldOrClone(upb_Converter* c, upb_Message* dst,
                                        const upb_MiniTableField* dst_f,
                                        const upb_Message* src,
                                        const upb_MiniTableField* src_f) {
  if (c->options & kUpb_ConvertOption_Alias) {
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(
        dst_f, UPB_PRIVATE(_upb_Message_MutableDataPtr)(dst, dst_f),
        UPB_PRIVATE(_upb_Message_DataPtr)(src, src_f));
    return;
  }

  // Deep copy the field.
  if (upb_MiniTableField_IsMap(dst_f)) {
    const upb_Map* src_map = upb_Message_GetMap(src, src_f);
    if (src_map) {
      const upb_MiniTable* dst_entry_mt =
          upb_MiniTable_MapEntrySubMessage(dst_f);
      upb_CType key_type =
          upb_MiniTableField_CType(upb_MiniTable_MapKey(dst_entry_mt));
      upb_CType val_type =
          upb_MiniTableField_CType(upb_MiniTable_MapValue(dst_entry_mt));
      upb_Map* dst_map = upb_Map_Clone(src_map, key_type, val_type,
                                       dst_entry_mt, c->arena, &c->err);
      upb_Message_SetBaseField(dst, dst_f, &dst_map);
    }
  } else if (upb_MiniTableField_IsArray(dst_f)) {
    const upb_Array* src_arr = upb_Message_GetArray(src, src_f);
    if (src_arr) {
      const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
      upb_Array* dst_arr =
          upb_Array_Clone(src_arr, upb_MiniTableField_CType(dst_f), dst_sub_mt,
                          c->arena, &c->err);
      upb_Message_SetBaseField(dst, dst_f, &dst_arr);
    }
  } else if (upb_MiniTableField_CType(dst_f) == kUpb_CType_String ||
             upb_MiniTableField_CType(dst_f) == kUpb_CType_Bytes) {
    upb_StringView src_str;
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(
        dst_f, &src_str, UPB_PRIVATE(_upb_Message_DataPtr)(src, src_f));
    upb_StringView dst_str = upb_StringView_Clone(src_str, c->arena, &c->err);
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(
        dst_f, UPB_PRIVATE(_upb_Message_MutableDataPtr)(dst, dst_f), &dst_str);
  } else {
    UPB_PRIVATE(_upb_MiniTableField_DataCopy)(
        dst_f, UPB_PRIVATE(_upb_Message_MutableDataPtr)(dst, dst_f),
        UPB_PRIVATE(_upb_Message_DataPtr)(src, src_f));
  }
}

static void upb_Message_SetExtensionOrClone(
    upb_Converter* c, upb_Message* dst, const upb_MiniTableField* dst_f,
    const upb_MiniTableExtension* dst_ext, const upb_MessageValue* src_val) {
  if (!src_val) return;

  if (c->options & kUpb_ConvertOption_Alias) {
    upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, src_val, c->arena);
    return;
  }

  UPB_ASSERT(!upb_MiniTableField_IsMap(dst_f));
  if (upb_MiniTableField_IsArray(dst_f)) {
    upb_MessageValue valid_val;
    valid_val.array_val = upb_Array_Clone(
        src_val->array_val, upb_MiniTableField_CType(dst_f),
        upb_MiniTableExtension_GetSubMessage(dst_ext), c->arena, &c->err);
    upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &valid_val, c->arena);
  } else if (upb_MiniTableField_CType(dst_f) == kUpb_CType_String ||
             upb_MiniTableField_CType(dst_f) == kUpb_CType_Bytes) {
    upb_MessageValue valid_val;
    valid_val.str_val =
        upb_StringView_Clone(src_val->str_val, c->arena, &c->err);
    upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &valid_val, c->arena);
  } else {
    upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, src_val, c->arena);
  }
}

static void upb_Message_SetMessageOrExtensionOrClone(
    upb_Converter* c, upb_Message* dst, const upb_MiniTableField* dst_f,
    const upb_MiniTableExtension* dst_ext, const upb_MiniTable* dst_sub_mt,
    const upb_MessageValue* msg_val) {
  if (!msg_val) return;
  if (c->options & kUpb_ConvertOption_Alias) {
    upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, msg_val, c->arena);
  } else {
    upb_MessageValue valid_val;
    valid_val.msg_val =
        upb_Message_Clone(msg_val->msg_val, dst_sub_mt, c->arena, &c->err);
    if (!valid_val.msg_val) return;
    upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &valid_val, c->arena);
  }
}

static void upb_Message_SetArrayOrExtensionOrClone(
    upb_Converter* c, upb_Message* dst, const upb_MiniTableField* dst_f,
    const upb_MiniTableExtension* dst_ext, const upb_MiniTable* dst_sub_mt,
    const upb_MessageValue* src_arr) {
  if (!src_arr) return;
  if (c->options & kUpb_ConvertOption_Alias) {
    upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, src_arr, c->arena);
  } else {
    upb_MessageValue cloned_val;
    cloned_val.array_val =
        upb_Array_Clone(src_arr->array_val, upb_MiniTableField_CType(dst_f),
                        dst_sub_mt, c->arena, &c->err);
    if (!cloned_val.array_val) return;
    upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &cloned_val, c->arena);
  }
}

static void upb_Message_SetOrCloneExtension(upb_Converter* c, upb_Message* dst,
                                            const upb_MiniTableExtension* ext,
                                            upb_MessageValue* val) {
  if (!(c->options & kUpb_ConvertOption_Alias)) {
    const upb_MiniTableField* ext_f = upb_MiniTableExtension_ToField(ext);
    if (upb_MiniTableField_IsArray(ext_f)) {
      val->array_val = upb_Array_Clone(
          val->array_val, upb_MiniTableField_CType(ext_f),
          upb_MiniTableExtension_GetSubMessage(ext), c->arena, &c->err);
    } else if (upb_MiniTableField_CType(ext_f) == kUpb_CType_Message) {
      val->msg_val = upb_Message_Clone(
          val->msg_val, upb_MiniTableExtension_GetSubMessage(ext), c->arena,
          &c->err);
    } else if (upb_MiniTableField_CType(ext_f) == kUpb_CType_String ||
               upb_MiniTableField_CType(ext_f) == kUpb_CType_Bytes) {
      val->str_val = upb_StringView_Clone(val->str_val, c->arena, &c->err);
    }
  }
  if (!upb_Message_SetExtension(dst, ext, val, c->arena)) {
    upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
  }
}

static void upb_Message_ConvertInternal(upb_Converter* c, upb_Message* dst,
                                        const upb_Message* src,
                                        const upb_MiniTable* dst_mt,
                                        const upb_MiniTable* src_mt,
                                        const upb_ExtensionRegistry* extreg,
                                        int depth);

static void upb_Array_DeepConvert(upb_Converter* c, upb_Array* dst,
                                  const upb_Array* src,
                                  const upb_MiniTable* dst_sub_mt,
                                  const upb_MiniTable* src_sub_mt,
                                  const upb_ExtensionRegistry* extreg,
                                  int depth) {
  size_t size = upb_Array_Size(src);
  if (!upb_Array_Resize(dst, size, c->arena)) {
    upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
  }
  for (size_t i = 0; i < size; ++i) {
    upb_MessageValue src_val = upb_Array_Get(src, i);
    const upb_Message* src_msg = src_val.msg_val;
    upb_Message* dst_msg = upb_Message_New(dst_sub_mt, c->arena);
    if (!dst_msg) {
      upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
    }
    upb_Message_ConvertInternal(c, dst_msg, src_msg, dst_sub_mt, src_sub_mt,
                                extreg, depth);
    upb_MessageValue dst_val;
    dst_val.msg_val = dst_msg;
    upb_Array_Set(dst, i, dst_val);
  }
}

static void upb_Map_DeepConvert(upb_Converter* c, upb_Map* dst,
                                const upb_Map* src,
                                const upb_MiniTable* dst_entry_mt,
                                const upb_MiniTable* src_entry_mt,
                                const upb_ExtensionRegistry* extreg,
                                int depth) {
  const upb_MiniTableField* dst_key_f = upb_MiniTable_MapKey(dst_entry_mt);
  const upb_MiniTableField* dst_val_f = upb_MiniTable_MapValue(dst_entry_mt);
  const upb_MiniTable* dst_val_mt = upb_MiniTable_SubMessage(dst_val_f);
  const upb_MiniTableField* src_val_f = upb_MiniTable_MapValue(src_entry_mt);
  const upb_MiniTable* src_val_mt = upb_MiniTable_SubMessage(src_val_f);
  const bool clone_str_keys =
      !(c->options & kUpb_ConvertOption_Alias) &&
      (upb_MiniTableField_CType(dst_key_f) == kUpb_CType_String ||
       upb_MiniTableField_CType(dst_key_f) == kUpb_CType_Bytes);

  size_t iter = kUpb_Map_Begin;
  upb_MessageValue key, src_val;
  while (upb_Map_Next(src, &key, &src_val, &iter)) {
    const upb_Message* src_msg = src_val.msg_val;
    upb_Message* dst_msg = upb_Message_New(dst_val_mt, c->arena);
    if (!dst_msg) {
      upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
    }
    upb_Message_ConvertInternal(c, dst_msg, src_msg, dst_val_mt, src_val_mt,
                                extreg, depth);
    upb_MessageValue dst_val;
    dst_val.msg_val = dst_msg;
    if (clone_str_keys) {
      key.str_val = upb_StringView_Clone(key.str_val, c->arena, &c->err);
    }
    if (!upb_Map_Set(dst, key, dst_val, c->arena)) {
      upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
    }
  }
}

static bool upb_Message_ConvertMapField(upb_Converter* c, upb_Message* dst,
                                        const upb_Message* src,
                                        const upb_MiniTableField* dst_f,
                                        const upb_MiniTableField* src_f,
                                        const upb_ExtensionRegistry* extreg,
                                        int depth) {
  const upb_Map* src_map = upb_Message_GetMap(src, src_f);
  if (!src_map) return true;

  const upb_MiniTable* dst_entry_mt = upb_MiniTable_MapEntrySubMessage(dst_f);
  const upb_MiniTable* src_entry_mt = upb_MiniTable_MapEntrySubMessage(src_f);

  if (dst_entry_mt != src_entry_mt) {
    const upb_MiniTableField* dst_val_f = upb_MiniTable_MapValue(dst_entry_mt);
    const upb_MiniTableField* src_val_f = upb_MiniTable_MapValue(src_entry_mt);
    if (upb_MiniTableField_CType(dst_val_f) == kUpb_CType_Message &&
        upb_MiniTableField_CType(src_val_f) == kUpb_CType_Message) {
      upb_Map* dst_map = upb_Map_New(
          c->arena,
          upb_MiniTableField_CType(upb_MiniTable_MapKey(dst_entry_mt)),
          kUpb_CType_Message);
      if (!dst_map) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
      }
      upb_Map_DeepConvert(c, dst_map, src_map, dst_entry_mt, src_entry_mt,
                          extreg, depth);
      upb_Message_SetBaseField(dst, dst_f, &dst_map);
      return true;
    }
  }
  return false;
}

static bool upb_Message_ConvertField(upb_Converter* c, upb_Message* dst,
                                     const upb_Message* src,
                                     const upb_MiniTableField* dst_f,
                                     const upb_MiniTableField* src_f,
                                     const upb_ExtensionRegistry* extreg,
                                     int depth) {
  if (upb_MiniTableField_Type(dst_f) != upb_MiniTableField_Type(src_f) ||
      upb_MiniTableField_IsArray(dst_f) != upb_MiniTableField_IsArray(src_f) ||
      upb_MiniTableField_IsMap(dst_f) != upb_MiniTableField_IsMap(src_f)) {
    return false;
  }

  if (upb_MiniTableField_IsMap(dst_f)) {
    const upb_MiniTable* dst_entry_mt = upb_MiniTable_MapEntrySubMessage(dst_f);
    const upb_MiniTable* src_entry_mt = upb_MiniTable_MapEntrySubMessage(src_f);
    const upb_MiniTableField* dst_key_f = upb_MiniTable_MapKey(dst_entry_mt);
    const upb_MiniTableField* src_key_f = upb_MiniTable_MapKey(src_entry_mt);
    const upb_MiniTableField* dst_val_f = upb_MiniTable_MapValue(dst_entry_mt);
    const upb_MiniTableField* src_val_f = upb_MiniTable_MapValue(src_entry_mt);

    if (upb_MiniTableField_Type(dst_key_f) !=
            upb_MiniTableField_Type(src_key_f) ||
        upb_MiniTableField_Type(dst_val_f) !=
            upb_MiniTableField_Type(src_val_f)) {
      return false;
    }
  }

  if (upb_MiniTableField_HasPresence(src_f)) {
    if (!upb_Message_HasBaseField(src, src_f)) return true;
  }

  if (upb_MiniTableField_CType(dst_f) == kUpb_CType_Message) {
    if (upb_MiniTableField_IsScalar(dst_f)) {
      const upb_Message* src_sub = upb_Message_GetMessage(src, src_f);
      if (!src_sub) return true;

      const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
      const upb_MiniTable* src_sub_mt = upb_MiniTable_SubMessage(src_f);

      if (dst_sub_mt == src_sub_mt) {
        upb_Message_SetMessageOrClone(c, dst, dst_f, src_sub, dst_sub_mt);
        return true;
      }

      upb_Message* dst_sub =
          upb_Message_GetOrCreateMutableMessage(dst, dst_f, c->arena);
      if (!dst_sub)
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
      upb_Message_ConvertInternal(c, dst_sub, src_sub, dst_sub_mt, src_sub_mt,
                                  extreg, depth);
      return true;
    } else if (upb_MiniTableField_IsArray(dst_f)) {
      const upb_Array* src_arr = upb_Message_GetArray(src, src_f);
      if (!src_arr) return true;

      const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
      const upb_MiniTable* src_sub_mt = upb_MiniTable_SubMessage(src_f);

      if (dst_sub_mt != src_sub_mt) {
        upb_Array* dst_arr = upb_Array_New(c->arena, kUpb_CType_Message);
        if (!dst_arr)
          upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
        upb_Array_DeepConvert(c, dst_arr, src_arr, dst_sub_mt, src_sub_mt,
                              extreg, depth);
        upb_Message_SetBaseField(dst, dst_f, &dst_arr);
        return true;
      }
      if (upb_MiniTableField_Type(dst_f) != upb_MiniTableField_Type(src_f)) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_Malformed);
      }
    } else if (upb_MiniTableField_IsMap(dst_f)) {
      if (upb_Message_ConvertMapField(c, dst, src, dst_f, src_f, extreg,
                                      depth)) {
        return true;
      }
    }
  }

  UPB_ASSERT(upb_MiniTableField_Type(dst_f) == upb_MiniTableField_Type(src_f));
  upb_Message_SetFieldOrClone(c, dst, dst_f, src, src_f);

  if (upb_MiniTableField_HasPresence(dst_f)) {
    UPB_PRIVATE(_upb_Message_SetPresence)(dst, dst_f);
  }
  return true;
}

static void upb_Message_ConvertExtensions(upb_Converter* c, upb_Message* dst,
                                          const upb_Message* src,
                                          const upb_MiniTable* dst_mt,
                                          const upb_ExtensionRegistry* extreg,
                                          int depth) {
  const upb_MiniTableExtension* ext;
  upb_MessageValue val;
  uintptr_t iter = kUpb_Message_ExtensionBegin;
  while (upb_Message_NextExtension(src, &ext, &val, &iter)) {
    const upb_MiniTableField* dst_f = upb_MiniTable_FindFieldByNumber(
        dst_mt, upb_MiniTableExtension_Number(ext));
    const upb_MiniTableExtension* dst_ext = NULL;
    if (!dst_f) {
      // Source extension not found in the destination schema. Check the
      // extension registry.
      if (extreg != NULL) {
        dst_ext = upb_ExtensionRegistry_Lookup(
            extreg, dst_mt, upb_MiniTableExtension_Number(ext));
        if (dst_ext) {
          dst_f = upb_MiniTableExtension_ToField(dst_ext);
        }
      }
    }

    if (dst_f) {
      const upb_MiniTableField* src_f = upb_MiniTableExtension_ToField(ext);

      UPB_ASSERT(!upb_MiniTableField_IsMap(src_f) &&
                 !upb_MiniTableField_IsMap(dst_f));
      if (upb_MiniTableField_CType(dst_f) != upb_MiniTableField_CType(src_f) ||
          upb_MiniTableField_IsArray(dst_f) !=
              upb_MiniTableField_IsArray(src_f)) {
        // Type mismatch. Encode the extension as an unknown field in the
        // destination message.
        upb_Message_EncodeExtensionAsUnknown(
            &c->encoder, dst, dst_mt, ext, val, depth,
            getAddUnknownOptions(c->options), &c->err);
      } else if (upb_MiniTableField_CType(dst_f) == kUpb_CType_Message) {
        const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
        const upb_MiniTable* src_sub_mt = upb_MiniTable_SubMessage(src_f);

        if (upb_MiniTableField_IsArray(dst_f)) {
          if (dst_sub_mt != src_sub_mt) {
            // Array of messages, and the sub message types differ. Perform
            // conversion.
            upb_Array* dst_arr = upb_Array_New(c->arena, kUpb_CType_Message);
            if (!dst_arr)
              upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
            upb_Array_DeepConvert(c, dst_arr, val.array_val, dst_sub_mt,
                                  src_sub_mt, extreg, depth);
            upb_MessageValue valid_val;
            valid_val.array_val = dst_arr;
            upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &valid_val,
                                            c->arena);
          } else {
            // Array of messages, and the sub message types are the same.
            upb_Message_SetArrayOrExtensionOrClone(c, dst, dst_f, dst_ext,
                                                   dst_sub_mt, &val);
          }
        } else if (dst_sub_mt == src_sub_mt) {
          // Scalar message, and the message types are the same.
          upb_Message_SetMessageOrExtensionOrClone(c, dst, dst_f, dst_ext,
                                                   dst_sub_mt, &val);
        } else {
          // Scalar message, and the message types differ. Perform conversion.
          upb_Message* dst_sub = upb_Message_New(dst_sub_mt, c->arena);
          if (!dst_sub)
            upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);

          upb_Message_ConvertInternal(c, dst_sub, val.msg_val, dst_sub_mt,
                                      src_sub_mt, extreg, depth);

          upb_MessageValue valid_val;
          valid_val.msg_val = dst_sub;
          upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &valid_val,
                                          c->arena);
        }
      } else {
        // Scalar non-message type.
        if (c->options & kUpb_ConvertOption_Alias) {
          upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &val, c->arena);
        } else {
          upb_Message_SetExtensionOrClone(c, dst, dst_f, dst_ext, &val);
        }
      }
    } else {
      if (dst_mt->UPB_PRIVATE(ext) == kUpb_ExtMode_NonExtendable) {
        // Destination message does not support extensions. Encode the extension
        // as an unknown field in the destination message.
        upb_Message_EncodeExtensionAsUnknown(
            &c->encoder, dst, dst_mt, ext, val, depth,
            getAddUnknownOptions(c->options), &c->err);
      } else {
        upb_Message_SetOrCloneExtension(c, dst, ext, &val);
      }
    }
  }
}

static void upb_Message_ConvertInternal(upb_Converter* c, upb_Message* dst,
                                        const upb_Message* src,
                                        const upb_MiniTable* dst_mt,
                                        const upb_MiniTable* src_mt,
                                        const upb_ExtensionRegistry* extreg,
                                        int depth) {
  UPB_ASSERT(dst != NULL);
  if (--depth == 0) {
    upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_MaxDepthExceeded);
  }

  const upb_MiniTableField* dst_f = NULL;
  const upb_MiniTableField* dst_first = NULL;
  const upb_MiniTableField* src_f = NULL;
  const upb_MiniTableField* src_first = NULL;

  if (upb_MiniTable_FieldCount(dst_mt) > 0) {
    dst_first = upb_MiniTable_GetFieldByIndex(dst_mt, 0);
    dst_f = dst_first + upb_MiniTable_FieldCount(dst_mt);
  }
  if (upb_MiniTable_FieldCount(src_mt) > 0) {
    src_first = upb_MiniTable_GetFieldByIndex(src_mt, 0);
    src_f = src_first + upb_MiniTable_FieldCount(src_mt);
  }

  int add_unknown_options = (c->options & kUpb_ConvertOption_Alias)
                                ? kUpb_AddUnknown_Alias
                                : kUpb_AddUnknown_Copy;

  while (dst_f != dst_first || src_f != src_first) {
    uint32_t dst_nr =
        dst_f != dst_first ? upb_MiniTableField_Number(dst_f - 1) : 0;
    uint32_t src_nr =
        src_f != src_first ? upb_MiniTableField_Number(src_f - 1) : 0;

    if (dst_nr == src_nr) {
      const upb_MiniTableField* dst_next = dst_f - 1;
      const upb_MiniTableField* src_next = src_f - 1;
      if (!upb_Message_ConvertField(c, dst, src, dst_next, src_next, extreg,
                                    depth)) {
        char* buf;
        size_t size;
        int encode_options = upb_Encode_LimitDepth(0, depth);
        UPB_PRIVATE(_upb_Encode_Field)(&c->encoder, src, src_next, &buf, &size,
                                       encode_options);
        if (size > 0) {
          if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, buf, size, c->arena,
                                                    add_unknown_options)) {
            upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
          }
        }
      }
      dst_f--;
      src_f--;
    } else if (dst_nr > src_nr) {
      dst_f--;
    } else {
      const upb_MiniTableField* src_next = src_f - 1;
      char* buf;
      size_t size;
      int encode_options = upb_Encode_LimitDepth(0, depth);
      UPB_PRIVATE(_upb_Encode_Field)(&c->encoder, src, src_next, &buf, &size,
                                     encode_options);
      if (size > 0) {
        if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, buf, size, c->arena,
                                                  add_unknown_options)) {
          upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
        }
      }
      src_f--;
    }
  }

  if (src_mt->UPB_PRIVATE(ext) != kUpb_ExtMode_NonExtendable) {
    upb_Message_ConvertExtensions(c, dst, src, dst_mt, extreg, depth);
  }

  upb_StringView data;
  size_t iter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown(src, &data, &iter)) {
    int decode_options =
        upb_Decode_LimitDepth(getDecodeOptions(c->options), depth);

    // Reuse d. Reset input stream.
    const char* ptr = data.data;
    upb_Decoder* d = &c->decoder;
    upb_EpsCopyInputStream_InitWithErrorHandler(&d->input, &ptr, data.size,
                                                d->err);
    upb_Decoder_Reset(d, decode_options, dst);
    _upb_Decoder_DecodeMessage(d, ptr, dst, dst_mt);

    if (d->end_group != DECODE_NOGROUP) {
      upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_Malformed);
    }
    if (d->missing_required) {
      upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_Malformed);
    }
  }
}

static bool upb_Message_DoConvert(upb_Converter* c, upb_Message* dst,
                                  const upb_Message* src,
                                  const upb_MiniTable* dst_mt,
                                  const upb_MiniTable* src_mt,
                                  const upb_ExtensionRegistry* extreg) {
  if (UPB_SETJMP(c->err.buf) == 0) {
    upb_Message_ConvertInternal(c, dst, src, dst_mt, src_mt, extreg, 100);
    return true;
  }
  return false;
}

const upb_Message* UPB_PRIVATE(_upb_Message_Convert)(
    upb_Message* dst_msg, const upb_MiniTable* dst_mt,
    const upb_Message* src_msg, const upb_MiniTable* src_mt,
    const upb_ExtensionRegistry* extreg, int options, upb_Arena* arena) {
  // Short-circuit if the source and destination schemas match, and we are
  // aliasing strings and unknown fields.
  if (dst_mt == src_mt && extreg == NULL &&
      options == kUpb_ConvertOption_Alias) {
    if (dst_msg == NULL) {
      return src_msg;
    } else if (upb_Message_ShallowCopy(dst_msg, src_msg, dst_mt, arena)) {
      // When the `dst_msg` is provided, we must copy into it before returning.
      return dst_msg;
    } else {
      return NULL;
    }
  }

  upb_Message* dst;
  if (dst_msg != NULL) {
    dst = dst_msg;
  } else {
    dst = upb_Message_New(dst_mt, arena);
    if (!dst) {
      return NULL;
    }
  }

  upb_Converter c;
  upb_ErrorHandler_Init(&c.err);
  c.options = options;

  // Initialize the decoder.
  // Initialize decoder once, performing SwapIn.
  // We use a NULL buffer initially, effectively a dummy init to set up the
  // arena and error handler. Note: we pass &c.err.
  upb_Decoder_Init(&c.decoder, NULL, 0, extreg, 0, arena, &c.err, NULL, 0);

  // Initialize the encoder.
  c.encoder.status = kUpb_EncodeStatus_Ok;
  c.encoder.err = &c.err.buf;
  c.encoder.arena = &c.decoder.arena;
  _upb_mapsorter_init(&c.encoder.sorter);

  c.arena = &c.decoder.arena;

  if (!upb_Message_DoConvert(&c, dst, src_msg, dst_mt, src_mt, extreg)) {
    dst = NULL;
  }
  upb_Decoder_Destroy(&c.decoder, arena);
  UPB_PRIVATE(_upb_encstate_destroy)(&c.encoder);
  return dst;
}
