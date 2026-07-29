// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/unknown_fields.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mem/internal/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/extension.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/reader.h"

// Must be last.
#include "upb/port/def.inc"

static upb_FindUnknownRet2 upb_FindUnknownRet2_ParseError(void) {
  return (upb_FindUnknownRet2){.status = kUpb_FindUnknown_ParseError};
}

upb_FindUnknownRet2 upb_Message_FindUnknown2(const struct upb_Message* msg,
                                             uint32_t field_number,
                                             int depth_limit) {
  depth_limit = depth_limit ? depth_limit : 100;
  upb_FindUnknownRet2 ret;
  ret.iter = kUpb_Message_UnknownBegin;
  while (upb_Message_NextUnknown2(msg, &ret.unknown, &ret.iter)) {
    if (ret.unknown.type == kUpb_MessageUnknownType_StringView) {
      upb_EpsCopyInputStream stream;
      const char* ptr = ret.unknown.value.bytes.data;
      upb_EpsCopyInputStream_Init(&stream, &ptr, ret.unknown.value.bytes.size);

      while (!upb_EpsCopyInputStream_IsDone(&stream, &ptr)) {
        uint32_t tag;
        const char* unknown_begin = ptr;
        ptr = upb_WireReader_ReadTag(ptr, &tag, &stream);
        if (!ptr) return upb_FindUnknownRet2_ParseError();
        if (field_number == upb_WireReader_GetFieldNumber(tag)) {
          upb_StringView data;
          ret.status = kUpb_FindUnknown_Ok;
          upb_EpsCopyCapture capture;
          upb_EpsCopyCapture_Start(&capture, &stream, unknown_begin);
          ptr = _upb_WireReader_SkipValue(ptr, tag, depth_limit, &stream);
          if (!ptr || !upb_EpsCopyCapture_End(&capture, &stream, ptr, &data)) {
            return upb_FindUnknownRet2_ParseError();
          }
          ret.unknown.value.bytes = data;
          return ret;
        }

        ptr = _upb_WireReader_SkipValue(ptr, tag, depth_limit, &stream);
        if (!ptr) return upb_FindUnknownRet2_ParseError();
      }
    } else if (ret.unknown.type ==
               kUpb_MessageUnknownType_NonCanonicalExtension) {
      uint32_t ext_field_number =
          upb_MiniTableExtension_Number(ret.unknown.value.extension->ext);
      if (ext_field_number == field_number) {
        ret.status = kUpb_FindUnknown_Ok;
        return ret;
      }
    }
  }
  ret.status = kUpb_FindUnknown_NotPresent;
  ret.unknown.type = kUpb_MessageUnknownType_StringView;
  ret.unknown.value.bytes.data = NULL;
  ret.unknown.value.bytes.size = 0;
  ret.iter = kUpb_Message_UnknownBegin;
  return ret;
}

upb_Message_DeleteUnknownStatus upb_Message_DeleteUnknown2(
    struct upb_Message* msg, struct upb_MessageUnknown* data, uintptr_t* iter,
    struct upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(*iter != kUpb_Message_UnknownBegin);
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  UPB_ASSERT(in);
  UPB_ASSERT(*iter <= in->size);
  upb_TaggedAuxPtr unknown_ptr = in->aux_data[*iter - 1];

  if (data->type == kUpb_MessageUnknownType_NonCanonicalExtension) {
    UPB_ASSERT(upb_TaggedAuxPtr_IsNonCanonicalExtension(unknown_ptr));
    // When the unknown is a non-canonical extension, we just remove it from the
    // aux data array.
    in->aux_data[*iter - 1] = upb_TaggedAuxPtr_Null();
    return upb_Message_NextUnknown2(msg, data, iter)
               ? kUpb_DeleteUnknown_IterUpdated
               : kUpb_DeleteUnknown_DeletedLast;
  }

  UPB_ASSERT(upb_TaggedAuxPtr_IsUnknownStringView(unknown_ptr));
  upb_StringView* unknown = upb_TaggedPtrAux_StringViewRepr(unknown_ptr);
  UPB_ASSERT(data->type == kUpb_MessageUnknownType_StringView);
  upb_StringView* data_bytes = &data->value.bytes;
  if (unknown->data == data_bytes->data && unknown->size == data_bytes->size) {
    // Remove whole field
    in->aux_data[*iter - 1] = upb_TaggedAuxPtr_Null();
  } else if (unknown->data == data_bytes->data) {
    // Strip prefix
    unknown->data += data_bytes->size;
    unknown->size -= data_bytes->size;
    *data_bytes = *unknown;
    return kUpb_DeleteUnknown_IterUpdated;
  } else if (unknown->data + unknown->size ==
             data_bytes->data + data_bytes->size) {
    // Truncate existing field
    unknown->size -= data_bytes->size;
    if (!upb_TaggedAuxPtr_IsUnknownAliased(unknown_ptr)) {
      in->aux_data[*iter - 1] =
          upb_TaggedAuxPtr_MakeUnknownDataAliased(unknown);
    }
  } else {
    UPB_ASSERT(unknown->data < data_bytes->data &&
               unknown->data + unknown->size >
                   data_bytes->data + data_bytes->size);
    // Split in the middle
    upb_StringView* prefix = unknown;
    upb_StringView* suffix = upb_Arena_Malloc(arena, sizeof(upb_StringView));
    if (!suffix) {
      return kUpb_DeleteUnknown_AllocFail;
    }
    if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, arena)) {
      return kUpb_DeleteUnknown_AllocFail;
    }
    in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
    if (*iter != in->size) {
      // Shift later entries down so that unknown field ordering is preserved
      memmove(&in->aux_data[*iter + 1], &in->aux_data[*iter],
              sizeof(upb_TaggedAuxPtr) * (in->size - *iter));
    }
    in->aux_data[*iter] = upb_TaggedAuxPtr_MakeUnknownDataAliased(suffix);
    if (!upb_TaggedAuxPtr_IsUnknownAliased(unknown_ptr)) {
      in->aux_data[*iter - 1] = upb_TaggedAuxPtr_MakeUnknownDataAliased(prefix);
    }
    in->size++;
    suffix->data = data_bytes->data + data_bytes->size;
    suffix->size = (prefix->data + prefix->size) - suffix->data;
    prefix->size = data_bytes->data - prefix->data;
  }
  return upb_Message_NextUnknown2(msg, data, iter)
             ? kUpb_DeleteUnknown_IterUpdated
             : kUpb_DeleteUnknown_DeletedLast;
}
