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
  EXPECT_THAT(manager.UnseenExpectedFailures(), ElementsAre("baz"));
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
  EXPECT_THAT(manager.UnseenExpectedFailures(),
              ElementsAre("bar.*", "baz", "foo"));
  EXPECT_THAT(manager.Finalize(), Not(IsOk()));
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
  EXPECT_EQ(manager.tolerated_recommended_failures(), 0);
}

TEST_F(TestManagerTest, ReportExpectedFailure) {
  CreateFailureList({{"foo", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", "abc"), IsOk());
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
  ASSERT_THAT(manager.ReportFailure("foo", "abc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo", "abc"), IsOk());
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

  EXPECT_THAT(manager.ReportFailure("foo.baz.bar", "abc"), IsOk());
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

  ASSERT_THAT(manager.ReportFailure("foo_failing", ""),
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

  ASSERT_THAT(manager.ReportFailure("foo_failing", ""),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("Unexpected failure"),
                             HasSubstr("foo_failing"))));
  ASSERT_THAT(manager.ReportFailure("foo", ""), IsOk());

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

  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("Unexpected failure"), HasSubstr("foo"),
                             HasSubstr("message"), HasSubstr("abc"))));
  EXPECT_THAT(manager.ReportFailure("foo.b.bar", "message"), IsOk());

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
  EXPECT_THAT(manager.ReportFailure("foo", "abc: more details"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
}

TEST_F(TestManagerTest, ReportFailureRejectsExpectedMessageSuffix) {
  CreateFailureList({{"foo", "abc: more details"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  // The expected message is longer than the actual one, so it's not a prefix.
  EXPECT_THAT(manager.ReportFailure("foo", "abc"),
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

  EXPECT_THAT(manager.ReportFailure("foo", "any message at all"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
}

TEST_F(TestManagerTest, ReportFailurePrefixMatchIgnoresNewlinesAndWhitespace) {
  CreateFailureList({{"foo", "abc def"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", "  abc def\nghi\n"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
}

TEST_F(TestManagerTest, PrefixMatchedFailureIsNotAnUnexpectedFailure) {
  CreateFailureList({{"foo", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(manager.ReportFailure("foo", "abc: more details"), IsOk());

  EXPECT_THAT(manager.UnexpectedFailures(), IsEmpty());
  EXPECT_THAT(manager.UnseenExpectedFailures(), IsEmpty());
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, UnseenExpectedFailures) {
  CreateFailureList({{"ccc"}, {"aaa"}, {"bbb.*.baz"}, {"ddd"}, {"eee"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.UnseenExpectedFailures(),
              ElementsAre("aaa", "bbb.*.baz", "ccc", "ddd", "eee"));

  // Failures, unexpected successes and skips all count as "seen".
  ASSERT_THAT(manager.ReportFailure("bbb.x.baz", ""), IsOk());
  ASSERT_THAT(manager.ReportSuccess("ccc"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSkip("ddd", "reason"), Not(IsOk()));
  // Tests that weren't selected to run do not.
  manager.ReportNotSelected("eee");

  EXPECT_THAT(manager.UnseenExpectedFailures(), ElementsAre("aaa", "eee"));
  EXPECT_THAT(manager.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("were not seen: aaa, eee")));
  EXPECT_THAT(manager.UnseenExpectedFailures(), ElementsAre("aaa", "eee"));
}

TEST_F(TestManagerTest, UnseenExpectedFailuresEmpty) {
  TestManager manager;
  EXPECT_THAT(manager.UnseenExpectedFailures(), IsEmpty());
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, UnmatchedExpectedFailures) {
  CreateFailureList({{"ccc"}, {"aaa"}, {"bbb.*.baz"}, {"ddd"}, {"eee", "msg"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  EXPECT_THAT(manager.UnmatchedExpectedFailures(),
              ElementsAre("aaa", "bbb.*.baz", "ccc", "ddd", "eee"));

  // Any report whose name matches an entry counts, whatever the outcome.
  ASSERT_THAT(manager.ReportFailure("bbb.x.baz", ""), IsOk());
  ASSERT_THAT(manager.ReportSuccess("ccc"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSkip("ddd", "reason"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("eee", "other message"), Not(IsOk()));
  // Reports that match no entry change nothing.
  ASSERT_THAT(manager.ReportSuccess("aaa.child"), IsOk());
  ASSERT_THAT(manager.ReportSkip("zzz", "reason"), IsOk());

  EXPECT_THAT(manager.UnmatchedExpectedFailures(), ElementsAre("aaa"));
  // Message mismatches are still unseen, though.
  EXPECT_THAT(manager.UnseenExpectedFailures(), ElementsAre("aaa", "eee"));
  manager.Finalize().IgnoreError();
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
  ASSERT_THAT(manager.ReportFailure("zzz", "  new\nfailure  "), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "other message"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("aaa", std::string(1000, 'x')),
              Not(IsOk()));
  // Expected failures, successes and skips are not reported.
  ASSERT_THAT(manager.ReportFailure("foo.b.bar", "expected: more"), IsOk());
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
  ASSERT_THAT(manager.ReportFailure("foo.baz.bar", "msg"), Not(IsOk()));

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
  ASSERT_THAT(manager.ReportFailure("foo", "first"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo", "second"), Not(IsOk()));

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
  ASSERT_THAT(manager.ReportFailure("foo.baz.bar", ""),
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

TEST_F(TestManagerTest, AddExpectedFailure) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo", "abc"), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo", "abc"), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());

  EXPECT_EQ(manager.expected_failures(), 1);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
}

TEST_F(TestManagerTest, AddExpectedFailureUnseen) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo", "abc"), IsOk());

  EXPECT_THAT(manager.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       AllOf(HasSubstr("were not seen"), HasSubstr("foo"))));
}

TEST_F(TestManagerTest, AddExpectedFailureUnexpectedSuccess) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo", "abc"), IsOk());

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
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo.*.bar", "abc"), IsOk());

  EXPECT_THAT(manager.ReportSuccess("foo.a.bar"),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("test foo.a.bar (matched to foo.*.bar) is in "
                                 "the failure list")));
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, EnforceRecommendedDefaultsToFalse) {
  TestManager manager;
  EXPECT_FALSE(manager.enforce_recommended());
  manager.set_enforce_recommended(true);
  EXPECT_TRUE(manager.enforce_recommended());
  manager.set_enforce_recommended(false);
  EXPECT_FALSE(manager.enforce_recommended());
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, ReportRecommendedFailure) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("listed", "abc"), IsOk());

  manager.ReportRecommendedFailure("foo");
  manager.ReportRecommendedFailure("bar");
  // Duplicates only count once.
  manager.ReportRecommendedFailure("foo");
  ASSERT_THAT(manager.ReportFailure("listed", "abc"), IsOk());

  EXPECT_EQ(manager.tolerated_recommended_failures(), 2);
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
  EXPECT_EQ(content, "");
}

TEST_F(TestManagerTest, ReportRecommendedFailureIsCountedOnceAcrossKinds) {
  TestManager manager;
  // The same test name can only be counted under one outcome.
  ASSERT_THAT(manager.ReportSuccess("foo"), IsOk());
  manager.ReportRecommendedFailure("foo");

  EXPECT_EQ(manager.expected_successes(), 1);
  EXPECT_EQ(manager.tolerated_recommended_failures(), 0);
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST(TestManagerDeathTest, ReportRecommendedFailureRejectsListedTest) {
  // A listed test must go through ReportFailure() whatever its priority,
  // so that the failure list can't go stale unnoticed.
  EXPECT_DEBUG_DEATH(
      {
        TestManager manager;
        ABSL_CHECK_OK(manager.AddExpectedFailure("listed.*", "abc"));
        manager.ReportRecommendedFailure("listed.foo");
        manager.Finalize().IgnoreError();
      },
      "Recommended test listed.foo is in the failure list and must be "
      "reported with ReportFailure\\(\\)");
}

TEST_F(TestManagerTest, AddExpectedFailureWildcard) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo.*.bar", "abc"), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo.baz.bar", "abc"), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
}

TEST_F(TestManagerTest, AddExpectedFailureDuplicate) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo", "abc"), IsOk());

  EXPECT_THAT(manager.AddExpectedFailure("foo", "zyx"),
              StatusIs(absl::StatusCode::kAlreadyExists,
                       AllOf(HasSubstr("already exists"), HasSubstr("foo"))));
  manager.Finalize().IgnoreError();
}

TEST_F(TestManagerTest, AddExpectedFailureOverlappingWildcard) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo.*.bar", "abc"), IsOk());

  EXPECT_THAT(manager.AddExpectedFailure("foo.bar.*", "zyx"),
              StatusIs(absl::StatusCode::kAlreadyExists,
                       AllOf(HasSubstr("foo.bar.*"), HasSubstr("foo.*.bar"))));
  manager.Finalize().IgnoreError();
}

TEST_F(TestManagerTest, AddExpectedFailureInvalidWildcard) {
  TestManager manager;
  EXPECT_THAT(manager.AddExpectedFailure("foo.b*r", "abc"),
              StatusIs(absl::StatusCode::kInvalidArgument));
  manager.Finalize().IgnoreError();
}

TEST_F(TestManagerTest, AddExpectedFailureNormalizesMessage) {
  TestManager manager;
  std::string message = absl::StrCat("  aa\n", std::string(1000, 'b'), "  ");
  ASSERT_THAT(manager.AddExpectedFailure("foo", message), IsOk());

  // The reported message goes through the same normalization.
  EXPECT_THAT(manager.ReportFailure("foo", message), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 1);
}

TEST_F(TestManagerTest, AddExpectedFailureAfterLoad) {
  CreateFailureList({{"foo", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.AddExpectedFailure("bar", "zyx"), IsOk());

  EXPECT_THAT(manager.AddExpectedFailure("foo", "zyx"),
              StatusIs(absl::StatusCode::kAlreadyExists));
  EXPECT_THAT(manager.ReportFailure("foo", "abc"), IsOk());
  EXPECT_THAT(manager.ReportFailure("bar", "zyx"), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 2);
}

TEST_F(TestManagerTest, IsExpectedToFail) {
  CreateFailureList({{"foo", "abc"}, {"bar.*.baz", "abc"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.AddExpectedFailure("qux", "abc"), IsOk());

  const TestManager& const_manager = manager;
  EXPECT_TRUE(const_manager.IsExpectedToFail("foo"));
  EXPECT_TRUE(const_manager.IsExpectedToFail("qux"));
  EXPECT_TRUE(const_manager.IsExpectedToFail("bar.a.baz"));
  EXPECT_TRUE(const_manager.IsExpectedToFail("bar.b.baz"));
  EXPECT_FALSE(const_manager.IsExpectedToFail("foo.bar"));
  EXPECT_FALSE(const_manager.IsExpectedToFail("bar"));
  EXPECT_FALSE(const_manager.IsExpectedToFail("bar.a"));
  EXPECT_FALSE(const_manager.IsExpectedToFail("bar.a.baz.qux"));
  EXPECT_FALSE(const_manager.IsExpectedToFail("other"));
  EXPECT_FALSE(const_manager.IsExpectedToFail(""));

  // Querying doesn't record anything.
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 0);
  EXPECT_THAT(manager.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("were not seen")));
}

TEST_F(TestManagerTest, IsExpectedToFailEmpty) {
  TestManager manager;
  EXPECT_FALSE(manager.IsExpectedToFail("foo"));
  EXPECT_THAT(manager.Finalize(), IsOk());
}

TEST_F(TestManagerTest, SaveFailureListNoop) {
  CreateFailureList(R"(
foo # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo", "abc"), IsOk());
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
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "abc"), IsOk());
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
  ASSERT_THAT(manager.ReportFailure("foo", "abc: more details"), IsOk());
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
  ASSERT_THAT(manager.ReportFailure("foo.b.bar", "abc"), IsOk());
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
  ASSERT_THAT(manager.ReportFailure("foo", "abc"), Not(IsOk()));
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
  ASSERT_THAT(manager.ReportFailure("foo", "abc"), Not(IsOk()));
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
  ASSERT_THAT(manager.ReportFailure("foo", "zyx"), Not(IsOk()));
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
  ASSERT_THAT(manager.ReportFailure("foo", "abc"), Not(IsOk()));
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
  ASSERT_THAT(manager.ReportFailure("foo", message), Not(IsOk()));
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
  ASSERT_THAT(manager.ReportFailure("aaa", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbb", "bbb"), IsOk());
  ASSERT_THAT(manager.ReportFailure("ccc", "ccc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("abc", "abc"), Not(IsOk()));
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
  ASSERT_THAT(manager.ReportFailure("aa", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbbb", " bbb "), IsOk());
  ASSERT_THAT(manager.ReportFailure("cc", "ccc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("abcdef", "abc"), Not(IsOk()));
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
