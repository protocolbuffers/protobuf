// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/message.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/accessors.h"
#include "upb/message/array.h"
#include "upb/message/internal/accessors.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/message/map.h"
#include "upb/message/value.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

upb_Message* upb_Message_New(const upb_MiniTable* m, upb_Arena* a) {
  return _upb_Message_New(m, a);
}

bool UPB_PRIVATE(_upb_Message_AddUnknown)(upb_Message* msg, const char* data,
                                          size_t len, upb_Arena* arena,
                                          bool alias) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  // TODO: b/376969853  - Add debug check that the unknown field is an overall
  // valid proto field
  if (!UPB_PRIVATE(_upb_Message_Realloc)(msg, len, arena)) return false;
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  memcpy(UPB_PTR_AT(in, in->unknown_end, char), data, len);
  in->unknown_end += len;
  return true;
}

bool UPB_PRIVATE(_upb_Message_AddUnknownV)(struct upb_Message* msg,
                                           upb_Arena* arena,
                                           upb_StringView data[],
                                           size_t count) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(count > 0);
  size_t total_len = 0;
  for (size_t i = 0; i < count; i++) {
    total_len += data[i].size;
  }
  if (!UPB_PRIVATE(_upb_Message_Realloc)(msg, total_len, arena)) return false;

  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  for (size_t i = 0; i < count; i++) {
    memcpy(UPB_PTR_AT(in, in->unknown_end, char), data[i].data, data[i].size);
    in->unknown_end += data[i].size;
  }
  // TODO: b/376969853  - Add debug check that the unknown field is an overall
  // valid proto field
  return true;
}

void _upb_Message_DiscardUnknown_shallow(upb_Message* msg) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (in) {
    in->unknown_end = sizeof(upb_Message_Internal);
  }
}

const char* upb_Message_GetUnknown(const upb_Message* msg, size_t* len) {
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (in) {
    *len = in->unknown_end - sizeof(upb_Message_Internal);
    return (char*)(in + 1);
  } else {
    *len = 0;
    return NULL;
  }
}

bool upb_Message_DeleteUnknown(upb_Message* msg, upb_StringView* data,
                               uintptr_t* iter) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(*iter == kUpb_Message_UnknownBegin + 1);
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  const char* internal_unknown_end = UPB_PTR_AT(in, in->unknown_end, char);

#ifndef NDEBUG
  size_t full_unknown_size;
  const char* full_unknown = upb_Message_GetUnknown(msg, &full_unknown_size);
  UPB_ASSERT((uintptr_t)data->data >= (uintptr_t)full_unknown);
  UPB_ASSERT((uintptr_t)data->data <
             (uintptr_t)(full_unknown + full_unknown_size));
  UPB_ASSERT((uintptr_t)(data->data + data->size) > (uintptr_t)data->data);
  UPB_ASSERT((uintptr_t)(data->data + data->size) <=
             (uintptr_t)internal_unknown_end);
#endif
  const char* end = data->data + data->size;
  size_t offset = data->data - (const char*)in;
  if (end != internal_unknown_end) {
    memmove(UPB_PTR_AT(in, offset, char), end, internal_unknown_end - end);
  }
  in->unknown_end -= data->size;
  data->size = in->unknown_end - offset;
  return data->size != 0;
}

size_t upb_Message_ExtensionCount(const upb_Message* msg) {
  size_t count;
  UPB_PRIVATE(_upb_Message_Getexts)(msg, &count);
  return count;
}

void upb_Message_Freeze(upb_Message* msg, const upb_MiniTable* m) {
  if (upb_Message_IsFrozen(msg)) return;
  UPB_PRIVATE(_upb_Message_ShallowFreeze)(msg);

  // Base Fields.
  const size_t field_count = upb_MiniTable_FieldCount(m);

  for (size_t i = 0; i < field_count; i++) {
    const upb_MiniTableField* f = upb_MiniTable_GetFieldByIndex(m, i);
    const upb_MiniTable* m2 = upb_MiniTable_SubMessage(m, f);

    switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(f)) {
      case kUpb_FieldMode_Array: {
        upb_Array* arr = upb_Message_GetMutableArray(msg, f);
        if (arr) upb_Array_Freeze(arr, m2);
        break;
      }
      case kUpb_FieldMode_Map: {
        upb_Map* map = upb_Message_GetMutableMap(msg, f);
        if (map) {
          const upb_MiniTableField* f2 = upb_MiniTable_MapValue(m2);
          const upb_MiniTable* m3 = upb_MiniTable_SubMessage(m2, f2);
          upb_Map_Freeze(map, m3);
        }
        break;
      }
      case kUpb_FieldMode_Scalar: {
        if (m2) {
          upb_Message* msg2 = upb_Message_GetMutableMessage(msg, f);
          if (msg2) upb_Message_Freeze(msg2, m2);
        }
        break;
      }
    }
  }

  // Extensions.
  uintptr_t iter = kUpb_Message_ExtensionBegin;
  const upb_MiniTableExtension* e;
  upb_MessageValue val;
  while (upb_Message_NextExtension(msg, &e, &val, &iter)) {
    const upb_MiniTableField* f = &e->UPB_PRIVATE(field);
    const upb_MiniTable* m2 = upb_MiniTableExtension_GetSubMessage(e);

    switch (UPB_PRIVATE(_upb_MiniTableField_Mode)(f)) {
      case kUpb_FieldMode_Array: {
        upb_Array* arr = (upb_Array*)val.array_val;
        if (arr) upb_Array_Freeze(arr, m2);
        break;
      }
      case kUpb_FieldMode_Map:
        UPB_UNREACHABLE();  // Maps cannot be extensions.
        break;
      case kUpb_FieldMode_Scalar:
        if (upb_MiniTableField_IsSubMessage(f)) {
          upb_Message* msg2 = (upb_Message*)val.msg_val;
          if (msg2) upb_Message_Freeze(msg2, m2);
        }
        break;
    }
  }
}
