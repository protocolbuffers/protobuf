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
#include <string.h>

#include "upb/mem/arena.h"
#include "upb/message/array.h"
#include "upb/message/internal/array.h"
#include "upb/message/internal/types.h"
#include "upb/message/message.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/eps_copy_input_stream.h"
#include "upb/wire/internal/decoder.h"
#include "upb/wire/types.h"

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
    case kUpb_DecodeFast_Repeated:
    case kUpb_DecodeFast_Packed: {
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
    if (upb_DecodeFast_IsRepeated(card) &&                  \
        fastdecode_flippacked(&data, tagbytes)) {           \
      UPB_MUSTTAIL return func(UPB_PARSE_ARGS);             \
    }                                                       \
    RETURN_GENERIC("packed check tag mismatch\n");          \
  }

// --- New cardinality functions ---

// Old functions will be removed once the new ones are in use.
//
// The new functions start from the premise that we should never hit an arena
// fallback path from the fasttable parser.
//
// We also use the new calling convention where we return an integer indicating
// the next function to call.  This is to work around musttail limitations
// without forcing all fasttable code to be in macros.

typedef struct {
  void* dst;  // For all fields, where to write the data.

  // For repeated fields.
  upb_Array* arr;
  void* end;
  int added_elems;
  uint16_t expected_tag;
} upb_DecodeFastField;

UPB_FORCEINLINE
void upb_DecodeFastField_AddArraySize(upb_DecodeFastField* field, int elems) {
  field->arr->UPB_PRIVATE(size) += elems;
}

UPB_FORCEINLINE
int upb_DecodeFast_MaskTag(uint16_t data, upb_DecodeFast_TagSize tagsize) {
  if (tagsize == kUpb_DecodeFast_Tag1Byte) {
    return data & 0xff;
  } else {
    return data;
  }
}

UPB_FORCEINLINE
bool upb_DecodeFast_MaskedTagIsZero(uint16_t data,
                                    upb_DecodeFast_TagSize tagsize) {
  return upb_DecodeFast_MaskTag(data, tagsize) == 0;
}

UPB_FORCEINLINE
bool upb_DecodeFast_CheckTag(uint16_t data, upb_DecodeFast_TagSize tagsize,
                             upb_DecodeFastNext* next) {
  // The dispatch sequence xors the actual tag with the expected tag, so
  // if the masked tag is zero, we know that the tag is valid.
  if (!upb_DecodeFast_MaskedTagIsZero(data, tagsize)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }
  return true;
}

// Checks to see if the tag is packed when we were expecting unpacked, or vice
// versa.  If so, flips the tag and returns true.
UPB_FORCEINLINE
bool upb_DecodeFast_TryFlipPacked(upb_DecodeFast_Type type,
                                  upb_DecodeFast_Cardinality card,
                                  upb_DecodeFast_TagSize tagsize,
                                  uint64_t* data) {
  if (!upb_DecodeFast_IsRepeated(card)) return false;
  *data ^= kUpb_WireType_Delimited ^ upb_DecodeFast_WireType(type);
  return upb_DecodeFast_MaskedTagIsZero(*data, tagsize);
}

UPB_FORCEINLINE
bool upb_DecodeFast_CheckPackableTag(upb_DecodeFast_Type type,
                                     upb_DecodeFast_Cardinality card,
                                     upb_DecodeFast_TagSize tagsize,
                                     uint64_t* data,
                                     upb_DecodeFastNext next_if_flip,
                                     upb_DecodeFastNext* next) {
  if (!upb_DecodeFast_IsRepeated(card)) {
    return upb_DecodeFast_CheckTag(*data, tagsize, next);
  }
  if (UPB_UNLIKELY(!upb_DecodeFast_MaskedTagIsZero(*data, tagsize))) {
    if (upb_DecodeFast_TryFlipPacked(type, card, tagsize, data)) {
      // We can jump directly to the decoder for the flipped tag.
      return UPB_DECODEFAST_EXIT(next_if_flip, next);
    }
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }
  return true;
}

