// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <google/protobuf/util/json_util.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/status.h>
#include <google/protobuf/stubs/statusor.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/util/internal/testdata/maps.pb.h>
#include <google/protobuf/util/json_format.pb.h>
#include <google/protobuf/util/json_format_proto3.pb.h>
#include <google/protobuf/util/json_format_proto3.pb.h>
#include <google/protobuf/util/type_resolver.h>
#include <google/protobuf/util/type_resolver_util.h>
#include <google/protobuf/stubs/status_macros.h>

// Must be included last.
#include <google/protobuf/port_def.inc>

namespace google {
namespace protobuf {
namespace util {
namespace {

using ::proto3::TestAny;
using ::proto3::TestEnumValue;
using ::proto3::TestMap;
using ::proto3::TestMessage;
using ::proto3::TestOneof;
using ::proto_util_converter::testing::MapIn;

// TODO(b/234474291): Use the gtest versions once that's available in OSS.
MATCHER_P(IsOkAndHolds, inner,
          StrCat("is OK and holds ", testing::PrintToString(inner))) {
  if (!arg.ok()) {
    *result_listener << arg.status();
    return false;
  }
  return testing::ExplainMatchResult(inner, *arg, result_listener);
}

util::Status GetStatus(const util::Status& s) { return s; }
template <typename T>
util::Status GetStatus(const util::StatusOr<T>& s) {
  return s.status();
}

MATCHER_P(StatusIs, status,
          StrCat(".status() is ", testing::PrintToString(status))) {
  return GetStatus(arg).code() == status;
}

#define EXPECT_OK(x) EXPECT_THAT(x, StatusIs(util::StatusCode::kOk))
#define ASSERT_OK(x) ASSERT_THAT(x, StatusIs(util::StatusCode::kOk))

// As functions defined in json_util.h are just thin wrappers around the
// JSON conversion code in //net/proto2/util/converter, in this test we
// only cover some very basic cases to make sure the wrappers have forwarded
// parameters to the underlying implementation correctly. More detailed
// tests are contained in the //net/proto2/util/converter directory.

util::StatusOr<std::string> ToJson(const Message& message,
                                   const JsonPrintOptions& options = {}) {
  std::string result;
  RETURN_IF_ERROR(MessageToJsonString(message, &result, options));
  return result;
}

util::Status FromJson(StringPiece json, Message* message,
                      const JsonParseOptions& options = {}) {
  return JsonStringToMessage(json, message, options);
}

TEST(JsonUtilTest, TestWhitespaces) {
  TestMessage m;
  m.mutable_message_value();

  EXPECT_THAT(ToJson(m), IsOkAndHolds("{\"messageValue\":{}}"));

  JsonPrintOptions options;
  options.add_whitespace = true;
  EXPECT_THAT(ToJson(m, options), IsOkAndHolds("{\n"
                                               " \"messageValue\": {}\n"
                                               "}\n"));
}

TEST(JsonUtilTest, TestDefaultValues) {
  TestMessage m;
  EXPECT_THAT(ToJson(m), IsOkAndHolds("{}"));

  JsonPrintOptions options;
  options.always_print_primitive_fields = true;
  EXPECT_THAT(ToJson(m, options), IsOkAndHolds("{\"boolValue\":false,"
                                               "\"int32Value\":0,"
                                               "\"int64Value\":\"0\","
                                               "\"uint32Value\":0,"
                                               "\"uint64Value\":\"0\","
                                               "\"floatValue\":0,"
                                               "\"doubleValue\":0,"
                                               "\"stringValue\":\"\","
                                               "\"bytesValue\":\"\","
                                               "\"enumValue\":\"FOO\","
                                               "\"repeatedBoolValue\":[],"
                                               "\"repeatedInt32Value\":[],"
                                               "\"repeatedInt64Value\":[],"
                                               "\"repeatedUint32Value\":[],"
                                               "\"repeatedUint64Value\":[],"
                                               "\"repeatedFloatValue\":[],"
                                               "\"repeatedDoubleValue\":[],"
                                               "\"repeatedStringValue\":[],"
                                               "\"repeatedBytesValue\":[],"
                                               "\"repeatedEnumValue\":[],"
                                               "\"repeatedMessageValue\":[]"
                                               "}"));

  options.always_print_primitive_fields = true;
  m.set_string_value("i am a test string value");
  m.set_bytes_value("i am a test bytes value");
  EXPECT_THAT(
      ToJson(m, options),
      IsOkAndHolds("{\"boolValue\":false,"
                   "\"int32Value\":0,"
                   "\"int64Value\":\"0\","
                   "\"uint32Value\":0,"
                   "\"uint64Value\":\"0\","
                   "\"floatValue\":0,"
                   "\"doubleValue\":0,"
                   "\"stringValue\":\"i am a test string value\","
                   "\"bytesValue\":\"aSBhbSBhIHRlc3QgYnl0ZXMgdmFsdWU=\","
                   "\"enumValue\":\"FOO\","
                   "\"repeatedBoolValue\":[],"
                   "\"repeatedInt32Value\":[],"
                   "\"repeatedInt64Value\":[],"
                   "\"repeatedUint32Value\":[],"
                   "\"repeatedUint64Value\":[],"
                   "\"repeatedFloatValue\":[],"
                   "\"repeatedDoubleValue\":[],"
                   "\"repeatedStringValue\":[],"
                   "\"repeatedBytesValue\":[],"
                   "\"repeatedEnumValue\":[],"
                   "\"repeatedMessageValue\":[]"
                   "}"));

  options.preserve_proto_field_names = true;
  m.set_string_value("i am a test string value");
  m.set_bytes_value("i am a test bytes value");
  EXPECT_THAT(
      ToJson(m, options),
      IsOkAndHolds("{\"bool_value\":false,"
                   "\"int32_value\":0,"
                   "\"int64_value\":\"0\","
                   "\"uint32_value\":0,"
                   "\"uint64_value\":\"0\","
                   "\"float_value\":0,"
                   "\"double_value\":0,"
                   "\"string_value\":\"i am a test string value\","
                   "\"bytes_value\":\"aSBhbSBhIHRlc3QgYnl0ZXMgdmFsdWU=\","
                   "\"enum_value\":\"FOO\","
                   "\"repeated_bool_value\":[],"
                   "\"repeated_int32_value\":[],"
                   "\"repeated_int64_value\":[],"
                   "\"repeated_uint32_value\":[],"
                   "\"repeated_uint64_value\":[],"
                   "\"repeated_float_value\":[],"
                   "\"repeated_double_value\":[],"
                   "\"repeated_string_value\":[],"
                   "\"repeated_bytes_value\":[],"
                   "\"repeated_enum_value\":[],"
                   "\"repeated_message_value\":[]"
                   "}"));
}

TEST(JsonUtilTest, TestPreserveProtoFieldNames) {
  TestMessage m;
  m.mutable_message_value();

  JsonPrintOptions options;
  options.preserve_proto_field_names = true;
  EXPECT_THAT(ToJson(m, options), IsOkAndHolds("{\"message_value\":{}}"));
}

TEST(JsonUtilTest, TestAlwaysPrintEnumsAsInts) {
  TestMessage orig;
  orig.set_enum_value(proto3::BAR);
  orig.add_repeated_enum_value(proto3::FOO);
  orig.add_repeated_enum_value(proto3::BAR);

  JsonPrintOptions print_options;
  print_options.always_print_enums_as_ints = true;

  auto printed = ToJson(orig, print_options);
  ASSERT_THAT(printed,
              IsOkAndHolds("{\"enumValue\":1,\"repeatedEnumValue\":[0,1]}"));

  TestMessage parsed;
  ASSERT_OK(FromJson(*printed, &parsed));

  EXPECT_EQ(parsed.enum_value(), proto3::BAR);
  EXPECT_EQ(parsed.repeated_enum_value_size(), 2);
  EXPECT_EQ(parsed.repeated_enum_value(0), proto3::FOO);
  EXPECT_EQ(parsed.repeated_enum_value(1), proto3::BAR);
}

TEST(JsonUtilTest, TestPrintEnumsAsIntsWithDefaultValue) {
  TestEnumValue orig;
  // orig.set_enum_value1(proto3::FOO)
  orig.set_enum_value2(proto3::FOO);
  orig.set_enum_value3(proto3::BAR);

  JsonPrintOptions print_options;
  print_options.always_print_enums_as_ints = true;
  print_options.always_print_primitive_fields = true;

  auto printed = ToJson(orig, print_options);
  ASSERT_THAT(
      printed,
      IsOkAndHolds("{\"enumValue1\":0,\"enumValue2\":0,\"enumValue3\":1}"));

  TestEnumValue parsed;
  ASSERT_OK(FromJson(*printed, &parsed));

  EXPECT_EQ(parsed.enum_value1(), proto3::FOO);
  EXPECT_EQ(parsed.enum_value2(), proto3::FOO);
  EXPECT_EQ(parsed.enum_value3(), proto3::BAR);
}

TEST(JsonUtilTest, TestPrintProto2EnumAsIntWithDefaultValue) {
  protobuf_unittest::TestDefaultEnumValue orig;

  JsonPrintOptions print_options;
  // use enum as int
  print_options.always_print_enums_as_ints = true;
  print_options.always_print_primitive_fields = true;

  // result should be int rather than string
  auto printed = ToJson(orig, print_options);
  ASSERT_THAT(printed, IsOkAndHolds("{\"enumValue\":2}"));

  protobuf_unittest::TestDefaultEnumValue parsed;
  ASSERT_OK(FromJson(*printed, &parsed));

  EXPECT_EQ(parsed.enum_value(), protobuf_unittest::DEFAULT);
}

TEST(JsonUtilTest, ParseMessage) {
  // Some random message but good enough to verify that the parsing wrapper
  // functions are working properly.
  TestMessage m;
  ASSERT_OK(FromJson(R"json(
    {
      "int32Value": 1234567891,
      "int64Value": 5302428716536692736,
      "floatValue": 3.4028235e+38,
      "repeatedInt32Value": [1, 2],
      "messageValue": {
        "value": 2048
      },
      "repeatedMessageValue": [
        {"value": 40},
        {"value": 96}
      ]
    }
  )json",
                     &m));

