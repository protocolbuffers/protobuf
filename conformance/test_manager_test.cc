#include "conformance/test_manager.h"

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
using ::testing::ElementsAre;
using ::testing::FieldsAre;
using ::testing::HasSubstr;
using ::testing::IsEmpty;
using ::testing::Not;
using ::testing::Optional;
using ::testing::Pair;
using ::testing::SizeIs;

class TestManagerTest : public ::testing::Test {
 protected:
  TestManagerTest()
      : tmp_dir_(absl::StrCat(TestTempDir(), "/test_manager_test/")),
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

  ~TestManagerTest() override {
    File::DeleteRecursively(failure_list_path_, NULL, NULL);
  }

 private:
  std::string tmp_dir_;
  std::string failure_list_path_;
  std::string output_path_;
};

TEST_F(TestManagerTest, RequiresFinalize) {
  EXPECT_DEATH({ TestManager manager; }, "Finalize");
}

TEST_F(TestManagerTest, ReportSuccess) {
  CreateFailureList({});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportSuccess("foo"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 1);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, ReportSkipped) {
  CreateFailureList({});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportSkip("foo", "reason"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 1);
  EXPECT_EQ(manager.listed_skips(), 0);
  EXPECT_THAT(manager.ListedSkips(), IsEmpty());
}

