#include "conformance/result_ledger.h"

#include <initializer_list>
#include <string>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include <gmock/gmock.h>
#include "google/protobuf/testing/googletest.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "absl/types/optional.h"
#include "conformance/failure_list.h"
#include "conformance/result_record.h"
#include "conformance/testee.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::FieldsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Optional;
using ::testing::Pair;
using ::testing::SizeIs;

class ResultLedgerTest : public ::testing::Test {
 protected:
  ResultLedgerTest()
      : tmp_dir_(absl::StrCat(TestTempDir(), "/result_ledger_test/")),
        failure_list_path_(absl::StrCat(
            tmp_dir_,
            testing::UnitTest::GetInstance()->current_test_info()->name())) {
    if (!File::Exists(tmp_dir_)) {
      ABSL_CHECK_OK(
          File::RecursivelyCreateDir(tmp_dir_, 0777));
    }
  }

  struct Failure {
    absl::string_view name;
    absl::string_view message;

    Failure(  // NOLINT(google-explicit-constructor)
        absl::string_view name)
        : name(name), message("") {}
    Failure(absl::string_view name, absl::string_view message)
        : name(name), message(message) {}
  };

  void CreateFailureList(
      std::initializer_list<const Failure> expected_failures) {
    std::string content;
    for (auto failure : expected_failures) {
      absl::StrAppend(&content, failure.name, " # ", failure.message, "\n");
    }
    ABSL_CHECK_OK(
        File::SetContents(failure_list_path_, content, true));
  }

  void CreateFailureList(absl::string_view content) {
    ABSL_CHECK_OK(
        File::SetContents(failure_list_path_, content, true));
  }

  absl::string_view failure_list() const { return failure_list_path_; }

  ~ResultLedgerTest() override {
    File::DeleteRecursively(failure_list_path_, NULL, NULL);
  }

 private:
  std::string tmp_dir_;
  std::string failure_list_path_;
  std::string output_path_;
};

TEST_F(ResultLedgerTest, RequiresFinalize) {
  EXPECT_DEATH({ ResultLedger ledger; }, "Finalize");
}

TEST_F(ResultLedgerTest, ReportSuccess) {
  CreateFailureList({});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportSuccess("foo"), IsOk());

  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 1);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
}

TEST_F(ResultLedgerTest, ReportSkipped) {
  CreateFailureList({});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportSkip("foo", "reason"), IsOk());

  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 1);
  EXPECT_EQ(ledger.listed_skips(), 0);
  EXPECT_THAT(ledger.ListedSkips(), IsEmpty());
}

