#ifndef GOOGLE_UPB_UPB_MESSAGE_MERGE_H__
#define GOOGLE_UPB_UPB_MESSAGE_MERGE_H__

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_API bool upb_Message_MergeFrom(upb_Message* dst, const upb_Message* src,
                                   const upb_MiniTable* mt,
                                   const upb_ExtensionRegistry* extreg,
                                   upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"
#endif  // GOOGLE_UPB_UPB_MESSAGE_MERGE_H__
