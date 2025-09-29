#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "rust/test/shared/utf8/feature_verify.pb.h"
#include "rust/test/shared/utf8/no_features_proto2.pb.h"
#include "rust/test/shared/utf8/no_features_proto3.pb.h"

namespace {

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Not;

// We use 0b1000_0000, since 0b1XXX_XXXX in UTF-8 denotes a byte 2-4, but never
// the first byte.
constexpr char kInvalidUtf8[] = "\x80";

TEST(Utf8Test, TestProto2) {
  utf8::NoFeaturesProto2 no_features_proto2;

  // No error on setter.
  no_features_proto2.set_my_field(kInvalidUtf8);
  EXPECT_THAT(no_features_proto2.my_field(),
              Eq(absl::string_view(kInvalidUtf8)));

  // No error on serialization.
  std::string serialized_nonutf8 = no_features_proto2.SerializeAsString();
  EXPECT_THAT(serialized_nonutf8, Not(IsEmpty()));

  // No error on parsing.
  utf8::NoFeaturesProto2 parsed;
  EXPECT_THAT(parsed.ParseFromString(serialized_nonutf8), Eq(true));
}

TEST(Utf8Test, TestProto3) {
  utf8::NoFeaturesProto3 no_features_proto3;

  // No error on setter.
  no_features_proto3.set_my_field(kInvalidUtf8);
  EXPECT_THAT(no_features_proto3.my_field(),
              Eq(absl::string_view(kInvalidUtf8)));

  // No error on serialization.
  std::string serialized_nonutf8 = no_features_proto3.SerializeAsString();
  EXPECT_THAT(serialized_nonutf8, Not(IsEmpty()));

  // Error on parsing.
  utf8::NoFeaturesProto3 parsed;
  EXPECT_THAT(parsed.ParseFromString(serialized_nonutf8), Eq(false));
}

TEST(Utf8Test, TestEditionsVerify) {
  utf8::Verify verify;

  // No error on setter.
  verify.set_my_field(kInvalidUtf8);
  EXPECT_THAT(verify.my_field(), Eq(absl::string_view(kInvalidUtf8)));

  // No error on serialization.
  std::string serialized_nonutf8 = verify.SerializeAsString();
  EXPECT_THAT(serialized_nonutf8, Not(IsEmpty()));

  // Error on parsing.
  utf8::Verify parsed;
  EXPECT_THAT(parsed.ParseFromString(serialized_nonutf8), Eq(false));
}

}  // namespace
