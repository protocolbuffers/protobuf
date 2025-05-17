// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_DECODE_FAST_FIELD_CARDINALITY_H_
#define UPB_WIRE_DECODE_FAST_FIELD_CARDINALITY_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/types.h"
#include "upb/message/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/internal/decoder.h"

// Must be last include.
#include "upb/port/def.inc"

/* singular, oneof, repeated field handling ***********************************/

typedef struct {
  upb_Array* arr;
  void* end;
} fastdecode_arr;

typedef enum {
  FD_NEXT_ATLIMIT,
  FD_NEXT_SAMEFIELD,
  FD_NEXT_OTHERFIELD
} fastdecode_next;

typedef struct {
  void* dst;
  fastdecode_next next;
  uint32_t tag;
} fastdecode_nextret;

UPB_FORCEINLINE
void* fastdecode_resizearr(upb_Decoder* d, void* dst, fastdecode_arr* farr,
                           int valbytes) {
  if (UPB_UNLIKELY(dst == farr->end)) {
    size_t old_capacity = farr->arr->UPB_PRIVATE(capacity);
    size_t old_bytes = old_capacity * valbytes;
    size_t new_capacity = old_capacity * 2;
    size_t new_bytes = new_capacity * valbytes;
    char* old_ptr = (char*)upb_Array_MutableDataPtr(farr->arr);
    char* new_ptr =
        (char*)upb_Arena_Realloc(&d->arena, old_ptr, old_bytes, new_bytes);
    uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
    UPB_PRIVATE(_upb_Array_SetTaggedPtr)(farr->arr, new_ptr, elem_size_lg2);
    farr->arr->UPB_PRIVATE(capacity) = new_capacity;
    dst = (void*)(new_ptr + (old_capacity * valbytes));
    farr->end = (void*)(new_ptr + (new_capacity * valbytes));
  }
  return dst;
}

UPB_FORCEINLINE
bool fastdecode_tagmatch(uint32_t tag, uint64_t data, int tagbytes) {
  if (tagbytes == 1) {
    return (uint8_t)tag == (uint8_t)data;
  } else {
    return (uint16_t)tag == (uint16_t)data;
  }
}

UPB_FORCEINLINE
void fastdecode_commitarr(void* dst, fastdecode_arr* farr, int valbytes) {
  farr->arr->UPB_PRIVATE(size) =
      (size_t)((char*)dst - (char*)upb_Array_MutableDataPtr(farr->arr)) /
      valbytes;
}

UPB_FORCEINLINE
fastdecode_nextret fastdecode_nextrepeated(upb_Decoder* d, void* dst,
                                           const char** ptr,
                                           fastdecode_arr* farr, uint64_t data,
                                           int tagbytes, int valbytes) {
  fastdecode_nextret ret;
  dst = (char*)dst + valbytes;

  if (UPB_LIKELY(!_upb_Decoder_IsDone(d, ptr))) {
    ret.tag = _upb_FastDecoder_LoadTag(*ptr);
    if (fastdecode_tagmatch(ret.tag, data, tagbytes)) {
      ret.next = FD_NEXT_SAMEFIELD;
    } else {
      fastdecode_commitarr(dst, farr, valbytes);
      ret.next = FD_NEXT_OTHERFIELD;
    }
  } else {
    fastdecode_commitarr(dst, farr, valbytes);
    d->message_is_done = true;
    ret.next = FD_NEXT_ATLIMIT;
  }

  ret.dst = dst;
  return ret;
}

UPB_FORCEINLINE
void* fastdecode_fieldmem(upb_Message* msg, uint64_t data) {
  size_t ofs = data >> 48;
  return (char*)msg + ofs;
}

UPB_FORCEINLINE
void* fastdecode_getfield(upb_Decoder* d, const char* ptr, upb_Message* msg,
                          uint64_t* data, uint64_t* hasbits,
                          fastdecode_arr* farr, int valbytes,
                          upb_DecodeFast_Cardinality card) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  switch (card) {
    case kUpb_DecodeFast_Scalar: {
      uint8_t hasbit_index = upb_DecodeFastData_GetPresence(*data);
      // Set hasbit and return pointer to scalar field.
      *hasbits |= 1ull << hasbit_index;
      return fastdecode_fieldmem(msg, *data);
    }
    case kUpb_DecodeFast_Oneof: {
      uint16_t case_ofs = upb_DecodeFastData_GetCaseOffset(*data);
      uint32_t* oneof_case = UPB_PTR_AT(msg, case_ofs, uint32_t);
      uint8_t field_number = upb_DecodeFastData_GetPresence(*data);
      *oneof_case = field_number;
      return fastdecode_fieldmem(msg, *data);
    }
    case kUpb_DecodeFast_Repeated: {
      // Get pointer to upb_Array and allocate/expand if necessary.
      uint8_t elem_size_lg2 = __builtin_ctz(valbytes);
      upb_Array** arr_p = (upb_Array**)fastdecode_fieldmem(msg, *data);
      char* begin;
      upb_DecodeFast_SetHasbits(msg, *hasbits);
      *hasbits = 0;
      if (UPB_LIKELY(!*arr_p)) {
        farr->arr = UPB_PRIVATE(_upb_Array_New)(&d->arena, 8, elem_size_lg2);
        *arr_p = farr->arr;
      } else {
        farr->arr = *arr_p;
      }
      begin = (char*)upb_Array_MutableDataPtr(farr->arr);
      farr->end = begin + (farr->arr->UPB_PRIVATE(capacity) * valbytes);
      *data = _upb_FastDecoder_LoadTag(ptr);
      return begin + (farr->arr->UPB_PRIVATE(size) * valbytes);
    }
    default:
      UPB_UNREACHABLE();
  }
}

UPB_FORCEINLINE
bool fastdecode_flippacked(uint64_t* data, int tagbytes) {
  *data ^= (0x2 ^ 0x0);  // Patch data to match packed wiretype.
  return fastdecode_checktag(*data, tagbytes);
}

#define FASTDECODE_CHECKPACKED(tagbytes, card, func)        \
  if (UPB_UNLIKELY(!fastdecode_checktag(data, tagbytes))) { \
    if (card == kUpb_DecodeFast_Repeated &&                 \
        fastdecode_flippacked(&data, tagbytes)) {           \
      UPB_MUSTTAIL return func(UPB_PARSE_ARGS);             \
    }                                                       \
    RETURN_GENERIC("packed check tag mismatch\n");          \
  }

#endif  // UPB_WIRE_DECODE_FAST_FIELD_CARDINALITY_H_
