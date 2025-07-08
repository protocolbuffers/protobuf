#ifndef PROTOBUF_FORCE_SPLIT

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

namespace google {
namespace protobuf {
namespace {

using ::google::protobuf::internal::v2::MatchesBoolField;
using ::google::protobuf::internal::v2::MatchesInt32Field;
using ::google::protobuf::internal::v2::MatchesV2Message;
using ::protobuf_unittest_v2_codegen::TestHugeNumberAllNumericTypes;
using ::testing::EqualsProto;

class ParseFromBoundedZeroCopyStreamV2Test : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a sample message for testing.
    test_message_.set_bool_n100000(true);
    test_message_.set_int32_n100001(10);
  }
  void SerializeMessage() {
    EXPECT_TRUE(
        internal::v2::codegen::SerializeToStringV2(test_message_, &data_));
    size_ = static_cast<int>(data_.size());
  }

  TestHugeNumberAllNumericTypes test_message_;
  std::string data_;
  int size_;
};

TEST_F(ParseFromBoundedZeroCopyStreamV2Test, SuccessfulParsing) {
  SerializeMessage();
  EXPECT_THAT(data_, MatchesV2Message(MatchesBoolField(100000, true),
                                      MatchesInt32Field(100001, 10)));
  io::ArrayInputStream input(data_.data(), size_);
  TestHugeNumberAllNumericTypes parsed_message;

  EXPECT_TRUE(parsed_message.ParseFromBoundedZeroCopyStreamV2(&input, size_));
  EXPECT_THAT(parsed_message, EqualsProto(test_message_));
}

TEST_F(ParseFromBoundedZeroCopyStreamV2Test, FailOnSizeTooLarge) {
  SerializeMessage();
  EXPECT_THAT(data_, MatchesV2Message(MatchesBoolField(100000, true),
                                      MatchesInt32Field(100001, 10)));
  io::ArrayInputStream input(data_.data(), size_);
  TestHugeNumberAllNumericTypes parsed_message;

  EXPECT_FALSE(
      parsed_message.ParseFromBoundedZeroCopyStreamV2(&input, size_ + 1));
}

TEST_F(ParseFromBoundedZeroCopyStreamV2Test, FailOnSizeTooSmall) {
  SerializeMessage();
  EXPECT_THAT(data_, MatchesV2Message(MatchesBoolField(100000, true),
                                      MatchesInt32Field(100001, 10)));
  io::ArrayInputStream input(data_.data(), size_);
  TestHugeNumberAllNumericTypes parsed_message;

  EXPECT_FALSE(
      parsed_message.ParseFromBoundedZeroCopyStreamV2(&input, size_ - 1));
}

TEST_F(ParseFromBoundedZeroCopyStreamV2Test, FailOnInvalidData) {
  std::string invalid_data = "invalid_data";
  size_ = static_cast<int>(invalid_data.size());
  io::ArrayInputStream input(invalid_data.data(), size_);
  TestHugeNumberAllNumericTypes parsed_message;

  EXPECT_FALSE(parsed_message.ParseFromBoundedZeroCopyStreamV2(&input, size_));
}

TEST_F(ParseFromBoundedZeroCopyStreamV2Test, FailOnEmptyData) {
  std::string empty_data = "";
  size_ = static_cast<int>(empty_data.size());
  io::ArrayInputStream input(empty_data.data(), size_);
  TestHugeNumberAllNumericTypes parsed_message;

  EXPECT_FALSE(parsed_message.ParseFromBoundedZeroCopyStreamV2(&input, size_));
}

}  // namespace
}  // namespace protobuf
}  // namespace google

#endif  // !PROTOBUF_FORCE_SPLIT
