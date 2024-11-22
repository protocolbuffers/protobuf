// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/wire/eps_copy_input_stream.h"

#include <string.h>

#include <string>

#include <gtest/gtest.h>
#include "upb/mem/arena.hpp"

namespace {

TEST(EpsCopyInputStreamTest, ZeroSize) {
  upb_EpsCopyInputStream stream;
  const char* ptr = nullptr;
  upb_EpsCopyInputStream_Init(&stream, &ptr, 0, false);
  EXPECT_TRUE(
      upb_EpsCopyInputStream_IsDoneWithCallback(&stream, &ptr, nullptr));
}

}  // namespace