  EXPECT_EQ(m.int32_value(), 1234567891);
  EXPECT_EQ(m.int64_value(), 5302428716536692736);
  EXPECT_EQ(m.float_value(), 3.402823466e+38f);
  ASSERT_EQ(m.repeated_int32_value_size(), 2);
  EXPECT_EQ(m.repeated_int32_value(0), 1);
  EXPECT_EQ(m.repeated_int32_value(1), 2);
  EXPECT_EQ(m.message_value().value(), 2048);
  ASSERT_EQ(m.repeated_message_value_size(), 2);
  EXPECT_EQ(m.repeated_message_value(0).value(), 40);
  EXPECT_EQ(m.repeated_message_value(1).value(), 96);
}

TEST(JsonUtilTest, ParseMap) {
  TestMap message;
  (*message.mutable_string_map())["hello"] = 1234;
  auto printed = ToJson(message);
  ASSERT_THAT(printed, IsOkAndHolds("{\"stringMap\":{\"hello\":1234}}"));

  TestMap other;
  ASSERT_OK(FromJson(*printed, &other));
  EXPECT_EQ(other.DebugString(), message.DebugString());
}

TEST(JsonUtilTest, ParsePrimitiveMapIn) {
  MapIn message;
  JsonPrintOptions print_options;
  print_options.always_print_primitive_fields = true;
  auto printed = ToJson(message, print_options);
  ASSERT_THAT(
      ToJson(message, print_options),
      IsOkAndHolds(
          "{\"other\":\"\",\"things\":[],\"mapInput\":{},\"mapAny\":{}}"));

  MapIn other;
  ASSERT_OK(FromJson(*printed, &other));
  EXPECT_EQ(other.DebugString(), message.DebugString());
}

TEST(JsonUtilTest, PrintPrimitiveOneof) {
  TestOneof message;
  JsonPrintOptions options;
  options.always_print_primitive_fields = true;
  message.mutable_oneof_message_value();
  EXPECT_THAT(ToJson(message, options),
              IsOkAndHolds("{\"oneofMessageValue\":{\"value\":0}}"));

  message.set_oneof_int32_value(1);
  EXPECT_THAT(ToJson(message, options),
              IsOkAndHolds("{\"oneofInt32Value\":1}"));
}

TEST(JsonUtilTest, TestParseIgnoreUnknownFields) {
  TestMessage m;
  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_OK(FromJson("{\"unknownName\":0}", &m, options));
}

TEST(JsonUtilTest, TestParseErrors) {
  TestMessage m;
  // Parsing should fail if the field name can not be recognized.
  EXPECT_THAT(FromJson(R"json({"unknownName": 0})json", &m),
              StatusIs(util::StatusCode::kInvalidArgument));
  // Parsing should fail if the value is invalid.
  EXPECT_THAT(FromJson(R"json("{"int32Value": 2147483648})json", &m),
              StatusIs(util::StatusCode::kInvalidArgument));
}

TEST(JsonUtilTest, TestDynamicMessage) {
  // Create a new DescriptorPool with the same protos as the generated one.
  DescriptorPoolDatabase database(*DescriptorPool::generated_pool());
  DescriptorPool pool(&database);
  // A dynamic version of the test proto.
  DynamicMessageFactory factory;
  std::unique_ptr<Message> message(
      factory.GetPrototype(pool.FindMessageTypeByName("proto3.TestMessage"))
          ->New());
  EXPECT_OK(FromJson(R"json(
    {
      "int32Value": 1024,
      "repeatedInt32Value": [1, 2],
      "messageValue": {
        "value": 2048
      },
      "repeatedMessageValue": [
        {"value": 40},
        {"value": 96}
      ]
    }
  )json",
                     message.get()));

