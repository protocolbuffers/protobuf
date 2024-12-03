// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "failure_list_trie_node.h"

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

TEST(FailureListTrieTest, InsertInvalidWildcardFails) {
  auto root_ = std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
  EXPECT_THAT(root_->Insert("This*Is.Not.A.Valid.Wildcard"),
              StatusIs(absl::StatusCode::kInvalidArgument,
                       HasSubstr("invalid wildcard")));
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