TEST_F(ResultLedgerTest, ReportSkipMarksListedEntrySeenAndMatched) {
  CreateFailureList({{"foo", "abc"}, {"bar.*", "abc"}, {"baz", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  // A listed skip is reported like an unexpected success, naming the entry.
  EXPECT_THAT(ledger.ReportSkip("foo", "not supported"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test foo (matched to foo) is in the failure list but "
                       "was skipped by the testee: not supported.  Remove its "
                       "match from the failure list."));
  EXPECT_THAT(ledger.ReportSkip("bar.x", "no"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test bar.x (matched to bar.*) is in the failure list "
                       "but was skipped by the testee: no.  Remove its match "
                       "from the failure list."));
  // Reporting a test twice returns the error again but counts it once.
  EXPECT_THAT(ledger.ReportSkip("bar.x", "no"), Not(IsOk()));

  // A skip is only counted as a skip, but the entries it matched are neither
  // unmatched nor unseen: dropping the match is left to SaveFailureList().
  EXPECT_THAT(ledger.UnmatchedExpectedFailures(), ElementsAre("baz"));
  EXPECT_THAT(ledger.Finalize(), StatusIs(absl::StatusCode::kFailedPrecondition,
                                          HasSubstr("were not seen: baz")));
  EXPECT_EQ(ledger.skipped(), 2);
  EXPECT_EQ(ledger.listed_skips(), 2);
  EXPECT_THAT(ledger.ListedSkips(),
              ElementsAre(Pair("bar.x", "bar.*"), Pair("foo", "foo")));
  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_THAT(ledger.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger.UnexpectedSuccesses(), IsEmpty());
}

TEST_F(ResultLedgerTest, ReportNotSelected) {
  CreateFailureList({{"foo"}, {"bar.*"}, {"baz"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  ledger.ReportNotSelected("foo");
  ledger.ReportNotSelected("bar.x");
  ledger.ReportNotSelected("unlisted");

  // Only the matched entries stop being unmatched; nothing else changes.
  EXPECT_THAT(ledger.UnmatchedExpectedFailures(), ElementsAre("baz"));
  EXPECT_THAT(ledger.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "The following expected failures were not seen: bar.*, "
                       "baz, foo"));
  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
  EXPECT_EQ(ledger.tolerated_failures(), 0);
}

TEST_F(ResultLedgerTest, WasReported) {
  CreateFailureList({});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  EXPECT_FALSE(ledger.WasReported("success"));

  EXPECT_THAT(ledger.ReportSuccess("success"), IsOk());
  EXPECT_THAT(ledger.ReportFailure("failure", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(ledger.ReportSkip("skip", "reason"), IsOk());
  ledger.ReportNotSelected("not_selected");

  EXPECT_TRUE(ledger.WasReported("success"));
  EXPECT_TRUE(ledger.WasReported("failure"));
  EXPECT_TRUE(ledger.WasReported("skip"));
  EXPECT_TRUE(ledger.WasReported("not_selected"));
  EXPECT_FALSE(ledger.WasReported("other"));
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, FailureListVerdictsRecordNothing) {
  CreateFailureList({{"foo", "abc"}, {"bar.*", "abc"}, {"baz", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ledger.set_enforcement_level(kP0);

  const FailureList& failure_list = ledger.failure_list();
  EXPECT_THAT(failure_list.VerdictOnSuccess("unlisted"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnSuccess("foo"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test foo (matched to foo) is in the failure list, but "
                       "test succeeded.  Remove its match from the failure "
                       "list."));
  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP0, "abc"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure("foo", kP0, "xyz"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure message for test: foo expected: "
                       "abc actual: xyz"));
  EXPECT_THAT(failure_list.VerdictOnFailure("bar.x", kP1, "abc"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnFailure("unlisted", kP0, "xyz"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "Unexpected failure for test: unlisted"));
  EXPECT_THAT(failure_list.VerdictOnFailure("unlisted", kP1, "xyz"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnSkip("unlisted", "reason"), IsOk());
  EXPECT_THAT(failure_list.VerdictOnSkip("bar.x", "reason"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test bar.x (matched to bar.*) is in the failure list "
                       "but was skipped by the testee: reason.  Remove its "
                       "match from the failure list."));

  // Nothing was reported.
  EXPECT_FALSE(ledger.WasReported("foo"));
  EXPECT_FALSE(ledger.WasReported("unlisted"));
  EXPECT_THAT(ledger.UnmatchedExpectedFailures(),
              ElementsAre("bar.*", "baz", "foo"));
  EXPECT_THAT(ledger.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("were not seen: bar.*, baz, foo")));
  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
  EXPECT_EQ(ledger.tolerated_failures(), 0);
}

TEST_F(ResultLedgerTest, FailureListLookupsRecordNothing) {
  CreateFailureList({{"foo", "abc"}, {"bar.*", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  const FailureList& failure_list = ledger.failure_list();
  EXPECT_THAT(failure_list.MatchingEntry("foo"), Optional(std::string("foo")));
  EXPECT_THAT(failure_list.MatchingEntry("bar.x"),
              Optional(std::string("bar.*")));
  EXPECT_EQ(failure_list.MatchingEntry("foo.x"), absl::nullopt);
  EXPECT_EQ(failure_list.MatchingEntry("unlisted"), absl::nullopt);
  EXPECT_THAT(failure_list.ExpectedMessage("foo"),
              Optional(std::string("abc")));
  EXPECT_THAT(failure_list.ExpectedMessage("bar.*"),
              Optional(std::string("abc")));
  // Entries are looked up as listed, not matched.
  EXPECT_EQ(failure_list.ExpectedMessage("bar.x"), absl::nullopt);

  // Looking up a name doesn't match its entry.
  EXPECT_THAT(ledger.UnmatchedExpectedFailures(), ElementsAre("bar.*", "foo"));
  ledger.Finalize().IgnoreError();
}

TEST_F(ResultLedgerTest, ReportRecordDispatchesOnTheStatus) {
  CreateFailureList({{"fail", "abc"},
                     {"crash", "abc"},
                     {"skip", "abc"},
                     {"not_selected", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  ledger.Report("pass", {kP0, ResultRecord::Status::kPass, ""});
  ledger.Report("fail", {kP0, ResultRecord::Status::kFail, "abc"});
  ledger.Report("crash", {kP0, ResultRecord::Status::kCrash, "xyz"});
  ledger.Report("skip", {kP0, ResultRecord::Status::kSkip, "reason"});
  ledger.Report("not_selected", {kP0, ResultRecord::Status::kNotSelected, ""});
  // The verdict is dropped: a tolerated failure is recorded like any other.
  ledger.set_enforcement_level(kP0);
  ledger.Report("tolerated", {kP1, ResultRecord::Status::kFail, "xyz"});

  EXPECT_TRUE(ledger.WasReported("pass"));
  EXPECT_TRUE(ledger.WasReported("not_selected"));
  EXPECT_EQ(ledger.expected_successes(), 1);
  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_THAT(ledger.UnexpectedFailures(),
              ElementsAre(FieldsAre("crash", "xyz", absl::nullopt)));
  EXPECT_EQ(ledger.unexpected_failures(), 1);
  EXPECT_THAT(ledger.ListedSkips(), ElementsAre(Pair("skip", "skip")));
  EXPECT_EQ(ledger.skipped(), 1);
  EXPECT_EQ(ledger.tolerated_failures(), 1);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  // A test the runner didn't select leaves its entry unseen.
  EXPECT_THAT(ledger.Finalize(), StatusIs(absl::StatusCode::kFailedPrecondition,
                                          HasSubstr("not_selected")));
}

TEST_F(ResultLedgerTest, ReportExpectedFailure) {
  CreateFailureList({{"foo", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportFailure("foo", kP0, "abc"), IsOk());
  EXPECT_THAT(ledger.Finalize(), IsOk());

  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
}

TEST_F(ResultLedgerTest, ReportDuplicates) {
  CreateFailureList({{"foo", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(ledger.ReportSuccess("bar"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("bar"), IsOk());
  ASSERT_THAT(ledger.ReportSkip("baz", "reason"), IsOk());
  ASSERT_THAT(ledger.ReportSkip("baz", "reason"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 1);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 1);
}

TEST_F(ResultLedgerTest, ReportExpectedFailureWildcard) {
  CreateFailureList({{"foo.*.bar", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportFailure("foo.baz.bar", kP0, "abc"), IsOk());
  EXPECT_THAT(ledger.Finalize(), IsOk());

  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
}

TEST_F(ResultLedgerTest, ReportUnseenFailure) {
  CreateFailureList({{"foo.bar"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.Finalize(), StatusIs(absl::StatusCode::kFailedPrecondition,
                                          AllOf(HasSubstr("were not seen"),
                                                HasSubstr("foo.bar"))));

  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
}

TEST_F(ResultLedgerTest, ReportUnseenFailureUnrelatedSkip) {
  CreateFailureList({{"foo.bar"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  // "foo" doesn't match the entry "foo.bar", so the entry stays unseen.
  ASSERT_THAT(ledger.ReportSkip("foo", "reason"), IsOk());

  EXPECT_THAT(ledger.Finalize(), StatusIs(absl::StatusCode::kFailedPrecondition,
                                          AllOf(HasSubstr("were not seen"),
                                                HasSubstr("foo.bar"))));

  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 1);
}

TEST_F(ResultLedgerTest, ReportUnexpectedFailure) {
  CreateFailureList({});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(ledger.ReportFailure("foo_failing", kP0, ""),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("Unexpected failure"),
                             HasSubstr("foo_failing"))));

  EXPECT_THAT(ledger.Finalize(), IsOk());

  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 1);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
}

TEST_F(ResultLedgerTest, ReportUnexpectedFailureMismatchedName) {
  CreateFailureList({{"foo"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(ledger.ReportFailure("foo_failing", kP0, ""),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("Unexpected failure"),
                             HasSubstr("foo_failing"))));
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, ""), IsOk());

  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 1);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
}

TEST_F(ResultLedgerTest, ReportUnexpectedFailureMismatchedMessage) {
  CreateFailureList({{"foo.*.bar", "message"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("Unexpected failure"), HasSubstr("foo"),
                             HasSubstr("message"), HasSubstr("abc"))));
  EXPECT_THAT(ledger.ReportFailure("foo.b.bar", kP0, "message"), IsOk());

  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 1);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_EQ(ledger.skipped(), 0);
}

TEST_F(ResultLedgerTest, ReportFailureMatchesExpectedMessagePrefix) {
  CreateFailureList({{"foo", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  // Like the legacy runner, the actual message only has to start with the
  // expected one.
  EXPECT_THAT(ledger.ReportFailure("foo", kP0, "abc: more details"), IsOk());

  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
}

TEST_F(ResultLedgerTest, ReportFailureRejectsExpectedMessageSuffix) {
  CreateFailureList({{"foo", "abc: more details"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  // The expected message is longer than the actual one, so it's not a prefix.
  EXPECT_THAT(ledger.ReportFailure("foo", kP0, "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Unexpected failure message")));

  // The mismatch is an unexpected failure (which --fix fixes); the entry
  // itself was seen, so Finalize() has nothing to add.
  EXPECT_THAT(ledger.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo", "abc", absl::nullopt)));
  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 0);
  EXPECT_EQ(ledger.unexpected_failures(), 1);
}

TEST_F(ResultLedgerTest, ReportFailureEmptyExpectedMessageMatchesAnything) {
  CreateFailureList({{"foo"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportFailure("foo", kP0, "any message at all"), IsOk());

  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
}

TEST_F(ResultLedgerTest, ReportFailurePrefixMatchIgnoresNewlinesAndWhitespace) {
  CreateFailureList({{"foo", "abc def"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportFailure("foo", kP0, "  abc def\nghi\n"), IsOk());

  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 1);
}

TEST_F(ResultLedgerTest, PrefixMatchedFailureIsNotAnUnexpectedFailure) {
  CreateFailureList({{"foo", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "abc: more details"), IsOk());

  EXPECT_THAT(ledger.UnexpectedFailures(), IsEmpty());
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, FinalizeReportsUnseenExpectedFailures) {
  CreateFailureList({{"ccc"}, {"aaa"}, {"bbb.*.baz"}, {"ddd"}, {"eee"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  // Failures, unexpected successes and skips all count as "seen".
  ASSERT_THAT(ledger.ReportFailure("bbb.x.baz", kP0, ""), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("ccc"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportSkip("ddd", "reason"), Not(IsOk()));
  // Tests that weren't selected to run do not.
  ledger.ReportNotSelected("eee");

  EXPECT_THAT(ledger.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "The following expected failures were not seen: aaa, "
                       "eee"));
}

TEST_F(ResultLedgerTest, UnmatchedExpectedFailures) {
  CreateFailureList({{"ccc"}, {"aaa"}, {"bbb.*.baz"}, {"ddd"}, {"eee", "msg"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.UnmatchedExpectedFailures(),
              ElementsAre("aaa", "bbb.*.baz", "ccc", "ddd", "eee"));

  // Any report whose name matches an entry counts, whatever the outcome.
  ASSERT_THAT(ledger.ReportFailure("bbb.x.baz", kP0, ""), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("ccc"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportSkip("ddd", "reason"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("eee", kP0, "other message"), Not(IsOk()));
  // Reports that match no entry change nothing.
  ASSERT_THAT(ledger.ReportSuccess("aaa.child"), IsOk());
  ASSERT_THAT(ledger.ReportSkip("zzz", "reason"), IsOk());

  EXPECT_THAT(ledger.UnmatchedExpectedFailures(), ElementsAre("aaa"));
  // A message mismatch is a verdict on the entry too: seen, not unseen.
  EXPECT_THAT(ledger.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "The following expected failures were not seen: aaa"));
}

TEST_F(ResultLedgerTest, UnmatchedExpectedFailuresEmpty) {
  ResultLedger ledger;
  EXPECT_THAT(ledger.UnmatchedExpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, UnexpectedFailures) {
  CreateFailureList({{"foo.*.bar", "expected"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.UnexpectedFailures(), IsEmpty());

  // Not in the failure list at all, in the list with another message, and the
  // messages are formatted as SaveFailureList() would write them.
  ASSERT_THAT(ledger.ReportFailure("zzz", kP0, "  new\nfailure  "),
              Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "other message"),
              Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("aaa", kP0, std::string(1000, 'x')),
              Not(IsOk()));
  // Expected failures, successes and skips are not reported.
  ASSERT_THAT(ledger.ReportFailure("foo.b.bar", kP0, "expected: more"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("bbb"), IsOk());
  ASSERT_THAT(ledger.ReportSkip("ccc", "reason"), IsOk());

  EXPECT_THAT(
      ledger.UnexpectedFailures(),
      ElementsAre(FieldsAre("aaa", std::string(128, 'x'), absl::nullopt),
                  FieldsAre("foo.a.bar", "other message", absl::nullopt),
                  FieldsAre("zzz", "newfailure", absl::nullopt)));
  EXPECT_EQ(ledger.unexpected_failures(), 3);
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, OverexpandedWildcardKeepsMatching) {
  CreateFailureList({{"foo.*.bar"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  for (int i = 0; i < 100; ++i) {
    ASSERT_THAT(ledger.ReportSuccess(absl::StrCat("foo.", i, ".bar")),
                Not(IsOk()));
  }
  // The entry is far over the cap, but the verdicts don't change: the test
  // environment fails the run over OverexpandedWildcards() instead.
  ASSERT_THAT(ledger.ReportFailure("foo.baz.bar", kP0, "msg"), IsOk());

  EXPECT_THAT(ledger.OverexpandedWildcards(), ElementsAre("foo.*.bar"));
  EXPECT_THAT(ledger.UnexpectedFailures(), IsEmpty());
  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_THAT(ledger.UnexpectedSuccesses(), SizeIs(100));
  EXPECT_EQ(ledger.unexpected_successes(), 100);
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, UnexpectedFailuresIgnoreDuplicateReports) {
  ResultLedger ledger;

  // Only the first report of a test counts.
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "first"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "second"), Not(IsOk()));

  EXPECT_THAT(ledger.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo", "first", absl::nullopt)));
  EXPECT_EQ(ledger.unexpected_failures(), 1);
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, UnexpectedSuccesses) {
  CreateFailureList({{"foo.*.bar", "wildcard message"}, {"zzz", "  zzz msg "}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.UnexpectedSuccesses(), IsEmpty());

  ASSERT_THAT(ledger.ReportSuccess("zzz"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportSuccess("foo.a.bar"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportSuccess("foo.a.bar"), Not(IsOk()));
  // Expected successes are not reported.
  ASSERT_THAT(ledger.ReportSuccess("aaa"), IsOk());

  // Sorted by test name; the message is the (normalized) one of the entry the
  // test matched.
  EXPECT_THAT(
      ledger.UnexpectedSuccesses(),
      ElementsAre(FieldsAre("foo.a.bar", "wildcard message",
                            Optional(std::string("foo.*.bar"))),
                  FieldsAre("foo.b.bar", "wildcard message",
                            Optional(std::string("foo.*.bar"))),
                  FieldsAre("zzz", "zzz msg", Optional(std::string("zzz")))));
  EXPECT_EQ(ledger.unexpected_successes(), 3);
  EXPECT_THAT(ledger.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, OverexpandedWildcards) {
  CreateFailureList({{"foo.*.bar", "abc"}, {"baz.*", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  // Expected failures and unexpected successes count toward the cap, each
  // test once however often it is reported.
  for (int i = 0; i < kMaximumWildcardExpansions; ++i) {
    ASSERT_THAT(
        ledger.ReportFailure(absl::StrCat("foo.", i, ".bar"), kP0, "abc"),
        IsOk());
    ASSERT_THAT(ledger.ReportSuccess(absl::StrCat("baz.", i)), Not(IsOk()));
  }
  ASSERT_THAT(ledger.ReportFailure("foo.0.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("baz.0"), Not(IsOk()));
  EXPECT_THAT(ledger.OverexpandedWildcards(), IsEmpty());

  // Skips and failures with another message don't count.
  ASSERT_THAT(ledger.ReportSkip("foo.skip.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.other.bar", kP0, "xyz"), Not(IsOk()));
  EXPECT_THAT(ledger.OverexpandedWildcards(), IsEmpty());

  // One more verdict puts an entry over the cap.  The list is sorted.
  ASSERT_THAT(ledger.ReportFailure("foo.more.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("baz.more"), Not(IsOk()));
  EXPECT_THAT(ledger.OverexpandedWildcards(),
              ElementsAre("baz.*", "foo.*.bar"));
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, LoadFailureListInvalidFile) {
  ResultLedger ledger;
  EXPECT_THAT(ledger.LoadFailureList(absl::StrCat(failure_list(), "/invalid")),
              StatusIs(absl::StatusCode::kInternal, HasSubstr("/invalid")));
  ledger.Finalize().IgnoreError();
}

TEST_F(ResultLedgerTest, LoadFailureListDuplicateFailure) {
  CreateFailureList(R"(
    foo # abc
    foo # zyx
)");
  ResultLedger ledger;

  EXPECT_THAT(ledger.LoadFailureList(failure_list()),
              StatusIs(absl::StatusCode::kAlreadyExists,
                       AllOf(HasSubstr("already exists"), HasSubstr("foo"))));
  ledger.Finalize().IgnoreError();
}

TEST_F(ResultLedgerTest, LoadFailureListOverlappingFailure) {
  CreateFailureList(R"(
    foo.*.bar # abc
    foo.bar.* # zyx
)");
  ResultLedger ledger;

  EXPECT_THAT(ledger.LoadFailureList(failure_list()),
              StatusIs(absl::StatusCode::kAlreadyExists,
                       AllOf(HasSubstr("foo.bar.*"), HasSubstr("foo.*.bar"))));
  ledger.Finalize().IgnoreError();
}

TEST_F(ResultLedgerTest, UnexpectedSuccessMessageNamesTheEntry) {
  CreateFailureList({{"foo", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  // The message is the legacy runner's, including the matched entry.
  EXPECT_THAT(ledger.ReportSuccess("foo"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test foo (matched to foo) is in the failure list, but "
                       "test succeeded.  Remove its match from the failure "
                       "list."));
  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.unexpected_successes(), 1);
}

TEST_F(ResultLedgerTest, UnexpectedSuccessMessageNamesTheWildcardEntry) {
  CreateFailureList({{"foo.*.bar", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportSuccess("foo.a.bar"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("test foo.a.bar (matched to foo.*.bar) is in "
                                 "the failure list")));
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, EveryPriorityIsEnforcedByDefault) {
  ResultLedger ledger;
  EXPECT_THAT(ledger.ReportFailure("p0", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(ledger.ReportFailure("p1", kP1, "abc"), Not(IsOk()));
  EXPECT_EQ(ledger.unexpected_failures(), 2);
  EXPECT_EQ(ledger.tolerated_failures(), 0);
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, EnforcementLevelZeroToleratesP1) {
  // Only kP0 is enforced: an unlisted kP1 failure is tolerated.
  ResultLedger ledger;
  ledger.set_enforcement_level(kP0);
  EXPECT_THAT(ledger.ReportFailure("p0", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(ledger.ReportFailure("p1", kP1, "abc"), IsOk());
  EXPECT_EQ(ledger.unexpected_failures(), 1);
  EXPECT_EQ(ledger.tolerated_failures(), 1);
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, ToleratedFailure) {
  CreateFailureList({{"listed", "abc"}});
  ResultLedger ledger;
  ledger.set_enforcement_level(kP0);
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(ledger.ReportFailure("foo", kP1, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bar", kP1, "xyz"), IsOk());
  // Duplicates only count once.
  ASSERT_THAT(ledger.ReportFailure("foo", kP1, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("listed", kP0, "abc"), IsOk());

  EXPECT_EQ(ledger.tolerated_failures(), 2);
  // A tolerated failure is neither a skip nor a failure of any kind, and it
  // isn't written to the failure list.
  EXPECT_EQ(ledger.skipped(), 0);
  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 0);
  EXPECT_EQ(ledger.expected_successes(), 0);
  EXPECT_EQ(ledger.unexpected_successes(), 0);
  EXPECT_THAT(ledger.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());
  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, "listed # abc\n");
}

TEST_F(ResultLedgerTest, ToleratedFailureIsCountedOnceAcrossKinds) {
  ResultLedger ledger;
  ledger.set_enforcement_level(kP0);
  // The same test name can only be counted under one outcome.
  ASSERT_THAT(ledger.ReportSuccess("foo"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo", kP1, "abc"), IsOk());

  EXPECT_EQ(ledger.expected_successes(), 1);
  EXPECT_EQ(ledger.tolerated_failures(), 0);
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, ListedFailureIsCheckedWhateverItsPriority) {
  // A listed test is never tolerated, so that the failure list can't go stale
  // unnoticed.
  CreateFailureList({{"foo", "abc"}, {"bar", "abc"}});
  ResultLedger ledger;
  ledger.set_enforcement_level(kP0);
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportFailure("foo", kP1, "abc"), IsOk());
  EXPECT_THAT(ledger.ReportFailure("bar", kP1, "xyz"), Not(IsOk()));

  EXPECT_EQ(ledger.expected_failures(), 1);
  EXPECT_EQ(ledger.unexpected_failures(), 1);
  EXPECT_EQ(ledger.tolerated_failures(), 0);
  ledger.Finalize().IgnoreError();
}

TEST_F(ResultLedgerTest, LoadFailureListInvalidWildcard) {
  CreateFailureList({{"foo.b*r", "abc"}});
  ResultLedger ledger;
  EXPECT_THAT(ledger.LoadFailureList(failure_list()),
              StatusIs(absl::StatusCode::kInvalidArgument));
  ledger.Finalize().IgnoreError();
}

TEST_F(ResultLedgerTest, LoadFailureListTruncatesMessage) {
  // Like a reported message, so that the two still compare equal.
  std::string message(1000, 'b');
  CreateFailureList({{"foo", message}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportFailure("foo", kP0, message), IsOk());
  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 1);
}

TEST_F(ResultLedgerTest, LoadSecondFailureList) {
  CreateFailureList({{"foo", "abc"}});
  const std::string second = absl::StrCat(failure_list(), ".2");
  ABSL_CHECK_OK(File::SetContents(second, "bar # zyx\n", true));
  const std::string third = absl::StrCat(failure_list(), ".3");
  ABSL_CHECK_OK(File::SetContents(third, "foo # zyx\n", true));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.LoadFailureList(second), IsOk());

  // Entries must not repeat across files either.
  EXPECT_THAT(ledger.LoadFailureList(third),
              StatusIs(absl::StatusCode::kAlreadyExists));
  EXPECT_THAT(ledger.ReportFailure("foo", kP0, "abc"), IsOk());
  EXPECT_THAT(ledger.ReportFailure("bar", kP0, "zyx"), IsOk());
  EXPECT_THAT(ledger.Finalize(), IsOk());
  EXPECT_EQ(ledger.expected_failures(), 2);
}

TEST_F(ResultLedgerTest, ExpectedMessagesAreNormalized) {
  CreateFailureList({{"foo", "  abc  "}, {"bar.*.baz", "xyz"}, {"qux"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  // Like reported messages (see ReportFailure()), so that SaveFailureList()
  // compares like with like.
  const FailureList& failure_list = ledger.failure_list();
  EXPECT_THAT(failure_list.ExpectedMessage("foo"), Optional(Eq("abc")));
  EXPECT_THAT(failure_list.ExpectedMessage("bar.*.baz"), Optional(Eq("xyz")));
  EXPECT_THAT(failure_list.ExpectedMessage("qux"), Optional(Eq("")));
  EXPECT_THAT(ledger.Finalize(), Not(IsOk()));
}

TEST_F(ResultLedgerTest, FailureListMatchesWholeNameComponents) {
  CreateFailureList({{"foo", "abc"}, {"bar.*.baz", "abc"}, {"qux", "abc"}});
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(ledger.ReportFailure("foo", kP0, "abc"), IsOk());
  EXPECT_THAT(ledger.ReportFailure("qux", kP0, "abc"), IsOk());
  EXPECT_THAT(ledger.ReportFailure("bar.a.baz", kP0, "abc"), IsOk());
  EXPECT_THAT(ledger.ReportFailure("bar.b.baz", kP0, "abc"), IsOk());
  // Neither a prefix nor an extension of an entry matches it, and a wildcard
  // stands for exactly one component.
  EXPECT_THAT(ledger.ReportFailure("foo.bar", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(ledger.ReportFailure("bar", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(ledger.ReportFailure("bar.a", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(ledger.ReportFailure("bar.a.baz.qux", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(ledger.ReportFailure("other", kP0, "abc"), Not(IsOk()));

  EXPECT_EQ(ledger.expected_failures(), 4);
  EXPECT_EQ(ledger.unexpected_failures(), 5);
  EXPECT_THAT(ledger.Finalize(), IsOk());
}

TEST_F(ResultLedgerTest, SaveFailureListNoop) {
  CreateFailureList(R"(
foo # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo # abc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListNoopWildcard) {
  CreateFailureList(R"(
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.*.bar # abc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListKeepsEntryWithInternalWhitespace) {
  // LoadFailureList() ignores whitespace anywhere in the name, so the entry
  // matches "foo.bar"; SaveFailureList() must find it under the same key.
  CreateFailureList(R"(
foo .bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo .bar # abc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListKeepsPrefixMatchedEntryVerbatim) {
  CreateFailureList(R"(
foo # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  // The actual message is longer than the entry's, which is only a prefix.
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "abc: more details"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo # abc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListRemoveNewPassing) {
  CreateFailureList(R"(
foo # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("foo"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
)");
}

TEST_F(ResultLedgerTest, SaveFailureListDropsUnseenEntry) {
  CreateFailureList(R"(
foo # abc
bar # def
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bar", kP0, "def"), IsOk());
  ASSERT_THAT(ledger.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "The following expected failures were not seen: foo"));

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
bar # def
)");
}

TEST_F(ResultLedgerTest, SaveFailureListExpandsWildcardWhenOneMatchPasses) {
  // The wildcard no longer describes the failures: the test that passed must
  // go, but the ones that still fail must stay listed, so the wildcard is
  // expanded into them, in its place.
  CreateFailureList(R"(
# Header kept above the expansion.
foo.*.bar # abc
zzz # zzz
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.c.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "abc: details"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("zzz", kP0, "zzz"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# Header kept above the expansion.
foo.a.bar # abc
foo.c.bar # abc
zzz       # zzz
)");
}

TEST_F(ResultLedgerTest, SaveFailureListDropsWildcardWhenEveryMatchPasses) {
  CreateFailureList(R"(
# Header left behind.
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("foo.a.bar"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# Header left behind.
)");
}

TEST_F(ResultLedgerTest,
       SaveFailureListExpandsWildcardWhenOneMatchChangesMessage) {
  // One match fails with a different message while another still fails with
  // the listed one: the wildcard is expanded so that each test gets its own
  // message, and the result loads again (it used to contain the wildcard
  // twice).
  CreateFailureList(R"(
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "xyz"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.b.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.a.bar # xyz
foo.b.bar # abc
)");
  ResultLedger reloaded;
  EXPECT_THAT(reloaded.LoadFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());
  EXPECT_THAT(reloaded.failure_list().ExpectedMessage("foo.a.bar"),
              Optional(std::string("xyz")));
  EXPECT_THAT(reloaded.failure_list().ExpectedMessage("foo.b.bar"),
              Optional(std::string("abc")));
  ASSERT_THAT(reloaded.Finalize(), Not(IsOk()));
}

TEST_F(ResultLedgerTest,
       SaveFailureListExpandsWildcardWhenMatchesChangeToDifferentMessages) {
  CreateFailureList(R"(
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "xyz"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.b.bar", kP0, "uvw"), Not(IsOk()));
  EXPECT_THAT(ledger.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo.a.bar", "xyz", absl::nullopt),
                          FieldsAre("foo.b.bar", "uvw", absl::nullopt)));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.a.bar # xyz
foo.b.bar # uvw
)");
}

TEST_F(ResultLedgerTest,
       SaveFailureListKeepsWildcardWhenEveryMatchChangesToTheSameMessage) {
  // The wildcard still describes the failures, only the message moved on, so
  // the author's wildcard is kept with the new message.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "xyz"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.b.bar", kP0, "xyz"), Not(IsOk()));
  EXPECT_THAT(ledger.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo.a.bar", "xyz", absl::nullopt),
                          FieldsAre("foo.b.bar", "xyz", absl::nullopt)));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.*.bar # xyz
)");
}

TEST_F(ResultLedgerTest, SaveFailureListExpandedWildcardDropsSkippedMatch) {
  // A skipped test can't be an expected failure, so when the wildcard is
  // expanded (here because another match passed) the skipped test gets no
  // line, like the one that passed.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportSkip("foo.a.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.c.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.c.bar # abc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListExpandsWildcardWhenOneMatchIsSkipped) {
  // A skip alone is reason to expand the wildcard, just like a pass: the tests
  // that still fail stay listed, the skipped one goes.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportSkip("foo.b.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.c.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.a.bar # abc
foo.c.bar # abc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListDropsWildcardWhenEveryMatchIsSkipped) {
  CreateFailureList(R"(
# Header left behind.
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportSkip("foo.a.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportSkip("foo.b.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# Header left behind.
)");
}

TEST_F(ResultLedgerTest, SaveFailureListKeepsWildcardWithTooManyMatches) {
  // Exceeding the wildcard expansion cap is reported separately (see
  // OverexpandedWildcards()); the tests do still fail with the listed
  // message, so the entry is kept.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  for (int i = 0; i <= kMaximumWildcardExpansions; ++i) {
    ASSERT_THAT(
        ledger.ReportFailure(absl::StrCat("foo.", i, ".bar"), kP0, "abc"),
        IsOk());
  }
  ASSERT_THAT(ledger.OverexpandedWildcards(), ElementsAre("foo.*.bar"));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.*.bar # abc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListExpansionInterleavesNewFailures) {
  // New failures are inserted by name among the expanded lines too, and the
  // alignment accounts for the expanded names.
  CreateFailureList(R"(
# Block one.
f.*.x # abc

# Block two.
zz # zzz
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("f.aaaa.x", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("f.b.x"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("f.cc.x", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("f.bb.y", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("zz", kP0, "zzz"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# Block one.
f.aaaa.x # abc
f.bb.y   # new
f.cc.x   # abc

# Block two.
zz       # zzz
)");
}

TEST_F(ResultLedgerTest, SaveFailureListChangedMessageStaysInPlace) {
  // An exact entry whose message changed is rewritten where it is, so that it
  // stays under its comment block.
  CreateFailureList(R"(
# About bbb.
bbb # abc

# About aaa.
aaa # aaa
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbb", kP0, "xyz"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("aaa", kP0, "aaa"), IsOk());
  EXPECT_THAT(ledger.UnexpectedFailures(),
              ElementsAre(FieldsAre("bbb", "xyz", absl::nullopt)));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# About bbb.
bbb # xyz

# About aaa.
aaa # aaa
)");
}

TEST_F(ResultLedgerTest, SaveFailureListKeepsMessagelessWildcardVerbatim) {
  CreateFailureList(R"(
foo.*.bar
zzz # zzz
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  // An empty expected message matches any failure.
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.b.bar", kP0, "xyz"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("zzz", kP0, "zzz"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.*.bar
zzz       # zzz
)");
}

TEST_F(ResultLedgerTest, SaveFailureListExpandsMessagelessWildcard) {
  // The expanded lines have no message either: no `#` and no trailing space.
  CreateFailureList(R"(
foo.*.bar
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo.a.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("foo.c.bar", kP0, ""), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.a.bar
foo.c.bar
)");
}

TEST_F(ResultLedgerTest, SaveFailureListDropsSkippedEntry) {
  // A skipped test can't be an expected failure, so its entry goes like that
  // of a test that passed (whether the skip itself is acceptable is decided,
  // and reported, by the caller).
  CreateFailureList(R"(
foo # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportSkip("foo", "reason"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
)");
}

TEST_F(ResultLedgerTest, SaveFailureListUnwritablePath) {
  ResultLedger ledger;
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  // A directory can't be opened for writing.
  const std::string directory = absl::StrCat(failure_list(), ".dir");
  ASSERT_THAT(File::RecursivelyCreateDir(directory, 0777),
              IsOk());
  EXPECT_THAT(ledger.SaveFailureList(directory),
              StatusIs(absl::StatusCode::kInternal, HasSubstr(directory)));
  // Neither can a file in a directory that doesn't exist.
  const std::string missing_directory =
      absl::StrCat(failure_list(), ".missing/list.txt");
  EXPECT_THAT(
      ledger.SaveFailureList(missing_directory),
      StatusIs(absl::StatusCode::kInternal, HasSubstr(missing_directory)));
}

TEST_F(ResultLedgerTest, SaveFailureListWriteFailure) {
  ResultLedger ledger;
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  // /dev/full opens fine but every write to it fails with ENOSPC, which is
  // the only way to reach the write-failure path without a filesystem mock.
  // It is a Linux device.
#ifdef __linux__
  EXPECT_THAT(ledger.SaveFailureList("/dev/full"),
              StatusIs(absl::StatusCode::kInternal,
                       HasSubstr("Failed to write failure list file")));
#else
  GTEST_SKIP() << "/dev/full is Linux-only";
#endif
}

TEST_F(ResultLedgerTest, SaveFailureListChangeFailureMessage) {
  CreateFailureList(R"(
foo # abc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "zyx"), Not(IsOk()));
  EXPECT_THAT(ledger.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo", "zyx", absl::nullopt)));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo # zyx
)");
}

TEST_F(ResultLedgerTest, SaveFailureListAddNewFailure) {
  ResultLedger ledger;
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(foo # abc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListAddNormalizedMessage) {
  ResultLedger ledger;
  std::string message = absl::StrCat("aa\n", std::string(1000, 'b'));
  std::string normalized_message = absl::StrCat("aa", std::string(126, 'b'));
  ASSERT_THAT(ledger.ReportFailure("foo", kP0, message), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::StrCat("foo # ", normalized_message, "\n"));
}

TEST_F(ResultLedgerTest, SaveFailureListInsertAlphabetically) {
  CreateFailureList(R"(
# This is a comment.
aaa # aaa
# This is another comment.
bbb # bbb

ccc # ccc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbb", kP0, "bbb"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("ccc", kP0, "ccc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("abc", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# This is a comment.
aaa # aaa
# This is another comment.
abc # abc
bbb # bbb

ccc # ccc
)");
}

TEST_F(ResultLedgerTest, SaveFailureListInsertAligned) {
  CreateFailureList(R"(
# This is a comment.
aa #  aaa
# This is another comment.
bbbb #bbb

cc #  ccc
)");
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aa", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbbb", kP0, " bbb "), IsOk());
  ASSERT_THAT(ledger.ReportFailure("cc", kP0, "ccc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("abcdef", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# This is a comment.
aa     # aaa
# This is another comment.
abcdef # abc
bbbb   # bbb

cc     # ccc
)");
}

// The copybara marker lines of a strip block (the region the open source
// export drops), spelled in two pieces so that the export's scrubber, which
// processes every file, leaves this test's data alone.
constexpr absl::string_view kStripBegin =
    "# copybara:"
    "strip_begin";
constexpr absl::string_view kStripEnd =
    "# copybara:"
    "strip_end";

TEST_F(ResultLedgerTest, SaveFailureListKeepsUnchangedStripBlockVerbatim) {
  // The block is aligned on its own, so a list whose block is narrower than
  // the rest is a no-op byte for byte.
  CreateFailureList(absl::Substitute(R"(
aaa.long.name # aaa
zzz           # zzz

# Google-only failures.
$0
bbb # bbb
ccc # ccc
$1
)",
                                     kStripBegin, kStripEnd));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa.long.name", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("zzz", kP0, "zzz"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbb", kP0, "bbb"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("ccc", kP0, "ccc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::Substitute(R"(
aaa.long.name # aaa
zzz           # zzz

# Google-only failures.
$0
bbb # bbb
ccc # ccc
$1
)",
                                      kStripBegin, kStripEnd));
}

TEST_F(ResultLedgerTest, SaveFailureListRewritesEntryInsideStripBlockInPlace) {
  CreateFailureList(absl::Substitute(R"(
aaa.long.name # aaa
$0
bbb # bbb
ccc # ccc
$1
)",
                                     kStripBegin, kStripEnd));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa.long.name", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbb", kP0, "new message"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("ccc", kP0, "ccc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::Substitute(R"(
aaa.long.name # aaa
$0
bbb # new message
ccc # ccc
$1
)",
                                      kStripBegin, kStripEnd));
}

TEST_F(ResultLedgerTest, SaveFailureListExpandsWildcardInsideStripBlock) {
  // The expansion takes the wildcard's place and widens the block's column
  // only.
  CreateFailureList(absl::Substitute(R"(
aaa # aaa
$0
b.*.b # bbb
ccc   # ccc
$1
)",
                                     kStripBegin, kStripEnd));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("b.long.b", kP0, "bbb"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("b.x.b"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("ccc", kP0, "ccc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::Substitute(R"(
aaa # aaa
$0
b.long.b # bbb
ccc      # ccc
$1
)",
                                      kStripBegin, kStripEnd));
}

TEST_F(ResultLedgerTest, SaveFailureListDropsPassingEntryInsideStripBlock) {
  CreateFailureList(absl::Substitute(R"(
aaa # aaa
$0
bbb # bbb
ccc # ccc
$1
)",
                                     kStripBegin, kStripEnd));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportSuccess("bbb"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("ccc", kP0, "ccc"), IsOk());
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::Substitute(R"(
aaa # aaa
$0
ccc # ccc
$1
)",
                                      kStripBegin, kStripEnd));
}

TEST_F(ResultLedgerTest, SaveFailureListInsertsNewFailuresOutsideStripBlock) {
  // A new failure goes in front of the first entry outside the block that
  // sorts after it: `aab` (which sorts before the block), `bbb` (which would
  // sort into it) and `yyy` (which sorts after it) all land after the block,
  // before `yyz`; `zzz` sorts last and goes at the end.  The new names only
  // widen the column outside the block.
  CreateFailureList(absl::Substitute(R"(
aaa # aaa
$0
bba # bba
bbc # bbc
$1
yyz # yyz
)",
                                     kStripBegin, kStripEnd));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bba", kP0, "bba"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbc", kP0, "bbc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("yyz", kP0, "yyz"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aab", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("bbb", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("yyy.long", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("zzz", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::Substitute(R"(
aaa      # aaa
$0
bba # bba
bbc # bbc
$1
aab      # new
bbb      # new
yyy.long # new
yyz      # yyz
zzz      # new
)",
                                      kStripBegin, kStripEnd));
}

TEST_F(ResultLedgerTest,
       SaveFailureListAppendsNewFailuresAfterTrailingStripBlock) {
  CreateFailureList(absl::Substitute(R"(
aaa # aaa
# Keep this block last.
$0
bbb # bbb
$1
)",
                                     kStripBegin, kStripEnd));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbb", kP0, "bbb"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("abc", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::Substitute(R"(
aaa # aaa
# Keep this block last.
$0
bbb # bbb
$1
abc # new
)",
                                      kStripBegin, kStripEnd));
}

TEST_F(ResultLedgerTest,
       SaveFailureListTreatsUnbalancedStripMarkersAsComments) {
  // A strip_end with no open block and a strip_begin with no strip_end after
  // it don't delimit anything: everything is one region, aligned together,
  // and new failures are inserted by name as usual.
  CreateFailureList(absl::Substitute(R"(
aaa # aaa
$1
ccc.long # ccc
$0
eee # eee
)",
                                     kStripBegin, kStripEnd));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("ccc.long", kP0, "ccc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("eee", kP0, "eee"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbb", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.ReportFailure("ddd", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::Substitute(R"(
aaa      # aaa
$1
bbb      # new
ccc.long # ccc
$0
ddd      # new
eee      # eee
)",
                                      kStripBegin, kStripEnd));
}

TEST_F(ResultLedgerTest, SaveFailureListIgnoresNestedStripBegin) {
  // A second strip_begin inside a block is a plain comment; the block ends at
  // the first strip_end, and the second strip_end is a plain comment too.
  CreateFailureList(absl::Substitute(R"(
$0
aaa.long # aaa
$0
bbb # bbb
$1
ccc # ccc
$1
)",
                                     kStripBegin, kStripEnd));
  ResultLedger ledger;
  ASSERT_THAT(ledger.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(ledger.ReportFailure("aaa.long", kP0, "aaa"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bbb", kP0, "bbb"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("ccc", kP0, "ccc"), IsOk());
  ASSERT_THAT(ledger.ReportFailure("bba", kP0, "new"), Not(IsOk()));
  ASSERT_THAT(ledger.Finalize(), IsOk());

  EXPECT_THAT(ledger.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::Substitute(R"(
$0
aaa.long # aaa
$0
bbb      # bbb
$1
bba # new
ccc # ccc
$1
)",
                                      kStripBegin, kStripEnd));
}

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
