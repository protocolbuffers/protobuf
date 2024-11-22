// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/*
** Our memory representation for parsing tables and messages themselves.
** Functions in this file are used by generated code and possibly reflection.
**
** The definitions in this file are internal to upb.
**/

#ifndef UPB_MESSAGE_INTERNAL_MESSAGE_H_
#define UPB_MESSAGE_INTERNAL_MESSAGE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/internal/extension.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

extern const float kUpb_FltInfinity;
extern const double kUpb_Infinity;
extern const double kUpb_NaN;

// Internal members of a upb_Message that track unknown fields and/or
// extensions. We can change this without breaking binary compatibility.

typedef struct upb_Message_Internal {
  // Total size of this structure, including the data that follows.
  // Must be aligned to 8, which is alignof(upb_Extension)
  uint32_t size;

  /* Offsets relative to the beginning of this structure.
   *
   * Unknown data grows forward from the beginning to unknown_end.
   * Extension data grows backward from size to ext_begin.
   * When the two meet, we're out of data and have to realloc.
   *
   * If we imagine that the final member of this struct is:
   *   char data[size - overhead];  // overhead = sizeof(upb_Message_Internal)
   *
   * Then we have:
   *   unknown data: data[0 .. (unknown_end - overhead)]
   *   extensions data: data[(ext_begin - overhead) .. (size - overhead)] */
  uint32_t unknown_end;
  uint32_t ext_begin;
  // Data follows, as if there were an array:
  //   char data[size - sizeof(upb_Message_Internal)];
} upb_Message_Internal;

#ifdef UPB_TRACING_ENABLED
UPB_API void upb_Message_LogNewMessage(const upb_MiniTable* m,
                                       const upb_Arena* arena);
UPB_API void upb_Message_SetNewMessageTraceHandler(
    void (*handler)(const upb_MiniTable*, const upb_Arena*));
#endif  // UPB_TRACING_ENABLED

// Inline version upb_Message_New(), for internal use.
UPB_INLINE struct upb_Message* _upb_Message_New(const upb_MiniTable* m,
                                                upb_Arena* a) {
#ifdef UPB_TRACING_ENABLED
  upb_Message_LogNewMessage(m, a);
#endif  // UPB_TRACING_ENABLED

  const int size = m->UPB_PRIVATE(size);
  struct upb_Message* msg = (struct upb_Message*)upb_Arena_Malloc(a, size);
  if (UPB_UNLIKELY(!msg)) return NULL;
  memset(msg, 0, size);
  return msg;
}

// Discards the unknown fields for this message only.
void _upb_Message_DiscardUnknown_shallow(struct upb_Message* msg);

// Adds unknown data (serialized protobuf data) to the given message. The data
// must represent one or more complete and well formed proto fields.
// The data is copied into the message instance.
bool UPB_PRIVATE(_upb_Message_AddUnknown)(struct upb_Message* msg,
                                          const char* data, size_t len,
                                          upb_Arena* arena, bool alias);

// Adds unknown data (serialized protobuf data) to the given message.
// The data is copied into the message instance. Data when concatenated together
// must represent one or more complete and well formed proto fields, but the
// individual spans may point only to partial fields.
bool UPB_PRIVATE(_upb_Message_AddUnknownV)(struct upb_Message* msg,
                                           upb_Arena* arena,
                                           upb_StringView data[], size_t count);

bool UPB_PRIVATE(_upb_Message_Realloc)(struct upb_Message* msg, size_t need,
                                       upb_Arena* arena);

#define kUpb_Message_UnknownBegin 0
#define kUpb_Message_ExtensionBegin 0

UPB_INLINE bool upb_Message_NextUnknown(const struct upb_Message* msg,
                                        upb_StringView* data, uintptr_t* iter) {
  const upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (in && *iter == kUpb_Message_UnknownBegin) {
    size_t len = in->unknown_end - sizeof(upb_Message_Internal);
    if (len != 0) {
      data->size = len;
      data->data = (const char*)(in + 1);
      (*iter)++;
      return true;
    }
  }
  data->size = 0;
  data->data = NULL;
  return false;
}

UPB_INLINE bool upb_Message_HasUnknown(const struct upb_Message* msg) {
  upb_StringView data;
  uintptr_t iter = kUpb_Message_UnknownBegin;
  return upb_Message_NextUnknown(msg, &data, &iter);
}

UPB_INLINE bool upb_Message_NextExtension(const struct upb_Message* msg,
                                          const upb_MiniTableExtension** out_e,
                                          upb_MessageValue* out_v,
                                          uintptr_t* iter) {
  size_t count;
  const upb_Extension* exts = UPB_PRIVATE(_upb_Message_Getexts)(msg, &count);
  size_t i = *iter;
  if (i < count) {
    // Extensions are stored in reverse wire order, so to iterate in wire order,
    // we need to iterate backwards.
    *out_e = exts[count - 1 - i].ext;
    *out_v = exts[count - 1 - i].data;
    *iter = i + 1;
    return true;
  }

  return false;
}

UPB_INLINE bool UPB_PRIVATE(_upb_Message_NextExtensionReverse)(
    const struct upb_Message* msg, const upb_MiniTableExtension** out_e,
    upb_MessageValue* out_v, uintptr_t* iter) {
  size_t count;
  const upb_Extension* exts = UPB_PRIVATE(_upb_Message_Getexts)(msg, &count);
  size_t i = *iter;
  if (i < count) {
    // Extensions are stored in reverse wire order
    *out_e = exts[i].ext;
    *out_v = exts[i].data;
    *iter = i + 1;
    return true;
  }

  return false;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_INTERNAL_MESSAGE_H_ */
