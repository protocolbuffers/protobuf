// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PYTHON_BUFFER_CONVERT_H__
#define UPB_PYTHON_BUFFER_CONVERT_H__

#include <stdbool.h>
#include <stddef.h>

#include "upb/base/descriptor_constants.h"
#include "upb/reflection/def.h"

// The result of an attempted buffer conversion.
typedef enum {
  kPyUpb_TryResult_Success,
  // Operation failed. Python error is set.
  kPyUpb_TryResult_Failure,
  // Operation is not supported. Python error is not set.
  kPyUpb_TryResult_NotSupported,
} PyUpb_TryResult;

// Represents the incoming buffer's underlying element type.
typedef enum {
  kPyUpb_SourceKind_None = 0,
  kPyUpb_SourceKind_Float,
  kPyUpb_SourceKind_Double,
  kPyUpb_SourceKind_Bool,
  kPyUpb_SourceKind_Int8,
  kPyUpb_SourceKind_UInt8,
  kPyUpb_SourceKind_Int16,
  kPyUpb_SourceKind_UInt16,
  kPyUpb_SourceKind_Int32,
  kPyUpb_SourceKind_UInt32,
  kPyUpb_SourceKind_Int64,
  kPyUpb_SourceKind_UInt64
} PyUpb_SourceKind;

// Helper to determine the source kind from a field's upb_CType.
PyUpb_SourceKind PyUpb_SourceKindFromCType(upb_CType ctype);
PyUpb_SourceKind PyUpb_SourceKindFromFormat(int itemsize, char format);

// Returns the byte size of the target CType for buffer assignment.
size_t PyUpb_GetTargetItemSize(upb_CType ctype);

// Attempts to convert an array of `count` elements of `src_kind` from `*buffer`
// into the type expected by `field`.
//
// If successful, returns kPyUpb_TryResult_Success. If conversion requires a new
// allocation (e.g. types differ but are compatible), `*temp_buffer` is
// allocated and `*buffer` is updated to point to it. The caller is responsible
// for freeing
// `*temp_buffer` if it is not NULL.
//
// If conversion is not supported, returns kPyUpb_TryResult_NotSupported.
//
// If conversion fails, returns kPyUpb_TryResult_Failure.
// If SourceKind is the same as the target CType, conversion is a no-op however
// enum values are still validated.
PyUpb_TryResult PyUpb_TryConvertBuffer(const upb_FieldDef* field,
                                       PyUpb_SourceKind src_kind,
                                       const void** buffer, size_t count,
                                       void** temp_buffer);

#endif  // UPB_PYTHON_BUFFER_CONVERT_H__
