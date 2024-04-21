#include "google/protobuf/unredacted_debug_format_for_test.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/unittest.pb.h"
#include "google/protobuf/unittest_lite.pb.h"

namespace {

using ::google::protobuf::util::UnredactedDebugFormatForTest;
using ::google::protobuf::util::UnredactedShortDebugFormatForTest;
using ::google::protobuf::util::UnredactedUtf8DebugFormatForTest;
using ::testing::StrEq;

TEST(UnredactedDebugFormatAPITest, MessageUnredactedDebugFormat) {
  protobuf_unittest::RedactedFields proto;
  protobuf_unittest::TestNestedMessageRedaction redacted_nested_proto;
  protobuf_unittest::TestNestedMessageRedaction unredacted_nested_proto;
  redacted_nested_proto.set_optional_unredacted_nested_string(
      "\350\260\267\346\255\214");
  unredacted_nested_proto.set_optional_unredacted_nested_string(
      "\350\260\267\346\255\214");
  *proto.mutable_optional_redacted_message() = redacted_nested_proto;
  *proto.mutable_optional_unredacted_message() = unredacted_nested_proto;

  EXPECT_THAT(UnredactedDebugFormatForTest(proto),
              StrEq("optional_redacted_message {\n  "
                    "optional_unredacted_nested_string: "
                    "\"\\350\\260\\267\\346\\255\\214\"\n}\n"
                    "optional_unredacted_message {\n  "
                    "optional_unredacted_nested_string: "
                    "\"\\350\\260\\267\\346\\255\\214\"\n}\n"));
}

TEST(UnredactedDebugFormatAPITest, MessageUnredactedShortDebugFormat) {
  protobuf_unittest::RedactedFields proto;
  protobuf_unittest::TestNestedMessageRedaction redacted_nested_proto;
  protobuf_unittest::TestNestedMessageRedaction unredacted_nested_proto;
  redacted_nested_proto.set_optional_unredacted_nested_string("hello");
  unredacted_nested_proto.set_optional_unredacted_nested_string("world");
  *proto.mutable_optional_redacted_message() = redacted_nested_proto;
  *proto.mutable_optional_unredacted_message() = unredacted_nested_proto;

  EXPECT_THAT(UnredactedShortDebugFormatForTest(proto),
              StrEq("optional_redacted_message { "
                    "optional_unredacted_nested_string: \"hello\" } "
                    "optional_unredacted_message { "
                    "optional_unredacted_nested_string: \"world\" }"));
}

TEST(UnredactedDebugFormatAPITest, MessageUnredactedUtf8DebugFormat) {
  protobuf_unittest::RedactedFields proto;
  protobuf_unittest::TestNestedMessageRedaction redacted_nested_proto;
  protobuf_unittest::TestNestedMessageRedaction unredacted_nested_proto;
  redacted_nested_proto.set_optional_unredacted_nested_string(
      "\350\260\267\346\255\214");
  unredacted_nested_proto.set_optional_unredacted_nested_string(
      "\350\260\267\346\255\214");
  *proto.mutable_optional_redacted_message() = redacted_nested_proto;
  *proto.mutable_optional_unredacted_message() = unredacted_nested_proto;

  EXPECT_THAT(UnredactedUtf8DebugFormatForTest(proto),
              StrEq("optional_redacted_message {\n  "
                    "optional_unredacted_nested_string: "
                    "\"\xE8\xB0\xB7\xE6\xAD\x8C\"\n}\n"
                    "optional_unredacted_message {\n  "
                    "optional_unredacted_nested_string: "
                    "\"\xE8\xB0\xB7\xE6\xAD\x8C\"\n}\n"));
}

TEST(UnredactedDebugFormatAPITest, LiteUnredactedDebugFormat) {
  protobuf_unittest::TestAllTypesLite message;
  EXPECT_EQ(UnredactedDebugFormatForTest(message), message.DebugString());
}

TEST(UnredactedDebugFormatAPITest, LiteUnredactedShortDebugFormat) {
  protobuf_unittest::TestAllTypesLite message;
  EXPECT_EQ(UnredactedShortDebugFormatForTest(message),
            message.ShortDebugString());
}

TEST(UnredactedDebugFormatAPITest, LiteUnredactedUtf8DebugFormat) {
  protobuf_unittest::TestAllTypesLite message;
  EXPECT_EQ(UnredactedUtf8DebugFormatForTest(message),
            message.Utf8DebugString());
}

}  // namespace
