/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "gtest/gtest.h"
#include "upb/base/descriptor_constants.h"
#include "upb/collections/array.h"
#include "upb/upb.hpp"

TEST(CollectionsTest, Arrays) {
  upb::Arena arena;
  upb::Status status;

  upb_Array* array = upb_Array_New(arena.ptr(), kUpb_CType_Int32);
  EXPECT_TRUE(array);

  for (int i = 0; i < 10; i++) {
    upb_MessageValue mv = {.int32_val = 3 * i};
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
