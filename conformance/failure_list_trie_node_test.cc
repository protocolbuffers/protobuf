// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/failure_list_trie_node.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/types/optional.h"

using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Optional;

absl::Status GetStatus(const absl::Status& s) { return s; }
template <typename T>
absl::Status GetStatus(const absl::StatusOr<T>& s) {
  return s.status();
}
MATCHER_P2(StatusIs, status, message,
           absl::StrCat(".status() is ", testing::PrintToString(status))) {
  return GetStatus(arg).code() == status &&
         testing::ExplainMatchResult(message, GetStatus(arg).message(),
                                     result_listener);
}
#define EXPECT_OK(x) EXPECT_THAT(x, StatusIs(absl::StatusCode::kOk, testing::_))
#define ASSERT_OK(x) ASSERT_THAT(x, StatusIs(absl::StatusCode::kOk, testing::_))

namespace google {
namespace protobuf {

TEST(FailureListTrieTest, WalkDownMatchWithoutWildcard) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Recommended.Proto2.ProtobufInput.World"));

  EXPECT_THAT(root_->WalkDownMatch("Recommended.Proto2.ProtobufInput.World"),
              Optional(Eq("Recommended.Proto2.ProtobufInput.World")));
}

TEST(FailureListTrieTest, WalkDownMatchWithoutWildcardNoMatch) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");

  ASSERT_OK(root_->Insert("Recommended.Proto2.JsonInput.World"));

  EXPECT_EQ(root_->WalkDownMatch("Recommended.Proto2.TextFormatInput"),
            absl::nullopt);
}

TEST(FailureListTrieTest, WalkDownMatchWithWildcard) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Recommended.*.ProtobufInput.World"));

  EXPECT_THAT(root_->WalkDownMatch("Recommended.Proto2.ProtobufInput.World"),
              Optional(Eq("Recommended.*.ProtobufInput.World")));
}

TEST(FailureListTrieTest, WalkDownMatchWithWildcardNoMatch) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Recommended.*.ProtobufInput.World"));

  EXPECT_EQ(root_->WalkDownMatch("Recommended.Proto2.JsonInput.World"),
            absl::nullopt);
}

TEST(FailureListTrieTest, WalkDownMatchTestLessNumberofSectionsNoMatch) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Recommended.*.*.*"));

  EXPECT_EQ(root_->WalkDownMatch("Recommended.Proto2.JsonInput"),
            absl::nullopt);
}

TEST(FailureListTrieTest, WalkDownMatchTestMoreNumberOfSectionsNoMatch) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("*"));

  EXPECT_EQ(root_->WalkDownMatch("Recommended.Proto2.JsonInput.World"),
            absl::nullopt);
}

TEST(FailureListTrieTest, WalkDownMatchTakeMoreThanOneBranch) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert(
      "Recommended.*.JsonInput.TrailingCommaInAnObjectWithSpaceCommaSpace"));
  ASSERT_OK(root_->Insert(
      "Recommended.Proto3.*.RepeatedFieldTrailingCommaWithSpaceCommaSpace"));

  EXPECT_THAT(
      root_->WalkDownMatch("Recommended.Proto3.JsonInput."
                           "RepeatedFieldTrailingCommaWithSpaceCommaSpace"),
      Optional(Eq("Recommended.Proto3.*."
                  "RepeatedFieldTrailingCommaWithSpaceCommaSpace")));
}

TEST(FailureListTrieTest, InsertWilcardedAmbiguousMatchFails) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert(
      "Recommended.*.JsonInput.TrailingCommaInAnObjectWithSpaceCommaSpace"));

  // Essentially a duplicated test name if inserted.
  EXPECT_THAT(
      root_->Insert(
          "Recommended.Proto3.*.TrailingCommaInAnObjectWithSpaceCommaSpace"),
      StatusIs(absl::StatusCode::kAlreadyExists, HasSubstr("already exists")));
}

TEST(FailureListTrieTest, InsertWilcardedAmbiguousMatchMutlipleWildcardsFails) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Recommended.*.JsonInput.FieldMaskInvalidCharacter"));

  // Essentially a duplicated test name if inserted.
  EXPECT_THAT(
      root_->Insert("Recommended.*.*.*"),
      StatusIs(absl::StatusCode::kAlreadyExists, HasSubstr("already exists")));
}

