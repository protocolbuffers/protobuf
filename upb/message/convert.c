// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/convert.h"

#include <stdint.h>
#include <string.h>

#include "upb/base/descriptor_constants.h"
#include "upb/base/error_handler.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/compare.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/message.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/enum.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/back_alloc.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/encoder.h"

// Must be last.
#include "upb/port/def.inc"

enum {
  kUpb_ConvertStatus_Ok = kUpb_ErrorCode_Ok,
  // The source and destination MiniTables are not compatible for conversion.
  kUpb_ConvertStatus_Incompatible = 10,
};

typedef struct {
  upb_Decoder decoder;
  upb_encstate encoder;
  upb_Arena* arena;
  upb_ErrorHandler err;
  int decode_options;
  int encode_options;
} upb_Converter;

// Minitable compatibility type check on the field, but not the
// submessage. Note: this check always succeeds for enums, whether the
// enum is open or closed.
UPB_INLINE bool _upb_MiniTableField_IsCompatible(
    const upb_MiniTableField* src_f, const upb_MiniTableField* dst_f) {
  return upb_MiniTableField_Type(src_f) == upb_MiniTableField_Type(dst_f) &&
         upb_MiniTableField_IsArray(src_f) ==
             upb_MiniTableField_IsArray(dst_f) &&
         upb_MiniTableField_IsMap(src_f) == upb_MiniTableField_IsMap(dst_f);
}

UPB_INLINE bool _upb_MiniTableField_IsMapEntryCompatible(
    const upb_MiniTableField* src_f, const upb_MiniTableField* dst_f) {
  const upb_MiniTable* src_entry_mt = upb_MiniTable_MapEntrySubMessage(src_f);
  const upb_MiniTable* dst_entry_mt = upb_MiniTable_MapEntrySubMessage(dst_f);
  if (src_entry_mt == dst_entry_mt) return true;
  return _upb_MiniTableField_IsCompatible(upb_MiniTable_MapKey(src_entry_mt),
                                          upb_MiniTable_MapKey(dst_entry_mt)) &&
         _upb_MiniTableField_IsCompatible(upb_MiniTable_MapValue(src_entry_mt),
                                          upb_MiniTable_MapValue(dst_entry_mt));
}

UPB_INLINE bool _upb_MiniTableField_IsExtensionCompatible(
    const upb_MiniTableField* src_f, const upb_MiniTableField* dst_f) {
  UPB_ASSERT(!upb_MiniTableField_IsMap(src_f));
  if (upb_MiniTableField_IsMap(dst_f)) return false;
  return upb_MiniTableField_Type(dst_f) == upb_MiniTableField_Type(src_f) &&
         upb_MiniTableField_IsArray(dst_f) == upb_MiniTableField_IsArray(src_f);
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

static void upb_Message_EncodeFieldAsUnknown(
    upb_encstate* e, upb_Message* dst, const upb_Message* src,
    const upb_MiniTableField* src_field, int depth, int options,
    upb_ErrorHandler* err) {
  size_t size;
  int encode_options = upb_Encode_LimitDepth(options, depth);
  char* buf = upb_BackAlloc_Init(&e->alloc, e->alloc.arena);
  UPB_PRIVATE(_upb_Encode_Field)(e, src, src_field, &buf, &size,
                                 encode_options);
  if (size > 0) {
    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, buf, size, e->alloc.arena,
                                              kUpb_AddUnknown_Alias)) {
      upb_ErrorHandler_ThrowError(err, kUpb_ErrorCode_OutOfMemory);
    }
  }
}

