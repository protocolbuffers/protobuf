// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_EPS_COPY_INPUT_STREAM_H_
#define UPB_WIRE_EPS_COPY_INPUT_STREAM_H_

#include <stddef.h>
#include <stdint.h>

#include "upb/base/error_handler.h"
#include "upb/base/string_view.h"
#include "upb/wire/internal/eps_copy_input_stream.h"

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct upb_EpsCopyInputStream upb_EpsCopyInputStream;

// Initializes a upb_EpsCopyInputStream using the contents of the buffer
// [*ptr, size].  Updates `*ptr` as necessary to guarantee that at least
// kUpb_EpsCopyInputStream_SlopBytes are available to read.
UPB_INLINE void upb_EpsCopyInputStream_Init(upb_EpsCopyInputStream* e,
                                            const char** ptr, size_t size);

// Like the previous function, but registers an error handler that will be
// called for any errors encountered.
UPB_INLINE void upb_EpsCopyInputStream_InitWithErrorHandler(
    upb_EpsCopyInputStream* e, const char** ptr, size_t size,
    upb_ErrorHandler* err);

// Returns true if the stream has an error handler.
//
// This function is marked const, which indicates to the compiler that the
// return value is solely a function of the pointer value.  This is not
// entirely true if the stream is reinitialized with
// upb_EpsCopyInputStream_Init*(), so users must not call this function in
// any context where the stream may be reinitialized between calls to this
// function, and the presence of an error handler changes when reinitialized.
UPB_ATTR_CONST
UPB_INLINE bool upb_EpsCopyInputStream_HasErrorHandler(
    const upb_EpsCopyInputStream* e);

// Returns true if the stream is in the error state. A stream enters the error
// state when the user reads past a limit (caught in IsDone()) or the
// ZeroCopyInputStream returns an error.
UPB_INLINE bool upb_EpsCopyInputStream_IsError(upb_EpsCopyInputStream* e);

// Returns true if the stream has hit a limit, either the current delimited
// limit or the overall end-of-stream. As a side effect, this function may flip
// the pointer to a new buffer if there are less than
// kUpb_EpsCopyInputStream_SlopBytes of data to be read in the current buffer.
//
// Postcondition: if the function returns false, there are at least
// kUpb_EpsCopyInputStream_SlopBytes of data available to read at *ptr.
//
// If this returns true, the user must call upb_EpsCopyInputStream_IsError()
// to distinguish between EOF and error.
UPB_INLINE bool upb_EpsCopyInputStream_IsDone(upb_EpsCopyInputStream* e,
                                              const char** ptr);

// Returns true if the given delimited field size is valid (it does not extend
// beyond any previously-pushed limits).  `ptr` should point to the beginning
// of the field data, after the delimited size.
//
// Note that this does *not* guarantee that all of the data for this field is in
// the current buffer.
UPB_INLINE bool upb_EpsCopyInputStream_CheckSize(
    const upb_EpsCopyInputStream* e, const char* ptr, int size);

// Marks the start of a capture operation.  Only one capture operation may be
// active at a time.  The capture operation will be finalized by a call to
// upb_EpsCopyInputStream_EndCapture().  The captured string will be returned in
// sv, and will point to the original input buffer if possible.
UPB_INLINE void upb_EpsCopyInputStream_StartCapture(upb_EpsCopyInputStream* e,
                                                    const char* ptr);

// Ends a capture operation and returns the captured string.  This may only be
// called once per capture operation.  Returns false if the capture operation
// was invalid (the parsing pointer extends beyond the end of the stream).
UPB_INLINE bool upb_EpsCopyInputStream_EndCapture(upb_EpsCopyInputStream* e,
                                                  const char* ptr,
                                                  upb_StringView* sv);

// Reads a string from the stream and advances the pointer accordingly.  The
// returned string view will always alias the input buffer.
//
// Returns NULL if size extends beyond the end of the current input buffer.
// Currently, we only support a single input buffer, so this function will only
// fail if `size` overflows the end of the stream.
//
// If/when we support multiple input buffers, there may be cases where this
// function returns failure, even if the requested region is valid, because the
// requested region spans multiple buffers. In this case, the caller must
// attempt to read the string using other string reading functions before
// signaling an error.
UPB_INLINE const char* upb_EpsCopyInputStream_ReadStringAlwaysAlias(
    upb_EpsCopyInputStream* e, const char* ptr, size_t size,
    upb_StringView* sv);

// Reads a string from the stream and advances the pointer accordingly.  The
// returned string view is ephemeral, only valid until the next call to
// upb_EpsCopyInputStream. It may point to the patch buffer.
//
// Returns NULL if size extends beyond the end of the current buffer (which may
// be the patch buffer).
//
// IMPORTANT NOTE: If `size` extends beyond the end of the stream, the returned
// data may contain garbage bytes from the patch buffer. For efficiency, this
// function does not check that `size` is within the current limit or even the
// end of the stream.
//
// The bytes are guaranteed to be safe to read ephemerally, but they may contain
// garbage data that does not correspond to anything in the input. This error
// will be detected later, when calling upb_EpsCopyInputStream_IsDone() (because
// we will not end at the proper limit), but it may result in nonsense bytes
// ending up in the output.
UPB_INLINE const char* upb_EpsCopyInputStream_ReadStringEphemeral(
    upb_EpsCopyInputStream* e, const char* ptr, size_t size,
    upb_StringView* sv);

// Pushes a limit onto the stack of limits for the current stream.  The limit
// will extend for `size` bytes beyond the position in `ptr`.  Future calls to
// upb_EpsCopyInputStream_IsDone() will return `true` when the stream position
// reaches this limit.
//
// Returns a delta that the caller must store and supply to PopLimit() below.
//
// A return value of <0 indicates that `size` is too large, and exceeds a
// previous limit. If this occurs, the stream is in an error state and must no
// longer be used.
UPB_INLINE ptrdiff_t upb_EpsCopyInputStream_PushLimit(upb_EpsCopyInputStream* e,
                                                      const char* ptr,
                                                      size_t size);

// Pops the last limit that was pushed on this stream.  This may only be called
// once IsDone() returns true.  The user must pass the delta that was returned
// from PushLimit().
UPB_INLINE void upb_EpsCopyInputStream_PopLimit(upb_EpsCopyInputStream* e,
                                                const char* ptr,
                                                ptrdiff_t saved_delta);

// Tries to perform a fast-path handling of the given delimited message data.
// If the sub-message beginning at `*ptr` and extending for `len` is short and
// fits within this buffer, calls `func` with `ctx` as a parameter, where the
// pushing and popping of limits is handled automatically and with lower cost
// than the normal PushLimit()/PopLimit() sequence.
UPB_FORCEINLINE bool upb_EpsCopyInputStream_TryParseDelimitedFast(
    upb_EpsCopyInputStream* e, const char** ptr, size_t size,
    upb_EpsCopyInputStream_ParseDelimitedFunc* func, void* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifdef __cplusplus
// Temporary overloads for functions whose signature has recently changed.
UPB_DEPRECATE_AND_INLINE()
UPB_INLINE void upb_EpsCopyInputStream_Init(upb_EpsCopyInputStream* e,
                                            const char** ptr, size_t size,
                                            bool enable_aliasing) {
  upb_EpsCopyInputStream_Init(e, ptr, size);
}
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_EPS_COPY_INPUT_STREAM_H_
