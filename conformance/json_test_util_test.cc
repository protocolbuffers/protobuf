// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "json_test_util.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "json/config.h"
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

TEST(HasJsonMemberThatTest, Matches) {
  Json::Value value;
  value["str"] = "1";
  value["num"] = 1;
  EXPECT_THAT(value, HasJsonMemberThat("str", IsJsonString("1")));
  EXPECT_THAT(value, Not(HasJsonMemberThat("str", IsJsonString("2"))));
  EXPECT_THAT(value, Not(HasJsonMemberThat("num", IsJsonString("1"))));
  EXPECT_THAT(value, Not(HasJsonMemberThat("missing", IsJsonString("1"))));
  // Only objects have members.
  EXPECT_THAT(Json::Value(Json::nullValue),
              Not(HasJsonMemberThat("str", IsJsonString("1"))));
  EXPECT_THAT(Json::Value(Json::arrayValue),
              Not(HasJsonMemberThat("str", IsJsonString("1"))));
}

TEST(HasJsonMemberThatTest, Description) {
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(
                HasJsonMemberThat("foo", IsJsonString("1"))),
            "has member \"foo\" that is the JSON string \"1\"");
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(
                HasJsonMemberThat("foo", IsJsonString("1")),
                /*negation=*/true),
            "doesn't have member \"foo\" that is the JSON string \"1\"");
}

TEST(IsJsonStringTest, Matches) {
  EXPECT_THAT(Json::Value("1"), IsJsonString("1"));
  EXPECT_THAT(Json::Value("2"), Not(IsJsonString("1")));
  EXPECT_THAT(Json::Value(1), Not(IsJsonString("1")));
  EXPECT_THAT(Json::Value(Json::nullValue), Not(IsJsonString("1")));
}

TEST(IsJsonStringTest, Description) {
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(IsJsonString("1")),
            "is the JSON string \"1\"");
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(IsJsonString("1"),
                                                /*negation=*/true),
            "isn't the JSON string \"1\"");
}

TEST(IsJsonIntTest, Matches) {
  EXPECT_THAT(Json::Value(123), IsJsonInt(123));
  EXPECT_THAT(Json::Value(-1), IsJsonInt(-1));
  EXPECT_THAT(Json::Value(124), Not(IsJsonInt(123)));
  // Only signed integers (Json::intValue) match, like the legacy validators.
  EXPECT_THAT(Json::Value("123"), Not(IsJsonInt(123)));
  EXPECT_THAT(Json::Value(123.0), Not(IsJsonInt(123)));
  EXPECT_THAT(Json::Value(Json::UInt(123)), Not(IsJsonInt(123)));
  EXPECT_THAT(Json::Value(Json::nullValue), Not(IsJsonInt(123)));
}

TEST(IsJsonIntTest, Description) {
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(IsJsonInt(123)),
            "is the JSON integer 123");
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(IsJsonInt(123),
                                                /*negation=*/true),
            "isn't the JSON integer 123");
}

TEST(IsJsonNullTest, Matches) {
  EXPECT_THAT(Json::Value(Json::nullValue), IsJsonNull());
  EXPECT_THAT(Json::Value(), IsJsonNull());
  EXPECT_THAT(Json::Value(0), Not(IsJsonNull()));
  EXPECT_THAT(Json::Value(""), Not(IsJsonNull()));
  EXPECT_THAT(Json::Value("null"), Not(IsJsonNull()));
  EXPECT_THAT(Json::Value(Json::objectValue), Not(IsJsonNull()));
}

TEST(IsJsonNullTest, Description) {
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(IsJsonNull()), "is JSON null");
  EXPECT_EQ(
      DescribeMatcher<const Json::Value&>(IsJsonNull(), /*negation=*/true),
      "isn't JSON null");
}

TEST(IsEmptyJsonObjectTest, Matches) {
  EXPECT_THAT(Json::Value(Json::objectValue), IsEmptyJsonObject());
  Json::Value value;
  value["foo"] = 1;
  EXPECT_THAT(value, Not(IsEmptyJsonObject()));
  // Only objects match, even though Json::Value::empty() is also true for
  // null and for an empty array.
  EXPECT_THAT(Json::Value(Json::nullValue), Not(IsEmptyJsonObject()));
  EXPECT_THAT(Json::Value(Json::arrayValue), Not(IsEmptyJsonObject()));
  EXPECT_THAT(Json::Value(""), Not(IsEmptyJsonObject()));
}

TEST(IsEmptyJsonObjectTest, ExplainsNonObject) {
  StringMatchResultListener listener;
  EXPECT_FALSE(ExplainMatchResult(IsEmptyJsonObject(),
                                  Json::Value(Json::arrayValue), &listener));
  EXPECT_EQ(listener.str(), "which is not a JSON object");
}

TEST(IsEmptyJsonObjectTest, Description) {
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(IsEmptyJsonObject()),
            "is an empty JSON object");
  EXPECT_EQ(DescribeMatcher<const Json::Value&>(IsEmptyJsonObject(),
                                                /*negation=*/true),
            "isn't an empty JSON object");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
