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

#include "upb/reflection/def_builder_internal.h"
#include "upb/reflection/enum_def.h"
#include "upb/reflection/enum_reserved_range_internal.h"
// #include "upb/reflection/extension_range_internal.h"
#include "upb/reflection/field_def.h"
// #include "upb/reflection/message_def.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_EnumReservedRange {
  int32_t start;
  int32_t end;
};

upb_EnumReservedRange* _upb_EnumReservedRange_At(const upb_EnumReservedRange* r,
                                                 int i) {
  return (upb_EnumReservedRange*)&r[i];
}

int32_t upb_EnumReservedRange_Start(const upb_EnumReservedRange* r) {
  return r->start;
}
int32_t upb_EnumReservedRange_End(const upb_EnumReservedRange* r) {
  return r->end;
}

upb_EnumReservedRange* _upb_EnumReservedRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(EnumDescriptorProto_EnumReservedRange) * const* protos,
    const upb_EnumDef* e) {
  upb_EnumReservedRange* r =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_EnumReservedRange) * n);

  for (int i = 0; i < n; i++) {
    const int32_t start =
        UPB_DESC(EnumDescriptorProto_EnumReservedRange_start)(protos[i]);
    const int32_t end =
        UPB_DESC(EnumDescriptorProto_EnumReservedRange_end)(protos[i]);

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.

    // Note: Not a typo! Unlike extension ranges and message reserved ranges,
    // the end value of an enum reserved range is *inclusive*!
    if (end < start) {
      _upb_DefBuilder_Errf(ctx, "Reserved range (%d, %d) is invalid, enum=%s\n",
                           (int)start, (int)end, upb_EnumDef_FullName(e));
    }

    r[i].start = start;
    r[i].end = end;
  }

  return r;
}
