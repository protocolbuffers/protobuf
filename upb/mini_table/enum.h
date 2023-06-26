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

#ifndef UPB_MINI_TABLE_ENUM_H_
#define UPB_MINI_TABLE_ENUM_H_

#include "upb/mini_table/internal/enum.h"

// Must be last
#include "upb/port/def.inc"

typedef struct upb_MiniTableEnum upb_MiniTableEnum;

// Validates enum value against range defined by enum mini table.
UPB_INLINE bool upb_MiniTableEnum_CheckValue(const struct upb_MiniTableEnum* e,
                                             uint32_t val) {
  _kUpb_FastEnumCheck_Status status = _upb_MiniTable_CheckEnumValueFast(e, val);
  if (UPB_UNLIKELY(status == _kUpb_FastEnumCheck_CannotCheckFast)) {
    return _upb_MiniTable_CheckEnumValueSlow(e, val);
  }
  return status == _kUpb_FastEnumCheck_ValueIsInEnum ? true : false;
}

#include "upb/port/undef.inc"

#endif /* UPB_MINI_TABLE_ENUM_H_ */