UPB_FORCEINLINE
bool upb_DecodeFast_GetArrayForAppend(upb_Decoder* d, const char* ptr,
                                      upb_Message* msg, uint64_t data,
                                      uint64_t* hasbits,
                                      upb_DecodeFastField* field,
                                      upb_DecodeFast_Type type, int elems,
                                      upb_DecodeFastNext* next) {
  UPB_ASSERT(elems > 0);

  upb_Array** arr_p =
      UPB_PTR_AT(msg, upb_DecodeFastData_GetOffset(data), upb_Array*);
  int elem_size_lg2 = upb_DecodeFast_ValueBytesLg2(type);

  // Sync hasbits so we don't have to preserve them across the repeated field.
  upb_DecodeFast_SetHasbits(msg, *hasbits);
  *hasbits = 0;

  if (UPB_LIKELY(!*arr_p)) {
    // upb_Array does not exist yet.  Create if with an appropriate initial
    // capacity, as long as the arena has enough size in the current block.
    int start_cap = 8;

    // A few arbitrary choices on the initial capacity, could be tuned later.
    while (start_cap < elems) start_cap *= 2;

    upb_Array* arr =
        UPB_PRIVATE(_upb_Array_TryFastNew)(&d->arena, start_cap, elem_size_lg2);
    if (!arr) {
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    }
    *arr_p = arr;
  } else if (upb_Array_Capacity(*arr_p) - upb_Array_Size(*arr_p) <
             (size_t)elems) {
    // upb_Array exists, but is too small.  Expand it as long as the arena
    // has enough size in the current block.
    int new_size = upb_Array_Size(*arr_p) + elems;
    if (!UPB_PRIVATE(_upb_Array_TryFastRealloc)(*arr_p, new_size, elem_size_lg2,
                                                &d->arena)) {
      return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    }
  }

  void* start = upb_Array_MutableDataPtr(*arr_p);
  int valbytes = upb_DecodeFast_ValueBytes(type);
  field->arr = *arr_p;
  field->dst = UPB_PTR_AT(start, upb_Array_Size(*arr_p) * valbytes, void);
  field->end = UPB_PTR_AT(start, upb_Array_Capacity(*arr_p) * valbytes, void);
  field->added_elems = 0;
  field->expected_tag = _upb_FastDecoder_LoadTag(ptr);

  return true;
}

UPB_FORCEINLINE
bool Upb_DecodeFast_GetField(upb_Decoder* d, const char* ptr, upb_Message* msg,
                             uint64_t data, uint64_t* hasbits,
                             upb_DecodeFastNext* ret,
                             upb_DecodeFastField* field,
                             upb_DecodeFast_Type type,
                             upb_DecodeFast_Cardinality card) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  switch (card) {
    case kUpb_DecodeFast_Scalar: {
      // Set hasbit and return pointer to scalar field.
      field->dst = UPB_PTR_AT(msg, upb_DecodeFastData_GetOffset(data), char);
      uint8_t hasbit_index = upb_DecodeFastData_GetPresence(data);
      *hasbits |= 1ull << hasbit_index;
      return true;
    }
    case kUpb_DecodeFast_Oneof: {
      field->dst = UPB_PTR_AT(msg, upb_DecodeFastData_GetOffset(data), char);
      uint16_t case_ofs = upb_DecodeFastData_GetCaseOffset(data);
      uint32_t* oneof_case = UPB_PTR_AT(msg, case_ofs, uint32_t);
      uint8_t field_number = upb_DecodeFastData_GetPresence(data);
      *oneof_case = field_number;
      return true;
    }
    case kUpb_DecodeFast_Repeated:
    case kUpb_DecodeFast_Packed:
      return upb_DecodeFast_GetArrayForAppend(d, ptr, msg, data, hasbits, field,
                                              type, 1, ret);
    default:
      UPB_UNREACHABLE();
  }
}

UPB_FORCEINLINE
bool upb_DecodeFast_TagMatches(uint16_t expected, uint16_t tag,
                               upb_DecodeFast_TagSize tagsize) {
  if (tagsize == kUpb_DecodeFast_Tag1Byte) {
    return (uint8_t)tag == (uint8_t)expected;
  } else {
    return (uint16_t)tag == (uint16_t)expected;
  }
}

