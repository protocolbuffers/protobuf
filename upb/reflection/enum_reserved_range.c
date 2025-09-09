// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/enum_reserved_range.h"

#include "upb/reflection/enum_def.h"
#include "upb/reflection/field_def.h"
#include "upb/reflection/internal/def_builder.h"

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
      UPB_DEFBUILDER_ALLOCARRAY(ctx, upb_EnumReservedRange, n);

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
