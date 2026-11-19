// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>

#include "upb/message/internal/message.h"
#include "upb/message/message.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/message.h"
#include "upb/mini_table/internal/sub.h"
#include "upb/mini_table/message.h"
#include "upb/wire/decode.h"
#include "upb/wire/decode_fast/cardinality.h"
#include "upb/wire/decode_fast/combinations.h"
#include "upb/wire/decode_fast/data.h"
#include "upb/wire/decode_fast/dispatch.h"
#include "upb/wire/decode_fast/field_parsers.h"
#include "upb/wire/internal/decoder.h"

// Must be last.
#include "upb/port/def.inc"

UPB_FORCEINLINE
bool upb_DecodeFast_SingleMessage(upb_Decoder* d, const char** ptr, void* dst,
                                  upb_DecodeFast_Type type,
                                  upb_DecodeFastNext* next, void* ctx) {
  upb_DecodeFast_MessageContext* c = ctx;
  void** submsg_dst = dst;

  if (c->is_repeated || UPB_LIKELY(*submsg_dst == NULL)) {
    c->msg = *submsg_dst = _upb_Message_New(c->table, &d->arena);
    if (c->msg == NULL) {
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
    }
  } else {
    c->msg = *submsg_dst;  // Reusing non-repeated message.
  }

  return upb_DecodeFast_Delimited(d, ptr, &upb_DecodeFast_MessageData, next, c);
}

// --- Repeated message runs -------------------------------------------------
//
// For a repeated message field, every element uses the same MiniTable. Rather
// than recursing into _upb_Decoder_DecodeMessage() for each element and
// returning to this field parser in between, we descend into the submessage's
// fast decoder once, and decode all consecutive elements of the field in that
// single descent: when an element reaches its end, the fast decoder tail calls
// upb_DecodeFast_RepeatedMessageNext(), which finishes that element and
// starts the next one in place. We return to the field parser only when there
// are no more elements, or the fast decoder hit something it can't handle (in
// which case we finish the element with the MiniTable decoder, then resume the
// run).
//
// This is analogous to how the map parser loops over map entries without
// recursing into the map entry message.

UPB_FORCEINLINE
bool upb_DecodeFast_CanRunRepeatedMessage(const upb_MiniTable* m) {
  // Mirrors the conditions in _upb_Decoder_DecodeMessage() and
  // _upb_Decoder_TryDecodeMessageFast(). We don't need to check
  // kUpb_DecodeOption_DisableFastTable, since we are already in the fast
  // decoder.
  if (m->UPB_PRIVATE(table_mask) == (unsigned char)-1) return false;
  // A message with no fields never has a fast table (see
  // upb_DecodeFast_BuildTable()), so it can't take
  // _upb_Decoder_DecodeMessage()'s empty message path.
  UPB_ASSERT(upb_MiniTable_FieldCount(m) != 0);
  return true;
}

// Element limits.
//
// All elements of a run are in the same parent message, so the parent's limit
// does not change during the run, unless the buffer is flipped. We save it in
// the run when the run starts. Each element that fits in the current buffer
// (the common case) gets its limit set directly, and at the end of the run we
// restore the parent's saved limit, as upb_EpsCopyInputStream_
// TryParseDelimitedFast() does. This is cheaper than PushLimit()/PopLimit(),
// and lets upb_DecodeFast_RepeatedMessageNext() check for the end of the
// parent and the next element's size against the saved limit, without first
// storing the parent's limit back to the stream.
//
// An element that doesn't fit in the current buffer may flip the buffer, which
// invalidates the saved limit, so we decode it on its own with
// PushLimit()/PopLimit(), then save the parent's limit again.

