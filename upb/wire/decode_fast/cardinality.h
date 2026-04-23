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

#if UPB_TRACE_FASTDECODER
#include "upb/wire/decode_fast/select.h"
#endif

// Must be last include.
#include "upb/port/def.inc"

// We use the new calling convention where we return an integer indicating the
// next function to call.  This is to work around musttail limitations without
// forcing all fasttable code to be in macros.

typedef enum {
  kUpb_DecodeFastNext_Dispatch = 0,

  // Fallback to the MiniTable decoder. This is used either to signal a fallback
  // to the mini table or the end of the message if d->message_is_done is true.
  kUpb_DecodeFastNext_FallbackToMiniTable = 1,

  // Signal an error.
  kUpb_DecodeFastNext_Error = 2,

  // Handle the case where ptr >= limit, which is either end-of-message or
  // end-of-buffer.
  kUpb_DecodeFastNext_MessageIsDoneFallback = 3,

  // Tail call to the function to parse the current field, except parse it as
  // packed instead of unpacked.
  kUpb_DecodeFastNext_TailCallPacked = 4,

  // Tail call to the function to parse the current field, except parse it as
  // unpacked instead of packed.
  kUpb_DecodeFastNext_TailCallUnpacked = 5,
} upb_DecodeFastNext;

UPB_INLINE bool upb_DecodeFast_SetExit(upb_DecodeFastNext* next,
                                       upb_DecodeFastNext val, const char* sym,
                                       const char* file, int line) {
#ifdef UPB_TRACE_FASTDECODER
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
#ifdef UPB_TRACE_FASTDECODER
  fprintf(stderr, "Fasttable error @ %s:%d -> %s (%d)\n", file, line, sym, val);
#endif
  d->err->code = val;
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

#define UPB_DECODEFAST_NEXTMAYBEPACKED(next, func_unpacked, func_packed)  \
  switch (next) {                                                         \
    case kUpb_DecodeFastNext_Dispatch:                                    \
      UPB_MUSTTAIL return upb_DecodeFast_Dispatch(UPB_PARSE_ARGS);        \
    case kUpb_DecodeFastNext_FallbackToMiniTable:                         \
      UPB_MUSTTAIL return _upb_FastDecoder_DecodeGeneric(UPB_PARSE_ARGS); \
    case kUpb_DecodeFastNext_Error:                                       \
      UPB_ASSERT(d->err->code != kUpb_DecodeStatus_Ok);                   \
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
  }

UPB_INLINE const char* UPB_PRESERVE_NONE
upb_DecodeFast_Unreachable(UPB_PARSE_PARAMS) {
  UPB_UNREACHABLE();
}

#define UPB_DECODEFAST_NEXT(next)                                  \
  UPB_DECODEFAST_NEXTMAYBEPACKED(next, upb_DecodeFast_Unreachable, \
                                 upb_DecodeFast_Unreachable)

UPB_FORCEINLINE
uint64_t upb_DecodeFast_LoadHasbits(upb_Message* msg) {
  return *(uint32_t*)&msg[1];
}

/* Error function that will abort decoding with longjmp(). We can't declare this
 * UPB_NORETURN, even though it is appropriate, because if we do then compilers
 * will "helpfully" refuse to tailcall to it
 * (see: https://stackoverflow.com/a/55657013), which will defeat a major goal
 * of our optimizations. That is also why we must declare it in a separate file,
 * otherwise the compiler will see that it calls longjmp() and deduce that it is
 * noreturn. */
const char* _upb_FastDecoder_ErrorJmp2(upb_Decoder* d);

UPB_INLINE
const char* _upb_FastDecoder_ErrorJmp(upb_Decoder* d, upb_DecodeStatus status) {
  d->err->code = status;
  return _upb_FastDecoder_ErrorJmp2(d);
}

// --- New cardinality functions ---

// Old functions will be removed once the new ones are in use.
//
// We use the new calling convention where we return an integer indicating the
// next function to call.  This is to work around musttail limitations without
// forcing all fasttable code to be in macros.

typedef struct {
  void* dst;
  upb_Array* arr;
  void* end;
  uint16_t expected_tag;
} upb_DecodeFastArray;

UPB_FORCEINLINE
void upb_DecodeFastField_SetArraySize(upb_DecodeFastArray* field,
                                      upb_DecodeFast_Type type) {
  field->arr->UPB_PRIVATE(size) =
      ((uintptr_t)field->dst -
       (uintptr_t)upb_Array_MutableDataPtr(field->arr)) /
      upb_DecodeFast_ValueBytes(type);
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
bool upb_DecodeFast_ArrayReserve(upb_Decoder* d, upb_Array* arr,
                                 upb_DecodeFast_Type type, int elems,
                                 upb_DecodeFastNext* next) {
  UPB_ASSERT(arr);

  size_t existing = upb_Array_Size(arr);
  if (upb_Array_Reserve(arr, existing + elems, &d->arena)) return true;

  return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_OutOfMemory, next);
}

UPB_FORCEINLINE
bool upb_DecodeFast_GetScalarField(upb_Decoder* d, const char* ptr,
                                   upb_Message* msg, uint64_t data,
                                   uint64_t* hasbits, upb_DecodeFastNext* ret,
                                   void** dst,
                                   upb_DecodeFast_Cardinality card) {
  UPB_ASSERT(!upb_Message_IsFrozen(msg));
  switch (card) {
    case kUpb_DecodeFast_Scalar: {
      // Set hasbit and return pointer to scalar field.
      *dst = UPB_PTR_AT(msg, upb_DecodeFastData_GetOffset(data), char);
      uint8_t hasbit_index = upb_DecodeFastData_GetPresence(data);
      *hasbits |= 1ull << hasbit_index;
      return true;
    }
    case kUpb_DecodeFast_Oneof: {
      *dst = UPB_PTR_AT(msg, upb_DecodeFastData_GetOffset(data), char);
      uint16_t case_ofs = upb_DecodeFastData_GetCaseOffset(data);
      uint32_t* oneof_case = UPB_PTR_AT(msg, case_ofs, uint32_t);
      uint8_t field_number = upb_DecodeFastData_GetPresence(data);
      *oneof_case = field_number;
      return true;
    }
    default:
      return false;
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
bool upb_DecodeFast_TryMatchTag(upb_Decoder* d, const char* ptr,
                                uint16_t expected, upb_DecodeFastNext* next,
                                upb_DecodeFast_TagSize tagsize) {
  int overrun;
  if (UPB_UNLIKELY(UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
                       &d->input, ptr, &overrun) !=
                   kUpb_IsDoneStatus_NotDone)) {
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_MessageIsDoneFallback, next);
  }

  uint16_t tag = _upb_FastDecoder_LoadTag(ptr);

  return upb_DecodeFast_TagMatches(expected, tag, tagsize);
}

UPB_FORCEINLINE
bool upb_DecodeFast_NextRepeated(upb_Decoder* d, const char** ptr,
                                 upb_DecodeFastNext* next,
                                 upb_DecodeFastArray* field, bool has_next,
                                 upb_DecodeFast_Type type,
                                 upb_DecodeFast_TagSize tagsize) {
  field->dst = UPB_PTR_AT(field->dst, upb_DecodeFast_ValueBytes(type), char);

  if (has_next && field->dst == field->end) {
    // Out of arena memory; fall back to MiniTable decoder which will realloc.
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
    has_next = false;
  }

  if (!has_next) {
    upb_DecodeFastField_SetArraySize(field, type);
    return false;
  }

  // Parse another instance of the repeated field.
  *ptr += upb_DecodeFast_TagSizeBytes(tagsize);
  return true;
}

UPB_FORCEINLINE
bool upb_DecodeFast_CheckTag(const char** ptr, upb_DecodeFast_Type type,
                             upb_DecodeFast_Cardinality card,
                             upb_DecodeFast_TagSize tagsize, uint64_t* data,
                             upb_DecodeFastNext flipped,
                             upb_DecodeFastNext* next) {
#if UPB_TRACE_FASTDECODER
  size_t idx = UPB_DECODEFAST_FUNCTION_IDX(type, card, tagsize);
  fprintf(stderr, "Fasttable enter (check tag) -> %s\n",
          upb_DecodeFast_GetFunctionName(idx));
#endif
  // The dispatch sequence xors the actual tag with the expected tag, so
  // if the masked tag is zero, we know that the tag is valid.
  if (UPB_UNLIKELY(!upb_DecodeFast_MaskedTagIsZero(*data, tagsize))) {
    // If this field is repeated and the field type is packable, we check
    // whether the tag can be flipped (ie. packed -> unpacked or vice versa).
    // If so, we can jump directly to the decoder for the flipped tag.
    if (flipped && upb_DecodeFast_TryFlipPacked(type, card, tagsize, data)) {
      // We can jump directly to the decoder for the flipped tag.
      return UPB_DECODEFAST_EXIT(flipped, next);
    }
    return UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackToMiniTable, next);
  }
  *ptr += upb_DecodeFast_TagSizeBytes(tagsize);
  return true;
}

