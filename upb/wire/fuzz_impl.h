// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_WIRE_FUZZ_IMPL_H__
#define GOOGLE_UPB_UPB_WIRE_FUZZ_IMPL_H__

#include "absl/strings/string_view.h"
#include "upb/test/fuzz_util.h"

void DecodeEncodeArbitrarySchemaAndPayload(
    const upb::fuzz::MiniTableFuzzInput& input, absl::string_view proto_payload,
    int decode_options, int encode_options, bool length_prefixed = false);

#endif  // GOOGLE_UPB_UPB_WIRE_FUZZ_IMPL_H__
