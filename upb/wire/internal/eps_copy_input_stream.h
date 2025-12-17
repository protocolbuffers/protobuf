// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_EPS_COPY_INPUT_STREAM_H_
#define UPB_WIRE_INTERNAL_EPS_COPY_INPUT_STREAM_H_

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "upb/base/error_handler.h"
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
  ptrdiff_t limit;            // Submessage limit relative to end
  upb_ErrorHandler* err;      // Error handler to use when things go wrong.
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

UPB_INLINE void upb_EpsCopyInputStream_InitWithErrorHandler(
    struct upb_EpsCopyInputStream* e, const char** ptr, size_t size,
    upb_ErrorHandler* err) {
  e->buffer_start = *ptr;
  e->capture_start = NULL;
  e->err = err;
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

UPB_INLINE void upb_EpsCopyInputStream_Init(struct upb_EpsCopyInputStream* e,
                                            const char** ptr, size_t size) {
  upb_EpsCopyInputStream_InitWithErrorHandler(e, ptr, size, NULL);
}

UPB_ATTR_CONST
UPB_INLINE bool upb_EpsCopyInputStream_HasErrorHandler(
    const struct upb_EpsCopyInputStream* e) {
  return e && e->err != NULL;
}

// Call this function to signal an error. If an error handler is set, it will be
// called and the function will never return. Otherwise, returns NULL to
// indicate an error.
const char* UPB_PRIVATE(upb_EpsCopyInputStream_ReturnError)(
    struct upb_EpsCopyInputStream* e);

UPB_INLINE const char* UPB_PRIVATE(upb_EpsCopyInputStream_AssumeResult)(
    struct upb_EpsCopyInputStream* e, const char* ptr) {
  UPB_MAYBE_ASSUME(upb_EpsCopyInputStream_HasErrorHandler(e), ptr != NULL);
  return ptr;
}

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

const char* UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallback)(
    struct upb_EpsCopyInputStream* e, const char* ptr, int overrun);

UPB_INLINE bool upb_EpsCopyInputStream_IsDone(struct upb_EpsCopyInputStream* e,
                                              const char** ptr) {
  int overrun;
  switch (UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneStatus)(e, *ptr, &overrun)) {
    case kUpb_IsDoneStatus_Done:
      UPB_PRIVATE(upb_EpsCopyInputStream_BoundsHit)(e);
      return true;
    case kUpb_IsDoneStatus_NotDone:
      UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(e);
      return false;
    case kUpb_IsDoneStatus_NeedFallback:
      *ptr =
          UPB_PRIVATE(upb_EpsCopyInputStream_IsDoneFallback)(e, *ptr, overrun);
      if (*ptr) {
        UPB_PRIVATE(upb_EpsCopyInputStream_BoundsChecked)(e);
      } else {
        UPB_PRIVATE(upb_EpsCopyInputStream_BoundsHit)(e);
      }
      return *ptr == NULL;
  }
  UPB_UNREACHABLE();
}

UPB_INLINE bool upb_EpsCopyInputStream_CheckSize(
    const struct upb_EpsCopyInputStream* e, const char* ptr, int size) {
  UPB_ASSERT(size >= 0);
  return size <= e->limit - (ptr - e->end);
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
  if (ptr - e->end > e->limit) {
    return UPB_PRIVATE(upb_EpsCopyInputStream_ReturnError)(e);
  }
  const char* end = UPB_PRIVATE(upb_EpsCopyInputStream_GetInputPtr)(e, ptr);
  sv->data = e->capture_start;
  sv->size = end - sv->data;
  e->capture_start = NULL;
  return true;
}

