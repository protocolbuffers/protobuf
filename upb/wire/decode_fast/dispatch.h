// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_DECODE_FAST_FIELD_DISPATCH_H_
#define UPB_WIRE_DECODE_FAST_FIELD_DISPATCH_H_

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "upb/message/message.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

// The standard set of arguments passed to each parsing function.
// Thanks to x86-64 calling conventions, these will stay in registers.
#define UPB_PARSE_PARAMS                                             \
  upb_Decoder *d, const char *ptr, upb_Message *msg, intptr_t table, \
      uint64_t hasbits, uint64_t data

#define UPB_PARSE_ARGS d, ptr, msg, table, hasbits, data

#define RETURN_GENERIC(m)                                 \
  /* Uncomment either of these for debugging purposes. */ \
  /* fprintf(stderr, m); */                               \
  /*__builtin_trap(); */                                  \
  return _upb_FastDecoder_DecodeGeneric(d, ptr, msg, table, hasbits, 0);

UPB_INLINE uint32_t _upb_FastDecoder_LoadTag(const char* ptr) {
  uint16_t tag;
  memcpy(&tag, ptr, 2);
  return tag;
}

UPB_INLINE UPB_PRESERVE_NONE const char* _upb_FastDecoder_TagDispatch(
    struct upb_Decoder* d, const char* ptr, upb_Message* msg, intptr_t table,
    uint64_t hasbits, uint64_t tag) {
  const upb_MiniTable* table_p = decode_totablep(table);
  uint8_t mask = table;
  size_t ofs = tag & mask;
  UPB_ASSUME((ofs & 0xf8) == ofs);

#ifdef __cplusplus
  // Unreachable, since this header is only used from C, but when the header
  // module is compiled for C++ we need to avoid a compilation error.
  UPB_UNREACHABLE();
  UPB_UNUSED(table_p);
  _upb_FastTable_Entry* ent = NULL;
#else
  const _upb_FastTable_Entry* ent = &table_p->UPB_PRIVATE(fasttable)[ofs >> 3];
#endif

  UPB_MUSTTAIL return ent->field_parser(d, ptr, msg, table, hasbits,
                                        ent->field_data ^ tag);
}

UPB_NOINLINE UPB_PRESERVE_NONE const char* upb_DecodeFast_MessageIsDoneFallback(
    UPB_PARSE_PARAMS);

UPB_FORCEINLINE UPB_PRESERVE_NONE const char* upb_DecodeFast_Dispatch(
    UPB_PARSE_PARAMS) {
  int overrun;
  upb_IsDoneStatus status =
      upb_EpsCopyInputStream_IsDoneStatus(&d->input, ptr, &overrun);

  if (UPB_UNLIKELY(status != kUpb_IsDoneStatus_NotDone)) {
    // End-of-message or end-of-buffer.
    UPB_MUSTTAIL return upb_DecodeFast_MessageIsDoneFallback(UPB_PARSE_ARGS);
  }

  // Read two bytes of tag data (for a one-byte tag, the high byte is junk).
  data = _upb_FastDecoder_LoadTag(ptr);
  _upb_Decoder_Trace(d, 'D');
  UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
bool fastdecode_checktag(uint16_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (data & 0xff) == 0;
  } else {
    return data == 0;
  }
}

UPB_FORCEINLINE
const char* fastdecode_longsize(const char* ptr, int* size) {
  int i;
  UPB_ASSERT(*size & 0x80);
  *size &= 0xff;
  for (i = 0; i < 3; i++) {
    ptr++;
    size_t byte = (uint8_t)ptr[-1];
    *size += (byte - 1) << (7 + 7 * i);
    if (UPB_LIKELY((byte & 0x80) == 0)) return ptr;
  }
  ptr++;
  size_t byte = (uint8_t)ptr[-1];
  // len is limited by 2gb not 4gb, hence 8 and not 16 as normally expected
  // for a 32 bit varint.
  if (UPB_UNLIKELY(byte >= 8)) return NULL;
  *size += (byte - 1) << 28;
  return ptr;
}

UPB_FORCEINLINE
const char* fastdecode_delimited(
    upb_Decoder* d, const char* ptr,
    upb_EpsCopyInputStream_ParseDelimitedFunc* func, void* ctx) {
  ptr++;

  // Sign-extend so varint greater than one byte becomes negative, causing
  // fast delimited parse to fail.
  int len = (int8_t)ptr[-1];

  if (!upb_EpsCopyInputStream_TryParseDelimitedFast(&d->input, &ptr, len, func,
                                                    ctx)) {
    // Slow case: Sub-message is >=128 bytes and/or exceeds the current buffer.
    // If it exceeds the buffer limit, limit/limit_ptr will change during
    // sub-message parsing, so we need to preserve delta, not limit.
    if (UPB_UNLIKELY(len & 0x80)) {
      // Size varint >1 byte (length >= 128).
      ptr = fastdecode_longsize(ptr, &len);
      if (!ptr) {
        // Corrupt wire format: size exceeded INT_MAX.
        return NULL;
      }
    }
    if (!upb_EpsCopyInputStream_CheckSize(&d->input, ptr, len)) {
      // Corrupt wire format: invalid limit.
      return NULL;
    }
    int delta = upb_EpsCopyInputStream_PushLimit(&d->input, ptr, len);
    ptr = func(&d->input, ptr, ctx);
    upb_EpsCopyInputStream_PopLimit(&d->input, ptr, delta);
  }
  return ptr;
}

