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
#include "upb/message/internal/accessors.h"
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
#include "upb/wire/eps_copy_input_stream.h"  // Added
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/encode.h"

// Must be last.
#include "upb/port/def.inc"

static void upb_Message_ConvertInternal(upb_Decoder* d, upb_Message* dst,
                                        const upb_Message* src,
                                        const upb_MiniTable* dst_mt,
                                        const upb_MiniTable* src_mt,
                                        const upb_ExtensionRegistry* extreg,
                                        upb_Arena* arena, int depth);

static void upb_Array_DeepConvert(upb_Decoder* d, upb_Array* dst,
                                  const upb_Array* src,
                                  const upb_MiniTable* dst_sub_mt,
                                  const upb_MiniTable* src_sub_mt,
                                  const upb_ExtensionRegistry* extreg,
                                  upb_Arena* arena, int depth) {
  size_t size = upb_Array_Size(src);
  if (!upb_Array_Resize(dst, size, &d->arena)) {
    upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
  }
  for (size_t i = 0; i < size; ++i) {
    upb_MessageValue src_val = upb_Array_Get(src, i);
    const upb_Message* src_msg = (const upb_Message*)src_val.msg_val;
    upb_Message* dst_msg = upb_Message_New(dst_sub_mt, &d->arena);
    if (!dst_msg) {
      upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
    }
    upb_Message_ConvertInternal(d, dst_msg, src_msg, dst_sub_mt, src_sub_mt,
                                extreg, arena, depth);
    upb_MessageValue dst_val;
    dst_val.msg_val = dst_msg;
    upb_Array_Set(dst, i, dst_val);
  }
}

static void upb_Map_DeepConvert(upb_Decoder* d, upb_Map* dst,
                                const upb_Map* src,
                                const upb_MiniTable* dst_entry_mt,
                                const upb_MiniTable* src_entry_mt,
                                const upb_ExtensionRegistry* extreg,
                                upb_Arena* arena, int depth) {
  const upb_MiniTableField* dst_val_f = upb_MiniTable_MapValue(dst_entry_mt);
  const upb_MiniTable* dst_val_mt = upb_MiniTable_SubMessage(dst_val_f);
  const upb_MiniTableField* src_val_f = upb_MiniTable_MapValue(src_entry_mt);
  const upb_MiniTable* src_val_mt = upb_MiniTable_SubMessage(src_val_f);

  size_t iter = kUpb_Map_Begin;
  upb_MessageValue key, src_val;
  while (upb_Map_Next(src, &key, &src_val, &iter)) {
    const upb_Message* src_msg = (const upb_Message*)src_val.msg_val;
    upb_Message* dst_msg = upb_Message_New(dst_val_mt, &d->arena);
    if (!dst_msg) {
      upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
    }
    upb_Message_ConvertInternal(d, dst_msg, src_msg, dst_val_mt, src_val_mt,
                                extreg, arena, depth);
    upb_MessageValue dst_val;
    dst_val.msg_val = dst_msg;
    if (!upb_Map_Insert(dst, key, dst_val, &d->arena)) {
      upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
    }
  }
}

static bool upb_Message_ConvertMapField(upb_Decoder* d, upb_Message* dst,
                                        const upb_Message* src,
                                        const upb_MiniTableField* dst_f,
                                        const upb_MiniTableField* src_f,
                                        const upb_ExtensionRegistry* extreg,
                                        upb_Arena* arena, int depth) {
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
          &d->arena,
          upb_MiniTableField_CType(upb_MiniTable_MapKey(dst_entry_mt)),
          kUpb_CType_Message);
      if (!dst_map)
        upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
      upb_Map_DeepConvert(d, dst_map, src_map, dst_entry_mt, src_entry_mt,
                          extreg, arena, depth);
      upb_Message_SetBaseField(dst, dst_f, &dst_map);
      return true;
    }
  }

  const upb_MiniTableField* dst_key_f = upb_MiniTable_MapKey(dst_entry_mt);
  const upb_MiniTableField* src_key_f = upb_MiniTable_MapKey(src_entry_mt);
  const upb_MiniTableField* dst_val_f = upb_MiniTable_MapValue(dst_entry_mt);
  const upb_MiniTableField* src_val_f = upb_MiniTable_MapValue(src_entry_mt);
  if (upb_MiniTableField_Type(dst_key_f) !=
          upb_MiniTableField_Type(src_key_f) ||
      upb_MiniTableField_Type(dst_val_f) !=
          upb_MiniTableField_Type(src_val_f)) {
    upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_Malformed);
  }
  return false;
}