TEST_F(TestManagerTest, ReportSkipMarksListedEntrySeenAndMatched) {
  CreateFailureList({{"foo", "abc"}, {"bar.*", "abc"}, {"baz", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  // A listed skip is reported like an unexpected success, naming the entry.
  EXPECT_THAT(manager.ReportSkip("foo", "not supported"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test foo (matched to foo) is in the failure list but "
                       "was skipped by the testee: not supported.  Remove its "
                       "match from the failure list."));
  EXPECT_THAT(manager.ReportSkip("bar.x", "no"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test bar.x (matched to bar.*) is in the failure list "
                       "but was skipped by the testee: no.  Remove its match "
                       "from the failure list."));
  // Reporting a test twice returns the error again but counts it once.
  EXPECT_THAT(manager.ReportSkip("bar.x", "no"), Not(IsOk()));

  // A skip is only counted as a skip, but the entries it matched are neither
  // unmatched nor unseen: whether the entry is still needed is unknown.
  EXPECT_THAT(manager.UnmatchedExpectedFailures(), ElementsAre("baz"));
  EXPECT_THAT(manager.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("were not seen: baz")));
  EXPECT_EQ(manager.skipped(), 2);
  EXPECT_EQ(manager.listed_skips(), 2);
  EXPECT_THAT(manager.ListedSkips(),
              ElementsAre(Pair("bar.x", "bar.*"), Pair("foo", "foo")));
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_THAT(manager.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(manager.UnexpectedSuccesses(), IsEmpty());
}

TEST_F(TestManagerTest, ReportNotSelected) {
  CreateFailureList({{"foo"}, {"bar.*"}, {"baz"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  manager.ReportNotSelected("foo");
  manager.ReportNotSelected("bar.x");
  manager.ReportNotSelected("unlisted");

  // Only the matched entries stop being unmatched; nothing else changes.
  EXPECT_THAT(manager.UnmatchedExpectedFailures(), ElementsAre("baz"));
  EXPECT_THAT(manager.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "The following expected failures were not seen: bar.*, "
                       "baz, foo"));
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
  EXPECT_EQ(manager.tolerated_failures(), 0);
}

TEST_F(TestManagerTest, WasReported) {
  CreateFailureList({});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  EXPECT_FALSE(manager.WasReported("success"));

  EXPECT_THAT(manager.ReportSuccess("success"), IsOk());
  EXPECT_THAT(manager.ReportFailure("failure", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(manager.ReportSkip("skip", "reason"), IsOk());
  manager.ReportNotSelected("not_selected");

  EXPECT_TRUE(manager.WasReported("success"));
  EXPECT_TRUE(manager.WasReported("failure"));
  EXPECT_TRUE(manager.WasReported("skip"));
  EXPECT_TRUE(manager.WasReported("not_selected"));
  EXPECT_FALSE(manager.WasReported("other"));
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, ReportExpectedFailure) {
  CreateFailureList({{"foo", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", kP0, "abc"), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());

  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, ReportDuplicates) {
  CreateFailureList({{"foo", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(manager.ReportSuccess("bar"), IsOk());
  ASSERT_THAT(manager.ReportSuccess("bar"), IsOk());
  ASSERT_THAT(manager.ReportSkip("baz", "reason"), IsOk());
  ASSERT_THAT(manager.ReportSkip("baz", "reason"), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "abc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "abc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 1);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 1);
}

TEST_F(TestManagerTest, ReportExpectedFailureWildcard) {
  CreateFailureList({{"foo.*.bar", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo.baz.bar", kP0, "abc"), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());

  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, ReportUnseenFailure) {
  CreateFailureList({{"foo.bar"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(
      manager.Finalize(),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               AllOf(HasSubstr("were not seen"), HasSubstr("foo.bar"))));

  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, ReportUnseenFailureUnrelatedSkip) {
  CreateFailureList({{"foo.bar"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  // "foo" doesn't match the entry "foo.bar", so the entry stays unseen.
  ASSERT_THAT(manager.ReportSkip("foo", "reason"), IsOk());

  EXPECT_THAT(
      manager.Finalize(),
      StatusIs(absl::StatusCode::kFailedPrecondition,
               AllOf(HasSubstr("were not seen"), HasSubstr("foo.bar"))));

  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 1);
}

TEST_F(TestManagerTest, ReportUnexpectedFailure) {
  CreateFailureList({});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(manager.ReportFailure("foo_failing", kP0, ""),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("Unexpected failure"),
                             HasSubstr("foo_failing"))));

  EXPECT_THAT(manager.Finalize(), IsOk());

  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 1);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, ReportUnexpectedFailureMismatchedName) {
  CreateFailureList({{"foo"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(manager.ReportFailure("foo_failing", kP0, ""),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("Unexpected failure"),
                             HasSubstr("foo_failing"))));
  ASSERT_THAT(manager.ReportFailure("foo", kP0, ""), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 1);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, ReportUnexpectedFailureMismatchedMessage) {
  CreateFailureList({{"foo.*.bar", "message"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(manager.ReportFailure("foo.a.bar", kP0, "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("Unexpected failure"), HasSubstr("foo"),
                             HasSubstr("message"), HasSubstr("abc"))));
  EXPECT_THAT(manager.ReportFailure("foo.b.bar", kP0, "message"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 1);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, ReportFailureMatchesExpectedMessagePrefix) {
  CreateFailureList({{"foo", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  // Like the legacy runner, the actual message only has to start with the
  // expected one.
  EXPECT_THAT(manager.ReportFailure("foo", kP0, "abc: more details"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
}

TEST_F(TestManagerTest, ReportFailureRejectsExpectedMessageSuffix) {
  CreateFailureList({{"foo", "abc: more details"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  // The expected message is longer than the actual one, so it's not a prefix.
  EXPECT_THAT(manager.ReportFailure("foo", kP0, "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("Unexpected failure message")));

  // Like any other message mismatch, the stale entry is still reported as
  // unseen so that --fix replaces it.
  EXPECT_THAT(manager.Finalize(), Not(IsOk()));
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 1);
}

TEST_F(TestManagerTest, ReportFailureEmptyExpectedMessageMatchesAnything) {
  CreateFailureList({{"foo"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", kP0, "any message at all"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
}

TEST_F(TestManagerTest, ReportFailurePrefixMatchIgnoresNewlinesAndWhitespace) {
  CreateFailureList({{"foo", "abc def"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", kP0, "  abc def\nghi\n"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
}

TEST_F(TestManagerTest, PrefixMatchedFailureIsNotAnUnexpectedFailure) {
  CreateFailureList({{"foo", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(manager.ReportFailure("foo", kP0, "abc: more details"), IsOk());

  EXPECT_THAT(manager.UnexpectedFailures(), IsEmpty());
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, FinalizeReportsUnseenExpectedFailures) {
  CreateFailureList({{"ccc"}, {"aaa"}, {"bbb.*.baz"}, {"ddd"}, {"eee"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  // Failures, unexpected successes and skips all count as "seen".
  ASSERT_THAT(manager.ReportFailure("bbb.x.baz", kP0, ""), IsOk());
  ASSERT_THAT(manager.ReportSuccess("ccc"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSkip("ddd", "reason"), Not(IsOk()));
  // Tests that weren't selected to run do not.
  manager.ReportNotSelected("eee");

  EXPECT_THAT(manager.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "The following expected failures were not seen: aaa, "
                       "eee"));
}

TEST_F(TestManagerTest, UnmatchedExpectedFailures) {
  CreateFailureList({{"ccc"}, {"aaa"}, {"bbb.*.baz"}, {"ddd"}, {"eee", "msg"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.UnmatchedExpectedFailures(),
              ElementsAre("aaa", "bbb.*.baz", "ccc", "ddd", "eee"));

  // Any report whose name matches an entry counts, whatever the outcome.
  ASSERT_THAT(manager.ReportFailure("bbb.x.baz", kP0, ""), IsOk());
  ASSERT_THAT(manager.ReportSuccess("ccc"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSkip("ddd", "reason"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("eee", kP0, "other message"), Not(IsOk()));
  // Reports that match no entry change nothing.
  ASSERT_THAT(manager.ReportSuccess("aaa.child"), IsOk());
  ASSERT_THAT(manager.ReportSkip("zzz", "reason"), IsOk());

  EXPECT_THAT(manager.UnmatchedExpectedFailures(), ElementsAre("aaa"));
  // Message mismatches are still unseen, though.
  EXPECT_THAT(manager.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "The following expected failures were not seen: aaa, "
                       "eee"));
}

TEST_F(TestManagerTest, UnmatchedExpectedFailuresEmpty) {
  TestManager manager;
  EXPECT_THAT(manager.UnmatchedExpectedFailures(), IsEmpty());
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, UnexpectedFailures) {
  CreateFailureList({{"foo.*.bar", "expected"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.UnexpectedFailures(), IsEmpty());

  // Not in the failure list at all, in the list with another message, and the
  // messages are formatted as SaveFailureList() would write them.
  ASSERT_THAT(manager.ReportFailure("zzz", kP0, "  new\nfailure  "),
              Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", kP0, "other message"),
              Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("aaa", kP0, std::string(1000, 'x')),
              Not(IsOk()));
  // Expected failures, successes and skips are not reported.
  ASSERT_THAT(manager.ReportFailure("foo.b.bar", kP0, "expected: more"),
              IsOk());
  ASSERT_THAT(manager.ReportSuccess("bbb"), IsOk());
  ASSERT_THAT(manager.ReportSkip("ccc", "reason"), IsOk());

  EXPECT_THAT(
      manager.UnexpectedFailures(),
      ElementsAre(FieldsAre("aaa", std::string(128, 'x'), absl::nullopt),
                  FieldsAre("foo.a.bar", "other message", absl::nullopt),
                  FieldsAre("zzz", "newfailure", absl::nullopt)));
  EXPECT_EQ(manager.unexpected_failures(), 3);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, UnexpectedFailuresIncludeExceededWildcardMatches) {
  CreateFailureList({{"foo.*.bar"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  for (int i = 0; i < 100; ++i) {
    ASSERT_THAT(manager.ReportSuccess(absl::StrCat("foo.", i, ".bar")),
                Not(IsOk()));
  }
  ASSERT_THAT(manager.ReportFailure("foo.baz.bar", kP0, "msg"), Not(IsOk()));

  EXPECT_THAT(manager.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo.baz.bar", "msg", absl::nullopt)));
  EXPECT_EQ(manager.unexpected_failures(), 1);
  EXPECT_THAT(manager.UnexpectedSuccesses(), SizeIs(100));
  EXPECT_EQ(manager.unexpected_successes(), 100);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, UnexpectedFailuresIgnoreDuplicateReports) {
  TestManager manager;

  // Only the first report of a test counts.
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "first"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "second"), Not(IsOk()));

  EXPECT_THAT(manager.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo", "first", absl::nullopt)));
  EXPECT_EQ(manager.unexpected_failures(), 1);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, UnexpectedSuccesses) {
  CreateFailureList({{"foo.*.bar", "wildcard message"}, {"zzz", "  zzz msg "}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.UnexpectedSuccesses(), IsEmpty());

  ASSERT_THAT(manager.ReportSuccess("zzz"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSuccess("foo.a.bar"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSuccess("foo.a.bar"), Not(IsOk()));
  // Expected successes are not reported.
  ASSERT_THAT(manager.ReportSuccess("aaa"), IsOk());

  // Sorted by test name; the message is the (normalized) one of the entry the
  // test matched.
  EXPECT_THAT(
      manager.UnexpectedSuccesses(),
      ElementsAre(FieldsAre("foo.a.bar", "wildcard message",
                            Optional(std::string("foo.*.bar"))),
                  FieldsAre("foo.b.bar", "wildcard message",
                            Optional(std::string("foo.*.bar"))),
                  FieldsAre("zzz", "zzz msg", Optional(std::string("zzz")))));
  EXPECT_EQ(manager.unexpected_successes(), 3);
  EXPECT_THAT(manager.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, ReportUnexpectedFailureTooManyWildcardMatches) {
  CreateFailureList({{"foo.*.bar"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  for (int i = 0; i < 100; ++i) {
    EXPECT_THAT(manager.ReportSuccess(absl::StrCat("foo.", i, ".bar")),
                StatusIs(absl::StatusCode::kFailedPrecondition));
  }
  ASSERT_THAT(manager.ReportFailure("foo.baz.bar", kP0, ""),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("too many test names"),
                             HasSubstr("foo.baz.bar"))));

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 1);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 100);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, LoadFailureListInvalidFile) {
  TestManager manager;
  EXPECT_THAT(manager.LoadFailureList(absl::StrCat(failure_list(), "/invalid")),
              StatusIs(absl::StatusCode::kInternal, HasSubstr("/invalid")));
  manager.Finalize().IgnoreError();
}

TEST_F(TestManagerTest, LoadFailureListDuplicateFailure) {
  CreateFailureList(R"(
    foo # abc
    foo # zyx
)");
  TestManager manager;

  EXPECT_THAT(manager.LoadFailureList(failure_list()),
              StatusIs(absl::StatusCode::kAlreadyExists,
                       AllOf(HasSubstr("already exists"), HasSubstr("foo"))));
  manager.Finalize().IgnoreError();
}

TEST_F(TestManagerTest, LoadFailureListOverlappingFailure) {
  CreateFailureList(R"(
    foo.*.bar # abc
    foo.bar.* # zyx
)");
  TestManager manager;

  EXPECT_THAT(manager.LoadFailureList(failure_list()),
              StatusIs(absl::StatusCode::kAlreadyExists,
                       AllOf(HasSubstr("foo.bar.*"), HasSubstr("foo.*.bar"))));
  manager.Finalize().IgnoreError();
}

TEST_F(TestManagerTest, UnexpectedSuccessMessageNamesTheEntry) {
  CreateFailureList({{"foo", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  // The message is the legacy runner's, including the matched entry.
  EXPECT_THAT(manager.ReportSuccess("foo"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       "test foo (matched to foo) is in the failure list, but "
                       "test succeeded.  Remove its match from the failure "
                       "list."));
  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.unexpected_successes(), 1);
}

TEST_F(TestManagerTest, UnexpectedSuccessMessageNamesTheWildcardEntry) {
  CreateFailureList({{"foo.*.bar", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportSuccess("foo.a.bar"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("test foo.a.bar (matched to foo.*.bar) is in "
                                 "the failure list")));
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, EveryPriorityIsEnforcedByDefault) {
  TestManager manager;
  EXPECT_THAT(manager.ReportFailure("p0", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(manager.ReportFailure("p1", kP1, "abc"), Not(IsOk()));
  EXPECT_EQ(manager.unexpected_failures(), 2);
  EXPECT_EQ(manager.tolerated_failures(), 0);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, EnforcementLevelZeroToleratesP1) {
  // Only kP0 is enforced: an unlisted kP1 failure is tolerated.
  TestManager manager;
  manager.set_enforcement_level(kP0);
  EXPECT_THAT(manager.ReportFailure("p0", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(manager.ReportFailure("p1", kP1, "abc"), IsOk());
  EXPECT_EQ(manager.unexpected_failures(), 1);
  EXPECT_EQ(manager.tolerated_failures(), 1);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, ToleratedFailure) {
  CreateFailureList({{"listed", "abc"}});
  TestManager manager;
  manager.set_enforcement_level(kP0);
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(manager.ReportFailure("foo", kP1, "abc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bar", kP1, "xyz"), IsOk());
  // Duplicates only count once.
  ASSERT_THAT(manager.ReportFailure("foo", kP1, "abc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("listed", kP0, "abc"), IsOk());

  EXPECT_EQ(manager.tolerated_failures(), 2);
  // A tolerated failure is neither a skip nor a failure of any kind, and it
  // isn't written to the failure list.
  EXPECT_EQ(manager.skipped(), 0);
  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_THAT(manager.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());
  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, "listed # abc\n");
}

TEST_F(TestManagerTest, ToleratedFailureIsCountedOnceAcrossKinds) {
  TestManager manager;
  manager.set_enforcement_level(kP0);
  // The same test name can only be counted under one outcome.
  ASSERT_THAT(manager.ReportSuccess("foo"), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo", kP1, "abc"), IsOk());

  EXPECT_EQ(manager.expected_successes(), 1);
  EXPECT_EQ(manager.tolerated_failures(), 0);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, ListedFailureIsCheckedWhateverItsPriority) {
  // A listed test is never tolerated, so that the failure list can't go stale
  // unnoticed.
  CreateFailureList({{"foo", "abc"}, {"bar", "abc"}});
  TestManager manager;
  manager.set_enforcement_level(kP0);
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", kP1, "abc"), IsOk());
  EXPECT_THAT(manager.ReportFailure("bar", kP1, "xyz"), Not(IsOk()));

  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 1);
  EXPECT_EQ(manager.tolerated_failures(), 0);
  manager.Finalize().IgnoreError();
}

TEST_F(TestManagerTest, LoadFailureListInvalidWildcard) {
  CreateFailureList({{"foo.b*r", "abc"}});
  TestManager manager;
  EXPECT_THAT(manager.LoadFailureList(failure_list()),
              StatusIs(absl::StatusCode::kInvalidArgument));
  manager.Finalize().IgnoreError();
}

TEST_F(TestManagerTest, LoadFailureListTruncatesMessage) {
  // Like a reported message, so that the two still compare equal.
  std::string message(1000, 'b');
  CreateFailureList({{"foo", message}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", kP0, message), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
}

TEST_F(TestManagerTest, LoadSecondFailureList) {
  CreateFailureList({{"foo", "abc"}});
  const std::string second = absl::StrCat(failure_list(), ".2");
  ABSL_CHECK_OK(File::SetContents(second, "bar # zyx\n", true));
  const std::string third = absl::StrCat(failure_list(), ".3");
  ABSL_CHECK_OK(File::SetContents(third, "foo # zyx\n", true));
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.LoadFailureList(second), IsOk());

  // Entries must not repeat across files either.
  EXPECT_THAT(manager.LoadFailureList(third),
              StatusIs(absl::StatusCode::kAlreadyExists));
  EXPECT_THAT(manager.ReportFailure("foo", kP0, "abc"), IsOk());
  EXPECT_THAT(manager.ReportFailure("bar", kP0, "zyx"), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 2);
}

TEST_F(TestManagerTest, FailureListMatchesWholeNameComponents) {
  CreateFailureList({{"foo", "abc"}, {"bar.*.baz", "abc"}, {"qux", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", kP0, "abc"), IsOk());
  EXPECT_THAT(manager.ReportFailure("qux", kP0, "abc"), IsOk());
  EXPECT_THAT(manager.ReportFailure("bar.a.baz", kP0, "abc"), IsOk());
  EXPECT_THAT(manager.ReportFailure("bar.b.baz", kP0, "abc"), IsOk());
  // Neither a prefix nor an extension of an entry matches it, and a wildcard
  // stands for exactly one component.
  EXPECT_THAT(manager.ReportFailure("foo.bar", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(manager.ReportFailure("bar", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(manager.ReportFailure("bar.a", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(manager.ReportFailure("bar.a.baz.qux", kP0, "abc"), Not(IsOk()));
  EXPECT_THAT(manager.ReportFailure("other", kP0, "abc"), Not(IsOk()));

  EXPECT_EQ(manager.expected_failures(), 4);
  EXPECT_EQ(manager.unexpected_failures(), 5);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, SaveFailureListNoop) {
  CreateFailureList(R"(
foo # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "abc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo # abc
)");
}

TEST_F(TestManagerTest, SaveFailureListNoopWildcard) {
  CreateFailureList(R"(
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.*.bar # abc
)");
}

TEST_F(TestManagerTest, SaveFailureListKeepsPrefixMatchedEntryVerbatim) {
  CreateFailureList(R"(
foo # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  // The actual message is longer than the entry's, which is only a prefix.
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "abc: more details"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo # abc
)");
}

TEST_F(TestManagerTest, SaveFailureListRemoveNewPassing) {
  CreateFailureList(R"(
foo # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportSuccess("foo"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
)");
}

TEST_F(TestManagerTest, SaveFailureListRemovePartialPassingWildcard) {
  // This is a weird case where the wildcard no longer fully matches the
  // failure.  Users would have to updated the failure list twice to get the
  // failure list to be correct.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportSuccess("foo.a.bar"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.b.bar", kP0, "abc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
)");
}

TEST_F(TestManagerTest, SaveFailureListKeepsSkippedEntry) {
  // A skipped test says nothing about whether its entry is still needed, so
  // the entry is kept verbatim (whether the skip itself is acceptable is
  // decided, and reported, by the caller).
  CreateFailureList(R"(
foo # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportSkip("foo", "reason"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo # abc
)");
}

TEST_F(TestManagerTest, SaveFailureListUnwritablePath) {
  TestManager manager;
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  // A directory can't be opened for writing.
  const std::string directory = absl::StrCat(failure_list(), ".dir");
  ASSERT_THAT(File::RecursivelyCreateDir(directory, 0777),
              IsOk());
  EXPECT_THAT(manager.SaveFailureList(directory),
              StatusIs(absl::StatusCode::kInternal, HasSubstr(directory)));
  // Neither can a file in a directory that doesn't exist.
  const std::string missing_directory =
      absl::StrCat(failure_list(), ".missing/list.txt");
  EXPECT_THAT(
      manager.SaveFailureList(missing_directory),
      StatusIs(absl::StatusCode::kInternal, HasSubstr(missing_directory)));
}

TEST_F(TestManagerTest, SaveFailureListWriteFailure) {
  TestManager manager;
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  // /dev/full opens fine but every write to it fails with ENOSPC, which is
  // the only way to reach the write-failure path without a filesystem mock.
  // It is a Linux device.
#ifdef __linux__
  EXPECT_THAT(manager.SaveFailureList("/dev/full"),
              StatusIs(absl::StatusCode::kInternal,
                       HasSubstr("Failed to write failure list file")));
#else
  GTEST_SKIP() << "/dev/full is Linux-only";
#endif
}

TEST_F(TestManagerTest, SaveFailureListChangeFailureMessage) {
  CreateFailureList(R"(
foo # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "zyx"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), Not(IsOk()));

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo # zyx
)");
}

TEST_F(TestManagerTest, SaveFailureListAddNewFailure) {
  TestManager manager;
  ASSERT_THAT(manager.ReportFailure("foo", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(foo # abc
)");
}

TEST_F(TestManagerTest, SaveFailureListAddNormalizedMessage) {
  TestManager manager;
  std::string message = absl::StrCat("aa\n", std::string(1000, 'b'));
  std::string normalized_message = absl::StrCat("aa", std::string(126, 'b'));
  ASSERT_THAT(manager.ReportFailure("foo", kP0, message), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, absl::StrCat("foo # ", normalized_message, "\n"));
}

TEST_F(TestManagerTest, SaveFailureListInsertAlphabetically) {
  CreateFailureList(R"(
# This is a comment.
aaa # aaa
# This is another comment.
bbb # bbb

ccc # ccc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa", kP0, "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbb", kP0, "bbb"), IsOk());
  ASSERT_THAT(manager.ReportFailure("ccc", kP0, "ccc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("abc", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListInsertAligned) {
  CreateFailureList(R"(
# This is a comment.
aa #  aaa
# This is another comment.
bbbb #bbb

cc #  ccc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aa", kP0, "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbbb", kP0, " bbb "), IsOk());
  ASSERT_THAT(manager.ReportFailure("cc", kP0, "ccc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("abcdef", kP0, "abc"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

}  // namespace
}  // namespace internal
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
