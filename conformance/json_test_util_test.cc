// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/json_test_util.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "json/value.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::testing::DescribeMatcher;
using ::testing::ExplainMatchResult;
using ::testing::Not;
using ::testing::StringMatchResultListener;

TEST(HasJsonMemberTest, Matches) {
  Json::Value value;
  value["foo"] = 1;
  EXPECT_THAT(value, HasJsonMember("foo"));
  EXPECT_THAT(value, Not(HasJsonMember("bar")));
  // Only objects have members.
  EXPECT_THAT(Json::Value(Json::nullValue), Not(HasJsonMember("foo")));
  EXPECT_THAT(Json::Value(Json::arrayValue), Not(HasJsonMember("foo")));
}

TEST(HasJsonMemberTest, ExplainsNonObject) {
  StringMatchResultListener listener;
  EXPECT_FALSE(ExplainMatchResult(HasJsonMember("foo"),
                                  Json::Value(Json::arrayValue), &listener));
  EXPECT_EQ(listener.str(), "which is not a JSON object");

  // Objects are matched by membership alone, with no further explanation.
  Json::Value value;
  value["foo"] = 1;
  StringMatchResultListener object_listener;
  EXPECT_TRUE(
      ExplainMatchResult(HasJsonMember("foo"), value, &object_listener));
  EXPECT_EQ(object_listener.str(), "");
}

TEST(HasJsonMemberTest, Description) {
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(HasJsonMember("foo")),
            "has member \"foo\"");
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(HasJsonMember("foo"),
                                                /*negation=*/true),
            "doesn't have member \"foo\"");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
