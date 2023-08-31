#include "google/protobuf/versions.h"

#include <gtest/gtest.h>


namespace google {
namespace protobuf {
namespace internal {

TEST(VersionsTest, VersionStringWithEmptySuffix) {
  EXPECT_EQ(VersionString(4024000, ""), "4.24.0");
}

TEST(VersionsTest, VersionStringWithDevSuffix) {
  EXPECT_EQ(VersionString(4024000, "-dev"), "4.24.0-dev");
}

}  // namespace internal
}  // namespace protobuf
}  // namespace google