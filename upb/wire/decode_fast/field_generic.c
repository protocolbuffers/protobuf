
#include <stdint.h>

#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

UPB_PRESERVE_NONE static upb_FastDecoder_Return
_upb_FastDecoder_FallbackToMiniTable(struct upb_Decoder* d, const char* ptr,
                                     upb_Message* msg,
                                     const upb_MiniTable* table,
                                     uint64_t hasbits, uint64_t data,
                                     uint64_t data2) {
  (void)data2;
  upb_DecodeFast_SetHasbits(msg, hasbits);
  return (upb_FastDecoder_Return){.ptr = ptr, .pass_back = data};
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeGeneric(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext ret;
  UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, &ret);
  return _upb_FastDecoder_FallbackToMiniTable(d, ptr, msg, table, hasbits, 0,
                                              0);
}

UPB_PRESERVE_NONE upb_FastDecoder_Return _upb_FastDecoder_DecodeGenericField(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg,
    const upb_MiniTable* table, uint64_t hasbits, uint64_t data,
    uint64_t data2) {
  upb_DecodeFastNext ret;
  UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToKnownField, &ret);
  return _upb_FastDecoder_FallbackToMiniTable(d, ptr, msg, table, hasbits, data,
                                              data2);
}