UPB_FORCEINLINE
uint64_t upb_DecodeFast_LoadHasbits(upb_Message* msg) {
  return *(uint32_t*)&msg[1];
}

UPB_FORCEINLINE
void upb_DecodeFast_SetHasbits(upb_Message* msg, uint64_t hasbits) {
  // TODO: Can we use `=` instead of` |=`?
  *(uint32_t*)&msg[1] |= hasbits;
}

typedef enum {
  // Call the dispatch function using musttail.
  kUpb_DecodeFastNext_TailCallDispatch = 0,

  // Return from the function with no tail call. This is used either to signal
  // a fallback to the mini table or the end of the message if
  // d->message_is_done is true.
  kUpb_DecodeFastNext_Return = 1,

  kUpb_DecodeFastNext_Error = 2,

  // Alias for clarity in the code.
  kUpb_DecodeFastNext_FallbackToMiniTable = kUpb_DecodeFastNext_Return,

  // Tail call to the function to parse the current field.
  kUpb_DecodeFastNext_MessageIsDoneFallback = 3,

  // Tail call to the function to parse the current field, except parse it as
  // packed instead of unpacked.
  kUpb_DecodeFastNext_TailCallPacked = 4,

  // Tail call to the function to parse the current field, except parse it as
  // unpacked instead of packed.
  kUpb_DecodeFastNext_TailCallUnpacked = 5,
} upb_DecodeFastNext;

const char* upb_DecodeFast_IsDoneFallback(UPB_PARSE_PARAMS);

#define UPB_DECODEFAST_NEXTMAYBEPACKED(next, func_unpacked, func_packed)    \
  if (UPB_UNLIKELY(next != kUpb_DecodeFastNext_TailCallDispatch)) {         \
    switch (next) {                                                         \
      case kUpb_DecodeFastNext_Return:                                      \
        UPB_MUSTTAIL return _upb_FastDecoder_DecodeGeneric(UPB_PARSE_ARGS); \
      case kUpb_DecodeFastNext_Error:                                       \
        UPB_ASSERT(d->status != kUpb_DecodeStatus_Ok);                      \
        return _upb_FastDecoder_ErrorJmp2(d);                               \
      case kUpb_DecodeFastNext_MessageIsDoneFallback:                       \
        UPB_MUSTTAIL return upb_DecodeFast_MessageIsDoneFallback(           \
            UPB_PARSE_ARGS);                                                \
      case kUpb_DecodeFastNext_TailCallPacked:                              \
        UPB_MUSTTAIL return func_packed(UPB_PARSE_ARGS);                    \
      case kUpb_DecodeFastNext_TailCallUnpacked:                            \
        UPB_MUSTTAIL return func_unpacked(UPB_PARSE_ARGS);                  \
      default:                                                              \
        UPB_UNREACHABLE();                                                  \
    }                                                                       \
  }                                                                         \
  UPB_MUSTTAIL return upb_DecodeFast_Dispatch(UPB_PARSE_ARGS);

// Uncomment this to see the exit points from the fast decoder.
// #define UPB_LOG_EXITS

UPB_INLINE bool upb_DecodeFast_SetExit(upb_DecodeFastNext* next,
                                       upb_DecodeFastNext val, const char* sym,
                                       const char* file, int line) {
#ifdef UPB_LOG_EXITS
  fprintf(stderr, "Fasttable fallback @ %s:%d -> %s (%d)\n", file, line, sym,
          val);
#endif
  *next = val;
  return false;
}

UPB_INLINE bool upb_DecodeFast_SetError(upb_Decoder* d,
                                        upb_DecodeFastNext* next,
                                        upb_DecodeStatus val, const char* sym,
                                        const char* file, int line) {
#ifdef UPB_LOG_EXITS
  fprintf(stderr, "Fasttable error @ %s:%d -> %s (%d)\n", file, line, sym, val);
#endif
  d->status = val;
  *next = kUpb_DecodeFastNext_Error;
  return false;
}

// Call using the following pattern:
//    // Will return false.
//    return UPB_EXIT_FASTTABLE(kUpb_DecodeFastNext_FallbackToMiniTable);
#define UPB_DECODEFAST_EXIT(n, next) \
  upb_DecodeFast_SetExit(next, n, #n, __FILE__, __LINE__)
#define UPB_DECODEFAST_ERROR(d, st, next) \
  upb_DecodeFast_SetError(d, next, st, #st, __FILE__, __LINE__)

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_DECODE_FAST_FIELD_DISPATCH_H_
