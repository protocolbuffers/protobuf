#include "google/protobuf/compiler/file_utils.h"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/statusor.h"
#include "google/protobuf/compiler/tmpfile_utils.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace {

using ::testing::EndsWith;

TEST(FileUtilsTest, WriteStringToTestTmpDirFile) {
  absl::StatusOr<std::string> path =
      WriteStringToTestTmpDirFile("my_file", "my text");
  EXPECT_TRUE(path.ok());
  EXPECT_THAT(path.value(), EndsWith("my_file"));
}

TEST(FileUtilsTest, ReadTestTmpFileToString) {
  std::string path = WriteStringToTestTmpDirFile("my_file", "my text").value();
  absl::StatusOr<std::string> result = ReadFileToString(path);
  EXPECT_TRUE(result.ok());
  EXPECT_EQ(result.value(), "my text");
}

}  // namespace
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
