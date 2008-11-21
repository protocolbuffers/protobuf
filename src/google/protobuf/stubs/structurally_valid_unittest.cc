// Copyright 2008 Google Inc. All Rights Reserved.
// Author: xpeng@google.com (Peter Peng)

#include <google/protobuf/stubs/common.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace internal {
namespace {

TEST(StructurallyValidTest, ValidUTF8String) {
  // On GCC, this string can be written as:
  //   "abcd 1234 - \u2014\u2013\u2212"
  // MSVC seems to interpret \u differently.
  string valid_str("abcd 1234 - \342\200\224\342\200\223\342\210\222");
  EXPECT_TRUE(IsStructurallyValidUTF8(valid_str.data(),
                                      valid_str.size()));
}

TEST(StructurallyValidTest, InvalidUTF8String) {
  string invalid_str("\xA0\xB0");
  EXPECT_FALSE(IsStructurallyValidUTF8(invalid_str.data(),
                                       invalid_str.size()));
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
