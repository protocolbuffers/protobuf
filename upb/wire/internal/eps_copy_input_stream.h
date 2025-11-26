// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_EPS_COPY_INPUT_STREAM_H_
#define UPB_WIRE_INTERNAL_EPS_COPY_INPUT_STREAM_H_

#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// The maximum number of bytes a single protobuf field can take up in the
// wire format.  We only want to do one bounds check per field, so the input
// stream guarantees that after upb_EpsCopyInputStream_IsDone() is called,
// the decoder can read this many bytes without performing another bounds
// check.  The stream will copy into a patch buffer as necessary to guarantee
// this invariant. Since tags can only be up to 5 bytes, and a max-length scalar
// field can be 10 bytes, only 15 is required; but sizing up to 16 permits more
// efficient fixed size copies.
#define kUpb_EpsCopyInputStream_SlopBytes 16

struct upb_EpsCopyInputStream {
  const char* end;        // Can read up to SlopBytes bytes beyond this.
  const char* limit_ptr;  // For bounds checks, = end + UPB_MIN(limit, 0)
  uintptr_t input_delta;  // Diff between the original input pointer and patch
  const char* buffer_start;   // Pointer to the original input buffer
  const char* capture_start;  // If non-NULL, the start of the captured region.
  int limit;                  // Submessage limit relative to end
  bool error;                 // To distinguish between EOF and error.
#ifndef NDEBUG
  int guaranteed_bytes;
#endif
  // Allocate double the size of what's required; this permits a fixed-size copy
  // from the input buffer, regardless of how many bytes actually remain in the
  // input buffer.
  char patch[kUpb_EpsCopyInputStream_SlopBytes * 2];
};

UPB_INLINE void UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(
    struct upb_EpsCopyInputStream* e);

UPB_INLINE bool upb_EpsCopyInputStream_IsError(
    struct upb_EpsCopyInputStream* e) {
  return e->error;
}

UPB_INLINE void upb_EpsCopyInputStream_Init(struct upb_EpsCopyInputStream* e,
                                            const char** ptr, size_t size) {
  e->buffer_start = *ptr;
  e->capture_start = NULL;
  if (size <= kUpb_EpsCopyInputStream_SlopBytes) {
    memset(&e->patch, 0, 32);
    if (size) memcpy(&e->patch, *ptr, size);
    e->input_delta = (uintptr_t)*ptr - (uintptr_t)e->patch;
    *ptr = e->patch;
    e->end = *ptr + size;
    e->limit = 0;
  } else {
    e->end = *ptr + size - kUpb_EpsCopyInputStream_SlopBytes;
    e->limit = kUpb_EpsCopyInputStream_SlopBytes;
    e->input_delta = 0;
  }
  e->limit_ptr = e->end;
  e->error = false;
  UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(e);
}

typedef const char* upb_EpsCopyInputStream_BufferFlipCallback(
    struct upb_EpsCopyInputStream* e, const char* old_end,
    const char* new_start);

typedef const char* upb_EpsCopyInputStream_IsDoneFallbackFunc(
    struct upb_EpsCopyInputStream* e, const char* ptr, int overrun);

////////////////////////////////////////////////////////////////////////////////

// Debug checks that attempt to ensure that no code paths will overrun the slop
// bytes even in the worst case. Since we are frequently parsing varints, it's
// possible that the user is trying to parse too many varints before calling
// upb_EpsCopyInputStream_IsDone(), but this error case is not detected because
// the varints are short. These checks ensure that will not overrun the slop
// bytes, even if each varint is its maximum possible length.

UPB_INLINE void UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(
    struct upb_EpsCopyInputStream* e) {
#ifndef NDEBUG
  e->guaranteed_bytes = kUpb_EpsCopyInputStream_SlopBytes;
#endif
}

UPB_INLINE void UPB_PRIVATE(upb_EpsCopyInputStream_BoundsHit)(
    struct upb_EpsCopyInputStream* e) {
#ifndef NDEBUG
  e->guaranteed_bytes = 0;
#endif
}

