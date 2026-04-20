// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_MINI_DESCRIPTOR_INTERNAL_ENCODE_FUZZ_IMPL_H__
#define GOOGLE_UPB_UPB_MINI_DESCRIPTOR_INTERNAL_ENCODE_FUZZ_IMPL_H__

#include <string_view>

void BuildMiniTable(std::string_view s, bool is_32bit);

#endif  // GOOGLE_UPB_UPB_MINI_DESCRIPTOR_INTERNAL_ENCODE_FUZZ_IMPL_H__