// Called at the end of an element (or at the start of the run) with `ptr`
// pointing just past it. Returns a pointer to the next element's length prefix
// if the run continues. Otherwise ends the run, restoring the parent's limit,
// and returns NULL.
UPB_FORCEINLINE
const char* upb_DecodeFast_RepeatedMessageNextTag(
    upb_Decoder* d, upb_DecodeFast_RepeatedMessageRun* run, const char* ptr,
    uint64_t data2, upb_DecodeFast_TagSize tagsize) {
  UPB_ASSUME_SEPARATE(run, d);
  _upb_Decoder_Trace(d, 'F');
  const char* parent_limit_ptr = run->parent_limit_ptr;
  uint16_t mask = tagsize == kUpb_DecodeFast_Tag2Byte ? 0xffff : 0xff;
  // If we are at the parent's limit, or at the end of the buffer, the run is
  // over; the parent's upb_DecodeFast_Dispatch() will sort out which.
  if (UPB_LIKELY(ptr < parent_limit_ptr) &&
      UPB_LIKELY(((_upb_FastDecoder_LoadTag(ptr) ^
                   upb_DecodeFastData2_GetRepeatedMessageTag(data2)) &
                  mask) == 0)) {
    // As upb_EpsCopyInputStream_IsDoneStatus() would, since ptr is before the
    // end of the buffer.
    UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)
    (&d->input, kUpb_EpsCopyInputStream_SlopBytes);
    return ptr + upb_DecodeFast_TagSizeBytes(tagsize);
  }
  // Loaded before the stores below, so they can be loaded with one ldp.
  void* prev = run->prev;
  ptrdiff_t parent_limit = run->parent_limit;
  d->input.limit_ptr = parent_limit_ptr;
  d->input.limit = parent_limit;
  d->fast_repeated_message_run = prev;
  return NULL;
}

UPB_NOINLINE UPB_PRESERVE_NONE static upb_FastDecoder_Return
    upb_DecodeFast_RepeatedMessageNext(UPB_PARSE_PARAMS);

UPB_NOINLINE UPB_PRESERVE_NONE static upb_FastDecoder_Return
upb_DecodeFast_RepeatedMessageOutOfMemory(UPB_PARSE_PARAMS) {
  return _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
}

// The general version of upb_DecodeFast_RepeatedMessageStart(), which handles
// every case, at the cost of needing a stack frame for the calls it makes.
UPB_NOINLINE UPB_PRESERVE_NONE static upb_FastDecoder_Return
upb_DecodeFast_RepeatedMessageStartSlow(UPB_PARSE_PARAMS) {
  upb_DecodeFast_RepeatedMessageRun* run = d->fast_repeated_message_run;
  UPB_ASSUME_SEPARATE(run, d);
  // We may get here with the previous element's limit still set.
  d->input.limit_ptr = run->parent_limit_ptr;
  d->input.limit = run->parent_limit;

  if (UPB_UNLIKELY(run->dst == run->end)) {
    upb_DecodeFast_GrowResult grown = upb_DecodeFast_GrowArray(
        d, run->arr, run->dst, kUpb_DecodeFast_Message);
    run->dst = grown.dst;
    run->end = grown.end;
  }

  int size;
  upb_DecodeFastNext next;
  if (UPB_UNLIKELY(!upb_DecodeFast_DecodeSize(d, &ptr, &size, &next))) {
    return _upb_FastDecoder_ErrorJmp2(d);
  }
  upb_Message** dst = run->dst;
  upb_Message* submsg = _upb_Message_New(table, &d->arena);
  if (UPB_UNLIKELY(submsg == NULL)) {
    return _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_OutOfMemory);
  }
  *dst = submsg;
  run->dst = dst + 1;

  if (UPB_LIKELY(size <= run->parent_limit_ptr - ptr)) {
    d->input.limit_ptr = ptr + size;
    d->input.limit = d->input.limit_ptr - d->input.end;
    UPB_MUSTTAIL return upb_DecodeFast_Dispatch(d, ptr, submsg, table, 0, data,
                                                data2);
  }

  // The element doesn't fit in the current buffer.
  ptrdiff_t delta = upb_EpsCopyInputStream_PushLimit(EPS(d), ptr, size);
  upb_DecodeFast_MessageContext ctx = {table, true, submsg};
  ptr = upb_DecodeFast_MessageData(&d->input, ptr, size, &ctx);
  upb_EpsCopyInputStream_PopLimit(EPS(d), ptr, delta);
  run->parent_limit_ptr = d->input.limit_ptr;
  run->parent_limit = d->input.limit;
  UPB_MUSTTAIL return upb_DecodeFast_RepeatedMessageNext(d, ptr, submsg, table,
                                                         0, data, data2);
}