// Signals the maximum number that the operation about to be performed may
// consume.
UPB_INLINE void UPB_PRIVATE(upb_EpsCopyInputStream_ConsumeBytes)(
    struct upb_EpsCopyInputStream* e, int n) {
#ifndef NDEBUG
  if (e) {
    UPB_ASSERT(e->guaranteed_bytes >= n);
    e->guaranteed_bytes -= n;
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////

typedef enum {
  // The current stream position is at a limit.
  kUpb_IsDoneStatus_Done,

  // The current stream position is not at a limit.
  kUpb_IsDoneStatus_NotDone,

  // The current stream position is not at a limit, and the stream needs to
  // be flipped to a new buffer before more data can be read.
  kUpb_IsDoneStatus_NeedFallback,
} upb_IsDoneStatus;

// Returns the status of the current stream position.  This is a low-level
// function, it is simpler to call upb_EpsCopyInputStream_IsDone() if possible.
UPB_INLINE upb_IsDoneStatus UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(
    struct upb_EpsCopyInputStream* e, const char* ptr, int* overrun) {
  *overrun = ptr - e->end;
  if (UPB_LIKELY(ptr < e->limit_ptr)) {
    UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(e);
    return kUpb_IsDoneStatus_NotDone;
  } else if (UPB_LIKELY(*overrun == e->limit)) {
    UPB_PRIVATE(upb_EpsCopyInputStream_BoundsHit)(e);
    return kUpb_IsDoneStatus_Done;
  } else {
    UPB_PRIVATE(upb_EpsCopyInputStream_BoundsHit)(e);
    return kUpb_IsDoneStatus_NeedFallback;
  }
}

UPB_INLINE bool UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneWithCallback)(
    struct upb_EpsCopyInputStream* e, const char** ptr,
    upb_EpsCopyInputStream_IsDoneFallbackFunc* func) {
  int overrun;
  switch (UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(e, *ptr, &overrun)) {
    case kUpb_IsDoneStatus_Done:
      UPB_PRIVATE(upb_EpsCopyInputStream_BoundsHit)(e);
      return true;
    case kUpb_IsDoneStatus_NotDone:
      UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(e);
      return false;
    case kUpb_IsDoneStatus_NeedFallback:
      *ptr = func(e, *ptr, overrun);
      if (*ptr) {
        UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(e);
      } else {
        UPB_PRIVATE(upb_EpsCopyInputStream_BoundsHit)(e);
      }
      return *ptr == NULL;
  }
  UPB_UNREACHABLE();
}

const char* UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallbackNoCallback)(
    struct upb_EpsCopyInputStream* e, const char* ptr, int overrun);

UPB_INLINE bool upb_EpsCopyInputStream_IsDone(struct upb_EpsCopyInputStream* e,
                                              const char** ptr) {
  return UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneWithCallback)(
      e, ptr, UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallbackNoCallback));
}

UPB_INLINE bool upb_EpsCopyInputStream_CheckSize(
    const struct upb_EpsCopyInputStream* e, const char* ptr, int size) {
  UPB_ASSERT(size >= 0);
  return size <= e->limit - (ptr - e->end);
}

// Returns the total number of bytes that are safe to read from the current
// buffer without reading uninitialized or unallocated memory.
//
// Note that this check does not respect any semantic limits on the stream,
// either limits from PushLimit() or the overall stream end, so some of these
// bytes may have unpredictable, nonsense values in them. The guarantee is only
// that the bytes are valid to read from the perspective of the C language
// (ie. you can read without triggering UBSAN or ASAN).
UPB_INLINE size_t UPB_PRIVATE(upb_EpsCopyInputStream_BytesAvailable)(
    struct upb_EpsCopyInputStream* e, const char* ptr) {
  return (e->end - ptr) + kUpb_EpsCopyInputStream_SlopBytes;
}

UPB_INLINE bool UPB_PRIVATE(upb_EpsCopyInputStream_CheckSizeAvailable)(
    struct upb_EpsCopyInputStream* e, const char* ptr, int size,
    bool submessage) {
  // This is one extra branch compared to the more normal:
  //   return (size_t)(end - ptr) < size;
  // However it is one less computation if we are just about to use "ptr + len":
  //   https://godbolt.org/z/35YGPz
  // In microbenchmarks this shows a small improvement.
  uintptr_t uptr = (uintptr_t)ptr;
  uintptr_t uend = (uintptr_t)e->limit_ptr;
  uintptr_t res = uptr + (size_t)size;
  if (!submessage) uend += kUpb_EpsCopyInputStream_SlopBytes;
  // NOTE: this check depends on having a linear address space.  This is not
  // technically guaranteed by uintptr_t.
  bool ret = res >= uptr && res <= uend;
  if (size < 0) UPB_ASSERT(!ret);
  return ret;
}

