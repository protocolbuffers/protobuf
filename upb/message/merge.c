#include "upb/message/merge.h"

#include <stdbool.h>
#include <stddef.h>
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
#include "upb/message/internal/array.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/unknown_fields.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct {
  upb_ErrorHandler* err;
  int options;
  upb_Arena* arena;
  int depth;
} upb_MergeCtx;

static upb_Message* upb_Merge_CloneOrAliasSubMessage(
    upb_MergeCtx* ctx, const upb_Message* sub_msg,
    const upb_MiniTable* sub_mt) {
  if (sub_msg == NULL) return NULL;
  if (ctx->options & kUpb_MergeOption_Alias) {
    return (upb_Message*)sub_msg;
  }
  upb_Message* ret = upb_Message_DeepClone(sub_msg, sub_mt, ctx->arena);
  if (!ret) upb_ErrorHandler_ThrowError(ctx->err, 1);
  return ret;
}

static upb_Array* upb_Merge_CloneOrAliasArray(upb_MergeCtx* ctx,
                                              const upb_Array* src_arr,
                                              upb_CType value_type,
                                              const upb_MiniTable* sub_mt) {
  if (src_arr == NULL) return NULL;
  if (ctx->options & kUpb_MergeOption_Alias) {
    return (upb_Array*)src_arr;
  }
  upb_Array* ret = upb_Array_DeepClone(src_arr, value_type, sub_mt, ctx->arena);
  if (!ret) upb_ErrorHandler_ThrowError(ctx->err, 1);
  return ret;
}

static const upb_Map* upb_Merge_CloneOrAliasMap(
    upb_MergeCtx* ctx, const upb_Map* src_map, upb_CType key_type,
    upb_CType value_type, const upb_MiniTable* map_entry_mt) {
  if (src_map == NULL) return NULL;
  if (ctx->options & kUpb_MergeOption_Alias) return src_map;
  upb_Map* ret = upb_Map_DeepClone(src_map, key_type, value_type, map_entry_mt,
                                   ctx->arena);
  if (!ret) upb_ErrorHandler_ThrowError(ctx->err, 1);
  return ret;
}

static upb_StringView upb_Merge_CloneOrAliasStringView(upb_MergeCtx* ctx,
                                                       upb_StringView str) {
  if (ctx->options & kUpb_MergeOption_Alias) return str;
  if (str.size == 0) return upb_StringView_FromDataAndSize(NULL, 0);

  void* cloned_data = upb_Arena_Malloc(ctx->arena, str.size);
  if (cloned_data == NULL) upb_ErrorHandler_ThrowError(ctx->err, 1);
  memcpy(cloned_data, str.data, str.size);
  return upb_StringView_FromDataAndSize(cloned_data, str.size);
}

static void upb_Merge_CloneOrAliasMessageValue(upb_MergeCtx* ctx,
                                               upb_MessageValue* value,
                                               upb_CType value_type,
                                               const upb_MiniTable* sub) {
  switch (value_type) {
    case kUpb_CType_Bool:
    case kUpb_CType_Float:
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
    case kUpb_CType_Double:
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      break;  // Scalar types are copied directly.
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      value->str_val = upb_Merge_CloneOrAliasStringView(ctx, value->str_val);
      break;
    case kUpb_CType_Message:
      value->msg_val =
          upb_Merge_CloneOrAliasSubMessage(ctx, value->msg_val, sub);
      break;
  }
}

static void upb_Message_MergeFromInternal(upb_MergeCtx* ctx, upb_Message* dst,
                                          const upb_Message* src,
                                          const upb_MiniTable* mt);

