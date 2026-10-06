// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/result_record.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::testing::FieldsAre;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Optional;
using ::testing::Property;
using ::testing::StartsWith;

TEST(ResultRecordTest, ToString) {
  EXPECT_EQ((ResultRecord{kP0, ResultRecord::Status::kPass, ""}).ToString(),
            "P0 PASS");
  EXPECT_EQ((ResultRecord{kP1, ResultRecord::Status::kFail, "boom"}).ToString(),
            "P1 FAIL: boom");
  EXPECT_EQ((ResultRecord{kP0, ResultRecord::Status::kCrash, "x"}).ToString(),
            "P0 CRASH: x");
  EXPECT_EQ((ResultRecord{kP0, ResultRecord::Status::kSkip, "why"}).ToString(),
            "P0 SKIP: why");
}

TEST(ResultRecordTest, ToStringStripsTrailingWhitespace) {
  EXPECT_EQ(
      (ResultRecord{kP0, ResultRecord::Status::kFail, "a: b \n\n"}).ToString(),
      "P0 FAIL: a: b");
  EXPECT_EQ((ResultRecord{kP0, ResultRecord::Status::kFail, " \n"}).ToString(),
            "P0 FAIL");
}

TEST(ResultRecordTest, ParseInvertsToString) {
  for (TestPriority priority : {kP0, kP1}) {
    for (ResultRecord::Status status :
         {ResultRecord::Status::kPass, ResultRecord::Status::kFail,
          ResultRecord::Status::kCrash, ResultRecord::Status::kSkip}) {
      for (absl::string_view message : {"", "msg", "a: b: c", "multi\nline"}) {
        ResultRecord record{priority, status, std::string(message)};
        EXPECT_THAT(ResultRecord::Parse(record.ToString()),
                    Optional(FieldsAre(priority, status, message)))
            << record.ToString();
      }
    }
  }
}

TEST(ResultRecordTest, ParseRejectsOtherValues) {
  EXPECT_EQ(ResultRecord::Parse(""), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("garbage"), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("P0"), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("P0 "), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("P2 PASS"), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("P0 BOGUS"), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("P0 pass"), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("P0 PASS extra"), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("P0 FAIL:"), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse("P0 FAIL:msg"), absl::nullopt);
  EXPECT_EQ(ResultRecord::Parse(" P0 PASS"), absl::nullopt);
}

TEST(ResultRecordMessageTest, IsOneMarkedLine) {
  EXPECT_EQ(ResultRecordMessage("Required.Foo",
                                {kP0, ResultRecord::Status::kPass, ""}),
            absl::StrCat(kResultRecordMarker, "\tRequired.Foo\tP0 PASS"));
  // Names and messages are escaped onto the one line.
  std::string message = ResultRecordMessage(
      "odd\tname", {kP1, ResultRecord::Status::kFail, "two\nlines\twith tab"});
  EXPECT_THAT(message, StartsWith(kResultRecordMarker));
  EXPECT_THAT(message, Not(HasSubstr("\n")));
}

TEST(ResultRecordMessageTest, ParseInvertsResultRecordMessage) {
  for (absl::string_view name : {"Required.Foo", "a b\tc", "multi\nline"}) {
    for (absl::string_view message :
         {"", "msg", "a: b: c", "tab\tbed", "multi\nline\n", "quote\"d\\"}) {
      ResultRecord record{kP1, ResultRecord::Status::kFail,
                          std::string(message)};
      EXPECT_THAT(ParseResultRecordMessage(ResultRecordMessage(name, record)),
                  IsOkAndHolds(FieldsAre(name, Property(&ResultRecord::ToString,
                                                        record.ToString()))))
          << ResultRecordMessage(name, record);
    }
  }
}

TEST(ResultRecordMessageTest, ParseFindsTheRecordAmongGtestsOwnLines) {
  // SUCCEED() prefixes its message and appends SCOPED_TRACE() context.
  ResultRecord record{kP0, ResultRecord::Status::kSkip, "not supported"};
  EXPECT_THAT(
      ParseResultRecordMessage(absl::StrCat(
          "Succeeded\n", ResultRecordMessage("Required.Foo", record),
          "\nGoogle Test trace:\nfoo_test.cc:12: P0 PASS looks like one")),
      IsOkAndHolds(FieldsAre(
          "Required.Foo",
          Property(&ResultRecord::ToString, "P0 SKIP: not supported"))));
}

TEST(ResultRecordMessageTest, ParseTellsOrdinarySuccessesApart) {
  EXPECT_THAT(ParseResultRecordMessage(""),
              StatusIs(absl::StatusCode::kNotFound));
  EXPECT_THAT(ParseResultRecordMessage("Succeeded"),
              StatusIs(absl::StatusCode::kNotFound));
  EXPECT_THAT(ParseResultRecordMessage("Succeeded\nP0 PASS"),
              StatusIs(absl::StatusCode::kNotFound));
  EXPECT_THAT(ParseResultRecordMessage(
                  absl::StrCat("Succeeded\n ", kResultRecordMarker, "\ta\tb")),
              StatusIs(absl::StatusCode::kNotFound));
}

TEST(ResultRecordMessageTest, ParseRejectsAMarkedLineItCantDecode) {
  for (absl::string_view rest :
       {"", "\tnot a record", "\tname\tP0 BOGUS", "\tname\tP0 PASS\textra",
        "x\tname\tP0 PASS", "\tbad\\escape\tP0 PASS"}) {
    EXPECT_THAT(
        ParseResultRecordMessage(absl::StrCat(kResultRecordMarker, rest)),
        StatusIs(absl::StatusCode::kInvalidArgument,
                 HasSubstr("Can't decode the conformance result record")))
        << rest;
  }
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