// Creates the next element and starts decoding it, with `ptr` pointing to its
// length prefix. Handles only the common case, where it makes no calls, so
// that upb_DecodeFast_RepeatedMessageNext() needs no stack frame; anything
// else tail calls upb_DecodeFast_RepeatedMessageStartSlow(), which must happen
// before any state is modified.
UPB_FORCEINLINE UPB_PRESERVE_NONE upb_FastDecoder_Return
upb_DecodeFast_RepeatedMessageStart(UPB_PARSE_PARAMS) {
  upb_DecodeFast_RepeatedMessageRun* run = d->fast_repeated_message_run;
  UPB_ASSUME_SEPARATE(run, d);
  // Loaded together (and before _upb_Message_New(), whose stores to the new
  // message the compiler can't otherwise tell apart from `run`).
  upb_Message** dst = run->dst;
  upb_Message** end = run->end;
  UPB_ASSUME_SEPARATE(run, dst);
  const char* p = ptr;
  uint32_t size = (uint8_t)p[0];
  if (UPB_LIKELY((size & 0x80) == 0)) {
    p += 1;
  } else if (UPB_LIKELY(((uint8_t)p[1] & 0x80) == 0)) {
    size = (size & 0x7f) | ((uint32_t)(uint8_t)p[1] << 7);
    p += 2;
  } else {
    UPB_MUSTTAIL return upb_DecodeFast_RepeatedMessageStartSlow(UPB_PARSE_ARGS);
  }
  upb_Arena* arena = &d->arena;
  size_t msg_size = table->UPB_PRIVATE(size);
  if (UPB_UNLIKELY(dst == end) ||
      UPB_UNLIKELY(UPB_PRIVATE(_upb_ArenaHas)(arena) < msg_size) ||
      // The element doesn't fit in the current buffer (or overruns the
      // parent).
      UPB_UNLIKELY((ptrdiff_t)size > run->parent_limit_ptr - p)) {
    UPB_MUSTTAIL return upb_DecodeFast_RepeatedMessageStartSlow(UPB_PARSE_ARGS);
  }

  upb_Message* submsg = _upb_Message_New(table, arena);
  if (UPB_UNLIKELY(submsg == NULL)) {
    // Only possible with allocation fault injection.
    UPB_MUSTTAIL return upb_DecodeFast_RepeatedMessageOutOfMemory(
        UPB_PARSE_ARGS);
  }
  *dst = submsg;
  run->dst = dst + 1;
  d->input.limit_ptr = p + size;
  d->input.limit = d->input.limit_ptr - d->input.end;
  UPB_MUSTTAIL return upb_DecodeFast_Dispatch(d, p, submsg, table, 0, data,
                                              data2);
}

// Reached from upb_DecodeFast_MessageIsDoneFallback() (through
// upb_Decoder.fast_repeated_message_next) when an element of a run is done,
// after it has synced the element's hasbits and checked its required fields.
UPB_NOINLINE UPB_PRESERVE_NONE static upb_FastDecoder_Return
upb_DecodeFast_RepeatedMessageNext(UPB_PARSE_PARAMS) {
  upb_DecodeFast_RepeatedMessageRun* run = d->fast_repeated_message_run;
  const char* p = upb_DecodeFastData2_RepeatedMessageRunTag2(data2)
                      ? upb_DecodeFast_RepeatedMessageNextTag(
                            d, run, ptr, data2, kUpb_DecodeFast_Tag2Byte)
                      : upb_DecodeFast_RepeatedMessageNextTag(
                            d, run, ptr, data2, kUpb_DecodeFast_Tag1Byte);
  if (UPB_UNLIKELY(p == NULL)) return (upb_FastDecoder_Return){.ptr = ptr};
  UPB_MUSTTAIL return upb_DecodeFast_RepeatedMessageStart(d, p, msg, table, 0,
                                                          data, data2);
}

// Starts the element at `ptr` (its length prefix) and decodes elements until
// the run ends or the fast decoder can't continue.
UPB_NOINLINE UPB_PRESERVE_NONE static upb_FastDecoder_Return
upb_DecodeFast_RepeatedMessageStartEntry(UPB_PARSE_PARAMS) {
  UPB_MUSTTAIL return upb_DecodeFast_RepeatedMessageStart(UPB_PARSE_ARGS);
}

UPB_FORCEINLINE
const upb_MiniTable* upb_DecodeFast_SubMessageTable(const upb_MiniTable* table,
                                                    uint64_t data) {
  uint32_t submsg_ofs = upb_DecodeFastData_GetSubofs(data) * 8;
  const upb_MiniTableSubInternal* sub = UPB_PTR_AT(
      table->UPB_ONLYBITS(fields), submsg_ofs, upb_MiniTableSubInternal);
  return sub->UPB_PRIVATE(submsg);
}