static bool upb_Array_Merge(upb_MergeCtx* ctx, upb_Array** dst_arr_ptr,
                            const upb_Array* src_arr,
                            const upb_MiniTableField* field,
                            const upb_MiniTable* sub_mt) {
  if (src_arr == NULL || upb_Array_Size(src_arr) == 0) return false;

  upb_Array* dst_arr = *dst_arr_ptr;
  if (dst_arr == src_arr) upb_ErrorHandler_ThrowError(ctx->err, 1);

  if (dst_arr == NULL) {
    // Array does not exist in destination message, so just clone/alias the
    // source array.
    *dst_arr_ptr = upb_Merge_CloneOrAliasArray(
        ctx, src_arr, upb_MiniTableField_CType(field), sub_mt);
    return true;  // Need to set the array in the destination message.
  }

  // Array already exists in destination message, so we need to append.
  upb_CType type = upb_MiniTableField_CType(field);
  const bool can_shallow_copy =
      (type != kUpb_CType_Message && type != kUpb_CType_String &&
       type != kUpb_CType_Bytes) ||
      (ctx->options & kUpb_MergeOption_Alias);
  if (can_shallow_copy) {
    if (!upb_Array_AppendAll(dst_arr, src_arr, ctx->arena)) {
      upb_ErrorHandler_ThrowError(ctx->err, 1);
    }
  } else {
    size_t src_size = upb_Array_Size(src_arr);
    size_t dst_size = upb_Array_Size(dst_arr);
    if (!upb_Array_Resize(dst_arr, dst_size + src_size, ctx->arena)) {
      upb_ErrorHandler_ThrowError(ctx->err, 1);
    }
    for (size_t i = 0; i < src_size; ++i) {
      upb_MessageValue val = upb_Array_Get(src_arr, i);
      upb_Merge_CloneOrAliasMessageValue(ctx, &val, type, sub_mt);
      upb_Array_Set(dst_arr, dst_size + i, val);
    }
  }

  return false;  // No need to set the array in the destination message.
}

// Merges an extension value from the source message into the destination
// message.
//
// Canonical extensions are merged recursively into the destination message if
// it already exists, while non-canonical extensions are copied directly.
static void upb_Message_MergeExtension(upb_MergeCtx* ctx, upb_Message* dst,
                                       const upb_MiniTableExtension* ext_def,
                                       upb_MessageValue msg_ext_val,
                                       upb_TaggedAuxType tag) {
  const upb_MiniTableField* ext_field = &ext_def->UPB_PRIVATE(field);

  if (!upb_MiniTableField_IsScalar(ext_field)) {
    UPB_ASSERT(upb_MiniTableField_IsArray(ext_field));
    const upb_Array* src_arr = (const upb_Array*)msg_ext_val.array_val;
    if (src_arr == NULL || upb_Array_Size(src_arr) == 0) return;
  }

  upb_Extension* dst_ext = NULL;
  upb_CType type = upb_MiniTableField_CType(ext_field);
  const upb_MiniTable* sub_mt = upb_MiniTableExtension_GetSubMessage(ext_def);

  // Only look up the extension if it is a canonical extension.
  // Simplify copy for non-canonical extensions.
  if (tag == kUpb_TaggedAuxType_CanonicalExtension) {
    dst_ext = (upb_Extension*)UPB_PRIVATE(_upb_Message_Getext)(dst, ext_def);
  }

  if (dst_ext) {
    // We might need to merge instead of just copying/aliasing.
    if (upb_MiniTableField_IsArray(ext_field)) {
      upb_Array* dst_arr = (upb_Array*)dst_ext->data.array_val;
      if (upb_Array_Merge(ctx, &dst_arr, msg_ext_val.array_val, ext_field,
                          sub_mt)) {
        dst_ext->data.array_val = dst_arr;
      }
      return;
    } else if (type == kUpb_CType_Message && dst_ext->data.msg_val) {
      if (msg_ext_val.msg_val != NULL) {
        upb_Message_MergeFromInternal(ctx, (upb_Message*)dst_ext->data.msg_val,
                                      msg_ext_val.msg_val, sub_mt);
      }
      return;
    }
  }

  // Simple copy/alias.

  dst_ext = UPB_PRIVATE(_upb_Message_GetOrCreateExtensionWithTag)(
      dst, ext_def, ctx->arena, tag);
  if (!dst_ext) upb_ErrorHandler_ThrowError(ctx->err, 1);

  if (upb_MiniTableField_IsArray(ext_field)) {
    dst_ext->data.array_val =
        upb_Merge_CloneOrAliasArray(ctx, msg_ext_val.array_val, type, sub_mt);
  } else {
    upb_MessageValue val = msg_ext_val;
    upb_Merge_CloneOrAliasMessageValue(ctx, &val, type, sub_mt);
    dst_ext->data = val;
  }
}

