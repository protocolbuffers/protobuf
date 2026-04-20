// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/mini_descriptor/internal/encode_fuzz_impl.h"

#include <string_view>

#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/mini_descriptor/decode.h"
#include "upb/mini_table/message.h"

void BuildMiniTable(std::string_view s, bool is_32bit) {
  upb::Arena arena;
  upb::Status status;
  _upb_MiniTable_Build(
      s.data(), s.size(),
      is_32bit ? kUpb_MiniTablePlatform_32Bit : kUpb_MiniTablePlatform_64Bit,
      arena.ptr(), status.ptr());
}
