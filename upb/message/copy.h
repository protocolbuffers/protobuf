// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_COPY_H_
#define UPB_MESSAGE_COPY_H_

#include <stdbool.h>

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

// Deep clones `msg` into `arena`, like `upb_Message_DeepClone()`, while
// enforcing `max_depth`.
//
// `max_depth` uses the same submessage nesting depth counting as
// `upb_EncodeOptions_MaxDepth` (the root message is at depth 0, map entries do
// not add an extra wrapper level, and serialized unknown fields are copied
// opaquely), except that 0 literally allows zero nested submessages rather than
// selecting a default; negative values fail immediately. Values above
// `UINT16_MAX` (65535) are clamped to `UINT16_MAX`. Callers should choose
// `max_depth` to fit their available call stack.
//
// Returns NULL on failure (both allocation failure and exceeding or passing a
// negative `max_depth` fail identically). Any partial allocations made before
// failure remain in `arena`.
UPB_NODISCARD upb_Message* upb_Message_DeepCloneWithMaxDepth(
    const upb_Message* msg, const upb_MiniTable* m, upb_Arena* arena,
    int max_depth);

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

// Deep copies the message from `src` to `dst`, allocating from `arena`, like
// `upb_Message_DeepCopy()`, while enforcing `max_depth` as in
// `upb_Message_DeepCloneWithMaxDepth()`.
//
// `dst` is cleared first (including when `max_depth` is negative). Returns
// false on failure (both allocation failure and exceeding or passing a
// negative `max_depth` fail identically), in which case `dst` remains cleared
// and any partial allocations remain in `arena`.
UPB_NODISCARD bool upb_Message_DeepCopyWithMaxDepth(upb_Message* dst,
                                                    const upb_Message* src,
                                                    const upb_MiniTable* m,
                                                    upb_Arena* arena,
                                                    int max_depth);

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
