// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_JSON_FUZZ_IMPL_H__
#define GOOGLE_UPB_UPB_JSON_FUZZ_IMPL_H__

#include <string_view>

namespace upb_test {

void DecodeEncodeArbitraryJson(std::string_view json);

}  // namespace upb_test

#endif  // GOOGLE_UPB_UPB_JSON_FUZZ_IMPL_H__