// A wildcard may cover part of a section: it matches any run of characters
// that contains no delimiter.
TEST(FailureListTrieTest, WalkDownMatchWithWildcardWithinASection) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Required.FooTest.Bar/*_INT32.ProtobufInput"));
  ASSERT_OK(root_->Insert("Required.FooTest.Baz/Proto*.ProtobufInput"));
  ASSERT_OK(root_->Insert("Required.FooTest.Qux/*_*_STRING.ProtobufInput"));

  EXPECT_THAT(root_->WalkDownMatch("Required.FooTest.Bar/Proto2_INT32."
                                   "ProtobufInput"),
              Optional(Eq("Required.FooTest.Bar/*_INT32.ProtobufInput")));
  EXPECT_THAT(root_->WalkDownMatch("Required.FooTest.Bar/Editions_Proto3_"
                                   "INT32.ProtobufInput"),
              Optional(Eq("Required.FooTest.Bar/*_INT32.ProtobufInput")));
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Bar/Proto2_INT64."
                                 "ProtobufInput"),
            absl::nullopt);
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Bar/Proto2_INT32.Print."
                                 "ProtobufInput"),
            absl::nullopt);

  EXPECT_THAT(root_->WalkDownMatch("Required.FooTest.Baz/Proto3.ProtobufInput"),
              Optional(Eq("Required.FooTest.Baz/Proto*.ProtobufInput")));
  EXPECT_THAT(root_->WalkDownMatch("Required.FooTest.Baz/Proto.ProtobufInput"),
              Optional(Eq("Required.FooTest.Baz/Proto*.ProtobufInput")));
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Baz/Editions_Proto3."
                                 "ProtobufInput"),
            absl::nullopt);

  EXPECT_THAT(root_->WalkDownMatch("Required.FooTest.Qux/Proto2_INT32_STRING."
                                   "ProtobufInput"),
              Optional(Eq("Required.FooTest.Qux/*_*_STRING.ProtobufInput")));
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Qux/Proto2_STRING."
                                 "ProtobufInput"),
            absl::nullopt);
}

// A wildcard never spans a delimiter, so "Bar/*" matches every parameter of
// Bar but neither an unparameterized Bar nor a suffixed one.
TEST(FailureListTrieTest, WildcardDoesNotSpanDelimiters) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Required.FooTest.Bar/*.ProtobufInput"));
  ASSERT_OK(root_->Insert("Required.FooTest.*.JsonInput"));

  EXPECT_THAT(
      root_->WalkDownMatch("Required.FooTest.Bar/Proto2_INT32.ProtobufInput"),
      Optional(Eq("Required.FooTest.Bar/*.ProtobufInput")));
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Bar.ProtobufInput"),
            absl::nullopt);
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Bar/Proto2.Print."
                                 "ProtobufInput"),
            absl::nullopt);

  EXPECT_THAT(root_->WalkDownMatch("Required.FooTest.Bar.JsonInput"),
              Optional(Eq("Required.FooTest.*.JsonInput")));
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Bar/Proto2.JsonInput"),
            absl::nullopt);
}

// '.' and '/' are both delimiters, but not interchangeable: a name lists its
// parameters after a '/', and an entry has to as well.
TEST(FailureListTrieTest, DelimitersAreNotInterchangeable) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Required.FooTest.Bar/Proto2.ProtobufInput"));
  ASSERT_OK(root_->Insert("Required.FooTest.Baz.*.ProtobufInput"));

  EXPECT_THAT(root_->WalkDownMatch("Required.FooTest.Bar/Proto2.ProtobufInput"),
              Optional(Eq("Required.FooTest.Bar/Proto2.ProtobufInput")));
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Bar.Proto2.ProtobufInput"),
            absl::nullopt);
  // The two spellings are different names, so both can be inserted.
  ASSERT_OK(root_->Insert("Required.FooTest.Bar.Proto2.ProtobufInput"));

  EXPECT_THAT(root_->WalkDownMatch("Required.FooTest.Baz.Print.ProtobufInput"),
              Optional(Eq("Required.FooTest.Baz.*.ProtobufInput")));
  EXPECT_EQ(root_->WalkDownMatch("Required.FooTest.Baz/Proto2.ProtobufInput"),
            absl::nullopt);
}

TEST(FailureListTrieTest, InsertWildcardWithinASectionOverlappingFails) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Required.FooTest.Bar/*_INT32.ProtobufInput"));

  // Both would match the (Proto2, INT32) instance of Bar.
  EXPECT_THAT(
      root_->Insert("Required.FooTest.Bar/Proto2_INT32.ProtobufInput"),
      StatusIs(absl::StatusCode::kAlreadyExists, HasSubstr("already exists")));
  EXPECT_THAT(
      root_->Insert("Required.FooTest.Bar/Proto2_*.ProtobufInput"),
      StatusIs(absl::StatusCode::kAlreadyExists, HasSubstr("already exists")));
  EXPECT_THAT(
      root_->Insert("Required.FooTest.Bar/*.ProtobufInput"),
      StatusIs(absl::StatusCode::kAlreadyExists, HasSubstr("already exists")));
  // Disjoint, so fine.
  ASSERT_OK(root_->Insert("Required.FooTest.Bar/*_INT64.ProtobufInput"));
}

TEST(FailureListTrieTest, PrefixMarkedAsTestNameRecognizedWithoutWildcards) {
  auto root_ = std::make_unique<FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Recommended.Proto2.ProtobufInput.World"));

  ASSERT_OK(root_->Insert("Recommended.Proto2"));
  EXPECT_THAT(root_->WalkDownMatch("Recommended.Proto2"),
              Optional(Eq("Recommended.Proto2")));
}

TEST(FailureListTrieTest, PrefixMarkedAsTestNameRecognizedWithWildcards) {
  auto root_ = std::make_unique<FailureListTrieNode>("dummy");
  ASSERT_OK(root_->Insert("Recommended.*.*.*"));

  ASSERT_OK(root_->Insert("Recommended.*.*"));
  EXPECT_THAT(root_->WalkDownMatch("Recommended.*.Hello"),
              Optional(Eq("Recommended.*.*")));
}
}  // namespace protobuf
}  // namespace google
