// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/get_type_name.h"

#include <array>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace google::protobuf::internal {

struct TestStruct {
  struct NestedStruct {};
  template <typename T>
  struct NestedTemplate {};
};
enum TestEnum {};

template <typename T, typename U = int32_t, typename V = std::vector<T>>
struct TestTemplate {
  struct NestedStruct {};
  template <typename W>
  struct NestedTemplate {};
};

namespace {

TEST(GetTypeNameTest, IntegralAndFloatingPointTypes) {
  EXPECT_EQ(GetTypeName<bool>(), "bool");
  EXPECT_EQ(GetTypeName<char>(), "char");
  EXPECT_EQ(GetTypeName<int8_t>(), "int8_t");
  EXPECT_EQ(GetTypeName<uint8_t>(), "uint8_t");
  EXPECT_EQ(GetTypeName<int16_t>(), "int16_t");
  EXPECT_EQ(GetTypeName<uint16_t>(), "uint16_t");
  EXPECT_EQ(GetTypeName<int32_t>(), "int32_t");
  EXPECT_EQ(GetTypeName<uint32_t>(), "uint32_t");
  EXPECT_EQ(GetTypeName<int64_t>(), "int64_t");
  EXPECT_EQ(GetTypeName<uint64_t>(), "uint64_t");
  EXPECT_EQ(GetTypeName<float>(), "float");
  EXPECT_EQ(GetTypeName<double>(), "double");
}

TEST(GetTypeNameTest, StringTypes) {
  EXPECT_EQ(GetTypeName<std::string>(), "std::string");
  EXPECT_EQ(GetTypeName<absl::string_view>(), "absl::string_view");
}

TEST(GetTypeNameTest, QualifiersPointersAndReferences) {
  EXPECT_EQ(GetTypeName<const int32_t>(), "const int32_t");
  EXPECT_EQ(GetTypeName<volatile int32_t>(), "volatile int32_t");
  EXPECT_EQ(GetTypeName<int32_t*>(), "int32_t*");
  EXPECT_EQ(GetTypeName<const int32_t*>(), "const int32_t*");
  EXPECT_EQ(GetTypeName<int32_t&>(), "int32_t&");
  EXPECT_EQ(GetTypeName<const std::string&>(), "const std::string&");
  EXPECT_EQ(GetTypeName<std::string&&>(), "std::string&&");
}

TEST(GetTypeNameTest, TemplatesDropDefaultArguments) {
  EXPECT_EQ(GetTypeName<std::vector<int32_t>>(), "std::vector<int32_t>");
  EXPECT_EQ(GetTypeName<std::deque<std::string>>(), "std::deque<std::string>");
  EXPECT_EQ((GetTypeName<std::map<std::string, int32_t>>()),
            "std::map<std::string, int32_t>");
  EXPECT_EQ((GetTypeName<std::pair<int32_t, std::vector<double>>>()),
            "std::pair<int32_t, std::vector<double>>");

  EXPECT_EQ((GetTypeName<TestTemplate<double>>()),
            "google::protobuf::internal::TestTemplate<double>");
  EXPECT_EQ((GetTypeName<TestTemplate<double, char>>()),
            "google::protobuf::internal::TestTemplate<double, char>");
  EXPECT_EQ((GetTypeName<TestTemplate<double, int32_t, std::deque<double>>>()),
            "google::protobuf::internal::TestTemplate<double, int32_t, "
            "std::deque<double>>");
}

TEST(GetTypeNameTest, UserDefinedTypes) {
  EXPECT_EQ(GetTypeName<TestStruct>(), "google::protobuf::internal::TestStruct");
  EXPECT_EQ(GetTypeName<TestStruct::NestedStruct>(),
            "google::protobuf::internal::TestStruct::NestedStruct");
  EXPECT_EQ(GetTypeName<TestStruct::NestedTemplate<int32_t>>(),
            "google::protobuf::internal::TestStruct::NestedTemplate<int32_t>");
  EXPECT_EQ(GetTypeName<TestEnum>(), "google::protobuf::internal::TestEnum");
}

TEST(GetTypeNameTest, UnsupportedTemplatesReturnNullopt) {
  EXPECT_EQ((GetTypeName<std::array<int, 5>>()), absl::nullopt);
  EXPECT_EQ((GetTypeName<TestTemplate<int>::NestedStruct>()), absl::nullopt);
  EXPECT_EQ((GetTypeName<TestTemplate<int>::NestedTemplate<double>>()),
            absl::nullopt);
  EXPECT_EQ((GetTypeName<std::vector<std::array<int, 5>>>()), absl::nullopt);
  EXPECT_EQ((GetTypeName<const std::array<int, 5>>()), absl::nullopt);
  EXPECT_EQ((GetTypeName<volatile std::array<int, 5>>()), absl::nullopt);
  EXPECT_EQ((GetTypeName<std::array<int, 5>*>()), absl::nullopt);
  EXPECT_EQ((GetTypeName<std::array<int, 5>&>()), absl::nullopt);
  EXPECT_EQ((GetTypeName<std::array<int, 5>&&>()), absl::nullopt);
}

}  // namespace
}  // namespace protobuf
}  // namespace google::internal