UPB_FORCEINLINE
bool upb_DecodeFast_GetArrayForAppend(upb_Decoder* d, const char* ptr,
                                      upb_Message* msg, uint64_t data,
                                      uint64_t* hasbits,
                                      upb_DecodeFastArray* field,
                                      upb_DecodeFast_Type type, int elems,
                                      upb_DecodeFastNext* next) {
  UPB_ASSERT(elems > 0);

  upb_Array** arr_p =
      UPB_PTR_AT(msg, upb_DecodeFastData_GetOffset(data), upb_Array*);
  upb_Array* arr = *arr_p;
  int lg2 = upb_DecodeFast_ValueBytesLg2(type);

  // Sync hasbits so we don't have to preserve them across the repeated field.
  upb_DecodeFast_SetHasbits(msg, *hasbits);
  *hasbits = 0;

  if (UPB_LIKELY(!arr)) {
    // upb_Array does not exist yet.  Create if with an appropriate initial
    // capacity, as long as the arena has enough size in the current block.
    int start_cap = 8;

    // A few arbitrary choices on the initial capacity, could be tuned later.
    while (start_cap < elems) start_cap *= 2;

    arr = UPB_PRIVATE(_upb_Array_New)(&d->arena, start_cap, lg2);
    if (!arr) {
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_OutOfMemory, next);
    }
    *arr_p = arr;
  } else if (!upb_DecodeFast_ArrayReserve(d, arr, type, elems, next)) {
    return false;
  }

  void* start = upb_Array_MutableDataPtr(arr);
  int valbytes = upb_DecodeFast_ValueBytes(type);

  field->arr = arr;
  field->dst = UPB_PTR_AT(start, upb_Array_Size(arr) * valbytes, void);
  field->end = UPB_PTR_AT(start, upb_Array_Capacity(arr) * valbytes, void);
  field->expected_tag = _upb_FastDecoder_LoadTag(ptr);

  return true;
}

