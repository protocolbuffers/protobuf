// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_WIRE_INTERNAL_DECODE_FAST_COMBINATIONS_H_
#define UPB_WIRE_INTERNAL_DECODE_FAST_COMBINATIONS_H_

#include <stdint.h>

#include "upb/base/internal/log2.h"
#include "upb/wire/types.h"

// Must be last.
#include "upb/port/def.inc"

// Each fast function is specialized for a cardinality, type, and tag size.
// These macros enumerate all possibilities for each of these three dimensions.
//
// These can be generated in any combination, except that non-primitive types
// cannot be packed.  So we have:
//   {1bt,2bt} x {s,o,r,p} x {b,v32,v64,z32,z64,f32,f64,ce}  // Primitive
//   {1bt,2bt} x {s,o,r}   x {s,b,m}                         // Non-primitive

#define UPB_DECODEFAST_CARDINALITIES(F, ...) \
  F(__VA_ARGS__, Scalar)                     \
  F(__VA_ARGS__, Oneof)                      \
  F(__VA_ARGS__, Repeated)                   \
  F(__VA_ARGS__, Packed)

#define UPB_DECODEFAST_TYPES(F, ...) \
  F(__VA_ARGS__, Bool)               \
  F(__VA_ARGS__, Varint32)           \
  F(__VA_ARGS__, Varint64)           \
  F(__VA_ARGS__, ZigZag32)           \
  F(__VA_ARGS__, ZigZag64)           \
  F(__VA_ARGS__, Fixed32)            \
  F(__VA_ARGS__, Fixed64)            \
  F(__VA_ARGS__, String)             \
  F(__VA_ARGS__, Bytes)              \
  F(__VA_ARGS__, Message)

#define UPB_DECODEFAST_TAGSIZES(F, ...) \
  F(__VA_ARGS__, Tag1Byte)              \
  F(__VA_ARGS__, Tag2Byte)

#define ENUM_VAL(_, x) kUpb_DecodeFast_##x,

typedef enum {
  UPB_DECODEFAST_CARDINALITIES(ENUM_VAL)  // kUpb_DecodeFast_Scalar = 0, etc.
} upb_DecodeFast_Cardinality;

typedef enum {
  UPB_DECODEFAST_TYPES(ENUM_VAL)  // kUpb_DecodeFast_Bool = 0, etc.
} upb_DecodeFast_Type;

typedef enum {
  UPB_DECODEFAST_TAGSIZES(ENUM_VAL)  // kUpb_DecodeFast_Tag1Byte = 0, etc.
} upb_DecodeFast_TagSize;

#undef ENUM_VAL

#define ADD(_, x) +1

enum {
  // Counts of the number of enum values for each dimension.
  kUpb_DecodeFast_CardinalityCount = UPB_DECODEFAST_CARDINALITIES(ADD),
  kUpb_DecodeFast_TypeCount = UPB_DECODEFAST_TYPES(ADD),
  kUpb_DecodeFast_TagSizeCount = UPB_DECODEFAST_TAGSIZES(ADD),
};

#undef ADD

// Attributes of the various dimensions.

UPB_INLINE int upb_DecodeFast_TagSizeBytes(upb_DecodeFast_TagSize size) {
  switch (size) {
    case kUpb_DecodeFast_Tag1Byte:
      return 1;
    case kUpb_DecodeFast_Tag2Byte:
      return 2;
    default:
      UPB_UNREACHABLE();
  }
}

UPB_INLINE int upb_DecodeFast_ValueBytes(upb_DecodeFast_Type type) {
  switch (type) {
    case kUpb_DecodeFast_Bool:
      return 1;
    case kUpb_DecodeFast_Varint32:
    case kUpb_DecodeFast_ZigZag32:
    case kUpb_DecodeFast_Fixed32:
      return 4;
    case kUpb_DecodeFast_Varint64:
    case kUpb_DecodeFast_ZigZag64:
    case kUpb_DecodeFast_Fixed64:
    case kUpb_DecodeFast_Message:
      return 8;
    case kUpb_DecodeFast_String:
    case kUpb_DecodeFast_Bytes:
      return 16;
    default:
      UPB_UNREACHABLE();
  }
}

UPB_INLINE upb_WireType upb_DecodeFast_WireType(upb_DecodeFast_Type type) {
  switch (type) {
    case kUpb_DecodeFast_Bool:
    case kUpb_DecodeFast_Varint32:
    case kUpb_DecodeFast_Varint64:
    case kUpb_DecodeFast_ZigZag32:
    case kUpb_DecodeFast_ZigZag64:
      return kUpb_WireType_Varint;
    case kUpb_DecodeFast_Fixed32:
      return kUpb_WireType_32Bit;
    case kUpb_DecodeFast_Fixed64:
      return kUpb_WireType_64Bit;
    case kUpb_DecodeFast_Message:
    case kUpb_DecodeFast_String:
    case kUpb_DecodeFast_Bytes:
      return kUpb_WireType_Delimited;
    default:
      UPB_UNREACHABLE();
  }
}

