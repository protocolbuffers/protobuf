// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/reflection/internal/extension_range.h"

#include "upb/reflection/field_def.h"
#include "upb/reflection/internal/def_builder.h"
#include "upb/reflection/message_def.h"

// Must be last.
#include "upb/port/def.inc"

struct upb_ExtensionRange {
  const UPB_DESC(ExtensionRangeOptions) * opts;
  int32_t start;
  int32_t end;
};

upb_ExtensionRange* _upb_ExtensionRange_At(const upb_ExtensionRange* r, int i) {
  return (upb_ExtensionRange*)&r[i];
}

const UPB_DESC(ExtensionRangeOptions) *
    upb_ExtensionRange_Options(const upb_ExtensionRange* r) {
  return r->opts;
}

bool upb_ExtensionRange_HasOptions(const upb_ExtensionRange* r) {
  return r->opts != (void*)kUpbDefOptDefault;
}

int32_t upb_ExtensionRange_Start(const upb_ExtensionRange* r) {
  return r->start;
}

int32_t upb_ExtensionRange_End(const upb_ExtensionRange* r) { return r->end; }

upb_ExtensionRange* _upb_ExtensionRanges_New(
    upb_DefBuilder* ctx, int n,
    const UPB_DESC(DescriptorProto_ExtensionRange) * const* protos,
    const upb_MessageDef* m) {
  upb_ExtensionRange* r =
      _upb_DefBuilder_Alloc(ctx, sizeof(upb_ExtensionRange) * n);

  for (int i = 0; i < n; i++) {
    const int32_t start =
        UPB_DESC(DescriptorProto_ExtensionRange_start)(protos[i]);
    const int32_t end = UPB_DESC(DescriptorProto_ExtensionRange_end)(protos[i]);
    const int32_t max = UPB_DESC(MessageOptions_message_set_wire_format)(
                            upb_MessageDef_Options(m))
                            ? INT32_MAX
                            : kUpb_MaxFieldNumber + 1;

    // A full validation would also check that each range is disjoint, and that
    // none of the fields overlap with the extension ranges, but we are just
    // sanity checking here.
    if (start < 1 || end <= start || end > max) {
      _upb_DefBuilder_Errf(ctx,
                           "Extension range (%d, %d) is invalid, message=%s\n",
                           (int)start, (int)end, upb_MessageDef_FullName(m));
    }

    r[i].start = start;
    r[i].end = end;
    UPB_DEF_SET_OPTIONS(r[i].opts, DescriptorProto_ExtensionRange,
                        ExtensionRangeOptions, protos[i]);
  }

  return r;
}