UPB_INLINE const char* upb_EpsCopyInputStream_ReadStringAlwaysAlias(
    struct upb_EpsCopyInputStream* e, const char* ptr, size_t size,
    upb_StringView* sv) {
  UPB_ASSERT(size <= PTRDIFF_MAX);
  // The `size` must be within the input buffer. If `ptr` is in the input
  // buffer, then using the slop bytes is fine (because they are real bytes from
  // the tail of the input buffer). If `ptr` is in the patch buffer, then slop
  // bytes represent bytes that do not actually exist in the original input
  // buffer, so we must fail if the size extends into the slop bytes.
  const char* limit =
      e->end + (e->input_delta == 0) * kUpb_EpsCopyInputStream_SlopBytes;
  if ((ptrdiff_t)size > limit - ptr) {
    // For the moment, we consider this an error.  In a multi-buffer world,
    // it could be that the requested string extends into the next buffer, which
    // is not an error and should be recoverable.
    return UPB_PRIVATE(upb_EpsCopyInputStream_ReturnError)(e);
  }
  const char* input = UPB_PRIVATE(upb_EpsCopyInputStream_GetInputPtr)(e, ptr);
  *sv = upb_StringView_FromDataAndSize(input, size);
  return ptr + size;
}

UPB_INLINE const char* upb_EpsCopyInputStream_ReadStringEphemeral(
    struct upb_EpsCopyInputStream* e, const char* ptr, size_t size,
    upb_StringView* sv) {
  UPB_ASSERT(size <= PTRDIFF_MAX);
  // Size must be within the current buffer (including slop bytes).
  const char* limit = e->end + kUpb_EpsCopyInputStream_SlopBytes;
  if ((ptrdiff_t)size > limit - ptr) {
    // For the moment, we consider this an error.  In a multi-buffer world,
    // it could be that the requested string extends into the next buffer, which
    // is not an error and should be recoverable.
    return UPB_PRIVATE(upb_EpsCopyInputStream_ReturnError)(e);
  }
  *sv = upb_StringView_FromDataAndSize(ptr, size);
  return ptr + size;
}

UPB_INLINE void UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(
    struct upb_EpsCopyInputStream* e) {
  UPB_ASSERT(e->limit_ptr == e->end + UPB_MIN(0, e->limit));
}

UPB_INLINE ptrdiff_t upb_EpsCopyInputStream_PushLimit(
    struct upb_EpsCopyInputStream* e, const char* ptr, size_t size) {
  UPB_ASSERT(size <= PTRDIFF_MAX);
  ptrdiff_t limit = (ptrdiff_t)size + (ptr - e->end);
  ptrdiff_t delta = e->limit - limit;
  UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(e);
  e->limit = limit;
  e->limit_ptr = e->end + UPB_MIN(0, limit);
  UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(e);
  if (UPB_UNLIKELY(delta < 0)) {
    UPB_PRIVATE(upb_EpsCopyInputStream_ReturnError)(e);
  }
  return delta;
}

// Pops the last limit that was pushed on this stream.  This may only be called
// once IsDone() returns true.  The user must pass the delta that was returned
// from PushLimit().
UPB_INLINE void upb_EpsCopyInputStream_PopLimit(
    struct upb_EpsCopyInputStream* e, const char* ptr, ptrdiff_t saved_delta) {
  UPB_ASSERT(ptr - e->end == e->limit);
  UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(e);
  e->limit += saved_delta;
  e->limit_ptr = e->end + UPB_MIN(0, e->limit);
  UPB_PRIVATE(upb_EpsCopyInputStream_CheckLimit)(e);
}

typedef const char* upb_EpsCopyInputStream_ParseDelimitedFunc(
    struct upb_EpsCopyInputStream* e, const char* ptr, int size, void* ctx);

UPB_FORCEINLINE bool upb_EpsCopyInputStream_TryParseDelimitedFast(
    struct upb_EpsCopyInputStream* e, const char** ptr, size_t size,
    upb_EpsCopyInputStream_ParseDelimitedFunc* func, void* ctx) {
  UPB_ASSERT(size <= PTRDIFF_MAX);
  if ((ptrdiff_t)size > e->limit_ptr - *ptr) {
    return false;
  }

  // Fast case: Sub-message is <128 bytes and fits in the current buffer.
  // This means we can preserve limit/limit_ptr verbatim.
  const char* saved_limit_ptr = e->limit_ptr;
  int saved_limit = e->limit;
  e->limit_ptr = *ptr + size;
  e->limit = e->limit_ptr - e->end;
  UPB_ASSERT(e->limit_ptr == e->end + UPB_MIN(0, e->limit));
  *ptr = func(e, *ptr, size, ctx);
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
