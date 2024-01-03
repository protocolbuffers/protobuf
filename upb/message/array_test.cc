// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/array.h"

#include <gtest/gtest.h>
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"

TEST(ArrayTest, Resize) {
  upb::Arena arena;
  upb::Status status;

  upb_Array* array = upb_Array_New(arena.ptr(), kUpb_CType_Int32);
  EXPECT_TRUE(array);

  for (int i = 0; i < 10; i++) {
    upb_MessageValue mv;
    mv.int32_val = 3 * i;

    upb_Array_Append(array, mv, arena.ptr());
    EXPECT_EQ(upb_Array_Size(array), i + 1);
    EXPECT_EQ(upb_Array_Get(array, i).int32_val, 3 * i);
  }

  upb_Array_Resize(array, 12, arena.ptr());
  EXPECT_EQ(upb_Array_Get(array, 10).int32_val, 0);
  EXPECT_EQ(upb_Array_Get(array, 11).int32_val, 0);

  upb_Array_Resize(array, 4, arena.ptr());
  EXPECT_EQ(upb_Array_Size(array), 4);

  upb_Array_Resize(array, 6, arena.ptr());
  EXPECT_EQ(upb_Array_Size(array), 6);

  EXPECT_EQ(upb_Array_Get(array, 3).int32_val, 9);
  EXPECT_EQ(upb_Array_Get(array, 4).int32_val, 0);
  EXPECT_EQ(upb_Array_Get(array, 5).int32_val, 0);
}