static bool upb_Message_ConvertField(upb_Decoder* d, upb_Message* dst,
                                     const upb_Message* src,
                                     const upb_MiniTableField* dst_f,
                                     const upb_MiniTableField* src_f,
                                     const upb_ExtensionRegistry* extreg,
                                     upb_Arena* arena, int depth) {
  if (upb_MiniTableField_CType(dst_f) != upb_MiniTableField_CType(src_f) ||
      upb_MiniTableField_IsArray(dst_f) != upb_MiniTableField_IsArray(src_f) ||
      upb_MiniTableField_IsMap(dst_f) != upb_MiniTableField_IsMap(src_f)) {
    return false;
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
        upb_Message_SetMessage(dst, dst_f, (upb_Message*)src_sub);
        return true;
      }

      upb_Message* dst_sub =
          upb_Message_GetOrCreateMutableMessage(dst, dst_f, &d->arena);
      if (!dst_sub)
        upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
      upb_Message_ConvertInternal(d, dst_sub, src_sub, dst_sub_mt, src_sub_mt,
                                  extreg, arena, depth);
      return true;
    } else if (upb_MiniTableField_IsArray(dst_f)) {
      const upb_Array* src_arr = upb_Message_GetArray(src, src_f);
      if (!src_arr) return true;

      const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
      const upb_MiniTable* src_sub_mt = upb_MiniTable_SubMessage(src_f);

      if (dst_sub_mt != src_sub_mt) {
        upb_Array* dst_arr = upb_Array_New(&d->arena, kUpb_CType_Message);
        if (!dst_arr)
          upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
        upb_Array_DeepConvert(d, dst_arr, src_arr, dst_sub_mt, src_sub_mt,
                              extreg, arena, depth);
        upb_Message_SetBaseField(dst, dst_f, &dst_arr);
        return true;
      }
      if (upb_MiniTableField_Type(dst_f) != upb_MiniTableField_Type(src_f)) {
        upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_Malformed);
      }
    } else if (upb_MiniTableField_IsMap(dst_f)) {
      if (upb_Message_ConvertMapField(d, dst, src, dst_f, src_f, extreg, arena,
                                      depth)) {
        return true;
      }
    }
  }

  UPB_ASSERT(upb_MiniTableField_Type(dst_f) == upb_MiniTableField_Type(src_f));
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)
  (dst_f, UPB_PRIVATE(_upb_Message_MutableDataPtr)(dst, dst_f),
   UPB_PRIVATE(_upb_Message_DataPtr)(src, src_f));

  if (upb_MiniTableField_HasPresence(dst_f)) {
    UPB_PRIVATE(_upb_Message_SetPresence)(dst, dst_f);
  }
  return true;
}

static void upb_Message_ConvertExtensions(upb_Decoder* d, upb_Message* dst,
                                          const upb_Message* src,
                                          const upb_MiniTable* dst_mt,
                                          const upb_ExtensionRegistry* extreg,
                                          upb_Arena* arena, int depth) {
  const upb_MiniTableExtension* ext;
  upb_MessageValue val;
  uintptr_t iter = kUpb_Message_ExtensionBegin;
  while (upb_Message_NextExtension(src, &ext, &val, &iter)) {
    const upb_MiniTableField* dst_f = upb_MiniTable_FindFieldByNumber(
        dst_mt, upb_MiniTableExtension_Number(ext));
    if (dst_f) {
      const upb_MiniTableField* src_f = upb_MiniTableExtension_ToField(ext);

      if (upb_MiniTableField_CType(dst_f) == kUpb_CType_Message) {
        if (upb_MiniTableField_CType(src_f) != kUpb_CType_Message) continue;
        if (upb_MiniTableField_IsArray(dst_f) ||
            upb_MiniTableField_IsMap(dst_f))
          continue;

        if (upb_MiniTableField_IsArray(dst_f)) {
          continue;
        }

        const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
        const upb_MiniTable* src_sub_mt = upb_MiniTable_SubMessage(src_f);

        upb_Message* dst_sub = upb_Message_New(dst_sub_mt, &d->arena);
        if (!dst_sub)
          upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);

        upb_Message_ConvertInternal(d, dst_sub, (const upb_Message*)val.msg_val,
                                    dst_sub_mt, src_sub_mt, extreg, arena,

                                    depth);

        upb_MessageValue valid_val;
        valid_val.msg_val = dst_sub;
        upb_Message_SetBaseField(dst, dst_f, &valid_val);

      } else {
        if (upb_MiniTableField_CType(dst_f) ==
            upb_MiniTableField_CType(src_f)) {
          upb_Message_SetBaseField(dst, dst_f, &val);
        }
      }
    } else {
      if (!upb_Message_SetExtension(dst, ext, &val, &d->arena)) {
        upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
      }
    }
  }
}

