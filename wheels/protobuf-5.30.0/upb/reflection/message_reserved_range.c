// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/enum_def.h"
#include "upb/reflection/field_def.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/internal/extension_range.h"
#include "upb/reflection/message_def.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_MessageReservedRange {
  int32_t start;
  int32_t end;
};

upb_MessageReservedRange* _upb_MessageReservedRange_At(
    const upb_MessageReservedRange* r, int i) {
  return (upb_MessageReservedRange*)&r[i];
}

int32_t upb_MessageReservedRange_Start(const upb_MessageReservedRange* r) {
  return r->start;
}
int32_t upb_MessageReservedRange_End(const upb_MessageReservedRange* r) {
  return r->end;
}

upb_MessageReservedRange* _upb_MessageReservedRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(DescriptorProto_ReservedRange) * const* protos,
    const upb_MessageDef* m) {
  upb_MessageReservedRange* r =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_MessageReservedRange) * n);

  for (int i = 0; i < n; i++) {
    const int32_t start =
        UPB_DESC(DescriptorProto_ReservedRange_start)(protos[i]);
    const int32_t end = UPB_DESC(DescriptorProto_ReservedRange_end)(protos[i]);
    const int32_t max = kUpb_MaxFieldNumber + 1;

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.
    if (start < 1 || end <= start || end > max) {
      _upb_DefBuilder_Errf(ctx,
                           "Reserved range (%d, %d) is invalid, message=%s\n",
                           (int)start, (int)end, upb_MessageDef_FullName(m));
    }

    r[i].start = start;
    r[i].end = end;
  }

  return r;
}