// The data2 that the elements of a run are decoded with. `data2` is the
// field parser's data2.
UPB_FORCEINLINE
uint64_t upb_DecodeFast_RepeatedMessageData2(const upb_MiniTable* subtable,
                                             uint64_t data2,
                                             upb_DecodeFast_TagSize tagsize) {
  return upb_DecodeFastData2_PackMask(subtable->UPB_PRIVATE(table_mask)) |
         kUpb_DecodeFastData2_InRepeatedMessageRun |
         (tagsize == kUpb_DecodeFast_Tag2Byte
              ? kUpb_DecodeFastData2_RepeatedMessageRunTag2
              : 0) |
         ((uint64_t)upb_DecodeFastData2_GetOriginalTag(data2)
          << kUpb_DecodeFastData2_RepeatedMessageTagShift);
}

// Called when the fast decoder could not finish the current element of a run
// (which may not be the first element of the run, if the run already
// advanced). Finishes that element with the MiniTable decoder, then keeps
// decoding elements (fast, if possible) until the run ends. Kept out of line
// so that the state it needs is not kept live across the run's calls.
UPB_NOINLINE UPB_PRESERVE_NONE static const char*
upb_DecodeFast_RepeatedMessageFallback(upb_Decoder* d, const char* p,
                                       const upb_MiniTable* subtable,
                                       uint64_t sub_data2,
                                       upb_DecodeFast_TagSize tagsize) {
  upb_DecodeFast_RepeatedMessageRun* run = d->fast_repeated_message_run;
  while (true) {
    // The field parser already established that `subtable` can be decoded
    // fast, so we skip _upb_Decoder_DecodeMessage()'s checks.
    p = _upb_Decoder_FinishMessage(d, p, run->dst[-1], subtable);
    if (UPB_UNLIKELY(d->end_group != DECODE_NOGROUP)) {
      _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_Malformed);
    }
    const char* next =
        upb_DecodeFast_RepeatedMessageNextTag(d, run, p, sub_data2, tagsize);
    if (next == NULL) return p;
    p = upb_DecodeFast_RepeatedMessageStartEntry(d, next, NULL, subtable, 0, 0,
                                                 sub_data2)
            .ptr;
    if (UPB_LIKELY(d->fast_repeated_message_run != run)) return p;
  }
}

// Returns the submessage MiniTable if this field can be decoded as a run, or
// NULL if it must go through upb_DecodeFast_Message().
UPB_FORCEINLINE
const upb_MiniTable* upb_DecodeFast_RepeatedMessageRunTable(
    const upb_MiniTable* table, uint64_t data, uint64_t data2,
    upb_DecodeFast_TagSize tagsize) {
  uint16_t expected = upb_DecodeFastData_GetExpectedTag(data);
  uint16_t actual = upb_DecodeFastData2_GetOriginalTag(data2);
  if (UPB_UNLIKELY(!upb_DecodeFast_TagMatches(expected, actual, tagsize))) {
    return NULL;
  }
  const upb_MiniTable* subtable = upb_DecodeFast_SubMessageTable(table, data);
  if (UPB_UNLIKELY(subtable == NULL) ||
      UPB_UNLIKELY(!upb_DecodeFast_CanRunRepeatedMessage(subtable))) {
    return NULL;
  }
  return subtable;
}

// Decodes all consecutive elements of a repeated message field as a run.
// Returns false (with *ret set) if the field could not be decoded at all.
UPB_FORCEINLINE
bool upb_DecodeFast_RepeatedMessage(upb_Decoder* d, const char** ptr,
                                    upb_Message* msg, uint64_t data,
                                    uint64_t data2, uint64_t* hasbits,
                                    const upb_MiniTable* subtable,
                                    upb_DecodeFast_TagSize tagsize,
                                    upb_DecodeFastNext* ret) {
  upb_DecodeFastArray arr;
  if (!upb_DecodeFast_GetArrayForAppend(d, *ptr, msg, data, hasbits, &arr,
                                        kUpb_DecodeFast_Message, 1, ret)) {
    return false;
  }

  if (--d->depth < 0) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);
  }

  // Assigned field by field rather than with an initializer, so the compiler
  // does not zero-fill padding.
  upb_DecodeFast_RepeatedMessageRun run;
  run.prev = d->fast_repeated_message_run;
  run.parent_limit = d->input.limit;
  run.parent_limit_ptr = d->input.limit_ptr;
  run.arr = arr.arr;
  run.dst = arr.dst;
  run.end = arr.end;
  d->fast_repeated_message_run = &run;
  d->fast_repeated_message_next = &upb_DecodeFast_RepeatedMessageNext;

  uint64_t sub_data2 =
      upb_DecodeFast_RepeatedMessageData2(subtable, data2, tagsize);
  const char* p = *ptr + upb_DecodeFast_TagSizeBytes(tagsize);
  p = upb_DecodeFast_RepeatedMessageStartEntry(d, p, NULL, subtable, 0, 0,
                                               sub_data2)
          .ptr;
  if (UPB_UNLIKELY(d->fast_repeated_message_run == &run)) {
    p = upb_DecodeFast_RepeatedMessageFallback(d, p, subtable, sub_data2,
                                               tagsize);
  }

  arr.arr = run.arr;
  arr.dst = run.dst;
  upb_DecodeFastField_SetArraySize(&arr, kUpb_DecodeFast_Message);
  d->depth++;
  *ptr = p;
  return true;
}

