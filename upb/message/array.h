// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef UPB_MESSAGE_ARRAY_H_
#define UPB_MESSAGE_ARRAY_H_

#include <stddef.h>

#include "upb/base/descriptor_constants.h"
#include "upb/mem/arena.h"
#include "upb/message/value.h"  // IWYU pragma: export

// Must be last.
#include "upb/port/def.inc"

typedef struct upb_Array upb_Array;

#ifdef __cplusplus
extern "C" {
#endif

// Creates a new array on the given arena that holds elements of this type.
UPB_API upb_Array* upb_Array_New(upb_Arena* a, upb_CType type);

// Returns the number of elements in the array.
UPB_API size_t upb_Array_Size(const upb_Array* arr);

// Returns the given element, which must be within the array's current size.
UPB_API upb_MessageValue upb_Array_Get(const upb_Array* arr, size_t i);

// Sets the given element, which must be within the array's current size.
UPB_API void upb_Array_Set(upb_Array* arr, size_t i, upb_MessageValue val);

// Appends an element to the array. Returns false on allocation failure.
UPB_API bool upb_Array_Append(upb_Array* array, upb_MessageValue val,
                              upb_Arena* arena);

// Moves elements within the array using memmove().
// Like memmove(), the source and destination elements may be overlapping.
UPB_API void upb_Array_Move(upb_Array* array, size_t dst_idx, size_t src_idx,
                            size_t count);

// Inserts one or more empty elements into the array.
// Existing elements are shifted right.
// The new elements have undefined state and must be set with `upb_Array_Set()`.
// REQUIRES: `i <= upb_Array_Size(arr)`
UPB_API bool upb_Array_Insert(upb_Array* array, size_t i, size_t count,
                              upb_Arena* arena);

// Deletes one or more elements from the array.
// Existing elements are shifted left.
// REQUIRES: `i + count <= upb_Array_Size(arr)`
UPB_API void upb_Array_Delete(upb_Array* array, size_t i, size_t count);

// Changes the size of a vector. New elements are initialized to NULL/0.
// Returns false on allocation failure.
UPB_API bool upb_Array_Resize(upb_Array* array, size_t size, upb_Arena* arena);

// Returns pointer to array data.
UPB_API const void* upb_Array_DataPtr(const upb_Array* arr);

// Returns mutable pointer to array data.
UPB_API void* upb_Array_MutableDataPtr(upb_Array* arr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_ARRAY_H_ */
