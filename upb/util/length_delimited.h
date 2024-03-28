#ifndef THIRD_PARTY_UPB_UPB_UTIL_LENGTH_DELIMITED_H_
#define THIRD_PARTY_UPB_UPB_UTIL_LENGTH_DELIMITED_H_

#include <stddef.h>

#include "upb/mem/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// Encodes the message prepended by a varint of the serialized length.
UPB_API upb_EncodeStatus upb_util_EncodeLengthDelimited(
    const upb_Message* msg, const upb_MiniTable* l, int options,
    upb_Arena* arena, char** buf, size_t* size);

UPB_API upb_DecodeStatus upb_util_DecodeLengthDelimited(
    const char* buf, size_t size, upb_Message* msg, size_t* num_bytes_read,
    const upb_MiniTable* mt, const upb_ExtensionRegistry* extreg, int options,
    upb_Arena* arena);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // THIRD_PARTY_UPB_UPB_UTIL_LENGTH_DELIMITED_H_
