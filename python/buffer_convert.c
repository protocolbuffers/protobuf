// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// clang-format off
#include "Python.h"
// clang-format on

#include "python/buffer_convert.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "upb/base/descriptor_constants.h"
#include "upb/reflection/def.h"

// Must be last.
#include "upb/port/def.inc"

#define SOURCE_KINDS(M)         \
  M(Bool, bool, UNSIGNED)       \
  M(Int8, int8_t, SIGNED)       \
  M(UInt8, uint8_t, UNSIGNED)   \
  M(Int16, int16_t, SIGNED)     \
  M(UInt16, uint16_t, UNSIGNED) \
  M(Int32, int32_t, SIGNED)     \
  M(UInt32, uint32_t, UNSIGNED) \
  M(Int64, int64_t, SIGNED)     \
  M(UInt64, uint64_t, UNSIGNED)

#define TARGET_KINDS(M, Source, SourceT, SourceSign)                           \
  M(Source, SourceT, SourceSign, Int32, int32_t, SIGNED, INT32_MIN, INT32_MAX) \
  M(Source, SourceT, SourceSign, UInt32, uint32_t, UNSIGNED, 0, UINT32_MAX)    \
  M(Source, SourceT, SourceSign, Int64, int64_t, SIGNED, INT64_MIN, INT64_MAX) \
  M(Source, SourceT, SourceSign, UInt64, uint64_t, UNSIGNED, 0, UINT64_MAX)

#define K(Source, SourceT, SourceSign, Target, TargetT, TargetSign, Min, Max) \
  kPyUpb_TargetKind_##Target,
typedef enum {
  kPyUpb_TargetKind_None = 0,
  kPyUpb_TargetKind_Float,
  kPyUpb_TargetKind_Double,
  kPyUpb_TargetKind_Bool,
  TARGET_KINDS(K, X, X, X)
} PyUpb_TargetKind;
#undef K

#define K(Source, SourceT, SourceSign, Target, TargetT, TargetSign, Min, Max) \
  sizeof(TargetT),

static const size_t kPyUpb_TargetSize[] = {
    0,               // kPyUpb_TargetKind_None
    sizeof(float),   // kPyUpb_TargetKind_Float
    sizeof(double),  // kPyUpb_TargetKind_Double
    sizeof(bool),    // kPyUpb_TargetKind_Bool
    TARGET_KINDS(K, X, X, X)};

#undef K

// --- Range Check Helpers ---
// Uses token pasting (SourceSign_TargetSign) to select safe casting bounds.
// We cast to intmax_t/uintmax_t to safely bridge widths before comparing.

// Source is SIGNED, Target is SIGNED
#define IN_RANGE_SIGNED_SIGNED(val, min, max) \
  ((intmax_t)(val) >= (intmax_t)(min) && (intmax_t)(val) <= (intmax_t)(max))

// Source is SIGNED, Target is UNSIGNED
#define IN_RANGE_SIGNED_UNSIGNED(val, min, max) \
  ((val) >= 0 && (uintmax_t)(val) <= (uintmax_t)(max))

// Source is UNSIGNED, Target is SIGNED
#define IN_RANGE_UNSIGNED_SIGNED(val, min, max) \
  ((uintmax_t)(val) <= (uintmax_t)(max))

// Source is UNSIGNED, Target is UNSIGNED
#define IN_RANGE_UNSIGNED_UNSIGNED(val, min, max) \
  ((uintmax_t)(val) <= (uintmax_t)(max))

// Dispatcher macro
#define IN_RANGE(val, SourceSign, TargetSign, min, max) \
  IN_RANGE_##SourceSign##_##TargetSign(val, min, max)
// ---------------------------

#define TARGET_CASE(Source, SourceT, SourceSign, Target, TargetT, TargetSign, \
                    Min, Max)                                                 \
  case kPyUpb_TargetKind_##Target: {                                          \
    TargetT* t = (TargetT*)target;                                            \
    for (size_t i = 0; i < count; ++i) {                                      \
      if (!IN_RANGE(s[i], SourceSign, TargetSign, Min, Max)) {                \
        return kPyUpb_TryResult_Failure;                                      \
      }                                                                       \
      t[i] = (TargetT)s[i];                                                   \
    }                                                                         \
    return kPyUpb_TryResult_Success;                                          \
  }

