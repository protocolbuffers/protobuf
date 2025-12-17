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

#include "upb/base/descriptor_constants.h"
#include "upb/base/error_handler.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mem/internal/arena.h"
#include "upb/message/message.h"
#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"
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
  upb_ErrorHandler err;

#ifndef NDEBUG
  const char* debug_tagstart;
  const char* debug_valstart;
  char* trace_buf;
  char* trace_ptr;
  char* trace_end;
#endif
} upb_Decoder;

UPB_INLINE const char* upb_Decoder_Init(upb_Decoder* d, const char* buf,
                                        size_t size,
                                        const upb_ExtensionRegistry* extreg,
                                        int options, upb_Arena* arena,
                                        char* trace_buf, size_t trace_size) {
  upb_ErrorHandler_Init(&d->err);
  upb_EpsCopyInputStream_InitWithErrorHandler(&d->input, &buf, size, &d->err);

  UPB_STATIC_ASSERT((int)kUpb_DecodeStatus_Ok == (int)kUpb_ErrorCode_Ok,
                    "mismatched error codes");
  UPB_STATIC_ASSERT(
      (int)kUpb_DecodeStatus_OutOfMemory == (int)kUpb_ErrorCode_OutOfMemory,
      "mismatched error codes");
  UPB_STATIC_ASSERT(
      (int)kUpb_DecodeStatus_Malformed == (int)kUpb_ErrorCode_Malformed,
      "mismatched error codes");

  if (options & kUpb_DecodeOption_AlwaysValidateUtf8) {
    // Fasttable decoder does not support this option.
    options |= kUpb_DecodeOption_DisableFastTable;
  }

  d->extreg = extreg;
  d->depth = upb_DecodeOptions_GetEffectiveMaxDepth(options);
  d->end_group = DECODE_NOGROUP;
  d->options = (uint16_t)options;
  d->missing_required = false;
  d->message_is_done = false;
#ifndef NDEBUG
  d->trace_buf = trace_buf;
  d->trace_ptr = trace_buf;
  d->trace_end = UPB_PTRADD(trace_buf, trace_size);
#endif
  if (trace_buf) *trace_buf = 0;  // Null-terminate.

  // Violating the encapsulation of the arena for performance reasons.
  // This is a temporary arena that we swap into and swap out of when we are
  // done.  The temporary arena only needs to be able to handle allocation,
  // not fuse or free, so it does not need many of the members to be initialized
  // (particularly parent_or_count).
  UPB_PRIVATE(_upb_Arena_SwapIn)(&d->arena, arena);
  return buf;
}

UPB_INLINE upb_DecodeStatus upb_Decoder_Destroy(upb_Decoder* d,
                                                upb_Arena* arena) {
  UPB_PRIVATE(_upb_Arena_SwapOut)(arena, &d->arena);
  return (upb_DecodeStatus)d->err.code;
}

#ifndef NDEBUG
UPB_INLINE bool _upb_Decoder_TraceBufferFull(upb_Decoder* d) {
  return d->trace_ptr == d->trace_end - 1;
}

UPB_INLINE bool _upb_Decoder_TraceBufferAlmostFull(upb_Decoder* d) {
  return d->trace_ptr == d->trace_end - 2;
}
#endif

UPB_INLINE char* _upb_Decoder_TraceNext(upb_Decoder* d) {
#ifndef NDEBUG
  return _upb_Decoder_TraceBufferAlmostFull(d) ? NULL : d->trace_ptr + 1;
#else
  return NULL;
#endif
}

UPB_INLINE char* _upb_Decoder_TracePtr(upb_Decoder* d) {
#ifndef NDEBUG
  return d->trace_ptr;
#else
  return NULL;
#endif
}

// Trace events are used to trace the progress of the decoder.
// Events:
//   'D'  Fast dispatch
//   'F'  Field successfully parsed fast.
//   '<'  Fallback to MiniTable parser.
//   'M'  Field successfully parsed with MiniTable.
//   'X'  Truncated -- trace buffer is full, further events were discarded.
UPB_INLINE void _upb_Decoder_Trace(upb_Decoder* d, char event) {
#ifndef NDEBUG
  if (d->trace_ptr == NULL) return;
  if (_upb_Decoder_TraceBufferFull(d)) {
    d->trace_ptr[-1] = 'X';  // Truncated.
    return;
  }
  d->trace_ptr[0] = event;
  d->trace_ptr[1] = '\0';
  d->trace_ptr++;
#endif
};

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

const char* _upb_Decoder_DecodeMessage(upb_Decoder* d, const char* ptr,
                                       upb_Message* msg,
                                       const upb_MiniTable* layout);

UPB_INLINE bool _upb_Decoder_FieldRequiresUtf8Validation(
    const upb_Decoder* d, const upb_MiniTableField* field) {
  if (field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_String) return true;

  if (field->UPB_PRIVATE(descriptortype) == kUpb_FieldType_Bytes &&
      (field->UPB_ONLYBITS(mode) & kUpb_LabelFlags_IsAlternate) &&
      (d->options & kUpb_DecodeOption_AlwaysValidateUtf8)) {
    return true;
  }

  return false;
}

UPB_INLINE bool _upb_Decoder_ReadString(upb_Decoder* d, const char** ptr,
                                        size_t size, upb_StringView* sv,
                                        bool validate_utf8) {
  upb_StringView tmp;
  *ptr =
      upb_EpsCopyInputStream_ReadStringAlwaysAlias(&d->input, *ptr, size, &tmp);
  if (*ptr == NULL) return false;
  if (validate_utf8 && !utf8_range_IsValid(tmp.data, tmp.size)) {
    upb_ErrorHandler_ThrowError(&d->err, kUpb_DecodeStatus_BadUtf8);
    return false;
  }
  if ((d->options & kUpb_DecodeOption_AliasString) == 0) {
    char* data = (char*)upb_Arena_Malloc(&d->arena, tmp.size);
    if (!data) return false;
    memcpy(data, tmp.data, tmp.size);
    tmp.data = data;
  }
  *sv = tmp;
  return true;
}

#include "upb/port/undef.inc"

#endif /* UPB_WIRE_INTERNAL_DECODER_H_ */