  // Convert to generated message for easy inspection.
  TestMessage generated;
  EXPECT_TRUE(generated.ParseFromString(message->SerializeAsString()));
  EXPECT_EQ(generated.int32_value(), 1024);
  ASSERT_EQ(generated.repeated_int32_value_size(), 2);
  EXPECT_EQ(generated.repeated_int32_value(0), 1);
  EXPECT_EQ(generated.repeated_int32_value(1), 2);
  EXPECT_EQ(generated.message_value().value(), 2048);
  ASSERT_EQ(generated.repeated_message_value_size(), 2);
  EXPECT_EQ(generated.repeated_message_value(0).value(), 40);
  EXPECT_EQ(generated.repeated_message_value(1).value(), 96);

  auto message_json = ToJson(*message);
  ASSERT_OK(message_json);
  auto generated_json = ToJson(generated);
  ASSERT_OK(generated_json);
  EXPECT_EQ(*message_json, *generated_json);
}

TEST(JsonUtilTest, TestParsingAny) {
  StringPiece input = R"json(
    {
      "value": {
        "@type": "type.googleapis.com/proto3.TestMessage",
        "int32_value": 5,
        "string_value": "expected_value",
        "message_value": {"value": 1}
      }
    }
  )json";

  TestAny m;
  ASSERT_OK(FromJson(input, &m));

