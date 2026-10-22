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

// Deep clones `msg` into `arena`.
//
// The result shares no memory with `msg`, and `msg` may be freed or mutated as
// soon as this returns. The result references only `m` (and minitables
// reachable from it) and the extension minitables of canonical extensions
// present in `msg`, which must outlive the result. Non-canonical extensions are
// serialized into unknown fields, since their minitables may belong to a schema
// that does not outlive the result.
//
// Returns NULL on failure.
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

// Deep copies the message from `src` to `dst`, allocating from `arena`.
//
// `dst` is cleared first. The result shares no memory with `src`, and `src` may
// be freed or mutated as soon as this returns. The result references only `m`
// (and minitables reachable from it) and the extension minitables of canonical
// extensions present in `src`, which must outlive `dst`. Non-canonical
// extensions are serialized into unknown fields, since their minitables may
// belong to a schema that does not outlive `dst`.
//
// Returns false on failure, in which case `dst` is cleared.
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
