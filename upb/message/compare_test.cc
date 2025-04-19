// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/message/compare.h"

#include <cmath>

#include <gtest/gtest.h>
#include "upb/base/descriptor_constants.h"
#include "upb/message/array.h"

// Must be last.
#include "upb/port/def.inc"

namespace {

TEST(MessageValueEqual, Float) {
  upb_MessageValue lhs;
  upb_MessageValue rhs;

  lhs.float_val = 0.0;
  rhs.float_val = 0.0;
  EXPECT_TRUE(upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Float, nullptr, 0));

  lhs.float_val = 0.0;
  rhs.float_val = -0.0;
  EXPECT_TRUE(upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Float, nullptr, 0));

  lhs.float_val = -0.0;
  rhs.float_val = 0.0;
  EXPECT_TRUE(upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Float, nullptr, 0));

  lhs.float_val = -0.0;
  rhs.float_val = -0.0;
  EXPECT_TRUE(upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Float, nullptr, 0));

  lhs.float_val = NAN;
  rhs.float_val = NAN;
  EXPECT_FALSE(
      upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Float, nullptr, 0));
}

TEST(MessageValueEqual, Double) {
  upb_MessageValue lhs;
  upb_MessageValue rhs;

  lhs.double_val = 0.0;
  rhs.double_val = 0.0;
  EXPECT_TRUE(
      upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Double, nullptr, 0));

  lhs.double_val = 0.0;
  rhs.double_val = -0.0;
  EXPECT_TRUE(
      upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Double, nullptr, 0));

  lhs.double_val = -0.0;
  rhs.double_val = 0.0;
  EXPECT_TRUE(
      upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Double, nullptr, 0));

  lhs.double_val = -0.0;
  rhs.double_val = -0.0;
  EXPECT_TRUE(
      upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Double, nullptr, 0));

  lhs.double_val = NAN;
  rhs.double_val = NAN;
  EXPECT_FALSE(
      upb_MessageValue_IsEqual(lhs, rhs, kUpb_CType_Double, nullptr, 0));
}

}  // namespace
