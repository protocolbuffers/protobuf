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

#include "upb/message/message.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode_fast/field_parsers.h"
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

UPB_NOINLINE
static const char* fastdecode_isdonefallback(UPB_PARSE_PARAMS) {
  int overrun = data;
  ptr = _upb_EpsCopyInputStream_IsDoneFallbackInline(
      &d->input, ptr, overrun, _upb_Decoder_BufferFlipCallback);
  data = _upb_FastDecoder_LoadTag(ptr);
  UPB_MUSTTAIL return _upb_FastDecoder_TagDispatch(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
const char* fastdecode_dispatch(UPB_PARSE_PARAMS) {
  int overrun;
  switch (upb_EpsCopyInputStream_IsDoneStatus(&d->input, ptr, &overrun)) {
    case kUpb_IsDoneStatus_Done: {
      ((uint32_t*)msg)[2] |= hasbits;  // Sync hasbits.
      const upb_MiniTable* m = decode_totablep(table);
      return UPB_UNLIKELY(m->UPB_PRIVATE(required_count))
                 ? _upb_Decoder_CheckRequired(d, ptr, msg, m)
                 : ptr;
    }
    case kUpb_IsDoneStatus_NotDone:
      break;
    case kUpb_IsDoneStatus_NeedFallback:
      data = overrun;
      UPB_MUSTTAIL return fastdecode_isdonefallback(UPB_PARSE_ARGS);
  }

  // Read two bytes of tag data (for a one-byte tag, the high byte is junk).
  data = _upb_FastDecoder_LoadTag(ptr);
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

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_DECODE_FAST_FIELD_DISPATCH_H_
