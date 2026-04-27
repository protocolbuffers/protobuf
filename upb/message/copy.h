// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_COPY_H_
#define UPB_MESSAGE_COPY_H_

#include "upb/base/descriptor_constants.h"
#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Deep clones a message using the provided target arena.
UPB_NODISCARD upb_Message* upb_Message_DeepClone(const upb_Message* msg,
                                                 const upb_MiniTable* m,
                                                 upb_Arena* arena);

// Shallow clones a message using the provided target arena.
// `msg` must outlive the returned message since all strings, repeated fields,
// maps, and unknown fields will alias the original message.
UPB_NODISCARD upb_Message* upb_Message_ShallowClone(const upb_Message* msg,
                                                    const upb_MiniTable* m,
                                                    upb_Arena* arena);

// Deep clones array contents.
UPB_NODISCARD upb_Array* upb_Array_DeepClone(const upb_Array* array,
                                             upb_CType value_type,
                                             const upb_MiniTable* sub,
                                             upb_Arena* arena);

// Deep clones map contents.
UPB_NODISCARD upb_Map* upb_Map_DeepClone(const upb_Map* map, upb_CType key_type,
                                         upb_CType value_type,
                                         const upb_MiniTable* map_entry_table,
                                         upb_Arena* arena);

// Deep copies the message from src to dst.
UPB_NODISCARD bool upb_Message_DeepCopy(upb_Message* dst,
                                        const upb_Message* src,
                                        const upb_MiniTable* m,
                                        upb_Arena* arena);

// Shallow copies the message from src to dst.
// `src` must outlive `dst` since all strings, repeated fields, maps, and
// unknown fields will alias the original message.
UPB_NODISCARD UPB_API bool upb_Message_ShallowCopy(upb_Message* dst,
                                                   const upb_Message* src,
                                                   const upb_MiniTable* m,
                                                   upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_COPY_H_
