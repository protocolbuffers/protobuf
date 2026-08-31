// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Public APIs for message operations that do not depend on the schema.
//
// MiniTable-based accessors live in accessors.h.

#ifndef UPB_MESSAGE_MESSAGE_H_
#define UPB_MESSAGE_MESSAGE_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/message.h"
#include "upb/message/internal/types.h"
#include "upb/mini_table/extension.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct upb_Message upb_Message;

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new message with the given mini_table on the given arena.
UPB_NODISCARD UPB_API upb_Message* upb_Message_New(const upb_MiniTable* m,
                                                   upb_Arena* arena);

//
// Unknown data may be stored non-contiguously. Each segment stores a block of
// unknown fields. To iterate over segments:
//
//   uintptr_t iter = kUpb_Message_UnknownBegin;
//   upb_StringView data;
//   while (upb_Message_NextUnknown(msg, &data, &iter)) {
//     // Use data
//   }
// Iterates in the order unknown fields were parsed.

#define kUpb_Message_UnknownBegin 0
#define kUpb_Message_ExtensionBegin 0

// TODO: b/510055656 - Legacy API that works with messages that only have
// unknown data in upb_StringView format. Use `upb_Message_NextUnknown2` for
// messages that may have non-canonical extensions.
UPB_INLINE bool upb_Message_NextUnknown(const upb_Message* msg,
                                        upb_StringView* data, uintptr_t* iter);

UPB_INLINE bool upb_Message_HasUnknown(const upb_Message* msg) {
  const upb_Message_Internal* in = UPB_PRIVATE(_upb_Message_GetInternal)(msg);
  if (!in) return false;
  for (size_t i = 0; i < in->size; i++) {
    upb_TaggedAuxPtr tagged_ptr = in->aux_data[i];
    if (!upb_TaggedAuxPtr_IsNull(tagged_ptr) &&
        !upb_TaggedAuxPtr_IsSemanticallyKnown(tagged_ptr)) {
      return true;
    }
  }
  return false;
}

// Returns the number of extensions present in this message.
size_t upb_Message_ExtensionCount(const upb_Message* msg);

// Iterates extensions in wire order
UPB_INLINE bool upb_Message_NextExtension(const upb_Message* msg,
                                          const upb_MiniTableExtension** out_e,
                                          upb_MessageValue* out_v,
                                          uintptr_t* iter);

// Iterates extensions in reverse wire order
UPB_INLINE bool UPB_PRIVATE(_upb_Message_NextExtensionReverse)(
    const struct upb_Message* msg, const upb_MiniTableExtension** out_e,
    upb_MessageValue* out_v, uintptr_t* iter);

#define kUpb_Message_SerializableFieldBegin 0

// Iterates over all fields in the message that are set/present.
//
// NOTE: Unset/NULL repeated fields and maps are not considered set and will be
// ignored. However, allocated repeated fields and maps (non-NULL pointer) with
// zero elements are considered set/present and will be returned by this
// iterator. Callers that need to skip empty collections should check their
// sizes explicitly.
//
// To start iterating, set `iter = kUpb_Message_SerializableFieldBegin`.
// Returns true if a serializable field was found, sets `*f` to that field,
// and updates `*iter`. Returns false when iteration is complete.
//
//   const upb_MiniTableField* f;
//   uintptr_t iter = kUpb_Message_SerializableFieldBegin;
//   while (upb_Message_NextSerializableField(msg, mt, &f, &iter)) {
//     // ...
//   }
UPB_NODISCARD UPB_API bool upb_Message_NextSerializableField(
    const upb_Message* msg, const upb_MiniTable* mt,
    const upb_MiniTableField** f, uintptr_t* iter);

// Mark a message and all of its descendents as frozen/immutable.
UPB_API void upb_Message_Freeze(upb_Message* msg, const upb_MiniTable* m);

// Returns whether a message has been frozen.
UPB_API_INLINE bool upb_Message_IsFrozen(const upb_Message* msg);

#ifdef UPB_TRACING_ENABLED
UPB_API void upb_Message_LogNewMessage(const upb_MiniTable* m,
                                       const upb_Arena* arena);

UPB_API void upb_Message_SetNewMessageTraceHandler(
    void (*handler)(const upb_MiniTable* m, const upb_Arena* arena));
#endif  // UPB_TRACING_ENABLED

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_MESSAGE_H_ */
