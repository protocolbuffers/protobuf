#include <memory>
#include <string>

#include <gtest/gtest.h>
#include "conformance_test.h"

using std::string;

namespace google {
namespace protobuf {

class FailureListTrieTest : public ::testing::Test {
 protected:
  std::unique_ptr<google::protobuf::FailureListTrieNode> root_ =
      std::make_unique<google::protobuf::FailureListTrieNode>("dummy");
};

TEST_F(FailureListTrieTest, AddChild) {
  root_->AddChild("hello");
  EXPECT_EQ(root_->children_.size(), 1);
  EXPECT_EQ(root_->children_[0]->data_, "hello");
}

TEST_F(FailureListTrieTest, AddMultipleChildren) {
  root_->AddChild("hello");
  root_->AddChild("world");
  EXPECT_EQ(root_->children_.size(), 2);
  EXPECT_EQ(root_->children_[0]->data_, "hello");
  EXPECT_EQ(root_->children_[1]->data_, "world");
}

TEST_F(FailureListTrieTest, Insert) {
  root_->Insert("Recommended.Proto2.ProtobufInput.World");
  root_->Insert("Recommended.Proto2.JsonInput.World");

  EXPECT_EQ(root_->children_.size(), 1);
  EXPECT_EQ(root_->children_[0]->data_, "Recommended");

  EXPECT_EQ(root_->children_[0]->children_.size(), 1);
  EXPECT_EQ(root_->children_[0]->children_[0]->data_, "Proto2");

  EXPECT_EQ(root_->children_[0]->children_[0]->children_.size(), 2);
  EXPECT_EQ(root_->children_[0]->children_[0]->children_[0]->data_,
            "ProtobufInput");
  EXPECT_EQ(root_->children_[0]->children_[0]->children_[1]->data_,
            "JsonInput");

  EXPECT_EQ(root_->children_[0]->children_[0]->children_[0]->children_.size(),
            1);
  EXPECT_EQ(
      root_->children_[0]->children_[0]->children_[0]->children_[0]->data_,
      "World");
}

TEST_F(FailureListTrieTest, WalkDownMatch) {
  root_->Insert("Recommended.Proto2.ProtobufInput.World");
  root_->Insert("Recommended.Proto2.JsonInput.World");
  // Wildcarded equivalent doesn't matter in this case, but we still need it
  // for the function call.
  string wildcarded_equivalent;
  EXPECT_TRUE(root_->WalkDownMatch("Recommended.Proto2.ProtobufInput.World",
                                   wildcarded_equivalent));
  EXPECT_TRUE(root_->WalkDownMatch("Recommended.Proto2.JsonInput.World",
                                   wildcarded_equivalent));
  EXPECT_FALSE(root_->WalkDownMatch("Recommended.Proto2.TextFormatInput",
                                    wildcarded_equivalent));
  EXPECT_FALSE(root_->WalkDownMatch("Recommended.Proto2.TextFormatInput.World",
                                    wildcarded_equivalent));
}

TEST_F(FailureListTrieTest, WalkDownMatchWithWildcard) {
  root_->Insert("Recommended.*.ProtobufInput.World");
  string wildcarded_equivalent1;
  EXPECT_TRUE(root_->WalkDownMatch("Recommended.Proto2.ProtobufInput.World",
                                   wildcarded_equivalent1));
  EXPECT_EQ(wildcarded_equivalent1, "Recommended.*.ProtobufInput.World");
  string wildcarded_equivalent2;
  EXPECT_FALSE(root_->WalkDownMatch("Recommended.Proto2.JsonInput.World",
                                    wildcarded_equivalent2));
  EXPECT_EQ(wildcarded_equivalent2, "");
}

TEST_F(FailureListTrieTest, WalkDownMatchTakeMoreThanOneBranch) {
  root_->Insert(
      "Recommended.*.JsonInput.TrailingCommaInAnObjectWithSpaceCommaSpace");
  root_->Insert(
      "Recommended.Proto3.*.RepeatedFieldTrailingCommaWithSpaceCommaSpace");
  string wildcarded_equivalent;
  EXPECT_TRUE(
      root_->WalkDownMatch("Recommended.Proto3.JsonInput."
                           "RepeatedFieldTrailingCommaWithSpaceCommaSpace",
                           wildcarded_equivalent));
  EXPECT_EQ(
      wildcarded_equivalent,
      "Recommended.Proto3.*.RepeatedFieldTrailingCommaWithSpaceCommaSpace");
}
}  // namespace protobuf
}  // namespace google