static void upb_Message_ConvertInternal(upb_Decoder* d, upb_Message* dst,
                                        const upb_Message* src,
                                        const upb_MiniTable* dst_mt,
                                        const upb_MiniTable* src_mt,
                                        const upb_ExtensionRegistry* extreg,
                                        upb_Arena* arena, int depth) {
  if (--depth == 0) {
    upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_MaxDepthExceeded);
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

  while (dst_f != dst_first || src_f != src_first) {
    uint32_t dst_nr =
        dst_f != dst_first ? upb_MiniTableField_Number(dst_f - 1) : 0;
    uint32_t src_nr =
        src_f != src_first ? upb_MiniTableField_Number(src_f - 1) : 0;

    if (dst_nr == src_nr) {
      const upb_MiniTableField* dst_next = dst_f - 1;
      const upb_MiniTableField* src_next = src_f - 1;
      if (!upb_Message_ConvertField(d, dst, src, dst_next, src_next, extreg,
                                    arena, depth)) {
        char* buf;
        size_t size;
        int encode_options = upb_Encode_LimitDepth(0, depth);
        // Using &d->err->buf (jmp_buf*) for _upb_Encode_Field
        // Using &d->arena for allocation (SwapIn)
        UPB_PRIVATE(_upb_Encode_Field)(&d->err->buf, src, src_next,
                                       encode_options, &d->arena, &buf, &size);
        if (size > 0) {
          if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, buf, size, &d->arena,
                                                    kUpb_AddUnknown_Alias)) {
            upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
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
      UPB_PRIVATE(_upb_Encode_Field)(&d->err->buf, src, src_next,
                                     encode_options, &d->arena, &buf, &size);
      if (size > 0) {
        if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, buf, size, &d->arena,
                                                  kUpb_AddUnknown_Alias)) {
          upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_OutOfMemory);
        }
      }
      src_f--;
    }
  }

  if (src_mt->UPB_PRIVATE(ext) != kUpb_ExtMode_NonExtendable) {
    upb_Message_ConvertExtensions(d, dst, src, dst_mt, extreg, arena, depth);
  }

  upb_StringView data;
  size_t iter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown(src, &data, &iter)) {
    int decode_options =
        upb_Decode_LimitDepth(kUpb_DecodeOption_AliasString, depth);

    // Reuse d. Reset input stream.
    const char* ptr = data.data;
    upb_EpsCopyInputStream_InitWithErrorHandler(&d->input, &ptr, data.size,
                                                d->err);
    d->depth = upb_DecodeOptions_GetEffectiveMaxDepth(decode_options);
    d->options = decode_options;
    d->end_group = DECODE_NOGROUP;
    d->missing_required = false;
    d->message_is_done = false;
    d->original_msg = dst;  // Assume dst is the message being decoded into for
                            // extension purposes if any

    _upb_Decoder_DecodeMessage(d, ptr, dst, dst_mt);

    if (d->end_group != DECODE_NOGROUP) {
      upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_Malformed);
    }
    if (d->missing_required) {
      upb_ErrorHandler_ThrowError(d->err, kUpb_ErrorCode_Malformed);
    }
  }
}

const upb_Message* upb_Message_Convert(const upb_Message* src,
                                       const upb_MiniTable* src_mt,
                                       const upb_MiniTable* dst_mt,
                                       const upb_ExtensionRegistry* extreg,
                                       upb_Arena* arena) {
  if (dst_mt == src_mt) return src;

  upb_Message* dst = upb_Message_New(dst_mt, arena);
  if (!dst) return NULL;

  upb_ErrorHandler err;
  upb_ErrorHandler_Init(&err);

  upb_Decoder d;
  // Initialize decoder once, performing SwapIn.
  // We use a NULL buffer initially, effectively a dummy init to set up the
  // arena and error handler. Note: we pass &err.
  upb_Decoder_Init(&d, NULL, 0, extreg, 0, arena, &err, NULL, 0);

  if (UPB_SETJMP(err.buf) == 0) {
    upb_Message_ConvertInternal(&d, dst, src, dst_mt, src_mt, extreg, arena,
                                100);
  } else {
    // On error, we must still Destroy the decoder to SwapOut the arena.
    // We return NULL.
    upb_Decoder_Destroy(&d, arena);
    return NULL;
  }

  // On success, Destroy/SwapOut and return dst.
  upb_Decoder_Destroy(&d, arena);
  return dst;
}
