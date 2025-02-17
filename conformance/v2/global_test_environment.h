#ifndef GOOGLE_PROTOBUF_CONFORMANCE_V2_GLOBAL_TEST_ENVIRONMENT_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_V2_GLOBAL_TEST_ENVIRONMENT_H__

#include <string>

#include <gtest/gtest.h>
#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "conformance/v2/test_manager.h"
#include "conformance/v2/testee.h"

namespace google {
namespace protobuf {

internal::Test RequiredTest();
internal::Test RequiredTest(absl::string_view test_name);
internal::Test RecommendedTest();
internal::Test RecommendedTest(absl::string_view test_name);

class ConformanceTest : public ::testing::Test {
 public:
  void SetUp() override;
  void TearDown() override;
  static void TearDownTestSuite();

 private:
  absl::flat_hash_map<std::string, int> test_initial_counts_;
  static absl::flat_hash_map<const ::testing::TestSuite*,
                             absl::flat_hash_map<std::string, int>>*
      suite_counts_;
};

namespace internal {

TestManager& GetGlobalTestManager();

}  // namespace internal

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_V2_GLOBAL_TEST_ENVIRONMENT_H__
