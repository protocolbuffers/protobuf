#include "test_manager.h"

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

namespace google {
namespace protobuf {
namespace conformance {
namespace internal {
namespace {

using ::absl_testing::IsOk;
using ::absl_testing::StatusIs;
using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::Not;

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

  EXPECT_THAT(manager.ReportSkip("foo"), IsOk());

  EXPECT_THAT(manager.Finalize(), IsOk());
  EXPECT_EQ(manager.expected_failures(), 0);
  EXPECT_EQ(manager.unexpected_failures(), 0);
  EXPECT_EQ(manager.expected_successes(), 0);
  EXPECT_EQ(manager.unexpected_successes(), 0);
  EXPECT_EQ(manager.skipped(), 1);
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
  ASSERT_THAT(manager.ReportSkip("baz"), IsOk());
  ASSERT_THAT(manager.ReportSkip("baz"), IsOk());
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

TEST_F(TestManagerTest, ReportUnseenFailureSkipped) {
  CreateFailureList({{"foo.bar"}});
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());

  ASSERT_THAT(manager.ReportSkip("foo"), IsOk());

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

TEST_F(TestManagerTest, SaveFailureListRemoveNewSkipped) {
  CreateFailureList(R"(
foo # abc
)");
  TestManager manager;
  ASSERT_THAT(manager.LoadFailureList(failure_list()), IsOk());
  ASSERT_THAT(manager.ReportSkip("foo"), IsOk());
  ASSERT_THAT(manager.Finalize(), Not(IsOk()));

  EXPECT_THAT(manager.SaveFailureList(absl::StrCat(failure_list(), ".new")),
              IsOk());

  std::string content;
  ASSERT_THAT(File::GetContents(absl::StrCat(failure_list(), ".new"), &content,
                                true),
              IsOk());
  EXPECT_EQ(content, R"(
)");
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
