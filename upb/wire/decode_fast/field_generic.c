
#include <stdint.h>

#include "upb/message/message.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/internal/decoder.h"

const char* _upb_FastDecoder_DecodeGeneric(struct upb_Decoder* d,
                                           const char* ptr, upb_Message* msg,
                                           intptr_t table, uint64_t hasbits,
                                           uint64_t data) {
  (void)data;
  upb_DecodeFast_SetHasbits(msg, hasbits);
  return ptr;
}