  TestMessage t;
  EXPECT_TRUE(m.value().UnpackTo(&t));
  EXPECT_EQ(t.int32_value(), 5);
  EXPECT_EQ(t.string_value(), "expected_value");
  EXPECT_EQ(t.message_value().value(), 1);

  EXPECT_THAT(
      ToJson(m),
      IsOkAndHolds(
          R"json({"value":{"@type":"type.googleapis.com/proto3.TestMessage","int32Value":5,"stringValue":"expected_value","messageValue":{"value":1}}})json"));
}

TEST(JsonUtilTest, TestParsingAnyMiddleAtType) {
  StringPiece input = R"json(
    {
      "value": {
        "int32_value": 5,
        "string_value": "expected_value",
        "@type": "type.googleapis.com/proto3.TestMessage",
        "message_value": {"value": 1}
      }
    }
  )json";

  TestAny m;
  ASSERT_OK(FromJson(input, &m));

  TestMessage t;
  EXPECT_TRUE(m.value().UnpackTo(&t));
  EXPECT_EQ(t.int32_value(), 5);
  EXPECT_EQ(t.string_value(), "expected_value");
  EXPECT_EQ(t.message_value().value(), 1);
}

TEST(JsonUtilTest, TestParsingAnyEndAtType) {
  StringPiece input = R"json(
    {
      "value": {
        "int32_value": 5,
        "string_value": "expected_value",
        "message_value": {"value": 1},
        "@type": "type.googleapis.com/proto3.TestMessage"
      }
    }
  )json";

  TestAny m;
  ASSERT_OK(FromJson(input, &m));

  TestMessage t;
  EXPECT_TRUE(m.value().UnpackTo(&t));
  EXPECT_EQ(t.int32_value(), 5);
  EXPECT_EQ(t.string_value(), "expected_value");
  EXPECT_EQ(t.message_value().value(), 1);
}

