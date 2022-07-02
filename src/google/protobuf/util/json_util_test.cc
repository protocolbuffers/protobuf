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
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <google/protobuf/wrappers.pb.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/status.h>
#include <google/protobuf/stubs/statusor.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/util/internal/testdata/maps.pb.h>
#include <google/protobuf/util/json_format.pb.h>
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
using ::proto3::TestWrapper;
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

enum class Codec {
  kReflective,
  kResolver,
};

class JsonTest : public testing::TestWithParam<Codec> {
 protected:
  util::StatusOr<std::string> ToJson(const Message& proto,
                                     JsonPrintOptions options = {}) {
    if (GetParam() == Codec::kReflective) {
      std::string result;
      RETURN_IF_ERROR(MessageToJsonString(proto, &result, options));
      return result;
    }
    std::string proto_data = proto.SerializeAsString();
    io::ArrayInputStream in(proto_data.data(), proto_data.size());

    std::string result;
    io::StringOutputStream out(&result);

    RETURN_IF_ERROR(BinaryToJsonStream(
        resolver_.get(),
        StrCat("type.googleapis.com/", proto.GetTypeName()), &in, &out,
        options));
    return result;
  }

  // The out parameter comes first since `json` tends to be a very long string,
  // and clang-format does a poor job if it is not the last parameter.
  util::Status ToProto(Message& proto, StringPiece json,
                       JsonParseOptions options = {}) {
    if (GetParam() == Codec::kReflective) {
      return JsonStringToMessage(json, &proto, options);
    }
    io::ArrayInputStream in(json.data(), json.size());

    std::string result;
    io::StringOutputStream out(&result);

    RETURN_IF_ERROR(JsonToBinaryStream(
        resolver_.get(),
        StrCat("type.googleapis.com/", proto.GetTypeName()), &in, &out,
        options));

    if (!proto.ParseFromString(result)) {
      return util::InternalError("wire format parse failed");
    }
    return util::OkStatus();
  }

  template <typename Proto>
  util::StatusOr<Proto> ToProto(StringPiece json,
                                JsonParseOptions options = {}) {
    Proto proto;
    RETURN_IF_ERROR(JsonStringToMessage(json, &proto, options));
    return proto;
  }

  std::unique_ptr<TypeResolver> resolver_{NewTypeResolverForDescriptorPool(
      "type.googleapis.com", DescriptorPool::generated_pool())};
};

INSTANTIATE_TEST_SUITE_P(JsonTestSuite, JsonTest,
                         testing::Values(Codec::kReflective, Codec::kResolver));

TEST_P(JsonTest, TestWhitespaces) {
  TestMessage m;
  m.mutable_message_value();

  EXPECT_THAT(ToJson(m), IsOkAndHolds("{\"messageValue\":{}}"));

  JsonPrintOptions options;
  options.add_whitespace = true;
  EXPECT_THAT(ToJson(m, options), IsOkAndHolds("{\n"
                                               " \"messageValue\": {}\n"
                                               "}\n"));
}