static void upb_Message_EncodeExtensionAsUnknown(
    upb_encstate* e, upb_Message* dst, const upb_MiniTable* dst_mt,
    const upb_MiniTableExtension* ext, upb_MessageValue val, int depth,
    int options, upb_ErrorHandler* err) {
  size_t size;
  int encode_options = upb_Encode_LimitDepth(options, depth);
  bool is_message_set = upb_MiniTable_IsMessageSet(dst_mt);
  char* buf = upb_BackAlloc_Init(&e->alloc, e->alloc.arena);
  UPB_PRIVATE(_upb_Encode_Extension)(e, ext, val, is_message_set, &buf, &size,
                                     encode_options);
  if (size > 0) {
    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, buf, size, e->alloc.arena,
                                              kUpb_AddUnknown_Alias)) {
      upb_ErrorHandler_ThrowError(err, kUpb_ErrorCode_OutOfMemory);
    }
  }
}

static void upb_Message_ConvertInternal(upb_Converter* c, upb_Message* dst,
                                        const upb_Message* src,
                                        const upb_MiniTable* dst_mt,
                                        const upb_MiniTable* src_mt,
                                        const upb_ExtensionRegistry* extreg,
                                        int depth);

static void upb_Array_DeepConvert(
    upb_Converter* c, upb_Array* dst, const upb_Array* src,
    const upb_MiniTable* dst_sub_mt, const upb_MiniTable* src_sub_mt,
    const upb_MiniTableField* dst_f, upb_Message* dst_msg,
    const upb_ExtensionRegistry* extreg, int depth) {
  size_t size = upb_Array_Size(src);
  if (!upb_Array_Resize(dst, size, c->arena)) {
    upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
  }
  size_t dst_i = 0;
  for (size_t i = 0; i < size; ++i) {
    upb_MessageValue src_val = upb_Array_Get(src, i);
    if (upb_MiniTableField_IsClosedEnum(dst_f)) {
      const upb_MiniTableEnum* dst_e = upb_MiniTable_GetSubEnumTable(dst_f);
      if (upb_MiniTableEnum_CheckValue(dst_e, src_val.int32_val)) {
        upb_MessageValue dst_val;
        dst_val.int32_val = src_val.int32_val;
        upb_Array_Set(dst, dst_i++, dst_val);
      } else if (!_upb_Encoder_AddEnumValueToUnknown(
                     dst_msg, dst_f, src_val.int32_val, c->arena)) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
      }
    } else if (dst_sub_mt) {
      const upb_Message* src_msg = src_val.msg_val;
      upb_Message* dst_sub = upb_Message_New(dst_sub_mt, c->arena);
      if (!dst_sub) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
      }
      upb_Message_ConvertInternal(c, dst_sub, src_msg, dst_sub_mt, src_sub_mt,
                                  extreg, depth);
      upb_MessageValue dst_val;
      dst_val.msg_val = dst_sub;
      upb_Array_Set(dst, dst_i++, dst_val);
    } else {
      // Open enum or primitive case.
      upb_Array_Set(dst, dst_i++, src_val);
    }
  }
  if (dst_i != size) {
    upb_Array_Resize(dst, dst_i, c->arena);
  }
}

static bool upb_Message_ConvertArrayField(upb_Converter* c, upb_Message* dst,
                                          const upb_Message* src,
                                          const upb_MiniTableField* dst_f,
                                          const upb_MiniTableField* src_f,
                                          const upb_ExtensionRegistry* extreg,
                                          int depth) {
  const upb_Array* src_arr = upb_Message_GetArray(src, src_f);
  if (!src_arr) return true;

  const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
  const upb_MiniTable* src_sub_mt = upb_MiniTable_SubMessage(src_f);

  if (dst_sub_mt != src_sub_mt || upb_MiniTableField_IsClosedEnum(dst_f) ||
      upb_MiniTableField_IsClosedEnum(src_f)) {
    upb_Array* dst_arr =
        upb_Array_New(c->arena, upb_MiniTableField_CType(dst_f));
    if (!dst_arr)
      upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
    upb_Array_DeepConvert(c, dst_arr, src_arr, dst_sub_mt, src_sub_mt, dst_f,
                          dst, extreg, depth);
    upb_Message_SetBaseField(dst, dst_f, &dst_arr);
    return true;
  }
  // Fall through to a shallow copy.
  return false;
}