TEST(JsonUtilTest, TestParsingNestedAnys) {
  StringPiece input = R"json(
    {
      "value": {
        "value": {
          "int32_value": 5,
          "string_value": "expected_value",
          "message_value": {"value": 1},
          "@type": "type.googleapis.com/proto3.TestMessage"
        },
        "@type": "type.googleapis.com/google.protobuf.Any"
      }
    }
  )json";

  TestAny m;
  ASSERT_OK(FromJson(input, &m));

  google::protobuf::Any inner;
  EXPECT_TRUE(m.value().UnpackTo(&inner));

  TestMessage t;
  EXPECT_TRUE(inner.UnpackTo(&t));
  EXPECT_EQ(t.int32_value(), 5);
  EXPECT_EQ(t.string_value(), "expected_value");
  EXPECT_EQ(t.message_value().value(), 1);

  EXPECT_THAT(
      ToJson(m),
      IsOkAndHolds(
          R"json({"value":{"@type":"type.googleapis.com/google.protobuf.Any","value":{"@type":"type.googleapis.com/proto3.TestMessage","int32Value":5,"stringValue":"expected_value","messageValue":{"value":1}}}})json"));
}

TEST(JsonUtilTest, TestParsingUnknownAnyFields) {
  StringPiece input = R"json(
    {
      "value": {
        "@type": "type.googleapis.com/proto3.TestMessage",
        "unknown_field": "UNKNOWN_VALUE",
        "string_value": "expected_value"
      }
    }
  )json";

  TestAny m;
  EXPECT_THAT(FromJson(input, &m),
              StatusIs(util::StatusCode::kInvalidArgument));

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_OK(FromJson(input, &m, options));

  TestMessage t;
  EXPECT_TRUE(m.value().UnpackTo(&t));
  EXPECT_EQ(t.string_value(), "expected_value");
}

TEST(JsonUtilTest, TestParsingUnknownEnumsProto2) {
  StringPiece input = R"json({"a": "UNKNOWN_VALUE"})json";
  protobuf_unittest::TestNumbers m;
  JsonParseOptions options;
  EXPECT_THAT(FromJson(input, &m, options),
              StatusIs(util::StatusCode::kInvalidArgument));

  options.ignore_unknown_fields = true;
  EXPECT_OK(FromJson(input, &m, options));
  EXPECT_FALSE(m.has_a());
}

TEST(JsonUtilTest, TestParsingUnknownEnumsProto3) {
  TestMessage m;
  StringPiece input = R"json({"enum_value":"UNKNOWN_VALUE"})json";

  m.set_enum_value(proto3::BAR);
  EXPECT_THAT(FromJson(input, &m),
              StatusIs(util::StatusCode::kInvalidArgument));
  ASSERT_EQ(m.enum_value(), proto3::BAR);  // Keep previous value

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_OK(FromJson(input, &m, options));
  EXPECT_EQ(m.enum_value(), 0);  // Unknown enum value must be decoded as 0
}

TEST(JsonUtilTest, TestParsingUnknownEnumsProto3FromInt) {
  TestMessage m;
  StringPiece input = R"json({"enum_value":12345})json";

  m.set_enum_value(proto3::BAR);
  EXPECT_OK(FromJson(input, &m));
  ASSERT_EQ(m.enum_value(), 12345);

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_OK(FromJson(input, &m, options));
  EXPECT_EQ(m.enum_value(), 12345);
}

