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
#include "upb/wire/eps_copy_input_stream.h"  // Added
#include "upb/wire/internal/decoder.h"
#include "upb/wire/internal/encoder.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_Decoder decoder;
  upb_encstate encoder;
  upb_Arena* arena;
  upb_ErrorHandler err;
} upb_Converter;

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
    upb_ErrorHandler* err) {
  char* buf;
  size_t size;
  int encode_options = upb_Encode_LimitDepth(0, depth);
  bool is_message_set = upb_MiniTable_IsMessageSet(dst_mt);
  UPB_PRIVATE(_upb_Encode_Extension)(e, ext, val, is_message_set, &buf, &size,
                                     encode_options);
  if (size > 0) {
    if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, buf, size, e->arena,
                                              kUpb_AddUnknown_Alias)) {
      if (e->err) {
        upb_ErrorHandler_ThrowError(err, kUpb_ErrorCode_OutOfMemory);
      }
    }
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
  const upb_MiniTableField* dst_val_f = upb_MiniTable_MapValue(dst_entry_mt);
  const upb_MiniTable* dst_val_mt = upb_MiniTable_SubMessage(dst_val_f);
  const upb_MiniTableField* src_val_f = upb_MiniTable_MapValue(src_entry_mt);
  const upb_MiniTable* src_val_mt = upb_MiniTable_SubMessage(src_val_f);

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
  if (upb_MiniTableField_HasPresence(src_f)) {
    if (!upb_Message_HasBaseField(src, src_f)) return true;
  }

  if (upb_MiniTableField_CType(dst_f) == kUpb_CType_Message) {
    if (upb_MiniTableField_IsScalar(dst_f)) {
      UPB_ASSERT(upb_MiniTableField_Type(dst_f) ==
                 upb_MiniTableField_Type(src_f));
      const upb_Message* src_sub = upb_Message_GetMessage(src, src_f);
      if (!src_sub) return true;

      const upb_MiniTable* dst_sub_mt = upb_MiniTable_SubMessage(dst_f);
      const upb_MiniTable* src_sub_mt = upb_MiniTable_SubMessage(src_f);

      if (dst_sub_mt == src_sub_mt) {
        upb_Message_SetMessage(dst, dst_f, (upb_Message*)src_sub);
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
      UPB_ASSERT(upb_MiniTableField_IsArray(dst_f) &&
                 upb_MiniTableField_Type(dst_f) ==
                     upb_MiniTableField_Type(src_f));
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
    } else if (upb_MiniTableField_IsMap(dst_f)) {
      UPB_ASSERT(upb_MiniTableField_IsMap(src_f));
      // Check that the key and value types are the same for the source and
      // destination map fields.
      const upb_MiniTable* dst_entry_mt =
          upb_MiniTable_MapEntrySubMessage(dst_f);
      const upb_MiniTable* src_entry_mt =
          upb_MiniTable_MapEntrySubMessage(src_f);
      const upb_MiniTableField* dst_key_f = upb_MiniTable_MapKey(dst_entry_mt);
      const upb_MiniTableField* src_key_f = upb_MiniTable_MapKey(src_entry_mt);
      const upb_MiniTableField* dst_val_f =
          upb_MiniTable_MapValue(dst_entry_mt);
      const upb_MiniTableField* src_val_f =
          upb_MiniTable_MapValue(src_entry_mt);
      UPB_ASSERT(upb_MiniTableField_Type(dst_key_f) ==
                     upb_MiniTableField_Type(src_key_f) &&
                 upb_MiniTableField_Type(dst_val_f) ==
                     upb_MiniTableField_Type(src_val_f));

      if (upb_Message_ConvertMapField(c, dst, src, dst_f, src_f, extreg,
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
      // Type mismatch in extension is undefined behavior.
      UPB_ASSERT(upb_MiniTableField_Type(dst_f) ==
                     upb_MiniTableField_Type(src_f) &&
                 upb_MiniTableField_IsArray(dst_f) ==
                     upb_MiniTableField_IsArray(src_f));

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
                                  src_sub_mt, extreg, depth);
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
        upb_Message_SetFieldOrExtension(dst, dst_f, dst_ext, &val, c->arena);
      }
    } else {
      if (dst_mt->UPB_PRIVATE(ext) == kUpb_ExtMode_NonExtendable) {
        // Destination message does not support extensions. Encode the extension
        // as an unknown field in the destination message.
        upb_Message_EncodeExtensionAsUnknown(&c->encoder, dst, dst_mt, ext, val,
                                             depth, &c->err);
      } else if (!upb_Message_SetExtension(dst, ext, &val, c->arena)) {
        upb_ErrorHandler_ThrowError(&c->err, kUpb_ErrorCode_OutOfMemory);
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
                                                    kUpb_AddUnknown_Alias)) {
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
                                                  kUpb_AddUnknown_Alias)) {
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
        upb_Decode_LimitDepth(kUpb_DecodeOption_AliasString, depth);

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
                                       upb_Arena* arena) {
  if (dst_mt == src_mt && extreg == NULL) return src;

  upb_Message* dst = upb_Message_New(dst_mt, arena);
  if (!dst) return NULL;

  upb_Converter c;
  upb_ErrorHandler_Init(&c.err);

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

  if (!upb_Message_DoConvert(&c, dst, src, dst_mt, src_mt, extreg)) {
    dst = NULL;
  }

#ifndef NDEBUG
  if (dst) {
    // Verifies that round-tripping through wire format yields an identical
    // message.
    char* wire_buf;
    size_t wire_size;
    upb_Arena* tmp_arena = upb_Arena_New();
    upb_EncodeStatus encode_status =
        upb_Encode(src, src_mt, 0, tmp_arena, &wire_buf, &wire_size);
    UPB_ASSERT(encode_status == kUpb_EncodeStatus_Ok);
    upb_Message* decoded_msg = upb_Message_New(dst_mt, tmp_arena);
    upb_DecodeStatus decode_status = upb_Decode(
        wire_buf, wire_size, decoded_msg, dst_mt, extreg, 0, tmp_arena);
    UPB_ASSERT(decode_status == kUpb_DecodeStatus_Ok);
    UPB_ASSERT(upb_Message_IsEqual(dst, decoded_msg, dst_mt, 0));
    upb_Arena_Free(tmp_arena);
  }
#endif

  upb_Decoder_Destroy(&c.decoder, arena);
  UPB_PRIVATE(_upb_encstate_destroy)(&c.encoder);
  return dst;
}