static bool upb_Map_Merge(upb_MergeCtx* ctx, upb_Map** dst_map_ptr,
                          const upb_Map* src_map,
                          const upb_MiniTableField* field,
                          const upb_MiniTable* sub_mt) {
  if (src_map == NULL || upb_Map_Size(src_map) == 0) return false;

  upb_Map* dst_map = *dst_map_ptr;
  const upb_MiniTable* map_entry_mt = sub_mt;
  const upb_MiniTableField* value_field = upb_MiniTable_MapValue(map_entry_mt);
  upb_CType value_type = upb_MiniTableField_CType(value_field);
  const upb_MiniTable* value_sub =
      value_type == kUpb_CType_Message
          ? upb_MiniTable_GetSubMessageTable(value_field)
          : NULL;

  // Self-merge is disallowed.
  if (*dst_map_ptr == src_map) upb_ErrorHandler_ThrowError(ctx->err, 1);

  if (*dst_map_ptr == NULL) {
    const upb_MiniTableField* key_field = upb_MiniTable_MapKey(map_entry_mt);
    *dst_map_ptr = upb_Merge_CloneOrAliasMap(
        ctx, src_map, upb_MiniTableField_CType(key_field), value_type,
        map_entry_mt);
    return true;  // Need to set the map in the destination message.
  }

  upb_MessageValue key, val;
  size_t iter = kUpb_Map_Begin;
  while (upb_Map_Next(src_map, &key, &val, &iter)) {
    upb_MessageValue dst_val;
    if (value_type == kUpb_CType_Message &&
        upb_Map_Get(dst_map, key, &dst_val) && dst_val.msg_val != NULL) {
      // Dst message exists, so recursively merge the submessage.
      upb_Message_MergeFromInternal(ctx, (upb_Message*)dst_val.msg_val,
                                    val.msg_val, value_sub);
    } else {
      upb_MessageValue cloned_val = val;
      upb_Merge_CloneOrAliasMessageValue(ctx, &cloned_val, value_type,
                                         value_sub);
      if (!upb_Map_Set(dst_map, key, cloned_val, ctx->arena)) {
        upb_ErrorHandler_ThrowError(ctx->err, 1);
      }
    }
  }

  return false;  // No need to set the map in the destination message.
}

static void upb_Message_MergeField(upb_MergeCtx* ctx, upb_Message* dst,
                                   const upb_Message* src,
                                   const upb_MiniTableField* field) {
  upb_MessageValue dfl = {0};
  upb_MessageValue src_val = upb_Message_GetField(src, field, dfl);
  const upb_MiniTable* sub_mt = upb_MiniTable_SubMessage(field);

  switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(field)) {
    case kUpb_FieldMode_Scalar: {
      if (upb_MiniTableField_CType(field) == kUpb_CType_Message) {
        if (!src_val.msg_val) break;
        upb_Message* dst_sub = upb_Message_GetMutableMessage(dst, field);
        if (dst_sub) {
          // Dst message exists, so recursively merge the submessage.
          upb_Message_MergeFromInternal(ctx, dst_sub, src_val.msg_val, sub_mt);
        } else {
          // Copy src value to dst.
          upb_Merge_CloneOrAliasMessageValue(ctx, &src_val, kUpb_CType_Message,
                                             sub_mt);
          upb_Message_SetBaseField(dst, field, &src_val);
        }
      } else {
        // Copy src value to dst.
        upb_Merge_CloneOrAliasMessageValue(
            ctx, &src_val, upb_MiniTableField_CType(field), sub_mt);
        upb_Message_SetBaseField(dst, field, &src_val);
      }
      break;
    }
    case kUpb_FieldMode_Array: {
      upb_Array* dst_arr = upb_Message_GetMutableArray(dst, field);
      if (upb_Array_Merge(ctx, &dst_arr, src_val.array_val, field, sub_mt)) {
        upb_Message_SetBaseFieldArray(dst, field, dst_arr, sub_mt);
      }
      break;
    }
    case kUpb_FieldMode_Map: {
      upb_Map* dst_map = upb_Message_GetMutableMap(dst, field);
      if (upb_Map_Merge(ctx, &dst_map, src_val.map_val, field, sub_mt)) {
        upb_Message_SetBaseFieldMap(dst, field, dst_map, sub_mt);
      }
      break;
    }
  }
}

