// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MESSAGE_INTERNAL_FIELD_DATA_H_
#define UPB_MESSAGE_INTERNAL_FIELD_DATA_H_

#include <stdbool.h>
#include <string.h>

#include "upb/base/string_view.h"
#include "upb/mini_table/field.h"
#include "upb/mini_table/internal/field.h"

// Must be last.
#include "upb/port/def.inc"

// Helpers that copy, compare, and clear the value of a field given only its
// representation. They are shared by message fields (accessors.h) and
// extensions (extension.h), which differ only in where the value is stored.

#if defined(__GNUC__) && !defined(__clang__)
// See the comment in accessors.h: GCC raises incorrect buffer overrun warnings
// for these functions because it fails to constant-propagate the field rep.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#if __GNUC__ >= 11
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE_IF_NOT_GCC void UPB_PRIVATE(_upb_MiniTableField_DataCopy)(
    const upb_MiniTableField* f, void* to, const void* from) {
  switch (UPB_PRIVATE(_upb_MiniTableField_GetRep)(f)) {
    case kUpb_FieldRep_1Byte:
      UPB_MEMCPY_INLINE(to, from, 1);
      return;
    case kUpb_FieldRep_4Byte:
      UPB_MEMCPY_INLINE(to, from, 4);
      return;
    case kUpb_FieldRep_8Byte:
      UPB_MEMCPY_INLINE(to, from, 8);
      return;
    case kUpb_FieldRep_StringView: {
      UPB_MEMCPY_INLINE(to, from, sizeof(upb_StringView));
      return;
    }
  }
  UPB_UNREACHABLE();
}

UPB_INLINE_IF_NOT_GCC bool UPB_PRIVATE(_upb_MiniTableField_DataEquals)(
    const upb_MiniTableField* f, const void* a, const void* b) {
  switch (UPB_PRIVATE(_upb_MiniTableField_GetRep)(f)) {
    case kUpb_FieldRep_1Byte:
      return memcmp(a, b, 1) == 0;
    case kUpb_FieldRep_4Byte:
      return memcmp(a, b, 4) == 0;
    case kUpb_FieldRep_8Byte:
      return memcmp(a, b, 8) == 0;
    case kUpb_FieldRep_StringView: {
      const upb_StringView sa = *(const upb_StringView*)a;
      const upb_StringView sb = *(const upb_StringView*)b;
      return upb_StringView_IsEqual(sa, sb);
    }
  }
  UPB_UNREACHABLE();
}

UPB_INLINE void UPB_PRIVATE(_upb_MiniTableField_DataClear)(
    const upb_MiniTableField* f, void* val) {
  UPB_ALIGN_AS(8) const char zero[16] = {0};
  UPB_PRIVATE(_upb_MiniTableField_DataCopy)(f, val, zero);
}

UPB_INLINE bool UPB_PRIVATE(_upb_MiniTableField_DataIsZero)(
    const upb_MiniTableField* f, const void* val) {
  UPB_ALIGN_AS(8) const char zero[16] = {0};
  return UPB_PRIVATE(_upb_MiniTableField_DataEquals)(f, val, zero);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "upb/port/undef.inc"

#endif  // UPB_MESSAGE_INTERNAL_FIELD_DATA_H_
