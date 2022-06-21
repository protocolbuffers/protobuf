/*
 * Copyright (c) 2009-2022, Google LLC
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

#ifndef UPB_INTERNAL_MINI_TABLE_ACCESSORS_H_
#define UPB_INTERNAL_MINI_TABLE_ACCESSORS_H_

#include "upb/msg_internal.h"

// Must be last.
#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

UPB_INLINE bool _upb_MiniTable_Field_InOneOf(const upb_MiniTable_Field* field) {
  return field->presence < 0;
}

UPB_INLINE void _upb_MiniTable_SetPresence(upb_Message* msg,
                                           const upb_MiniTable_Field* field) {
  if (field->presence > 0) {
    _upb_sethas_field(msg, field);
  } else if (_upb_MiniTable_Field_InOneOf(field)) {
    *_upb_oneofcase_field(msg, field) = field->number;
  }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  // UPB_INTERNAL_MINI_TABLE_ACCESSORS_H_
