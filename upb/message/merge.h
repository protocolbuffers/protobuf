#ifndef GOOGLE_UPB_UPB_MESSAGE_MERGE_H__
#define GOOGLE_UPB_UPB_MESSAGE_MERGE_H__

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  /* If set, string/bytes fields, submessages (when absent in destination),
   * repeated fields, maps, and unknown fields will alias source memory when
   * possible instead of deep-copying into the destination arena.
   *
   * - Repeated fields (arrays) and maps: If absent in destination, the
   *   container pointer directly aliases the source array/map. If already
   *   present in destination, new elements/entries are appended/inserted into
   *   a destination container allocated on `arena`, but string and submessage
   *   elements/values alias their source data pointers.
   * - Submessages: If absent in destination, the pointer aliases the source
   *   submessage directly. If already present, submessages are recursively
   *   merged.
   * - Strings, bytes, and unknown fields: String views and unknown fields
   *   alias the source memory buffer.
   */
  kUpb_MergeOption_Alias = 1,
};

// Merges the contents of the source message `src` into the destination
// message `dst`.
//
// In detail:
// - Normal fields and canonical extensions are merged according to the rules of
// proto merging.
// - Unknown fields and non-canonical extensions are copied directly to the
// destination without being parsed or promoted as normal fields or canonical
// extensions.
//
// Both `dst` and `src` must conform to the schema and layout defined by `mt`.
// If the messages are defined by different (but compatible) minitables,
// encode and then decode, or use `upb_Message_Convert()` to first convert the
// source message to the destination minitable if applicable.
//
// Self-merge is not supported and will return false. This check guards
// against self-merging at any of the following levels:
// - The root messages (`dst` and `src` pointing to the same message)
// - Submessage fields recursively at any depth
// - Repeated fields (arrays)
// - Map fields
UPB_NODISCARD UPB_API bool upb_Message_MergeFrom(upb_Message* dst,
                                                 const upb_Message* src,
                                                 const upb_MiniTable* mt,
                                                 int options, upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"
#endif  // GOOGLE_UPB_UPB_MESSAGE_MERGE_H__
