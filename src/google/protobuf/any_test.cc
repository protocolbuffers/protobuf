// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <limits.h>

#include <cstdlib>
#include <string>
#include <utility>

#include "google/protobuf/any.pb.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "google/protobuf/any_test.pb.h"
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_import.pb.h"


// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace {

TEST(AnyTest, TestPackAndUnpack) {
  std::string data;
  {
    proto2_unittest::TestAny submessage;
    submessage.set_int32_value(12345);
    proto2_unittest::TestAny message;
    ASSERT_TRUE(message.mutable_any_value()->PackFrom(submessage));

    data = message.SerializeAsString();
  }

  proto2_unittest::TestAny message;
  ASSERT_TRUE(message.ParseFromString(data));
  EXPECT_TRUE(message.has_any_value());
  proto2_unittest::TestAny submessage;
  ASSERT_TRUE(message.any_value().UnpackTo(&submessage));
  EXPECT_EQ(12345, submessage.int32_value());
}

TEST(AnyTest, TestPackFromSerializationExceedsSizeLimit) {
  if (sizeof(size_t) == 4) {
    GTEST_SKIP() << "This toolchain can't allocate that much memory.";
  }
  proto2_unittest::TestAny submessage;
  submessage.mutable_text()->resize(INT_MAX, 'a');
  proto2_unittest::TestAny message;
  EXPECT_FALSE(message.mutable_any_value()->PackFrom(submessage));
}

TEST(AnyTest, TestUnpackWithTypeMismatch) {
  proto2_unittest::TestAny payload;
  payload.set_int32_value(13);
  google::protobuf::Any any;
  ABSL_CHECK(any.PackFrom(payload));

  // Attempt to unpack into the wrong type.
  proto2_unittest::TestAllTypes dest;
  EXPECT_FALSE(any.UnpackTo(&dest));
}

TEST(AnyTest, TestPackAndUnpackAny) {
  std::string data;
  {
    // We can pack an Any message inside another Any message.
    proto2_unittest::TestAny submessage;
    submessage.set_int32_value(12345);
    google::protobuf::Any any;
    ABSL_CHECK(any.PackFrom(submessage));
    proto2_unittest::TestAny message;
    ABSL_CHECK(message.mutable_any_value()->PackFrom(any));

    data = message.SerializeAsString();
  }

  proto2_unittest::TestAny message;
  ASSERT_TRUE(message.ParseFromString(data));
  EXPECT_TRUE(message.has_any_value());
  google::protobuf::Any any;
  ASSERT_TRUE(message.any_value().UnpackTo(&any));
  proto2_unittest::TestAny submessage;
  ASSERT_TRUE(any.UnpackTo(&submessage));
  EXPECT_EQ(12345, submessage.int32_value());
}

TEST(AnyTest, TestPackWithCustomTypeUrl) {
  google::protobuf::Any any;
  {
    proto2_unittest::TestAny submessage;
    submessage.set_int32_value(12345);
    // Pack with a custom type URL prefix.
    ABSL_CHECK(any.PackFrom(submessage, "type.myservice.com"));
    EXPECT_EQ("type.myservice.com/proto2_unittest.TestAny", any.type_url());
    // Pack with a custom type URL prefix ending with '/'.
    ABSL_CHECK(any.PackFrom(submessage, "type.myservice.com/"));
    EXPECT_EQ("type.myservice.com/proto2_unittest.TestAny", any.type_url());
    // Pack with an empty type URL prefix.
    ABSL_CHECK(any.PackFrom(submessage, ""));
    EXPECT_EQ("/proto2_unittest.TestAny", any.type_url());
  }

  // Test unpacking the type.
  proto2_unittest::TestAny submessage;
  EXPECT_TRUE(any.UnpackTo(&submessage));
  EXPECT_EQ(12345, submessage.int32_value());
}

TEST(AnyTest, TestIs) {
  proto2_unittest::TestAny submessage;
  submessage.set_int32_value(12345);
  google::protobuf::Any any;
  ABSL_CHECK(any.PackFrom(submessage));
  ASSERT_TRUE(any.ParseFromString(any.SerializeAsString()));
  EXPECT_TRUE(any.Is<proto2_unittest::TestAny>());
  EXPECT_FALSE(any.Is<google::protobuf::Any>());

  proto2_unittest::TestAny message;
  ABSL_CHECK(message.mutable_any_value()->PackFrom(any));
  ASSERT_TRUE(message.ParseFromString(message.SerializeAsString()));
  EXPECT_FALSE(message.any_value().Is<proto2_unittest::TestAny>());
  EXPECT_TRUE(message.any_value().Is<google::protobuf::Any>());

  any.set_type_url("/proto2_unittest.TestAny");
  EXPECT_TRUE(any.Is<proto2_unittest::TestAny>());
  // The type URL must contain at least one "/".
  any.set_type_url("proto2_unittest.TestAny");
  EXPECT_FALSE(any.Is<proto2_unittest::TestAny>());
  // The type name after the slash must be fully qualified.
  any.set_type_url("/TestAny");
  EXPECT_FALSE(any.Is<proto2_unittest::TestAny>());
}

TEST(AnyTest, MoveConstructor) {
  google::protobuf::Any src;
  {
    proto2_unittest::TestAny payload;
    payload.set_int32_value(12345);
    ABSL_CHECK(src.PackFrom(payload));
  }

  const char* type_url = src.type_url().data();

  google::protobuf::Any dst(std::move(src));
  EXPECT_EQ(type_url, dst.type_url().data());
  proto2_unittest::TestAny payload;
  ASSERT_TRUE(dst.UnpackTo(&payload));
  EXPECT_EQ(12345, payload.int32_value());
}

TEST(AnyTest, MoveAssignment) {
  google::protobuf::Any src;
  {
    proto2_unittest::TestAny payload;
    payload.set_int32_value(12345);
    ABSL_CHECK(src.PackFrom(payload));
  }

  const char* type_url = src.type_url().data();

  google::protobuf::Any dst;
  dst = std::move(src);
  EXPECT_EQ(type_url, dst.type_url().data());
  proto2_unittest::TestAny payload;
  ASSERT_TRUE(dst.UnpackTo(&payload));
  EXPECT_EQ(12345, payload.int32_value());
}

#if GTEST_HAS_DEATH_TEST
#ifndef NDEBUG
TEST(AnyTest, PackSelfDeath) {
  google::protobuf::Any any;
  EXPECT_DEATH(ABSL_CHECK(any.PackFrom(any)), "&message");
  EXPECT_DEATH(ABSL_CHECK(any.PackFrom(any, "")), "&message");
}
#endif  // !NDEBUG
#endif  // GTEST_HAS_DEATH_TEST


}  // namespace
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
