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
#include "absl/strings/substitute.h"
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
using ::testing::Eq;
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
  // unmatched nor unseen: dropping the match is left to SaveFailureList().
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

  // The mismatch is an unexpected failure (which --fix fixes); the entry
  // itself was seen, so Finalize() has nothing to add.
  EXPECT_THAT(manager.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo", "abc", absl::nullopt)));
  EXPECT_THAT(manager.UnseenExpectedFailures(), IsEmpty());
  EXPECT_THAT(manager.Finalize(), IsOk());
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
  // A message mismatch is a verdict on the entry too: seen, not unseen.
  EXPECT_THAT(manager.UnseenExpectedFailures(), ElementsAre("aaa"));
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

TEST_F(TestManagerTest, AddExpectedFailureWildcardWithinASection) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo.b*r", "abc"), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo.bar", "abc"), IsOk());
  EXPECT_THAT(manager.ReportFailure("foo.bazaar", "abc"), IsOk());
  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 2);
}

TEST_F(TestManagerTest, AddExpectedFailureWildcardOfParameters) {
  TestManager manager;
  ASSERT_THAT(manager.AddExpectedFailure("foo.bar/*.baz", "abc"), IsOk());

  EXPECT_THAT(manager.ReportFailure("foo.bar/Proto2_INT32.baz", "abc"), IsOk());
  EXPECT_THAT(manager.ReportFailure("foo.bar.baz", "abc"),
              StatusIs(absl::StatusCode::kFailedPrecondition));
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

TEST_F(TestManagerTest, ExpectedFailureMessage) {
  CreateFailureList({{"foo", "  abc  "}, {"bar.*.baz", "xyz"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.AddExpectedFailure("qux", ""), IsOk());

  const TestManager& const_manager = manager;
  // Messages are normalized like reported ones (see ReportFailure()).
  EXPECT_THAT(const_manager.ExpectedFailureMessage("foo"), Optional(Eq("abc")));
  EXPECT_THAT(const_manager.ExpectedFailureMessage("bar.*.baz"),
              Optional(Eq("xyz")));
  EXPECT_THAT(const_manager.ExpectedFailureMessage("qux"), Optional(Eq("")));
  // Entries are looked up as listed, not matched like test names.
  EXPECT_EQ(const_manager.ExpectedFailureMessage("bar.a.baz"), absl::nullopt);
  EXPECT_EQ(const_manager.ExpectedFailureMessage("other"), absl::nullopt);

  // Querying doesn't record anything.
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_THAT(manager.UnseenExpectedFailures(),
              ElementsAre("bar.*.baz", "foo", "qux"));
  EXPECT_THAT(manager.Finalize(),
              StatusIs(absl::StatusCode::kFailedPrecondition,
                       HasSubstr("were not seen")));
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

TEST_F(TestManagerTest, SaveFailureListKeepsEntryWithInternalWhitespace) {
  // LoadFailureList() ignores whitespace anywhere in the name, so the entry
  // matches "foo.bar"; SaveFailureList() must find it under the same key.
  CreateFailureList(R"(
foo .bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.bar", "abc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo .bar # abc
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

TEST_F(TestManagerTest, SaveFailureListExpandsWildcardWhenOneMatchPasses) {
  // The wildcard no longer describes the failures: the test that passed must
  // go, but the ones that still fail must stay listed, so the wildcard is
  // expanded into them, in its place.
  CreateFailureList(R"(
# Header kept above the expansion.
foo.*.bar # abc
zzz # zzz
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.c.bar", "abc"), IsOk());
  ASSERT_THAT(manager.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "abc: details"), IsOk());
  ASSERT_THAT(manager.ReportFailure("zzz", "zzz"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListDropsWildcardWhenEveryMatchPasses) {
  CreateFailureList(R"(
# Header left behind.
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportSuccess("foo.a.bar"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# Header left behind.
)");
}

TEST_F(TestManagerTest,
       SaveFailureListExpandsWildcardWhenOneMatchChangesMessage) {
  // One match fails with a different message while another still fails with
  // the listed one: the wildcard is expanded so that each test gets its own
  // message, and the result loads again (it used to contain the wildcard
  // twice).
  CreateFailureList(R"(
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "xyz"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.b.bar", "abc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.a.bar # xyz
foo.b.bar # abc
)");
  TestManager reloaded;
  EXPECT_THAT(reloaded.LoadFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());
  EXPECT_THAT(reloaded.ExpectedFailureMessage("foo.a.bar"),
              Optional(std::string("xyz")));
  EXPECT_THAT(reloaded.ExpectedFailureMessage("foo.b.bar"),
              Optional(std::string("abc")));
  ASSERT_THAT(reloaded.Finalize(), Not(IsOk()));
}

TEST_F(TestManagerTest,
       SaveFailureListExpandsWildcardWhenMatchesChangeToDifferentMessages) {
  CreateFailureList(R"(
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "xyz"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.b.bar", "uvw"), Not(IsOk()));
  EXPECT_THAT(manager.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo.a.bar", "xyz", absl::nullopt),
                          FieldsAre("foo.b.bar", "uvw", absl::nullopt)));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest,
       SaveFailureListKeepsWildcardWhenEveryMatchChangesToTheSameMessage) {
  // The wildcard still describes the failures, only the message moved on, so
  // the author's wildcard is kept with the new message.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "xyz"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.b.bar", "xyz"), Not(IsOk()));
  EXPECT_THAT(manager.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo.a.bar", "xyz", absl::nullopt),
                          FieldsAre("foo.b.bar", "xyz", absl::nullopt)));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.*.bar # xyz
)");
}

TEST_F(TestManagerTest, SaveFailureListExpandedWildcardDropsSkippedMatch) {
  // A skipped test can't be an expected failure, so when the wildcard is
  // expanded (here because another match passed) the skipped test gets no
  // line, like the one that passed.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportSkip("foo.a.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.c.bar", "abc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
foo.c.bar # abc
)");
}

TEST_F(TestManagerTest, SaveFailureListExpandsWildcardWhenOneMatchIsSkipped) {
  // A skip alone is reason to expand the wildcard, just like a pass: the tests
  // that still fail stay listed, the skipped one goes.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "abc"), IsOk());
  ASSERT_THAT(manager.ReportSkip("foo.b.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.c.bar", "abc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListDropsWildcardWhenEveryMatchIsSkipped) {
  CreateFailureList(R"(
# Header left behind.
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportSkip("foo.a.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(manager.ReportSkip("foo.b.bar", "reason"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
# Header left behind.
)");
}

TEST_F(TestManagerTest, SaveFailureListKeepsWildcardWithTooManyMatches) {
  // Exceeding the wildcard expansion cap is reported as unexpected failures,
  // but the tests do still fail with the listed message: the entry is kept.
  CreateFailureList(R"(
foo.*.bar # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  for (int i = 0; i <= 20; ++i) {
    ASSERT_THAT(manager.ReportFailure(absl::StrCat("foo.", i, ".bar"), "abc"),
                IsOk());
  }
  ASSERT_THAT(manager.ReportFailure("foo.21.bar", "abc"), Not(IsOk()));
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

TEST_F(TestManagerTest, SaveFailureListExpansionInterleavesNewFailures) {
  // New failures are inserted by name among the expanded lines too, and the
  // alignment accounts for the expanded names.
  CreateFailureList(R"(
# Block one.
f.*.x # abc

# Block two.
zz # zzz
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("f.aaaa.x", "abc"), IsOk());
  ASSERT_THAT(manager.ReportSuccess("f.b.x"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("f.cc.x", "abc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("f.bb.y", "new"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("zz", "zzz"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListChangedMessageStaysInPlace) {
  // An exact entry whose message changed is rewritten where it is, so that it
  // stays under its comment block.
  CreateFailureList(R"(
# About bbb.
bbb # abc

# About aaa.
aaa # aaa
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbb", "xyz"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("aaa", "aaa"), IsOk());
  EXPECT_THAT(manager.UnexpectedFailures(),
              ElementsAre(FieldsAre("bbb", "xyz", absl::nullopt)));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListKeepsMessagelessWildcardVerbatim) {
  CreateFailureList(R"(
foo.*.bar
zzz # zzz
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  // An empty expected message matches any failure.
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "abc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.b.bar", "xyz"), IsOk());
  ASSERT_THAT(manager.ReportFailure("zzz", "zzz"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListExpandsMessagelessWildcard) {
  // The expanded lines have no message either: no `#` and no trailing space.
  CreateFailureList(R"(
foo.*.bar
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("foo.a.bar", "abc"), IsOk());
  ASSERT_THAT(manager.ReportSuccess("foo.b.bar"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("foo.c.bar", ""), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListDropsSkippedEntry) {
  // A skipped test can't be an expected failure, so its entry goes like that
  // of a test that passed (whether the skip itself is acceptable is decided,
  // and reported, by the caller).
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
  EXPECT_THAT(manager.UnexpectedFailures(),
              ElementsAre(FieldsAre("foo", "zyx", absl::nullopt)));
  ASSERT_THAT(manager.Finalize(), IsOk());

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

// The copybara marker lines of a strip block (the region the open source
// export drops), spelled in two pieces so that the export's scrubber, which
// processes every file, leaves this test's data alone.
constexpr absl::string_view kStripBegin =
    "# copybara:"
    "strip_begin";
constexpr absl::string_view kStripEnd =
    "# copybara:"
    "strip_end";

TEST_F(TestManagerTest, SaveFailureListKeepsUnchangedStripBlockVerbatim) {
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
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa.long.name", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("zzz", "zzz"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbb", "bbb"), IsOk());
  ASSERT_THAT(manager.ReportFailure("ccc", "ccc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListRewritesEntryInsideStripBlockInPlace) {
  CreateFailureList(absl::Substitute(R"(
aaa.long.name # aaa
$0
bbb # bbb
ccc # ccc
$1
)",
                                     kStripBegin, kStripEnd));
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa.long.name", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbb", "new message"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("ccc", "ccc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListExpandsWildcardInsideStripBlock) {
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
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("b.long.b", "bbb"), IsOk());
  ASSERT_THAT(manager.ReportSuccess("b.x.b"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("ccc", "ccc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListDropsPassingEntryInsideStripBlock) {
  CreateFailureList(absl::Substitute(R"(
aaa # aaa
$0
bbb # bbb
ccc # ccc
$1
)",
                                     kStripBegin, kStripEnd));
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportSuccess("bbb"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("ccc", "ccc"), IsOk());
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListInsertsNewFailuresOutsideStripBlock) {
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
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bba", "bba"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbc", "bbc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("yyz", "yyz"), IsOk());
  ASSERT_THAT(manager.ReportFailure("aab", "new"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("bbb", "new"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("yyy.long", "new"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("zzz", "new"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest,
       SaveFailureListAppendsNewFailuresAfterTrailingStripBlock) {
  CreateFailureList(absl::Substitute(R"(
aaa # aaa
# Keep this block last.
$0
bbb # bbb
$1
)",
                                     kStripBegin, kStripEnd));
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbb", "bbb"), IsOk());
  ASSERT_THAT(manager.ReportFailure("abc", "new"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListTreatsUnbalancedStripMarkersAsComments) {
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
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("ccc.long", "ccc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("eee", "eee"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbb", "new"), Not(IsOk()));
  ASSERT_THAT(manager.ReportFailure("ddd", "new"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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

TEST_F(TestManagerTest, SaveFailureListIgnoresNestedStripBegin) {
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
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportFailure("aaa.long", "aaa"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bbb", "bbb"), IsOk());
  ASSERT_THAT(manager.ReportFailure("ccc", "ccc"), IsOk());
  ASSERT_THAT(manager.ReportFailure("bba", "new"), Not(IsOk()));
  ASSERT_THAT(manager.Finalize(), IsOk());

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
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
