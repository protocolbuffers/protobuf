// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/failure_list.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "absl/types/optional.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Optional;

TEST(FormatFailureMessageTest, StripsSurroundingWhitespace) {
  EXPECT_EQ(FormatFailureMessage("abc"), "abc");
  EXPECT_EQ(FormatFailureMessage(" \t\nabc \n"), "abc");
  EXPECT_EQ(FormatFailureMessage(" \n "), "");
  EXPECT_EQ(FormatFailureMessage(""), "");
}

TEST(FormatFailureMessageTest, RemovesNewlines) {
  // Other whitespace within the message is kept.
  EXPECT_EQ(FormatFailureMessage("a\nb \tc\n\nd"), "ab \tcd");
}

TEST(FormatFailureMessageTest, TruncatesTo128Characters) {
  const std::string limit(128, 'a');
  EXPECT_EQ(FormatFailureMessage(limit), limit);
  EXPECT_EQ(FormatFailureMessage(absl::StrCat(limit, "b")), limit);
  // Stripped whitespace and removed newlines don't count toward the limit.
  const std::string a(64, 'a');
  const std::string b(64, 'b');
  EXPECT_EQ(FormatFailureMessage(absl::StrCat("  ", a, "\n", b, "\n  ")),
            absl::StrCat(a, b));
}

TEST(FailureListTest, EmptyListExpectsNoFailure) {
  FailureList failure_list;
  EXPECT_EQ(failure_list.MatchingEntry("foo"), absl::nullopt);
  EXPECT_EQ(failure_list.ExpectedMessage("foo"), absl::nullopt);
  EXPECT_THAT(failure_list.VerdictOnSuccess("foo"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnSkip("foo", "reason"), IsOk());
}

TEST(FailureListTest, AddNormalizesTheExpectedMessage) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "  abc\ndef  "), IsOk());
  ASSERT_THAT(failure_list.Add("bar", std::string(1000, 'x')), IsOk());
  ASSERT_THAT(failure_list.Add("baz", ""), IsOk());

  EXPECT_THAT(failure_list.ExpectedMessage("foo"), Optional(Eq("abcdef")));
  EXPECT_THAT(failure_list.ExpectedMessage("bar"),
              Optional(std::string(128, 'x')));
  EXPECT_THAT(failure_list.ExpectedMessage("baz"), Optional(IsEmpty()));
}

TEST(FailureListTest, AddRejectsADuplicateEntry) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "abc"), IsOk());

  EXPECT_THAT(failure_list.Add("foo", "xyz"),
              StatusIs(absl::StatusCode::kAlreadyExists,
                       AllOf(HasSubstr("already exists"), HasSubstr("foo"))));
  // The first entry is kept.
  EXPECT_THAT(failure_list.ExpectedMessage("foo"), Optional(Eq("abc")));
}

TEST(FailureListTest, AddRejectsAnEntryOverlappingAWildcard) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo.*.bar", "abc"), IsOk());

  EXPECT_THAT(failure_list.Add("foo.bar.*", "xyz"),
              StatusIs(absl::StatusCode::kAlreadyExists,
                       AllOf(HasSubstr("foo.bar.*"), HasSubstr("foo.*.bar"))));
  EXPECT_THAT(
      failure_list.Add("foo.baz.bar", "xyz"),
      StatusIs(absl::StatusCode::kAlreadyExists,
               AllOf(HasSubstr("foo.baz.bar"), HasSubstr("foo.*.bar"))));
  EXPECT_EQ(failure_list.ExpectedMessage("foo.bar.*"), absl::nullopt);
  EXPECT_EQ(failure_list.ExpectedMessage("foo.baz.bar"), absl::nullopt);
}

TEST(FailureListTest, AddRejectsAnInvalidWildcard) {
  FailureList failure_list;

  EXPECT_THAT(failure_list.Add("foo.b*r", "abc"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  EXPECT_EQ(failure_list.MatchingEntry("foo.bar"), absl::nullopt);
  EXPECT_EQ(failure_list.ExpectedMessage("foo.b*r"), absl::nullopt);
}

TEST(FailureListTest, MatchingEntryMatchesWholeNameComponents) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "abc"), IsOk());
  ASSERT_THAT(failure_list.Add("bar.*.baz", "abc"), IsOk());

  EXPECT_THAT(failure_list.MatchingEntry("foo"), Optional(Eq("foo")));
  EXPECT_THAT(failure_list.MatchingEntry("bar.a.baz"),
              Optional(Eq("bar.*.baz")));
  EXPECT_THAT(failure_list.MatchingEntry("bar.b.baz"),
              Optional(Eq("bar.*.baz")));
  // Neither a prefix nor an extension of an entry matches it, and a wildcard
  // stands for exactly one component.
  EXPECT_EQ(failure_list.MatchingEntry("fo"), absl::nullopt);
  EXPECT_EQ(failure_list.MatchingEntry("foo.bar"), absl::nullopt);
  EXPECT_EQ(failure_list.MatchingEntry("bar"), absl::nullopt);
  EXPECT_EQ(failure_list.MatchingEntry("bar.a"), absl::nullopt);
  EXPECT_EQ(failure_list.MatchingEntry("bar.a.baz.qux"), absl::nullopt);
  EXPECT_EQ(failure_list.MatchingEntry("other"), absl::nullopt);
}