// Trying to pass an object as an enum field value is always treated as an
// error
TEST(JsonUtilTest, TestParsingUnknownEnumsProto3FromObject) {
  TestMessage m;
  StringPiece input = R"json({"enum_value": {}})json";

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_THAT(FromJson(input, &m, options),
              StatusIs(util::StatusCode::kInvalidArgument));

  EXPECT_THAT(FromJson(input, &m),
              StatusIs(util::StatusCode::kInvalidArgument));
}

TEST(JsonUtilTest, TestParsingUnknownEnumsProto3FromArray) {
  TestMessage m;
  StringPiece input = R"json({"enum_value": []})json";

  EXPECT_THAT(FromJson(input, &m),
              StatusIs(util::StatusCode::kInvalidArgument));

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_THAT(FromJson(input, &m, options),
              StatusIs(util::StatusCode::kInvalidArgument));
}

TEST(JsonUtilTest, TestParsingEnumCaseSensitive) {
  TestMessage m;

  StringPiece input = R"json({"enum_value": "bar"})json";

  m.set_enum_value(proto3::FOO);
  EXPECT_THAT(FromJson(input, &m),
              StatusIs(util::StatusCode::kInvalidArgument));
  // Default behavior is case-sensitive, so keep previous value.
  ASSERT_EQ(m.enum_value(), proto3::FOO);
}

TEST(JsonUtilTest, TestParsingEnumIgnoreCase) {
  TestMessage m;
  StringPiece input = R"json({"enum_value":"bar"})json";

  m.set_enum_value(proto3::FOO);
  JsonParseOptions options;
  options.case_insensitive_enum_parsing = true;
  EXPECT_OK(FromJson(input, &m, options));
  ASSERT_EQ(m.enum_value(), proto3::BAR);
}

// A ZeroCopyOutputStream that writes to multiple buffers.
class SegmentedZeroCopyOutputStream : public io::ZeroCopyOutputStream {
 public:
  explicit SegmentedZeroCopyOutputStream(
      std::vector<StringPiece> segments)
      : segments_(segments) {
    // absl::c_* functions are not cloned in OSS.
    std::reverse(segments_.begin(), segments_.end());
  }

  bool Next(void** buffer, int* length) override {
    if (segments_.empty()) {
      return false;
    }
    last_segment_ = segments_.back();
    segments_.pop_back();
    // TODO(b/234159981): This is only ever constructed in test code, and only
    // from non-const bytes, so this is a valid cast. We need to do this since
    // OSS proto does not yet have absl::Span; once we take a full Abseil
    // dependency we should use that here instead.
    *buffer = const_cast<char*>(last_segment_.data());
    *length = static_cast<int>(last_segment_.size());
    byte_count_ += static_cast<int64_t>(last_segment_.size());
    return true;
  }

  void BackUp(int length) override {
    GOOGLE_CHECK(length <= static_cast<int>(last_segment_.size()));

    size_t backup = last_segment_.size() - static_cast<size_t>(length);
    segments_.push_back(last_segment_.substr(backup));
    last_segment_ = last_segment_.substr(0, backup);
    byte_count_ -= static_cast<int64_t>(length);
  }

  int64_t ByteCount() const override { return byte_count_; }

 private:
  std::vector<StringPiece> segments_;
  StringPiece last_segment_;
  int64_t byte_count_ = 0;
};