UPB_FORCEINLINE
void upb_DecodeFast_Message(upb_Decoder* d, const char** ptr, upb_Message* msg,
                            const upb_MiniTable* table, uint64_t* hasbits,
                            uint64_t* data, upb_DecodeFastNext* ret,
                            upb_DecodeFast_Type type,
                            upb_DecodeFast_Cardinality card,
                            upb_DecodeFast_TagSize tagsize, uint64_t data2) {
  uint32_t submsg_ofs = upb_DecodeFastData_GetSubofs(*data) * 8;

  const upb_MiniTableSubInternal* sub = UPB_PTR_AT(
      table->UPB_ONLYBITS(fields), submsg_ofs, upb_MiniTableSubInternal);
  const upb_MiniTable* subtablep = sub->UPB_PRIVATE(submsg);

  uint16_t expected = upb_DecodeFastData_GetExpectedTag(*data);
  uint16_t actual = upb_DecodeFastData2_GetOriginalTag(data2);
  if (UPB_UNLIKELY(!upb_DecodeFast_TagMatches(expected, actual, tagsize))) {
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_FallbackMismatchedSlot, ret);
    return;
  }

  upb_DecodeFast_MessageContext ctx = {subtablep,
                                       card == kUpb_DecodeFast_Repeated};

  if (subtablep == NULL) {
    // Unlinked messages are treated as unknown fields.
#ifndef NDEBUG
    uint16_t case_offset = upb_DecodeFastData_GetCaseOffset(*data);
    if (case_offset != 0) {
      uint8_t field_number = upb_DecodeFastData_GetPresence(*data);
      const upb_MiniTableField* field =
          upb_MiniTable_FindFieldByNumber(table, field_number);
      UPB_ASSERT(field);
      _upb_Decoder_VerifyOneofUnlinked(table, field);
    }
#endif
    UPB_DECODEFAST_EXIT(kUpb_DecodeFastNext_DecodeUnknown, ret);
    return;
  }

  if (--d->depth < 0) {
    _upb_FastDecoder_ErrorJmp(d, kUpb_DecodeStatus_MaxDepthExceeded);
  }

  upb_DecodeFast_Unpacked(d, ptr, msg, data, hasbits, ret, type, card, tagsize,
                          &upb_DecodeFast_SingleMessage, &ctx, data2);

  d->depth++;
}

#define F(type, card, tagsize)                                               \
  upb_FastDecoder_Return UPB_PRESERVE_NONE UPB_DECODEFAST_FUNCNAME(          \
      type, card, tagsize)(UPB_PARSE_PARAMS) {                               \
    upb_DecodeFastNext next = kUpb_DecodeFastNext_Dispatch;                  \
    const upb_MiniTable* run_table;                                          \
    if (kUpb_DecodeFast_##card == kUpb_DecodeFast_Repeated &&                \
        (run_table = upb_DecodeFast_RepeatedMessageRunTable(                 \
             table, data, data2, kUpb_DecodeFast_##tagsize))) {              \
      if (UPB_LIKELY(upb_DecodeFast_RepeatedMessage(                         \
              d, &ptr, msg, data, data2, &hasbits, run_table,                \
              kUpb_DecodeFast_##tagsize, &next))) {                          \
        UPB_MUSTTAIL return upb_DecodeFast_Dispatch(UPB_PARSE_ARGS);         \
      }                                                                      \
    } else {                                                                 \
      upb_DecodeFast_Message(d, &ptr, msg, table, &hasbits, &data, &next,    \
                             kUpb_DecodeFast_##type, kUpb_DecodeFast_##card, \
                             kUpb_DecodeFast_##tagsize, data2);              \
    }                                                                        \
    UPB_DECODEFAST_NEXT(next);                                               \
  }

UPB_DECODEFAST_CARDINALITIES(UPB_DECODEFAST_TAGSIZES, F, Message)

#undef F
#undef FASTDECODE_SUBMSG
