// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#include <gtest/gtest.h>

#include "google/protobuf/stubs/common.h"
#include "google/protobuf/testing/googletest.h"
#include "google/protobuf/unittest_well_known_types.pb.h"

namespace google {
namespace protobuf {
namespace {

// This test only checks whether well-known types are included in protobuf
// runtime library. The test passes if it compiles.
TEST(WellKnownTypesTest, AllKnownTypesAreIncluded) {
  protobuf_unittest::TestWellKnownTypes message;
  EXPECT_EQ(0, message.any_field().ByteSize());
  EXPECT_EQ(0, message.api_field().ByteSize());
  EXPECT_EQ(0, message.duration_field().ByteSize());
  EXPECT_EQ(0, message.empty_field().ByteSize());
  EXPECT_EQ(0, message.field_mask_field().ByteSize());
  EXPECT_EQ(0, message.source_context_field().ByteSize());
  EXPECT_EQ(0, message.struct_field().ByteSize());
  EXPECT_EQ(0, message.timestamp_field().ByteSize());
  EXPECT_EQ(0, message.type_field().ByteSize());
  EXPECT_EQ(0, message.int32_field().ByteSize());
}

}  // namespace

}  // namespace protobuf
}  // namespace google