UPB_INLINE bool UPB_PRIVATE(upb_EpsCopyInputStream_CheckDataSizeAvailable)(
    struct upb_EpsCopyInputStream* e, const char* ptr, int size) {
  return UPB_PRIVATE(upb_EpsCopyInputStream_CheckSizeAvailable)(e, ptr, size,
                                                                false);
}

// Returns true if the given sub-message size is valid (it does not extend
// beyond any previously-pushed limited) *and* all of the data for this
// sub-message is available to be parsed in the current buffer.
//
// This implies that all fields from the sub-message can be parsed from the
// current buffer while maintaining the invariant that we always have at
// least kUpb_EpsCopyInputStream_SlopBytes of data available past the
// beginning of any individual field start.
//
// If the size is negative, this function will always return false. This
// property can be useful in some cases.
UPB_INLINE bool UPB_PRIVATE(
    upb_EpsCopyInputStream_CheckSubMessageSizeAvailable)(
    struct upb_EpsCopyInputStream* e, const char* ptr, int size) {
  return UPB_PRIVATE(upb_EpsCopyInputStream_CheckSizeAvailable)(e, ptr, size,
                                                                true);
}

// Returns a pointer into an input buffer that corresponds to the parsing
// pointer `ptr`.  The returned pointer may be the same as `ptr`, but also may
// be different if we are currently parsing out of the patch buffer.
UPB_INLINE const char* UPB_PRIVATE(upb_EpsCopyInputStream_GetInputPtr)(
    struct upb_EpsCopyInputStream* e, const char* ptr) {
  // This somewhat silly looking add-and-subtract behavior provides provenance
  // from the original input buffer's pointer. After optimization it produces
  // the same assembly as just casting `(uintptr_t)ptr+input_delta`
  // https://godbolt.org/z/zosG88oPn
  size_t position =
      (uintptr_t)ptr + e->input_delta - (uintptr_t)e->buffer_start;
  return e->buffer_start + position;
}

UPB_INLINE void upb_EpsCopyInputStream_StartCapture(
    struct upb_EpsCopyInputStream* e, const char* ptr) {
  UPB_ASSERT(e->capture_start == NULL);
  e->capture_start = UPB_PRIVATE(upb_EpsCopyInputStream_GetInputPtr)(e, ptr);
}

UPB_INLINE bool upb_EpsCopyInputStream_EndCapture(
    struct upb_EpsCopyInputStream* e, const char* ptr, upb_StringView* sv) {
  UPB_ASSERT(e->capture_start != NULL);
  if (ptr - e->end > e->limit) return false;
  const char* end = UPB_PRIVATE(upb_EpsCopyInputStream_GetInputPtr)(e, ptr);
  sv->data = e->capture_start;
  sv->size = end - sv->data;
  e->capture_start = NULL;
  return true;
}

UPB_INLINE const char* upb_EpsCopyInputStream_Skip(
    struct upb_EpsCopyInputStream* e, const char* ptr, int size) {
  if (!UPB_PRIVATE(upb_EpsCopyInputStream_CheckDataSizeAvailable)(e, ptr,
                                                                  size)) {
    return NULL;
  }
  return ptr + size;
}

// Copies `size` bytes of data from the input `ptr` into the buffer `to`, and
// returns a pointer past the end. Returns NULL on end of stream or error.
UPB_INLINE const char* UPB_PRIVATE(upb_EpsCopyInputStream_Copy)(
    struct upb_EpsCopyInputStream* e, const char* ptr, void* to, int size) {
  if (!UPB_PRIVATE(upb_EpsCopyInputStream_CheckDataSizeAvailable)(e, ptr,
                                                                  size)) {
    return NULL;
  }
  memcpy(to, ptr, size);
  return ptr + size;
}

