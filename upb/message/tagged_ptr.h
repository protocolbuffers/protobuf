/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UPB_MINI_TABLE_TYPES_H_
#define UPB_MINI_TABLE_TYPES_H_

#include <stdint.h>

// Must be last.
#include "upb/port/def.inc"

#ifdef __cplusplus
extern "C" {
#endif

typedef void upb_Message;

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
