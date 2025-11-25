// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_EPS_COPY_INPUT_STREAM_H_
#define UPB_WIRE_EPS_COPY_INPUT_STREAM_H_

#include <stdint.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
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
                                            const char** ptr, size_t size,
                                            bool enable_aliasing);

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

// Returns true if the given delimited field size is valid (it does not extend
// beyond any previously-pushed limited) *and* all of the data for this field is
// available to be read in the current buffer.
//
// If the size is negative, this function will always return false. This
// property can be useful in some cases.
UPB_INLINE bool upb_EpsCopyInputStream_CheckDataSizeAvailable(
    upb_EpsCopyInputStream* e, const char* ptr, int size);

// Returns a pointer into an input buffer that corresponds to the parsing
// pointer `ptr`.  The returned pointer may be the same as `ptr`, but also may
// be different if we are currently parsing out of the patch buffer.
//
// REQUIRES: Aliasing must be available for the given pointer. If the input is a
// flat buffer and aliasing is enabled, then aliasing will always be available.
UPB_INLINE const char* upb_EpsCopyInputStream_GetAliasedPtr(
    upb_EpsCopyInputStream* e, const char* ptr);

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

// Skips `size` bytes of data from the input and returns a pointer past the end.
// Returns NULL on end of stream or error.
UPB_INLINE const char* upb_EpsCopyInputStream_Skip(upb_EpsCopyInputStream* e,
                                                   const char* ptr, int size);

// Reads string data from the input, aliasing into the input buffer instead of
// copying. The parsing pointer is passed in `*ptr`, and will be updated if
// necessary to point to the actual input buffer. Returns the new parsing
// pointer, which will be advanced past the string data.
//
// REQUIRES: Aliasing must be available for this data region (test with
// upb_EpsCopyInputStream_AliasingAvailable().
UPB_INLINE const char* upb_EpsCopyInputStream_ReadStringAliased(
    upb_EpsCopyInputStream* e, const char** ptr, size_t size);

// Reads string data from the stream and advances the pointer accordingly.
// If aliasing was enabled when the stream was initialized, then the returned
// pointer will point into the input buffer if possible, otherwise new data
// will be allocated from arena and copied into. We may be forced to copy even
// if aliasing was enabled if the input data spans input buffers.
//
// Returns NULL if memory allocation failed, or we reached a premature EOF.
UPB_INLINE const char* upb_EpsCopyInputStream_ReadString(
    upb_EpsCopyInputStream* e, const char** ptr, size_t size, upb_Arena* arena);

// Reads a string from the stream and advances the pointer accordingly.
//
// Unlike upb_EpsCopyInputStream_ReadString(), this function will always alias
// the input buffer, regardless of whether aliasing was enabled when the stream
// was initialized. This is useful for cases where the string will only be used
// for the duration of the current function, so there is no risk of the
// underlying data being freed in the meantime.
//
// Returns NULL if size extends beyond the end of the stream.
//
// NOTE: If/when EpsCopyInputStream supports multiple input buffers, this will
// need to fall back to copying if the input data spans multiple buffers, which
// will also require an additional arena parameter.
UPB_INLINE const char* upb_EpsCopyInputStream_ReadStringAlwaysAlias(
    upb_EpsCopyInputStream* e, const char* ptr, size_t size,
    upb_StringView* sv);

// Pushes a limit onto the stack of limits for the current stream.  The limit
// will extend for `size` bytes beyond the position in `ptr`.  Future calls to
// upb_EpsCopyInputStream_IsDone() will return `true` when the stream position
// reaches this limit.
//
// Returns a delta that the caller must store and supply to PopLimit() below.
UPB_INLINE int upb_EpsCopyInputStream_PushLimit(upb_EpsCopyInputStream* e,
                                                const char* ptr, int size);

// Pops the last limit that was pushed on this stream.  This may only be called
// once IsDone() returns true.  The user must pass the delta that was returned
// from PushLimit().
UPB_INLINE void upb_EpsCopyInputStream_PopLimit(upb_EpsCopyInputStream* e,
                                                const char* ptr,
                                                int saved_delta);

// Tries to perform a fast-path handling of the given delimited message data.
// If the sub-message beginning at `*ptr` and extending for `len` is short and
// fits within this buffer, calls `func` with `ctx` as a parameter, where the
// pushing and popping of limits is handled automatically and with lower cost
// than the normal PushLimit()/PopLimit() sequence.
UPB_FORCEINLINE bool upb_EpsCopyInputStream_TryParseDelimitedFast(
    upb_EpsCopyInputStream* e, const char** ptr, int len,
    upb_EpsCopyInputStream_ParseDelimitedFunc* func, void* ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_EPS_COPY_INPUT_STREAM_H_
