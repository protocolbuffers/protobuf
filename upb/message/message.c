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

UPB_NOINLINE bool UPB_PRIVATE(_upb_Message_AddUnknownSlowPath)(upb_Message* msg,
                                                               const char* data,
                                                               size_t len,
                                                               upb_Arena* arena,
                                                               bool alias) {
  {
    upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
    // Alias fast path was already checked in the inline function that calls
    // this one
    if (!alias && in && in->size) {
      upb_TaggedAuxPtr ptr = in->aux_data[in->size - 1];
      if (upb_TaggedAuxPtr_IsUnknown(ptr)) {
        upb_StringView* existing = upb_TaggedAuxPtr_UnknownData(ptr);
        if (!upb_TaggedAuxPtr_IsUnknownAliased(ptr)) {
          // If part of the existing field was deleted at the beginning, we can
          // reconstruct it by comparing the address of the end with the address
          // of the entry itself; having the non-aliased tag means that the
          // string_view and the data it points to are part of the same original
          // upb_Arena_Malloc allocation, and the end of the string view
          // represents the end of that allocation.
          size_t prev_alloc_size =
              (existing->data + existing->size) - (char*)existing;
          if (SIZE_MAX - prev_alloc_size >= len) {
            size_t new_alloc_size = prev_alloc_size + len;
            if (upb_Arena_TryExtend(arena, existing, prev_alloc_size,
                                    new_alloc_size)) {
              memcpy(UPB_PTR_AT(existing, prev_alloc_size, void), data, len);
              existing->size += len;
              return true;
            }
          }
        }
      }
    }
  }
  // TODO: b/376969853  - Add debug check that the unknown field is an overall
  // valid proto field
  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, arena)) {
    return false;
  }
  upb_StringView* view;
  if (alias) {
    view = upb_Arena_Malloc(arena, sizeof(upb_StringView));
    if (!view) return false;
    view->data = data;
  } else {
    view = upb_Arena_Malloc(arena, sizeof(upb_StringView) + len);
    if (!view) return false;
    char* copy = UPB_PTR_AT(view, sizeof(upb_StringView), char);
    memcpy(copy, data, len);
    view->data = copy;
  }
  view->size = len;
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  in->aux_data[in->size++] = alias
                                 ? upb_TaggedAuxPtr_MakeUnknownDataAliased(view)
                                 : upb_TaggedAuxPtr_MakeUnknownData(view);
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
    if (SIZE_MAX - total_len < data[i].size) {
      return false;
    }
    total_len += data[i].size;
  }

  {
    upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
    if (in && in->size) {
      upb_TaggedAuxPtr ptr = in->aux_data[in->size - 1];
      if (upb_TaggedAuxPtr_IsUnknown(ptr)) {
        upb_StringView* existing = upb_TaggedAuxPtr_UnknownData(ptr);
        if (!upb_TaggedAuxPtr_IsUnknownAliased(ptr)) {
          size_t prev_alloc_size =
              (existing->data + existing->size) - (char*)existing;
          if (SIZE_MAX - prev_alloc_size >= total_len) {
            size_t new_alloc_size = prev_alloc_size + total_len;
            if (upb_Arena_TryExtend(arena, existing, prev_alloc_size,
                                    new_alloc_size)) {
              char* copy = UPB_PTR_AT(existing, prev_alloc_size, char);
              for (size_t i = 0; i < count; i++) {
                memcpy(copy, data[i].data, data[i].size);
                copy += data[i].size;
              }
              existing->size += total_len;
              return true;
            }
          }
        }
      }
    }
  }

  if (SIZE_MAX - sizeof(upb_StringView) < total_len) return false;
  if (!UPB_PRIVATE(_upb_Message_ReserveSlot)(msg, arena)) return false;

  upb_StringView* view =
      upb_Arena_Malloc(arena, sizeof(upb_StringView) + total_len);
  if (!view) return false;
  char* copy = UPB_PTR_AT(view, sizeof(upb_StringView), char);
  view->data = copy;
  view->size = total_len;
  for (size_t i = 0; i < count; i++) {
    memcpy(copy, data[i].data, data[i].size);
    copy += data[i].size;
  }
  // TODO: b/376969853  - Add debug check that the unknown field is an overall
  // valid proto field
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  in->aux_data[in->size++] = upb_TaggedAuxPtr_MakeUnknownData(view);
  return true;
}

void _upb_Message_DiscardUnknown_shallow(upb_Message* msg) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return;
  uint32_t size = 0;
  for (uint32_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (upb_TaggedAuxPtr_IsExtension(tagged_ptr)) {
      in->aux_data[size++] = tagged_ptr;
    }
  }
  in->size = size;
}

upb_Message_DeleteUnknownStatus upb_Message_DeleteUnknown(upb_Message* msg,
                                                          upb_StringView* data,
                                                          uintptr_t* iter,
                                                          upb_Arena* arena) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  UPB_ASSERT(*iter != kUpb_Message_UnknownBegin);
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  UPB_ASSERT(in);
  UPB_ASSERT(*iter <= in->size);
  upb_TaggedAuxPtr unknown_ptr = in->aux_data[*iter - 1];
  UPB_ASSERT(upb_TaggedAuxPtr_IsUnknown(unknown_ptr));
  upb_StringView* unknown = upb_TaggedAuxPtr_UnknownData(unknown_ptr);
  if (unknown->data == data->data && unknown->size == data->size) {
    // Remove whole field
    in->aux_data[*iter - 1] = upb_TaggedAuxPtr_Null();
  } else if (unknown->data == data->data) {
    // Strip prefix
    unknown->data += data->size;
    unknown->size -= data->size;
    *data = *unknown;
    return kUpb_DeleteUnknown_IterUpdated;
  } else if (unknown->data + unknown->size == data->data + data->size) {
    // Truncate existing field
    unknown->size -= data->size;
    if (!upb_TaggedAuxPtr_IsUnknownAliased(unknown_ptr)) {
      in->aux_data[*iter - 1] =
          upb_TaggedAuxPtr_MakeUnknownDataAliased(unknown);
    }
  } else {
    UPB_ASSERT(unknown->data < data->data &&
               unknown->data + unknown->size > data->data + data->size);
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
    suffix->data = data->data + data->size;
    suffix->size = (prefix->data + prefix->size) - suffix->data;
    prefix->size = data->data - prefix->data;
  }
  return upb_Message_NextUnknown(msg, data, iter)
             ? kUpb_DeleteUnknown_IterUpdated
             : kUpb_DeleteUnknown_DeletedLast;
}

size_t upb_Message_ExtensionCount(const upb_Message* msg) {
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return 0;
  const upb_MiniTableExtension* ext;
  upb_MessageValue val;
  uintptr_t iter = kUpb_Message_ExtensionBegin;
  size_t count = 0;
  while (upb_Message_NextExtension(msg, &ext, &val, &iter)) {
    count++;
  }
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
  upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  // TODO: b/376969853 - use iterator API
  uint32_t size = in ? in->size : 0;
  for (size_t i = 0; i < size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (!upb_TaggedAuxPtr_IsExtension(tagged_ptr)) {
      continue;
    }
    const upb_Extension* ext = upb_TaggedAuxPtr_Extension(tagged_ptr);
    const upb_MiniTableExtension* e = ext->ext;
    const upb_MiniTableField* f = &e->UPB_PRIVATE(field);
    const upb_MiniTable* m2 = upb_MiniTableExtension_GetSubMessage(e);

    upb_MessageValue val;
    memcpy(&val, &(ext->data), sizeof(upb_MessageValue));

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
