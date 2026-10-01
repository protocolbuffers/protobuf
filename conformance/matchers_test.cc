// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/matchers.h"

#include <cmath>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "conformance/binary_wireformat.h"
#include "google/protobuf/message.h"
#include "google/protobuf/test_messages_proto2.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::protobuf_test_messages::proto2::TestAllTypesProto2;
using ::testing::Not;

// Runs `m` on `arg` once.  The returned AssertionResult succeeds iff `m`
// matched and its message is the matcher's explanation; check both with
// Accepts()/Rejects() below, so that a wrong verdict is reported at the caller.
template <class M, class T>
testing::AssertionResult Explain(const M& m, const T& arg) {
  testing::StringMatchResultListener listener;
  const bool matched = testing::ExplainMatchResult(m, arg, &listener);
  return (matched ? testing::AssertionSuccess() : testing::AssertionFailure())
         << listener.str();
}

// Matches an Explain() result whose matcher rejected the value and whose
// explanation matches `explanation` (a string or a matcher on std::string).
MATCHER_P(Rejects, explanation,
          absl::StrCat(negation ? "isn't rejected, or is" : "is",
                       " rejected with an explanation that ",
                       testing::DescribeMatcher<std::string>(explanation))) {
  if (arg) {
    *result_listener << "the matcher accepted the value: " << arg.message();
    return false;
  }
  return testing::ExplainMatchResult(explanation, std::string(arg.message()),
                                     result_listener);
}

// ---------------------------------------------------------------------------
// Descriptions
// ---------------------------------------------------------------------------

TEST(MatcherDescriptionTest, EqualsTextProto) {
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsTextProto("optional_int32: 9")),
            "equals text proto \"optional_int32: 9\"");
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsTextProto("optional_int32: 9"), /*negation=*/true),
            "doesn't equal text proto \"optional_int32: 9\"");
}

TEST(MatcherDescriptionTest, EqualsBinaryProto) {
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsBinaryProto(VarintField(1, 9))),
            "equals binary proto \"\\010\\t\"");
  EXPECT_EQ(testing::DescribeMatcher<const Message&>(
                EqualsBinaryProto(Wire("\010\t")), /*negation=*/true),
            "doesn't equal binary proto \"\\010\\t\"");
}

// ---------------------------------------------------------------------------
// EqualsTextProto() / EqualsBinaryProto() used directly on messages
// ---------------------------------------------------------------------------

TEST(EqualsTextProtoTest, Success) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  EXPECT_THAT(message, EqualsTextProto("optional_int32: 9"));
}

TEST(EqualsTextProtoTest, MatchesNan) {
  TestAllTypesProto2 message;
  message.set_optional_float(std::nanf(""));
  EXPECT_THAT(message, EqualsTextProto("optional_float: nan"));
}

TEST(EqualsTextProtoTest, Failure) {
  TestAllTypesProto2 message;
  message.set_optional_float(std::nanf(""));
  auto matcher = EqualsTextProto("optional_float: 1.0");
  EXPECT_THAT(Explain(matcher, message),
              Rejects("Output was not equivalent to reference message: "
                      "modified: optional_float: 1 -> nan\n"));
  EXPECT_THAT(message, Not(matcher));
}

TEST(EqualsTextProtoTest, WorksOnBaseMessage) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  const Message& base = message;
  EXPECT_THAT(base, EqualsTextProto("optional_int32: 9"));
  EXPECT_THAT(base, Not(EqualsTextProto("optional_int32: 8")));
}

TEST(EqualsTextProtoDeathTest, ParseFailure) {
  TestAllTypesProto2 message;
  EXPECT_DEATH((void)Explain(EqualsTextProto("unknown: 1.0"), message),
               "Failed to parse expected text proto.*unknown: 1.0");
}

TEST(EqualsBinaryProtoTest, Success) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  EXPECT_THAT(message, EqualsBinaryProto(VarintField(1, 9)));
  EXPECT_THAT(message, EqualsBinaryProto(Wire("\010\t")));
}

TEST(EqualsBinaryProtoTest, Failure) {
  TestAllTypesProto2 message;
  message.set_optional_int32(9);
  auto matcher = EqualsBinaryProto(VarintField(1, 8));
  EXPECT_THAT(Explain(matcher, message),
              Rejects("Output was not equivalent to reference message: "
                      "modified: optional_int32: 8 -> 9\n"));
  EXPECT_THAT(message, Not(matcher));
}

TEST(EqualsBinaryProtoDeathTest, ParseFailure) {
  TestAllTypesProto2 message;
  EXPECT_DEATH((void)Explain(EqualsBinaryProto(Wire("\001")), message),
               "Failed to parse expected wire data");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