TEST_P(JsonTest, TestDefaultValues) {
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

TEST_P(JsonTest, TestPreserveProtoFieldNames) {
  if (GetParam() == Codec::kResolver) {
    GTEST_SKIP();
  }

  TestMessage m;
  m.mutable_message_value();

  JsonPrintOptions options;
  options.preserve_proto_field_names = true;
  EXPECT_THAT(ToJson(m, options), IsOkAndHolds("{\"message_value\":{}}"));
}

TEST_P(JsonTest, TestAlwaysPrintEnumsAsInts) {
  TestMessage orig;
  orig.set_enum_value(proto3::BAR);
  orig.add_repeated_enum_value(proto3::FOO);
  orig.add_repeated_enum_value(proto3::BAR);

  JsonPrintOptions print_options;
  print_options.always_print_enums_as_ints = true;

  auto printed = ToJson(orig, print_options);
  ASSERT_THAT(printed,
              IsOkAndHolds("{\"enumValue\":1,\"repeatedEnumValue\":[0,1]}"));

  auto parsed = ToProto<TestMessage>(*printed);
  ASSERT_OK(parsed);

  EXPECT_EQ(parsed->enum_value(), proto3::BAR);
  EXPECT_EQ(parsed->repeated_enum_value_size(), 2);
  EXPECT_EQ(parsed->repeated_enum_value(0), proto3::FOO);
  EXPECT_EQ(parsed->repeated_enum_value(1), proto3::BAR);
}

TEST_P(JsonTest, TestPrintEnumsAsIntsWithDefaultValue) {
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

  auto parsed = ToProto<TestEnumValue>(*printed);

  EXPECT_EQ(parsed->enum_value1(), proto3::FOO);
  EXPECT_EQ(parsed->enum_value2(), proto3::FOO);
  EXPECT_EQ(parsed->enum_value3(), proto3::BAR);
}

TEST_P(JsonTest, DISABLED_TestPrintProto2EnumAsIntWithDefaultValue) {
  protobuf_unittest::TestDefaultEnumValue orig;

  JsonPrintOptions print_options;
  print_options.always_print_enums_as_ints = true;
  print_options.always_print_primitive_fields = true;

  auto printed = ToJson(orig, print_options);
  ASSERT_THAT(printed, IsOkAndHolds("{\"enumValue\":2}"));

  auto parsed = ToProto<protobuf_unittest::TestDefaultEnumValue>(*printed);
  ASSERT_OK(parsed);

  EXPECT_EQ(parsed->enum_value(), protobuf_unittest::DEFAULT);
}

TEST_P(JsonTest, ParseMessage) {
  auto m = ToProto<TestMessage>(R"json(
    {
      "boolValue": true,
      "int32Value": 1234567891,
      "int64Value": -5302428716536692736,
      "uint32Value": 42,
      "uint64Value": 530242871653669,
      "floatValue": 3.4e+38,
      "doubleValue": -55.5,
      "stringValue": "foo bar baz",
      "enumValue": "BAR",
      "messageValue": {
        "value": 2048
      },

      "repeatedBoolValue": [true],
      "repeatedInt32Value": [0, -42],
      "repeatedUint64Value": [1, 2],
      "repeatedDoubleValue": [1.5, -2],
      "repeatedStringValue": ["foo", "bar ", ""],
      "repeatedEnumValue": [1, "FOO"],
      "repeatedMessageValue": [
        {"value": 40},
        {"value": 96}
      ]
    }
  )json");
  ASSERT_OK(m);

  EXPECT_TRUE(m->bool_value());
  EXPECT_EQ(m->int32_value(), 1234567891);
  EXPECT_EQ(m->int64_value(), -5302428716536692736);
  EXPECT_EQ(m->uint32_value(), 42);
  EXPECT_EQ(m->uint64_value(), 530242871653669);
  EXPECT_EQ(m->float_value(), 3.4e+38f);
  EXPECT_EQ(m->double_value(), -55.5);
  EXPECT_EQ(m->string_value(), "foo bar baz");
  EXPECT_EQ(m->enum_value(), proto3::EnumType::BAR);
  EXPECT_EQ(m->message_value().value(), 2048);

  ASSERT_EQ(m->repeated_bool_value_size(), 1);
  EXPECT_TRUE(m->repeated_bool_value(0));

  ASSERT_EQ(m->repeated_int32_value_size(), 2);
  EXPECT_EQ(m->repeated_int32_value(0), 0);
  EXPECT_EQ(m->repeated_int32_value(1), -42);

  ASSERT_EQ(m->repeated_uint64_value_size(), 2);
  EXPECT_EQ(m->repeated_uint64_value(0), 1);
  EXPECT_EQ(m->repeated_uint64_value(1), 2);

  ASSERT_EQ(m->repeated_double_value_size(), 2);
  EXPECT_EQ(m->repeated_double_value(0), 1.5);
  EXPECT_EQ(m->repeated_double_value(1), -2);

  ASSERT_EQ(m->repeated_string_value_size(), 3);
  EXPECT_EQ(m->repeated_string_value(0), "foo");
  EXPECT_EQ(m->repeated_string_value(1), "bar ");
  EXPECT_EQ(m->repeated_string_value(2), "");

  ASSERT_EQ(m->repeated_enum_value_size(), 2);
  EXPECT_EQ(m->repeated_enum_value(0), proto3::EnumType::BAR);
  EXPECT_EQ(m->repeated_enum_value(1), proto3::EnumType::FOO);

  ASSERT_EQ(m->repeated_message_value_size(), 2);
  EXPECT_EQ(m->repeated_message_value(0).value(), 40);
  EXPECT_EQ(m->repeated_message_value(1).value(), 96);

  EXPECT_THAT(
      ToJson(*m),
      IsOkAndHolds(
          R"({"boolValue":true,"int32Value":1234567891,"int64Value":"-5302428716536692736",)"
          R"("uint32Value":42,"uint64Value":"530242871653669","floatValue":3.4e+38,)"
          R"("doubleValue":-55.5,"stringValue":"foo bar baz","enumValue":"BAR",)"
          R"("messageValue":{"value":2048},"repeatedBoolValue":[true],"repeatedInt32Value":[0,-42])"
          R"(,"repeatedUint64Value":["1","2"],"repeatedDoubleValue":[1.5,-2],)"
          R"("repeatedStringValue":["foo","bar ",""],"repeatedEnumValue":["BAR","FOO"],)"
          R"("repeatedMessageValue":[{"value":40},{"value":96}]})"));
}

TEST_P(JsonTest, ParseMap) {
  TestMap message;
  (*message.mutable_string_map())["hello"] = 1234;
  auto printed = ToJson(message);
  ASSERT_THAT(printed, IsOkAndHolds(R"({"stringMap":{"hello":1234}})"));

  auto other = ToProto<TestMap>(*printed);
  ASSERT_OK(other);
  EXPECT_EQ(other->DebugString(), message.DebugString());
}

TEST_P(JsonTest, ParsePrimitiveMapIn) {
  MapIn message;
  JsonPrintOptions print_options;
  print_options.always_print_primitive_fields = true;
  auto printed = ToJson(message, print_options);
  ASSERT_THAT(
      ToJson(message, print_options),
      IsOkAndHolds(R"({"other":"","things":[],"mapInput":{},"mapAny":{}})"));

  auto other = ToProto<MapIn>(*printed);
  ASSERT_OK(other);
  EXPECT_EQ(other->DebugString(), message.DebugString());
}

TEST_P(JsonTest, PrintPrimitiveOneof) {
  TestOneof message;
  JsonPrintOptions options;
  options.always_print_primitive_fields = true;
  message.mutable_oneof_message_value();
  EXPECT_THAT(ToJson(message, options),
              IsOkAndHolds(R"({"oneofMessageValue":{"value":0}})"));

  message.set_oneof_int32_value(1);
  EXPECT_THAT(ToJson(message, options),
              IsOkAndHolds(R"({"oneofInt32Value":1})"));
}

TEST_P(JsonTest, TestParseIgnoreUnknownFields) {
  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_OK(ToProto<TestMessage>(R"({"unknownName":0})", options));
}

TEST_P(JsonTest, TestParseErrors) {
  // Parsing should fail if the field name can not be recognized.
  EXPECT_THAT(ToProto<TestMessage>(R"({"unknownName": 0})"),
              StatusIs(util::StatusCode::kInvalidArgument));
  // Parsing should fail if the value is invalid.
  EXPECT_THAT(ToProto<TestMessage>(R"("{"int32Value": 2147483648})"),
              StatusIs(util::StatusCode::kInvalidArgument));
}

TEST_P(JsonTest, TestDynamicMessage) {
  // Create a new DescriptorPool with the same protos as the generated one.
  DescriptorPoolDatabase database(*DescriptorPool::generated_pool());
  DescriptorPool pool(&database);
  // A dynamic version of the test proto.
  DynamicMessageFactory factory;
  std::unique_ptr<Message> message(
      factory.GetPrototype(pool.FindMessageTypeByName("proto3.TestMessage"))
          ->New());
  ASSERT_OK(ToProto(*message, R"json(
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
  )json"));

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

TEST_P(JsonTest, TestParsingAny) {
  auto m = ToProto<TestAny>(R"json(
    {
      "value": {
        "@type": "type.googleapis.com/proto3.TestMessage",
        "int32_value": 5,
        "string_value": "expected_value",
        "message_value": {"value": 1}
      }
    }
  )json");
  ASSERT_OK(m);

  TestMessage t;
  ASSERT_TRUE(m->value().UnpackTo(&t));
  EXPECT_EQ(t.int32_value(), 5);
  EXPECT_EQ(t.string_value(), "expected_value");
  EXPECT_EQ(t.message_value().value(), 1);

  EXPECT_THAT(
      ToJson(*m),
      IsOkAndHolds(
          R"({"value":{"@type":"type.googleapis.com/proto3.TestMessage",)"
          R"("int32Value":5,"stringValue":"expected_value","messageValue":{"value":1}}})"));
}

TEST_P(JsonTest, TestParsingAnyMiddleAtType) {
  auto m = ToProto<TestAny>(R"json(
    {
      "value": {
        "int32_value": 5,
        "string_value": "expected_value",
        "@type": "type.googleapis.com/proto3.TestMessage",
        "message_value": {"value": 1}
      }
    }
  )json");
  ASSERT_OK(m);

  TestMessage t;
  ASSERT_TRUE(m->value().UnpackTo(&t));
  EXPECT_EQ(t.int32_value(), 5);
  EXPECT_EQ(t.string_value(), "expected_value");
  EXPECT_EQ(t.message_value().value(), 1);
}

TEST_P(JsonTest, TestParsingAnyEndAtType) {
  auto m = ToProto<TestAny>(R"json(
    {
      "value": {
        "int32_value": 5,
        "string_value": "expected_value",
        "message_value": {"value": 1},
        "@type": "type.googleapis.com/proto3.TestMessage"
      }
    }
  )json");
  ASSERT_OK(m);

  TestMessage t;
  ASSERT_TRUE(m->value().UnpackTo(&t));
  EXPECT_EQ(t.int32_value(), 5);
  EXPECT_EQ(t.string_value(), "expected_value");
  EXPECT_EQ(t.message_value().value(), 1);
}

TEST_P(JsonTest, TestParsingNestedAnys) {
  auto m = ToProto<TestAny>(R"json(
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
  )json");
  ASSERT_OK(m);

  google::protobuf::Any inner;
  ASSERT_TRUE(m->value().UnpackTo(&inner));

  TestMessage t;
  ASSERT_TRUE(inner.UnpackTo(&t));
  EXPECT_EQ(t.int32_value(), 5);
  EXPECT_EQ(t.string_value(), "expected_value");
  EXPECT_EQ(t.message_value().value(), 1);

  EXPECT_THAT(
      ToJson(*m),
      IsOkAndHolds(
          R"({"value":{"@type":"type.googleapis.com/google.protobuf.Any",)"
          R"("value":{"@type":"type.googleapis.com/proto3.TestMessage",)"
          R"("int32Value":5,"stringValue":"expected_value","messageValue":{"value":1}}}})"));
}

TEST_P(JsonTest, ParseWrappers) {
  auto m = ToProto<TestWrapper>(R"json(
    {
      "boolValue": true,
      "int32Value": 42,
      "stringValue": "ieieo",
    }
  )json");
  ASSERT_OK(m);

  EXPECT_TRUE(m->bool_value().value());
  EXPECT_EQ(m->int32_value().value(), 42);
  EXPECT_EQ(m->string_value().value(), "ieieo");

  EXPECT_THAT(
      ToJson(*m),
      IsOkAndHolds(
          R"({"boolValue":true,"int32Value":42,"stringValue":"ieieo"})"));
}

TEST_P(JsonTest, TestParsingUnknownAnyFields) {
  StringPiece input = R"json(
    {
      "value": {
        "@type": "type.googleapis.com/proto3.TestMessage",
        "unknown_field": "UNKNOWN_VALUE",
        "string_value": "expected_value"
      }
    }
  )json";

  EXPECT_THAT(ToProto<TestAny>(input),
              StatusIs(util::StatusCode::kInvalidArgument));

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  auto m = ToProto<TestAny>(input, options);
  ASSERT_OK(m);

  TestMessage t;
  ASSERT_TRUE(m->value().UnpackTo(&t));
  EXPECT_EQ(t.string_value(), "expected_value");
}

TEST_P(JsonTest, TestParsingUnknownEnumsProto2) {
  StringPiece input = R"json({"ayuLmao": "UNKNOWN_VALUE"})json";

  EXPECT_THAT(ToProto<protobuf_unittest::TestNumbers>(input),
              StatusIs(util::StatusCode::kInvalidArgument));

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  auto m = ToProto<protobuf_unittest::TestNumbers>(input, options);
  ASSERT_OK(m);
  EXPECT_FALSE(m->has_a());
}

TEST_P(JsonTest, TestParsingUnknownEnumsProto3) {
  TestMessage m;
  StringPiece input = R"json({"enum_value":"UNKNOWN_VALUE"})json";

  m.set_enum_value(proto3::BAR);
  ASSERT_THAT(ToProto(m, input), StatusIs(util::StatusCode::kInvalidArgument));
  EXPECT_EQ(m.enum_value(), proto3::BAR);  // Keep previous value

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  ASSERT_OK(ToProto(m, input, options));
  EXPECT_EQ(m.enum_value(), 0);  // Unknown enum value must be decoded as 0
}

TEST_P(JsonTest, TestParsingUnknownEnumsProto3FromInt) {
  TestMessage m;
  StringPiece input = R"json({"enum_value":12345})json";

  m.set_enum_value(proto3::BAR);
  ASSERT_OK(ToProto(m, input));
  EXPECT_EQ(m.enum_value(), 12345);

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  ASSERT_OK(ToProto(m, input, options));
  EXPECT_EQ(m.enum_value(), 12345);
}

// Trying to pass an object as an enum field value is always treated as an
// error
TEST_P(JsonTest, TestParsingUnknownEnumsProto3FromObject) {
  StringPiece input = R"json({"enum_value": {}})json";

  EXPECT_THAT(ToProto<TestMessage>(input),
              StatusIs(util::StatusCode::kInvalidArgument));

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_THAT(ToProto<TestMessage>(input, options),
              StatusIs(util::StatusCode::kInvalidArgument));
}

TEST_P(JsonTest, TestParsingUnknownEnumsProto3FromArray) {
  StringPiece input = R"json({"enum_value": []})json";

  EXPECT_THAT(ToProto<TestMessage>(input),
              StatusIs(util::StatusCode::kInvalidArgument));

  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_THAT(ToProto<TestMessage>(input, options),
              StatusIs(util::StatusCode::kInvalidArgument));
}

TEST_P(JsonTest, TestParsingEnumCaseSensitive) {
  TestMessage m;
  m.set_enum_value(proto3::FOO);
  EXPECT_THAT(ToProto(m, R"json({"enum_value": "bar"})json"),
              StatusIs(util::StatusCode::kInvalidArgument));
  // Default behavior is case-sensitive, so keep previous value.
  EXPECT_EQ(m.enum_value(), proto3::FOO);
}

TEST_P(JsonTest, TestParsingEnumIgnoreCase) {
  TestMessage m;
  m.set_enum_value(proto3::FOO);

  JsonParseOptions options;
  options.case_insensitive_enum_parsing = true;
  ASSERT_OK(ToProto(m, R"json({"enum_value":"bar"})json", options));
  EXPECT_EQ(m.enum_value(), proto3::BAR);
}

TEST_P(JsonTest, DISABLED_HtmlEscape) {
  TestMessage m;
  m.set_string_value("</script>");
  EXPECT_THAT(ToJson(m),
              IsOkAndHolds("{\"stringValue\":\"\\u003c/script\\u003e\"}"));
}

}  // namespace
}  // namespace util
}  // namespace protobuf
}  // namespace google