typedef bool upb_DecodeFast_Single(upb_Decoder* d, const char** ptr, void* dst,
                                   upb_DecodeFast_Type type,
                                   upb_DecodeFastNext* next, void* ctx);

UPB_FORCEINLINE
bool upb_DecodeFast_Unpacked(upb_Decoder* d, const char** ptr, upb_Message* msg,
                             uint64_t* data, uint64_t* hasbits,
                             upb_DecodeFastNext* ret, upb_DecodeFast_Type type,
                             upb_DecodeFast_Cardinality card,
                             upb_DecodeFast_TagSize tagsize,
                             upb_DecodeFast_Single* single, void* ctx) {
  const char* p = *ptr;
  if (!upb_DecodeFast_CheckTag(&p, type, card, tagsize, data,
                               kUpb_DecodeFastNext_TailCallPacked, ret)) {
    return false;
  }

  void* dst;

  if (upb_DecodeFast_GetScalarField(d, p, msg, *data, hasbits, ret, &dst,
                                    card)) {
    if (!single(d, &p, dst, type, ret, ctx)) return false;
    *ptr = p;
    _upb_Decoder_Trace(d, 'F');
    return true;
  }

  upb_DecodeFastArray arr;
  if (!upb_DecodeFast_GetArrayForAppend(d, *ptr, msg, *data, hasbits, &arr,
                                        type, 1, ret)) {
    return false;
  }

  bool next_tag_matches;
  do {
    if (!single(d, &p, arr.dst, type, ret, ctx)) {
      upb_DecodeFastField_SetArraySize(&arr, type);
      return false;
    }
    *ptr = p;
    _upb_Decoder_Trace(d, 'F');
    next_tag_matches =
        upb_DecodeFast_TryMatchTag(d, p, arr.expected_tag, ret, tagsize);
  } while (upb_DecodeFast_NextRepeated(d, &p, ret, &arr, next_tag_matches, type,
                                       tagsize));

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
bool upb_DecodeFast_Delimited(upb_Decoder* d, const char** ptr,
                              upb_EpsCopyInputStream_ParseDelimitedFunc* func,
                              upb_DecodeFastNext* ret, void* ctx) {
  const char* p = *ptr;
  int size;

  if (!upb_DecodeFast_DecodeSize(d, &p, &size, ret)) return false;

  if (upb_EpsCopyInputStream_TryParseDelimitedFast(&d->input, &p, size, func,
                                                   ctx)) {
    if (UPB_UNLIKELY(p == NULL)) goto fail;
  } else {
    ptrdiff_t delta = upb_EpsCopyInputStream_PushLimit(&d->input, p, size);
    if (UPB_UNLIKELY(delta < 0)) {
      // Corrupt wire format: invalid limit.
      *ptr = NULL;
      return UPB_DECODEFAST_ERROR(d, kUpb_DecodeStatus_Malformed, ret);
    }
    p = func(&d->input, p, size, ctx);
    if (UPB_UNLIKELY(p == NULL)) goto fail;
    upb_EpsCopyInputStream_PopLimit(&d->input, p, delta);
  }

  *ptr = p;
  return true;

fail:
  // We can't fall back to the mini table here because we may have already
  // advanced past the previous buffer.
  UPB_ASSERT(*ret != kUpb_DecodeFastNext_FallbackToMiniTable);
  *ptr = NULL;
  return false;
}

UPB_FORCEINLINE
bool upb_DecodeFast_Packed(upb_Decoder* d, const char** ptr,
                           upb_DecodeFast_Type type,
                           upb_DecodeFast_Cardinality card,
                           upb_DecodeFast_TagSize tagsize, uint64_t* data,
                           upb_EpsCopyInputStream_ParseDelimitedFunc* func,
                           upb_DecodeFastNext* ret, void* ctx) {
  const char* p = *ptr;

  if (!upb_DecodeFast_CheckTag(&p, type, card, tagsize, data,
                               kUpb_DecodeFastNext_TailCallUnpacked, ret) ||
      !upb_DecodeFast_Delimited(d, &p, func, ret, ctx)) {
    return false;
  }

  *ptr = p;
  _upb_Decoder_Trace(d, 'F');
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

// Workaround for b/177688959. We need to ensure that this function never goes
// through PLT lookup. It follows that this function may not be called by
// any other cc_library().
__attribute__((visibility("hidden"))) UPB_PRESERVE_MOST const char*
upb_DecodeFast_IsDoneFallback(upb_Decoder* d, const char* ptr);

UPB_FORCEINLINE
bool upb_DecodeFast_IsDone(upb_Decoder* d, const char** ptr) {
  int overrun;
  upb_IsDoneStatus status = UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
      &d->input, *ptr, &overrun);
  switch (status) {
    case kUpb_IsDoneStatus_Done:
      return true;
    case kUpb_IsDoneStatus_NotDone:
      return false;
    case kUpb_IsDoneStatus_NeedFallback:
      *ptr = upb_DecodeFast_IsDoneFallback(d, *ptr);
      return false;
    default:
      UPB_UNREACHABLE();
  }
}

#endif  // UPB_WIRE_DECODE_FAST_FIELD_CARDINALITY_H_