UPB_INLINE int upb_DecodeFast_ValueBytesLg2(upb_DecodeFast_Type type) {
  return upb_Log2Ceiling(upb_DecodeFast_ValueBytes(type));
}

UPB_INLINE bool upb_DecodeFast_IsRepeated(upb_DecodeFast_Cardinality card) {
  return card == kUpb_DecodeFast_Repeated || card == kUpb_DecodeFast_Packed;
}

UPB_INLINE bool upb_DecodeFast_IsZigZag(upb_DecodeFast_Type type) {
  switch (type) {
    case kUpb_DecodeFast_ZigZag32:
    case kUpb_DecodeFast_ZigZag64:
      return true;
    default:
      return false;
  }
}

// The canonical ordering of functions, used by the arrays of function pointers
// and function names.  This macro generates the full cross product of the
// cardinality, type, and tag size enums.
//
// This ordering generates some combinations that are not actually used (like
// packed strings or messages), but it's simpler than trying to avoid them.
// There are only 14 impossible combinations out of 120 total, so it's not
// worth optimizing for.
#define UPB_DECODEFAST_FUNCTIONS(F) \
  UPB_DECODEFAST_TYPES(UPB_DECODEFAST_CARDINALITIES, UPB_DECODEFAST_TAGSIZES, F)

// The canonical index of a given function.  This must be kept in sync with the
// ordering above, such that this index selects the same function as the
// corresponding UPB_DECODEFAST_FUNCNAME() macro.
#define UPB_DECODEFAST_FUNCTION_IDX(type, card, size)   \
  (((uint32_t)type * kUpb_DecodeFast_CardinalityCount * \
    kUpb_DecodeFast_TagSizeCount) +                     \
   ((uint32_t)card * kUpb_DecodeFast_TagSizeCount) + (uint32_t)size)

// Functions to decompose a function index into its constituent parts.
UPB_INLINE upb_DecodeFast_TagSize
upb_DecodeFast_GetTagSize(uint32_t function_idx) {
  return (upb_DecodeFast_TagSize)(function_idx % kUpb_DecodeFast_TagSizeCount);
}

UPB_INLINE upb_DecodeFast_Cardinality
upb_DecodeFast_GetCardinality(uint32_t function_idx) {
  return (upb_DecodeFast_Cardinality)((function_idx /
                                       kUpb_DecodeFast_TagSizeCount) %
                                      kUpb_DecodeFast_CardinalityCount);
}

UPB_INLINE upb_DecodeFast_Type upb_DecodeFast_GetType(uint32_t function_idx) {
  return (upb_DecodeFast_Type)(function_idx /
                               (kUpb_DecodeFast_TagSizeCount *
                                kUpb_DecodeFast_CardinalityCount));
}

// The canonical function name for a given cardinality, type, and tag size.
#define UPB_DECODEFAST_FUNCNAME(type, card, size) \
  upb_DecodeFast_##type##_##card##_##size

// Returns true if we should disable fast decode for this function_idx.  This is
// useful for field types that are known not to work yet.  It is also useful for
// bisecting a test failure to find which function(s) are broken.
//
// This must be a macro because it must evaluate to a compile-time constant,
// since we use it when initializing the fastdecode function array.
//
// This function only applies to field types that have been assigned a function
// index.  Some field types (eg. groups) do not even have a function index at
// the moment, and so will be rejected by upb_DecodeFast_TryFillEntry() before
// we even get here.
#define UPB_DECODEFAST_COMBINATION_IS_ENABLED(type, card, size) \
  (type == kUpb_DecodeFast_Fixed32 || type == kUpb_DecodeFast_Fixed64)

#ifdef UPB_DECODEFAST_DISABLE_FUNCTIONS_ABOVE
#define UPB_DECODEFAST_ISENABLED(type, card, size)            \
  (UPB_DECODEFAST_COMBINATION_IS_ENABLED(type, card, size) && \
   (UPB_DECODEFAST_FUNCION_IDX(type, card, size) <=           \
    UPB_DECODEFAST_DISABLE_FUNCTIONS_ABOVE))
#else
#define UPB_DECODEFAST_ISENABLED(type, card, size) \
  UPB_DECODEFAST_COMBINATION_IS_ENABLED(type, card, size)
#endif

#include "upb/port/undef.inc"

#endif  // UPB_WIRE_INTERNAL_DECODE_FAST_COMBINATIONS_H_
