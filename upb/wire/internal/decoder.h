// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/*
 * Internal implementation details of the decoder that are shared between
 * decode.c and decode_fast.c.
 */

#ifndef UPB_WIRE_INTERNAL_DECODER_H_
#define UPB_WIRE_INTERNAL_DECODER_H_

#include <setjmp.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/mem/internal/arena.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "utf8_range.h"

// Must be last.
#include "upb/port/def.inc"

#define DECODE_NOGROUP (uint32_t)-1

typedef struct upb_Decoder {
  upb_EpsCopyInputStream input;
  const upb_ExtensionRegistry* extreg;
  upb_Message* original_msg;  // Pointer to preserve data to
  int depth;                  // Tracks recursion depth to bound stack usage.
  uint32_t end_group;  // field number of END_GROUP tag, else DECODE_NOGROUP.
  uint16_t options;
  bool missing_required;
  bool message_is_done;
  union {
    upb_Arena arena;
    void* foo[UPB_ARENA_SIZE_HACK];
  };
  upb_DecodeStatus status;
  jmp_buf err;

#ifndef NDEBUG
  const char* debug_tagstart;
  const char* debug_valstart;
#endif
} upb_Decoder;

/* Error function that will abort decoding with longjmp(). We can't declare this
 * UPB_NORETURN, even though it is appropriate, because if we do then compilers
 * will "helpfully" refuse to tailcall to it
 * (see: https://stackoverflow.com/a/55657013), which will defeat a major goal
 * of our optimizations. That is also why we must declare it in a separate file,
 * otherwise the compiler will see that it calls longjmp() and deduce that it is
 * noreturn. */
const char* _upb_FastDecoder_ErrorJmp(upb_Decoder* d, int status);

UPB_INLINE
bool _upb_Decoder_VerifyUtf8Inline(const char* ptr, int len) {
  return utf8_range_IsValid(ptr, len);
}

const char* _upb_Decoder_CheckRequired(upb_Decoder* d, const char* ptr,
                                       const upb_Message* msg,
                                       const upb_MiniTable* m);

/* x86-64 pointers always have the high 16 bits matching. So we can shift
 * left 8 and right 8 without loss of information. */
UPB_INLINE intptr_t decode_totable(const upb_MiniTable* tablep) {
  return ((intptr_t)tablep << 8) | tablep->UPB_PRIVATE(table_mask);
}

UPB_INLINE const upb_MiniTable* decode_totablep(intptr_t table) {
  return (const upb_MiniTable*)(table >> 8);
}

const char* _upb_Decoder_IsDoneFallback(upb_EpsCopyInputStream* e,
                                        const char* ptr, int overrun);

const char* _upb_Decoder_DecodeMessage(upb_Decoder* d, const char* ptr,
                                       upb_Message* msg,
                                       const upb_MiniTable* layout);

UPB_INLINE bool _upb_Decoder_IsDone(upb_Decoder* d, const char** ptr) {
  return upb_EpsCopyInputStream_IsDoneWithCallback(
      &d->input, ptr, &_upb_Decoder_IsDoneFallback);
}

UPB_INLINE const char* _upb_Decoder_BufferFlipCallback(
    upb_EpsCopyInputStream* e, const char* old_end, const char* new_start) {
  upb_Decoder* d = (upb_Decoder*)e;
  if (!old_end) _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
  return new_start;
}

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_INTERNAL_DECODER_H_ */
