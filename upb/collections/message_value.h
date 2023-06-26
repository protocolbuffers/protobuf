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

// Users should include array.h or map.h instead.
// IWYU pragma: private, include "upb/collections/array.h"

#ifndef UPB_MESSAGE_VALUE_H_
#define UPB_MESSAGE_VALUE_H_

#include "upb/base/string_view.h"
#include "upb/message/tagged_ptr.h"
#include "upb/mini_table/message.h"

// Must be last.
#include "upb/port/def.inc"

typedef struct upb_Array upb_Array;
typedef struct upb_Map upb_Map;

typedef union {
  bool bool_val;
  float float_val;
  double double_val;
  int32_t int32_val;
  int64_t int64_val;
  uint32_t uint32_val;
  uint64_t uint64_val;
  const upb_Array* array_val;
  const upb_Map* map_val;
  const upb_Message* msg_val;
  upb_StringView str_val;

  // EXPERIMENTAL: A tagged upb_Message*.  Users must use this instead of
  // msg_val if unlinked sub-messages may possibly be in use.  See the
  // documentation in kUpb_DecodeOption_ExperimentalAllowUnlinked for more
  // information.
  upb_TaggedMessagePtr tagged_msg_val;
} upb_MessageValue;

typedef union {
  upb_Array* array;
  upb_Map* map;
  upb_Message* msg;
} upb_MutableMessageValue;

#include "upb/port/undef.inc"

#endif /* UPB_MESSAGE_VALUE_H_ */