#define SOURCES_CASE(Source, SourceT, SourceSign)              \
  case kPyUpb_SourceKind_##Source: {                           \
    const SourceT* s = (const SourceT*)source;                 \
    switch (target_kind) {                                     \
      case kPyUpb_TargetKind_Bool: {                           \
        bool* t = (bool*)target;                               \
        for (size_t i = 0; i < count; ++i) {                   \
          t[i] = s[i] != (SourceT)0;                           \
        }                                                      \
        return kPyUpb_TryResult_Success;                       \
      }                                                        \
        TARGET_KINDS(TARGET_CASE, Source, SourceT, SourceSign) \
      default:                                                 \
        return kPyUpb_TryResult_NotSupported;                  \
    }                                                          \
  } break;

static PyUpb_TryResult PyUpb_IntegralCast(PyUpb_TargetKind target_kind,
                                          void* target,  //
                                          PyUpb_SourceKind source_kind,
                                          const void* source, size_t count) {
  switch (source_kind) {
    SOURCE_KINDS(SOURCES_CASE)
    default:
      return kPyUpb_TryResult_NotSupported;
  }
}

static void PyUpb_FloatCast(PyUpb_TargetKind target_kind, void* target,  //
                            PyUpb_SourceKind source_kind, const void* source,
                            size_t count) {
  if (target_kind == kPyUpb_TargetKind_Float) {
    assert(source_kind == kPyUpb_SourceKind_Double);
    for (size_t i = 0; i < count; ++i) {
      ((float*)target)[i] = ((double*)source)[i];
    }
  } else {
    assert(source_kind == kPyUpb_SourceKind_Float);
    for (size_t i = 0; i < count; ++i) {
      ((double*)target)[i] = ((float*)source)[i];
    }
  }
}

#undef IN_RANGE_SIGNED_SIGNED
#undef IN_RANGE_SIGNED_UNSIGNED
#undef IN_RANGE_UNSIGNED_SIGNED
#undef IN_RANGE_UNSIGNED_UNSIGNED
#undef IN_RANGE
#undef TARGET_CASE
#undef TARGET_KINDS
#undef SOURCES_CASE
#undef SOURCE_KINDS

PyUpb_SourceKind PyUpb_SourceKindFromCType(upb_CType ctype) {
  switch (ctype) {
    case kUpb_CType_Bool:
      return kPyUpb_SourceKind_Bool;
    case kUpb_CType_Enum:
    case kUpb_CType_Int32:
      return kPyUpb_SourceKind_Int32;
    case kUpb_CType_UInt32:
      return kPyUpb_SourceKind_UInt32;
    case kUpb_CType_Int64:
      return kPyUpb_SourceKind_Int64;
    case kUpb_CType_UInt64:
      return kPyUpb_SourceKind_UInt64;
    case kUpb_CType_Float:
      return kPyUpb_SourceKind_Float;
    case kUpb_CType_Double:
      return kPyUpb_SourceKind_Double;
    default:
      return kPyUpb_SourceKind_None;
  }
}

static PyUpb_TargetKind PyUpb_TargetKindFromCType(upb_CType ctype) {
  switch (ctype) {
    case kUpb_CType_Bool:
      return kPyUpb_TargetKind_Bool;
    case kUpb_CType_Enum:
    case kUpb_CType_Int32:
      return kPyUpb_TargetKind_Int32;
    case kUpb_CType_UInt32:
      return kPyUpb_TargetKind_UInt32;
    case kUpb_CType_Int64:
      return kPyUpb_TargetKind_Int64;
    case kUpb_CType_UInt64:
      return kPyUpb_TargetKind_UInt64;
    case kUpb_CType_Float:
      return kPyUpb_TargetKind_Float;
    case kUpb_CType_Double:
      return kPyUpb_TargetKind_Double;
    default:
      return kPyUpb_TargetKind_None;
  }
}

size_t PyUpb_GetTargetItemSize(upb_CType ctype) {
  return kPyUpb_TargetSize[PyUpb_TargetKindFromCType(ctype)];
}