static void upb_Message_MergeFromInternal(upb_MergeCtx* ctx, upb_Message* dst,
                                          const upb_Message* src,
                                          const upb_MiniTable* mt) {
  // Self-merge is disallowed.
  if (dst == NULL || dst == src) upb_ErrorHandler_ThrowError(ctx->err, 1);
  // If src is NULL, we don't need to merge anything.
  if (src == NULL) return;
  if (--ctx->depth == 0) upb_ErrorHandler_ThrowError(ctx->err, 1);

  const upb_MiniTableField* field = NULL;
  uintptr_t iter = kUpb_Message_SerializableFieldBegin;
  while (upb_Message_NextSerializableField(src, mt, &field, &iter)) {
    upb_Message_MergeField(ctx, dst, src, field);
  }

  // Merge extensions (canonical extensions only).
  const upb_MiniTableExtension* ext = NULL;
  upb_MessageValue ext_val;
  uintptr_t ext_iter = kUpb_Message_ExtensionBegin;
  while (upb_Message_NextExtension(src, &ext, &ext_val, &ext_iter)) {
    upb_Message_MergeExtension(ctx, dst, ext, ext_val,
                               kUpb_TaggedAuxType_CanonicalExtension);
  }

  // Merge unknown fields & non-canonical extensions.
  uintptr_t unknown_iter = kUpb_Message_UnknownBegin;
  upb_MessageUnknown unknown;
  upb_AddUnknownMode unknown_mode = (ctx->options & kUpb_MergeOption_Alias)
                                        ? kUpb_AddUnknown_Alias
                                        : kUpb_AddUnknown_Copy;
  while (upb_Message_NextUnknown2(src, &unknown, &unknown_iter)) {
    if (unknown.type == kUpb_MessageUnknownType_StringView) {
      upb_StringView data = unknown.value.bytes;
      if (!UPB_PRIVATE(_upb_Message_AddUnknown)(dst, data.data, data.size,
                                                ctx->arena, unknown_mode)) {
        upb_ErrorHandler_ThrowError(ctx->err, 1);
      }
    } else {
      UPB_ASSERT(unknown.type == kUpb_MessageUnknownType_NonCanonicalExtension);
      const upb_Extension* ext = unknown.value.extension;
      upb_Message_MergeExtension(ctx, dst, ext->ext, ext->data,
                                 kUpb_TaggedAuxType_NonCanonicalExtension);
    }
  }

  ctx->depth++;
}

static bool upb_Message_DoMerge(upb_MergeCtx* ctx, upb_Message* dst,
                                const upb_Message* src,
                                const upb_MiniTable* mt) {
  if (UPB_SETJMP(ctx->err->buf) == 0) {
    upb_Message_MergeFromInternal(ctx, dst, src, mt);
    return true;
  } else {
    return false;
  }
}

bool upb_Message_MergeFrom(upb_Message* dst, const upb_Message* src,
                           const upb_MiniTable* mt, int options,
                           upb_Arena* arena) {
  upb_MergeCtx ctx = {
      .options = options,
      .arena = arena,
      .depth = 100,
  };
  upb_ErrorHandler err;
  upb_ErrorHandler_Init(&err);
  ctx.err = &err;
  return upb_Message_DoMerge(&ctx, dst, src, mt);
}
