#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/cpp_file_options_test.pb.h"
#include "google/protobuf/descriptor_test_utils.h"

using ::testing::NotNull;

namespace google {
namespace protobuf {
namespace descriptor_unittest {

class CppFileOptionsTest : public ValidationErrorTest {
 protected:
  void SetUp() override { ValidationErrorTest::SetUp(); }
};

TEST_F(CppFileOptionsTest, NewNamespaceSymbolAndProtoName) {
  BuildFileInTestPool(cpp::file::NewMessage::descriptor()->file());
  ASSERT_THAT(ParseAndBuildFile("vis.proto", R"schema(
          edition = "UNSTABLE";
          package vis.test;

          import "google/protobuf/cpp_file_options_test.proto";

          message TopLevelMessage {
            cpp.file.options.test.NewMessage nested_message = 1;
          }
        )schema"),
              NotNull());
}

}  // namespace descriptor_unittest
}  // namespace protobuf
}  // namespace google