UPB_FORCEINLINE
bool upb_DecodeFast_DoNextRepeated(upb_Decoder* d, const char** ptr,
                                   uint64_t data, upb_DecodeFastNext* next,
                                   upb_DecodeFastField* field,
                                   upb_DecodeFast_Type type,
                                   upb_DecodeFast_Cardinality card,
                                   upb_DecodeFast_TagSize tagsize) {
  int overrun;
  if (UPB_UNLIKELY(
          upb_EpsCopyInputStream_IsDoneStatus(&d->input, *ptr, &overrun) !=
          kUpb_IsDoneStatus_NotDone)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_MessageIsDoneFallback, next);
  }

  uint16_t tag = _upb_FastDecoder_LoadTag(*ptr);

  if (!upb_DecodeFast_TagMatches(field->expected_tag, tag, tagsize)) {
    // A different tag is encountered; perform regular dispatch.
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_TailCallDispatch, next);
  }

  field->dst = UPB_PTR_AT(field->dst, upb_DecodeFast_ValueBytes(type), char);

  if (field->dst == field->end) {
    // Out of arena memory; fall back to MiniTable decoder which will realloc.
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_Return, next);
  }

  // Parse another instance of the repeated field.
  return true;
}

UPB_FORCEINLINE
bool upb_DecodeFast_NextRepeated(upb_Decoder* d, const char** ptr,
                                 uint64_t data, upb_DecodeFastNext* next,
                                 upb_DecodeFastField* field,
                                 upb_DecodeFast_Type type,
                                 upb_DecodeFast_Cardinality card,
                                 upb_DecodeFast_TagSize tagsize) {
  if (!upb_DecodeFast_IsRepeated(card)) {
    // No repetition is possible; perform regular dispatch.
    *next = kUpb_DecodeFastNext_TailCallDispatch;
    return false;
  }

  field->added_elems++;

  if (!upb_DecodeFast_DoNextRepeated(d, ptr, data, next, field, type, card,
                                     tagsize)) {
    // Commit elements already added to the array.
    upb_DecodeFastField_AddArraySize(field, field->added_elems);
    return false;
  }

  return true;
}

UPB_FORCEINLINE
bool upb_DecodeFast_DecodeSize(upb_Decoder* d, const char** pp, int* size,
                               upb_DecodeFastNext* next) {
  const char* ptr = *pp;
  if ((ptr[0] & 0x80) == 0) {
    *pp = ptr + 1;
    *size = (uint8_t)*ptr;
    return true;
  } else if ((ptr[1] & 0x80) == 0) {
    *pp = ptr + 2;
    *size = ((uint8_t)ptr[1] << 7) | (ptr[0] & 0x7f);
    return true;
  }
  // We don't know if this is valid wire format or not, we didn't look at
  // enough bytes to know if the varint is encoded overlong or the value
  // is too large for the current message.  So we let the MiniTable decoder
  // handle it.
  return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
}

UPB_FORCEINLINE
bool upb_DecodeFast_DecodeShortSizeForImmediateRead(upb_Decoder* d,
                                                    const char** ptr, int* size,
                                                    upb_DecodeFastNext* next) {
  if (!upb_DecodeFast_DecodeSize(d, ptr, size, next)) return false;
  if (!upb_EpsCopyInputStream_CheckDataSizeAvailable(&d->input, *ptr, *size)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }
  return true;
}

UPB_FORCEINLINE
void upb_DecodeFast_InlineMemcpy(void* dst, const char* src, size_t size) {
  // Disabled for now because we haven't yet measured a benefit to justify
  // the additional complexity.
#if false && defined(__x86_64__) && defined(__GNUC__) && \
    !UPB_HAS_FEATURE(memory_sanitizer)
  // This is nearly as fast as memcpy(), but saves us from calling an external
  // function and spilling all our registers.
  __asm__ __volatile__("rep movsb"
                       : "+D"(dst), "+S"(src), "+c"(size)::"memory");
#else
  memcpy(dst, src, size);
#endif
}

#endif  // UPB_WIRE_DECODE_FAST_FIELD_CARDINALITY_H_
