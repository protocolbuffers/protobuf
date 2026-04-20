// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "upb/mini_descriptor/internal/encode_fuzz_impl.h"

namespace {

TEST(FuzzTest, BuildMiniTableRegression) {
  BuildMiniTable("g}{v~fq{\271", false);
}

}  // namespace