PyUpb_SourceKind PyUpb_SourceKindFromFormat(int itemsize, char format) {
  switch (itemsize) {
    case 1:
      switch (format) {
        case '?':
          return kPyUpb_SourceKind_Bool;
        case '\0':
        case 'B':
          return kPyUpb_SourceKind_UInt8;
        case 'b':
          return kPyUpb_SourceKind_Int8;
        default:
          return kPyUpb_SourceKind_None;
      }
    case 2:
      switch (format) {
        case 'H':
          return kPyUpb_SourceKind_UInt16;
        case 'h':
          return kPyUpb_SourceKind_Int16;
        default:
          return kPyUpb_SourceKind_None;
      }
    case 4:
      switch (format) {
        case 'I':
          return kPyUpb_SourceKind_UInt32;
        case 'l':
        case 'i':
          return kPyUpb_SourceKind_Int32;
        case 'f':
          return kPyUpb_SourceKind_Float;
        default:
          return kPyUpb_SourceKind_None;
      }
    case 8:
      switch (format) {
        case 'Q':
          return kPyUpb_SourceKind_UInt64;
        case 'l':
        case 'q':
          return kPyUpb_SourceKind_Int64;
        case 'd':
          return kPyUpb_SourceKind_Double;
        default:
          return kPyUpb_SourceKind_None;
      }
    default:
      return kPyUpb_SourceKind_None;
  }
}

static bool PyUpb_ValidateClosedEnum(const upb_FieldDef* field,
                                     const void* data, Py_ssize_t count) {
  if (upb_FieldDef_CType(field) != kUpb_CType_Enum) return true;
  const upb_EnumDef* e = upb_FieldDef_EnumSubDef(field);
  if (!upb_EnumDef_IsClosed(e)) return true;
  const int32_t* i32 = (const int32_t*)data;
  for (Py_ssize_t i = 0; i < count; i++) {
    if (!upb_EnumDef_CheckNumber(e, i32[i])) {
      PyErr_Format(PyExc_ValueError, "invalid enumerator %d", (int)i32[i]);
      return false;
    }
  }
  return true;
}

PyUpb_TryResult PyUpb_TryConvertBuffer(const upb_FieldDef* field,
                                       PyUpb_SourceKind src_kind,
                                       const void** buffer, size_t count,
                                       void** temp_buffer) {
  upb_CType ctype = upb_FieldDef_CType(field);
  const PyUpb_TargetKind tgt_kind = PyUpb_TargetKindFromCType(ctype);
  if (tgt_kind == kPyUpb_TargetKind_None ||
      src_kind == kPyUpb_SourceKind_None) {
    return kPyUpb_TryResult_NotSupported;
  }
  const bool src_is_float = src_kind == kPyUpb_SourceKind_Float ||
                            src_kind == kPyUpb_SourceKind_Double;
  const bool dst_is_float = tgt_kind == kPyUpb_TargetKind_Float ||
                            tgt_kind == kPyUpb_TargetKind_Double;
  if (src_is_float != dst_is_float) {
    return kPyUpb_TryResult_NotSupported;
  }
  if (PyUpb_SourceKindFromCType(ctype) != src_kind) {
    *temp_buffer = PyMem_Malloc(count * kPyUpb_TargetSize[tgt_kind]);
    if (!*temp_buffer) {
      PyErr_NoMemory();
      return kPyUpb_TryResult_Failure;
    }
    if (!src_is_float) {
      switch (PyUpb_IntegralCast(tgt_kind, *temp_buffer, src_kind, *buffer,
                                 count)) {
        case kPyUpb_TryResult_Success:
          break;
        case kPyUpb_TryResult_Failure:
          PyMem_Free(*temp_buffer);
          *temp_buffer = NULL;
          PyErr_SetString(PyExc_OverflowError, "Integer value out of range");
          return kPyUpb_TryResult_Failure;
        case kPyUpb_TryResult_NotSupported:
          PyMem_Free(*temp_buffer);
          *temp_buffer = NULL;
          return kPyUpb_TryResult_NotSupported;
      }
    } else {
      PyUpb_FloatCast(tgt_kind, *temp_buffer, src_kind, *buffer, count);
    }
    *buffer = *temp_buffer;
  }
  if (!PyUpb_ValidateClosedEnum(field, *buffer, count)) {
    if (*temp_buffer) {
      PyMem_Free(*temp_buffer);
      *temp_buffer = NULL;
    }
    return kPyUpb_TryResult_Failure;
  }
  return kPyUpb_TryResult_Success;
}

#include "upb/port/undef.inc"