static void upb_Map_DeepConvert(
    upb_Converter* c, upb_Map* dst, const upb_Map* src,
    const upb_MiniTable* dst_entry_mt, const upb_MiniTable* src_entry_mt,
    const upb_MiniTableField* dst_map_f, upb_Message* dst_msg,
    const upb_ExtensionRegistry* extreg, int depth) {
  const upb_MiniTableField* dst_val_f = upb_MiniTable_MapValue(dst_entry_mt);
  const upb_MiniTable* dst_val_mt = upb_MiniTable_SubMessage(dst_val_f);
  const upb_MiniTableField* src_val_f = upb_MiniTable_MapValue(src_entry_mt);
  const upb_MiniTable* src_val_mt = upb_MiniTable_SubMessage(src_val_f);

  size_t iter = kUpb_Map_Begin;
  upb_MessageValue key, src_val;
  while (upb_Map_Next(src, &key, &src_val, &iter)) {
    if (dst_val_mt && src_val_mt) {
      const upb_Message* src_msg = src_val.msg_val;
      upb_Message* dst_sub = upb_Message_New(dst_val_mt, c->arena);
      if (!dst_sub) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
      }
      upb_Message_ConvertInternal(c, dst_sub, src_msg, dst_val_mt, src_val_mt,
                                  extreg, depth);
      upb_MessageValue dst_val;
      dst_val.msg_val = dst_sub;
      if (!upb_Map_Set(dst, key, dst_val, c->arena)) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
      }
    } else {
      // Scalar value.
      if (upb_MiniTableField_IsClosedEnum(dst_val_f)) {
        const upb_MiniTableEnum* dst_e =
            upb_MiniTable_GetSubEnumTable(dst_val_f);
        if (upb_MiniTableEnum_CheckValue(dst_e, src_val.int32_val)) {
          if (!upb_Map_Set(dst, key, src_val, c->arena)) {
            upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
          }
        } else {
          upb_Message* ent_msg = upb_Message_New(src_entry_mt, c->arena);
          if (!ent_msg) {
            upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
          }
          upb_Message_SetBaseField(ent_msg, upb_MiniTable_MapKey(src_entry_mt),
                                   &key);
          upb_Message_SetBaseField(
              ent_msg, upb_MiniTable_MapValue(src_entry_mt), &src_val);
          _upb_Encoder_AddMapEntryUnknown(dst_msg, dst_map_f, ent_msg,
                                          src_entry_mt, c->arena);
        }
      } else {
        if (!upb_Map_Set(dst, key, src_val, c->arena)) {
          upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
        }
      }
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
    upb_Map* dst_map = upb_Map_New(
        c->arena, upb_MiniTableField_CType(upb_MiniTable_MapKey(dst_entry_mt)),
        upb_MiniTableField_CType(dst_val_f));
    if (!dst_map) {
      upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
    }
    upb_Map_DeepConvert(c, dst_map, src_map, dst_entry_mt, src_entry_mt, dst_f,
                        dst, extreg, depth);
    upb_Message_SetBaseField(dst, dst_f, &dst_map);
    return true;
  }
  return false;
}

UPB_INLINE bool upb_Converter_NeedsClosedEnumDeepConvert(
    const upb_MiniTableField* dst_f, const upb_MiniTableField* src_f) {
  // Determines if an enum conversion requires deep conversion based on the
  // following combinations of closed/open src_f and dst_f:
  // 1. Open -> Open: Shallow copy. Never needs deep conversion (behaves like
  // primitives).
  // 2. Open -> Closed: Always needs deep conversion to validate values and
  //    move invalid values to unknowns.
  // 3. Closed -> Open: Needs deep conversion/copy for arrays to avoid mutating
  // the source array when unknowns are decoded into the open destination.
  // 4. Closed -> Closed: Only needs deep conversion if the target and source
  //    schemas differ. If the schemas match, we can safely shallow copy.
  if (upb_MiniTableField_IsClosedEnum(dst_f)) {
    return !upb_MiniTableField_IsClosedEnum(src_f) ||
           upb_MiniTable_GetSubEnumTable(dst_f) !=
               upb_MiniTable_GetSubEnumTable(src_f);
  }
  return upb_MiniTableField_IsClosedEnum(src_f) &&
         upb_MiniTableField_IsArray(src_f);
}

