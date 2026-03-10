// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_BASE_ERROR_HANDLER_H__
#define GOOGLE_UPB_UPB_BASE_ERROR_HANDLER_H__

#include <setjmp.h>

// Must be last.
#include "upb/port/def.inc"

// upb_ErrorHandler is a standard longjmp()-based exception handler for UPB.
// It is used for efficient error handling in cases where longjmp() is safe to
// use, such as in highly performance-sensitive C parsing code.
//
// This structure contains both a jmp_buf and an error code; the error code is
// stored in the structure prior to calling longjmp(). This is necessary because
// per the C standard, it is not possible to store the result of setjmp(), so
// the error code must be passed out-of-band.
//
// upb_ErrorHandler is generally not C++-compatible, because longjmp() does not
// run C++ destructors.  So any library that supports upb_ErrorHandler should
// also support a regular return-based error handling mechanism. (Note: we
// could conceivably extend this to take a callback, which could either call
// longjmp() or throw a C++ exception. But since C++ exceptions are forbidden
// by the C++ style guide, there's not likely to be a demand for this.)
//
// To support both cases (longjmp() or return-based status) efficiently, code
// can be written like this:
//
//   UPB_ATTR_CONST bool upb_Arena_HasErrHandler(const upb_Arena* a);
//
//   INLINE void* upb_Arena_Malloc(upb_Arena* a, size_t size) {
//     if (UPB_UNLIKELY(a->end - a->ptr < size)) {
//         void* ret = upb_Arena_MallocFallback(a, size);
//         UPB_MAYBE_ASSUME(upb_Arena_HasErrHandler(a), ret != NULL);
//         return ret;
//     }
//     void* ret = a->ptr;
//     a->ptr += size;
//     UPB_ASSUME(ret != NULL);
//     return ret;
//   }
//
// If the optimizer can prove that an error handler is present, it can assume
// that upb_Arena_Malloc() will not return NULL.

// We need to standardize on any error code that might be thrown by an error
// handler.

typedef enum {
  kUpb_ErrorCode_Ok = 0,
  kUpb_ErrorCode_OutOfMemory = 1,
  kUpb_ErrorCode_Malformed = 2,
} upb_ErrorCode;

typedef struct {
  int code;
  jmp_buf buf;
} upb_ErrorHandler;

UPB_INLINE void upb_ErrorHandler_Init(upb_ErrorHandler* e) {
  e->code = kUpb_ErrorCode_Ok;
}

UPB_INLINE UPB_NORETURN void upb_ErrorHandler_ThrowError(upb_ErrorHandler* e,
                                                         int code) {
  UPB_ASSERT(code != kUpb_ErrorCode_Ok);
  e->code = code;
  UPB_LONGJMP(e->buf, 1);
}

#include "upb/port/undef.inc"

#endif  // GOOGLE_UPB_UPB_BASE_ERROR_HANDLER_H__