UPB_INLINE const char* upb_EpsCopyInputStream_ReadStringAlwaysAlias(
    struct upb_EpsCopyInputStream* e, const char* ptr, size_t size,
    upb_StringView* sv) {
  if (!UPB_PRIVATE(upb_EpsCopyInputStream_CheckDataSizeAvailable)(e, ptr,
                                                                  size)) {
    return NULL;
  }
  const char* input = UPB_PRIVATE(upb_EpsCopyInputStream_GetInputPtr)(e, ptr);
  *sv = upb_StringView_FromDataAndSize(input, size);
  return ptr + size;
}

UPB_INLINE void UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(
    struct upb_EpsCopyInputStream* e) {
  UPB_ASSERT(e->limit_ptr == e->end + UPB_MIN(0, e->limit));
}

UPB_INLINE int upb_EpsCopyInputStream_PushLimit(
    struct upb_EpsCopyInputStream* e, const char* ptr, int size) {
  int limit = size + (int)(ptr - e->end);
  int delta = e->limit - limit;
  UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(e);
  UPB_ASSERT(limit <= e->limit);
  e->limit = limit;
  e->limit_ptr = e->end + UPB_MIN(0, limit);
  UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(e);
  return delta;
}

// Pops the last limit that was pushed on this stream.  This may only be called
// once IsDone() returns true.  The user must pass the delta that was returned
// from PushLimit().
UPB_INLINE void upb_EpsCopyInputStream_PopLimit(
    struct upb_EpsCopyInputStream* e, const char* ptr, int saved_delta) {
  UPB_ASSERT(ptr - e->end == e->limit);
  UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(e);
  e->limit += saved_delta;
  e->limit_ptr = e->end + UPB_MIN(0, e->limit);
  UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(e);
}

UPB_INLINE const char* UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallbackInline)(
    struct upb_EpsCopyInputStream* e, const char* ptr, int overrun,
    upb_EpsCopyInputStream_BufferFlipCallback* callback) {
  if (overrun < e->limit) {
    // Need to copy remaining data into patch buffer.
    UPB_ASSERT(overrun < kUpb_EpsCopyInputStream_SlopBytes);
    const char* old_end = ptr;
    const char* new_start = &e->patch[0] + overrun;
    memset(e->patch + kUpb_EpsCopyInputStream_SlopBytes, 0,
           kUpb_EpsCopyInputStream_SlopBytes);
    memcpy(e->patch, e->end, kUpb_EpsCopyInputStream_SlopBytes);
    ptr = new_start;
    e->end = &e->patch[kUpb_EpsCopyInputStream_SlopBytes];
    e->limit -= kUpb_EpsCopyInputStream_SlopBytes;
    e->limit_ptr = e->end + e->limit;
    UPB_ASSERT(ptr < e->limit_ptr);
    e->input_delta = (uintptr_t)old_end - (uintptr_t)new_start;
    const char* ret = callback(e, old_end, new_start);
    if (ret) {
      UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(e);
    }
    return ret;
  } else {
    UPB_ASSERT(overrun > e->limit);
    e->error = true;
    return callback(e, NULL, NULL);
  }
}

typedef const char* upb_EpsCopyInputStream_ParseDelimitedFunc(
    struct upb_EpsCopyInputStream* e, const char* ptr, int size, void* ctx);

UPB_FORCEINLINE bool upb_EpsCopyInputStream_TryParseDelimitedFast(
    struct upb_EpsCopyInputStream* e, const char** ptr, int len,
    upb_EpsCopyInputStream_ParseDelimitedFunc* func, void* ctx) {
  if (!UPB_PRIVATE(upb_EpsCopyInputStream_CheckSubMessageSizeAvailable)(e, *ptr,
                                                                        len)) {
    return false;
  }

  // Fast case: Sub-message is <128 bytes and fits in the current buffer.
  // This means we can preserve limit/limit_ptr verbatim.
  const char* saved_limit_ptr = e->limit_ptr;
  int saved_limit = e->limit;
  e->limit_ptr = *ptr + len;
  e->limit = e->limit_ptr - e->end;
  UPB_ASSERT(e->limit_ptr == e->end + UPB_MIN(0, e->limit));
  *ptr = func(e, *ptr, len, ctx);
  e->limit_ptr = saved_limit_ptr;
  e->limit = saved_limit;
  UPB_ASSERT(e->limit_ptr == e->end + UPB_MIN(0, e->limit));
  return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_INTERNAL_EPS_COPY_INPUT_STREAM_H_
