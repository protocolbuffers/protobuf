// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
// #include "google/protobuf/compiler/hpb/tests/test_model.hpb.h"
#include "hpb_generator/tests/trivial.hpb.h"
#include "hpb/arena.h"
#include "hpb/hpb.h"
#include "hpb/ptr.h"

namespace hpb::testing {
namespace {

// using ::hpb_unittest::protos::TestModel;
// using ::hpb_unittest::protos::TestModel_Category_IMAGES;
using ::hpb_unittest::protos::Trivial;

// Tests in this file are run against both backends {upb, cpp} to ensure
// api conformance, compatibility, and correctness.
/*
TEST(CppBackend, CanCreateMessage) {
  hpb::Arena arena;
  hpb::Ptr<TestModel> test_model_ptr = hpb::CreateMessage<TestModel>(arena);
  (void)test_model_ptr;
}

TEST(CppBackend, MessageEnums) { EXPECT_EQ(5, TestModel_Category_IMAGES); }
*/

TEST(CppBackend, Booleans) {
  ::hpb::Arena arena;
  auto trivialModel = ::hpb::CreateMessage<Trivial>(arena);
  EXPECT_FALSE(trivialModel.b());
  /*
  testModel.set_b1(true);
  EXPECT_TRUE(testModel.b1());
  testModel.set_b1(false);
  EXPECT_FALSE(testModel.b1());
  testModel.set_b1(true);
  EXPECT_TRUE(testModel.b1());
  testModel.clear_b1();
  EXPECT_FALSE(testModel.has_b1());*/
}
}  // namespace
}  // namespace hpb::testing