static void upb_Message_ConvertField(upb_Converter* c, upb_Message* dst,
                                     const upb_Message* src,
                                     const upb_MiniTableField* dst_f,
                                     const upb_MiniTableField* src_f,
                                     const upb_ExtensionRegistry* extreg,
                                     int depth) {
  if (upb_MiniTableField_HasPresence(src_f)) {
    if (!upb_Message_HasBaseField(src, src_f)) return;
  } else if (upb_MiniTableField_IsScalar(src_f)) {
    // For proto3 implicit scalar fields, we only need to copy if the source
    // field is set.
    const void* src_data = UPB_PRIVATE(_upb_Message_DataPtr)(src, src_f);
    if (UPB_PRIVATE(_upb_MiniTableField_DataIsZero)(src_f, src_data)) return;
  }

  if (upb_MiniTableField_CType(dst_f) == kUpb_CType_Message) {
    if (upb_MiniTableField_IsScalar(dst_f)) {
      const upb_Message* src_sub = upb_Message_GetMessage(src, src_f);
      if (!src_sub) return;

      const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
      const upb_MiniTable* src_sub_mt = upb_MiniTable_SubMessage(src_f);

      if (dst_sub_mt == src_sub_mt) {
        upb_Message_SetMessage(dst, dst_f, (upb_Message*)src_sub);
        return;
      }

      upb_Message* dst_sub =
          upb_Message_GetOrCreateMutableMessage(dst, dst_f, c->arena);
      if (!dst_sub)
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
      upb_Message_ConvertInternal(c, dst_sub, src_sub, dst_sub_mt, src_sub_mt,
                                  extreg, depth);
      return;
    } else if (upb_MiniTableField_IsArray(dst_f)) {
      if (upb_Message_ConvertArrayField(c, dst, src, dst_f, src_f, extreg,
                                        depth)) {
        return;
      }
    } else if (upb_MiniTableField_IsMap(dst_f)) {
      if (UPB_UNLIKELY(
              !_upb_MiniTableField_IsMapEntryCompatible(src_f, dst_f))) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ConvertStatus_Incompatible);
      }
      if (upb_Message_ConvertMapField(c, dst, src, dst_f, src_f, extreg,
                                      depth)) {
        return;
      }
    }
  } else if (upb_Converter_NeedsClosedEnumDeepConvert(dst_f, src_f)) {
    if (upb_MiniTableField_IsArray(dst_f)) {
      if (upb_Message_ConvertArrayField(c, dst, src, dst_f, src_f, extreg,
                                        depth)) {
        return;
      }
    } else if (upb_MiniTableField_IsMap(dst_f)) {
      if (upb_Message_ConvertMapField(c, dst, src, dst_f, src_f, extreg,
                                      depth)) {
        return;
      }
    } else {
      int32_t val;
      memcpy(&val, UPB_PRIVATE(_upb_Message_DataPtr)(src, src_f), 4);
      const upb_MiniTableEnum* dst_e = upb_MiniTable_GetSubEnumTable(dst_f);
      if (!upb_MiniTableEnum_CheckValue(dst_e, val)) {
        if (!_upb_Encoder_AddEnumValueToUnknown(dst, dst_f, val, c->arena)) {
          upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
        }
        return;
      }
    }
  }

  UPB_PRIVATE(_upb_MiniTableField_DataCopy)
  (dst_f, UPB_PRIVATE(_upb_Message_MutableDataPtr)(dst, dst_f),
   UPB_PRIVATE(_upb_Message_DataPtr)(src, src_f));

  if (upb_MiniTableField_HasPresence(dst_f)) {
    UPB_PRIVATE(_upb_Message_SetPresence)(dst, dst_f);
  }
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

      UPB_ASSERT(!upb_MiniTableField_IsMap(src_f));
      if (UPB_UNLIKELY(
              !_upb_MiniTableField_IsExtensionCompatible(src_f, dst_f))) {
        // Return an error due to type mismatch.
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ConvertStatus_Incompatible);
      }

      if (upb_MiniTableField_CType(dst_f) == kUpb_CType_Message) {
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
                                  src_sub_mt, dst_f, dst, extreg, depth);
            upb_MessageValue valid_val;
            valid_val.array_val = dst_arr;
            upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &valid_val,
                                            c->arena);
          } else {
            // Array of messages, and the sub message types are the same.
            // Shallow copy.
            upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &val,
                                            c->arena);
          }
        } else if (dst_sub_mt == src_sub_mt) {
          // Scalar message, and the message types are the same.
          // Shallow copy.
          upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &val, c->arena);
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
        if (upb_MiniTableField_IsClosedEnum(dst_f)) {
          if (upb_MiniTableField_IsArray(dst_f)) {
            upb_Array* dst_arr = upb_Array_New(c->arena, kUpb_CType_Int32);
            if (!dst_arr)
              upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
            upb_Array_DeepConvert(c, dst_arr, val.array_val, NULL, NULL, dst_f,
                                  dst, extreg, depth);
            upb_MessageValue valid_val;
            valid_val.array_val = dst_arr;
            upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &valid_val,
                                            c->arena);
            continue;
          } else {
            const upb_MiniTableEnum* dst_e =
                dst_ext ? upb_MiniTableExtension_GetSubEnum(dst_ext)
                        : upb_MiniTable_GetSubEnumTable(dst_f);
            if (!upb_MiniTableEnum_CheckValue(dst_e, val.int32_val)) {
              if (!_upb_Encoder_AddEnumValueToUnknown(dst, dst_f, val.int32_val,
                                                      c->arena)) {
                upb_ErrorHandler_ThrowError(&c->err,
                                            kUpb_ErrorCode_OutOfMemory);
              }
              continue;
            }
          }
        }
        upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &val, c->arena);
      }
    } else {
      // Since this extension is not known in the destination schema, encode it
      // as an unknown field.
      // TODO - b/510055656: to handle this as a non-canonical extension
      upb_Message_EncodeExtensionAsUnknown(&c->encoder, dst, dst_mt, ext, val,
                                           depth, c->encode_options, &c->err);
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

  // Bails out if the source and destination are not both MessageSets.
  if (upb_MiniTable_IsMessageSet(dst_mt) !=
      upb_MiniTable_IsMessageSet(src_mt)) {
    upb_ErrorHandler_ThrowError(&c->err, kUpb_ConvertStatus_Incompatible);
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

  // Convert fields in descending order of field number.
  while (dst_f != dst_first || src_f != src_first) {
    uint32_t dst_nr =
        dst_f != dst_first ? upb_MiniTableField_Number(dst_f - 1) : 0;
    uint32_t src_nr =
        src_f != src_first ? upb_MiniTableField_Number(src_f - 1) : 0;

    if (dst_nr == src_nr) {
      const upb_MiniTableField* dst_next = dst_f - 1;
      const upb_MiniTableField* src_next = src_f - 1;

      if (UPB_UNLIKELY(!_upb_MiniTableField_IsCompatible(src_next, dst_next))) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ConvertStatus_Incompatible);
      }
      if (upb_MiniTableField_IsInOneof(dst_next) &&
          UPB_PRIVATE(_upb_Message_GetOneofCase)(dst, dst_next) != 0) {
        // Since fields are processed in descending order, the first encountered
        // oneof field is the one that wins. We ignore subsequent ones to match
        // the encoding-then-decoding behavior.
      } else {
        upb_Message_ConvertField(c, dst, src, dst_next, src_next, extreg,
                                 depth);
      }
      dst_f--;
      src_f--;
    } else if (dst_nr > src_nr) {
      dst_f--;
    } else {
      const upb_MiniTableField* src_next = src_f - 1;
      upb_Message_EncodeFieldAsUnknown(&c->encoder, dst, src, src_next, depth,
                                       c->encode_options, &c->err);
      src_f--;
    }
  }

  // Convert extensions.
  if (src_mt->UPB_PRIVATE(ext) != kUpb_ExtMode_NonExtendable) {
    upb_Message_ConvertExtensions(c, dst, src, dst_mt, extreg, depth);
  }

  // Convert unknown fields.
  upb_StringView data;
  size_t iter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown(src, &data, &iter)) {
    int decode_options = upb_Decode_LimitDepth(
        c->decode_options | kUpb_DecodeOption_AliasString, depth);

    // Reuse d. Reset input stream.
    const char* ptr = data.data;
    upb_Decoder* d = &c->decoder;
    upb_EpsCopyInputStream_InitWithErrorHandler(&d->input, &ptr, data.size,
                                                d->err);
    upb_Decoder_Reset(d, decode_options, dst);
    _upb_Decoder_DecodeMessage(d, ptr, dst, dst_mt);
    UPB_ASSERT(d->end_group == DECODE_NOGROUP);
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

const upb_Message* upb_Message_Convert(const upb_Message* src,
                                       const upb_MiniTable* src_mt,
                                       const upb_MiniTable* dst_mt,
                                       const upb_ExtensionRegistry* extreg,
                                       int decode_options, int encode_options,
                                       upb_Arena* arena) {
  if (dst_mt == src_mt && extreg == NULL) return src;

  upb_Message* dst = upb_Message_New(dst_mt, arena);
  if (!dst) return NULL;

  upb_Converter c;
  upb_ErrorHandler_Init(&c.err);
  c.decode_options = decode_options;
  c.encode_options = encode_options;

  // Initialize the decoder.
  // Initialize decoder once, performing SwapIn.
  // We use a NULL buffer initially, effectively a dummy init to set up the
  // arena and error handler. Note: we pass &c.err.
  upb_Decoder_Init(&c.decoder, NULL, 0, extreg, decode_options, arena, &c.err,
                   NULL, 0);

  // Initialize the encoder.
  UPB_PRIVATE(_upb_encstate_init)(&c.encoder, &c.err.buf, &c.decoder.arena);

  c.arena = &c.decoder.arena;

  if (!upb_Message_DoConvert(&c, dst, src, dst_mt, src_mt, extreg)) {
    dst = NULL;
  }

#ifndef NDEBUG
  if (dst) {
    char* wire_buf;
    size_t wire_size;
    upb_Arena* tmp_arena = upb_Arena_New();

    // Compare the encoded/decoded round-trip of the original message to the
    // converted message.
    // Encode/decode original message `src`
    upb_EncodeStatus encode_status = upb_Encode(
        src, src_mt, encode_options, tmp_arena, &wire_buf, &wire_size);
    if (encode_status != kUpb_EncodeStatus_MaxDepthExceeded) {
      UPB_ASSERT(encode_status == kUpb_EncodeStatus_Ok);
      upb_Message* decoded_msg = upb_Message_New(dst_mt, tmp_arena);
      upb_DecodeStatus decode_status =
          upb_Decode(wire_buf, wire_size, decoded_msg, dst_mt, extreg,
                     decode_options, tmp_arena);
      if (decode_status != kUpb_DecodeStatus_MaxDepthExceeded) {
        UPB_ASSERT(decode_status == kUpb_DecodeStatus_Ok);
        // Compare the decoded message to the converted message.
        UPB_ASSERT(upb_Message_IsEqual(decoded_msg, dst, dst_mt, 0));
      }
    }
    upb_Arena_Free(tmp_arena);
  }
#endif

  upb_Decoder_Destroy(&c.decoder, arena);
  UPB_PRIVATE(_upb_encstate_destroy)(&c.encoder);
  return dst;
}