// This test splits the output buffer and also the input data into multiple
// segments and checks that the implementation of ZeroCopyStreamByteSink
// handles all possible cases correctly.
TEST(ZeroCopyStreamByteSinkTest, TestAllInputOutputPatterns) {
  static constexpr int kOutputBufferLength = 10;
  // An exhaustive test takes too long, skip some combinations to make the test
  // run faster.
  static constexpr int kSkippedPatternCount = 7;

  char buffer[kOutputBufferLength];
  for (int split_pattern = 0; split_pattern < (1 << (kOutputBufferLength - 1));
       split_pattern += kSkippedPatternCount) {
    // Split the buffer into small segments according to the split_pattern.
    std::vector<StringPiece> segments;
    int segment_start = 0;
    for (int i = 0; i < kOutputBufferLength - 1; ++i) {
      if (split_pattern & (1 << i)) {
        segments.push_back(
            StringPiece(buffer + segment_start, i - segment_start + 1));
        segment_start = i + 1;
      }
    }
    segments.push_back(StringPiece(buffer + segment_start,
                                         kOutputBufferLength - segment_start));

    // Write exactly 10 bytes through the ByteSink.
    std::string input_data = "0123456789";
    for (int input_pattern = 0; input_pattern < (1 << (input_data.size() - 1));
         input_pattern += kSkippedPatternCount) {
      memset(buffer, 0, sizeof(buffer));
      {
        SegmentedZeroCopyOutputStream output_stream(segments);
        internal::ZeroCopyStreamByteSink byte_sink(&output_stream);
        int start = 0;
        for (int j = 0; j < input_data.length() - 1; ++j) {
          if (input_pattern & (1 << j)) {
            byte_sink.Append(&input_data[start], j - start + 1);
            start = j + 1;
          }
        }
        byte_sink.Append(&input_data[start], input_data.length() - start);
      }
      EXPECT_EQ(std::string(buffer, input_data.length()), input_data);
    }

    // Write only 9 bytes through the ByteSink.
    input_data = "012345678";
    for (int input_pattern = 0; input_pattern < (1 << (input_data.size() - 1));
         input_pattern += kSkippedPatternCount) {
      memset(buffer, 0, sizeof(buffer));
      {
        SegmentedZeroCopyOutputStream output_stream(segments);
        internal::ZeroCopyStreamByteSink byte_sink(&output_stream);
        int start = 0;
        for (int j = 0; j < input_data.length() - 1; ++j) {
          if (input_pattern & (1 << j)) {
            byte_sink.Append(&input_data[start], j - start + 1);
            start = j + 1;
          }
        }
        byte_sink.Append(&input_data[start], input_data.length() - start);
      }
      EXPECT_EQ(std::string(buffer, input_data.length()), input_data);
      EXPECT_EQ(buffer[input_data.length()], 0);
    }

    // Write 11 bytes through the ByteSink. The extra byte will just
    // be ignored.
    input_data = "0123456789A";
    for (int input_pattern = 0; input_pattern < (1 << (input_data.size() - 1));
         input_pattern += kSkippedPatternCount) {
      memset(buffer, 0, sizeof(buffer));
      {
        SegmentedZeroCopyOutputStream output_stream(segments);
        internal::ZeroCopyStreamByteSink byte_sink(&output_stream);
        int start = 0;
        for (int j = 0; j < input_data.length() - 1; ++j) {
          if (input_pattern & (1 << j)) {
            byte_sink.Append(&input_data[start], j - start + 1);
            start = j + 1;
          }
        }
        byte_sink.Append(&input_data[start], input_data.length() - start);
      }
      EXPECT_EQ(input_data.substr(0, kOutputBufferLength),
                std::string(buffer, kOutputBufferLength));
    }
  }
}

TEST(JsonUtilTest, TestWrongJsonInput) {
  StringPiece json = "{\"unknown_field\":\"some_value\"}";
  io::ArrayInputStream input_stream(json.data(), json.size());
  char proto_buffer[10000];

  io::ArrayOutputStream output_stream(proto_buffer, sizeof(proto_buffer));
  std::string message_type = "type.googleapis.com/proto3.TestMessage";

  auto* resolver = NewTypeResolverForDescriptorPool(
      "type.googleapis.com", DescriptorPool::generated_pool());

  EXPECT_THAT(
      JsonToBinaryStream(resolver, message_type, &input_stream, &output_stream),
      StatusIs(util::StatusCode::kInvalidArgument));
  delete resolver;
}

TEST(JsonUtilTest, HtmlEscape) {
  TestMessage m;
  m.set_string_value("</script>");
  JsonPrintOptions options;
  EXPECT_THAT(ToJson(m, options),
              IsOkAndHolds("{\"stringValue\":\"\\u003c/script\\u003e\"}"));
}

}  // namespace
}  // namespace util
}  // namespace protobuf
}  // namespace google
