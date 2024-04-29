// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_MINI_TABLE_TYPES_H_
#define UPB_MINI_TABLE_TYPES_H_

#include <stdint.h>

#include "upb/message/types.h"  // IWYU pragma: export

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

// When a upb_Message* is stored in a message, array, or map, it is stored in a
// tagged form.  If the tag bit is set, the referenced upb_Message is of type
// _kUpb_MiniTable_Empty (a sentinel message type with no fields) instead of
// that field's true message type.  This forms the basis of what we call
// "dynamic tree shaking."
//
// See the documentation for kUpb_DecodeOption_ExperimentalAllowUnlinked for
// more information.
typedef uintptr_t upb_TaggedMessagePtr;

// Internal-only because empty messages cannot be created by the user.
UPB_INLINE upb_TaggedMessagePtr _upb_TaggedMessagePtr_Pack(upb_Message* ptr,
                                                           bool empty) {
  UPB_ASSERT(((uintptr_t)ptr & 1) == 0);
  return (uintptr_t)ptr | (empty ? 1 : 0);
}

// Users who enable unlinked sub-messages must use this to test whether a
// message is empty before accessing it.  If a message is empty, it must be
// first promoted using the interfaces in message/promote.h.
UPB_INLINE bool upb_TaggedMessagePtr_IsEmpty(upb_TaggedMessagePtr ptr) {
  return ptr & 1;
}

UPB_INLINE upb_Message* _upb_TaggedMessagePtr_GetMessage(
    upb_TaggedMessagePtr ptr) {
  return (upb_Message*)(ptr & ~(uintptr_t)1);
}

UPB_INLINE upb_Message* upb_TaggedMessagePtr_GetNonEmptyMessage(
    upb_TaggedMessagePtr ptr) {
  UPB_ASSERT(!upb_TaggedMessagePtr_IsEmpty(ptr));
  return _upb_TaggedMessagePtr_GetMessage(ptr);
}

UPB_INLINE upb_Message* _upb_TaggedMessagePtr_GetEmptyMessage(
    upb_TaggedMessagePtr ptr) {
  UPB_ASSERT(upb_TaggedMessagePtr_IsEmpty(ptr));
  return _upb_TaggedMessagePtr_GetMessage(ptr);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_TYPES_H_ */