TEST(FailureListTest, ExpectedMessageLooksUpEntriesAsListed) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "abc"), IsOk());
  ASSERT_THAT(failure_list.Add("bar.*", "xyz"), IsOk());

  EXPECT_THAT(failure_list.ExpectedMessage("foo"), Optional(Eq("abc")));
  EXPECT_THAT(failure_list.ExpectedMessage("bar.*"), Optional(Eq("xyz")));
  // Not matched like a test name.
  EXPECT_EQ(failure_list.ExpectedMessage("bar.x"), absl::nullopt);
  EXPECT_EQ(failure_list.ExpectedMessage("unlisted"), absl::nullopt);
}

TEST(FailureListTest, VerdictOnSuccessNamesTheMatchedEntry) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "abc"), IsOk());
  ASSERT_THAT(failure_list.Add("bar.*.baz", "abc"), IsOk());

  EXPECT_THAT(failure_list.VerdictOnSuccess("unlisted"), IsOk());
  // The message is the legacy runner's.
  EXPECT_THAT(failure_list.VerdictOnSuccess("foo"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test foo (matched to foo) is in the failure list, but "
                       "test succeeded.  Remove its match from the failure "
                       "list."));
  EXPECT_THAT(failure_list.VerdictOnSuccess("bar.a.baz"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("test bar.a.baz (matched to bar.*.baz) is in "
                                 "the failure list")));
}

TEST(FailureListTest, VerdictOnSkipNamesTheMatchedEntry) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("bar.*", "abc"), IsOk());

  EXPECT_THAT(failure_list.VerdictOnSkip("unlisted", "reason"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnSkip("bar.x", "reason"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test bar.x (matched to bar.*) is in the failure list "
                       "but was skipped by the testee: reason.  Remove its "
                       "match from the failure list."));
}

TEST(FailureListTest, VerdictOnFailureChecksTheExpectedMessage) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "abc"), IsOk());
  ASSERT_THAT(failure_list.Add("bar.*", "abc"), IsOk());

  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP0, "abc"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure("bar.x", kP0, "abc"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP0, "xyz"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure message for test: foo expected: "
                       "abc actual: xyz"));
  EXPECT_THAT(failure_list.VerdictOnFailure("bar.x", kP0, "xyz"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure message for test: bar.x expected: "
                       "abc actual: xyz"));
}

TEST(FailureListTest, VerdictOnFailureMatchesTheExpectedMessageAsAPrefix) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "abc"), IsOk());
  ASSERT_THAT(failure_list.Add("bar", ""), IsOk());

  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP0, "abc: more details"),
              IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP0, "xabc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure message for test: foo expected: "
                       "abc actual: xabc"));
  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP0, "ab"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure message for test: foo expected: "
                       "abc actual: ab"));
  // An empty expected message matches any failure.
  EXPECT_THAT(failure_list.VerdictOnFailure("bar", kP0, "xyz"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure("bar", kP0, ""), IsOk());
}

TEST(FailureListTest, VerdictOnFailureComparesNormalizedMessages) {
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "abc\ndef"), IsOk());
  ASSERT_THAT(failure_list.Add("bar", std::string(1000, 'x')), IsOk());

  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP0, "  ab\ncdef\n"),
              IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure("bar", kP0, std::string(1000, 'x')),
              IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure(
                  "foo", kP0, absl::StrCat("\nx", std::string(1000, 'y'))),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       absl::StrCat("Unexpected failure message for test: foo "
                                    "expected: abcdef actual: x",
                                    std::string(127, 'y'))));
}

TEST(FailureListTest, EveryPriorityIsEnforcedByDefault) {
  FailureList failure_list;

  EXPECT_EQ(failure_list.enforcement_level(), kLowestPriority);
  EXPECT_THAT(failure_list.VerdictOnFailure("p0", kP0, "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure for test: p0"));
  EXPECT_THAT(failure_list.VerdictOnFailure("p1", kP1, "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure for test: p1"));
}

TEST(FailureListTest, EnforcementLevelToleratesUnlistedLowerPriorities) {
  FailureList failure_list;
  failure_list.set_enforcement_level(kP0);

  EXPECT_EQ(failure_list.enforcement_level(), kP0);
  EXPECT_THAT(failure_list.VerdictOnFailure("p0", kP0, "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure for test: p0"));
  EXPECT_THAT(failure_list.VerdictOnFailure("p1", kP1, "abc"), IsOk());
}

TEST(FailureListTest, ListedFailureIsCheckedWhateverItsPriority) {
  // So that the failure list can't go stale unnoticed.
  FailureList failure_list;
  ASSERT_THAT(failure_list.Add("foo", "abc"), IsOk());
  failure_list.set_enforcement_level(kP0);

  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP1, "abc"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP1, "xyz"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure message for test: foo expected: "
                       "abc actual: xyz"));
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
