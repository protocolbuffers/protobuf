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

#include <google/protobuf/duration.pb.h>
#include <google/protobuf/field_mask.pb.h>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/wrappers.pb.h>
#include <google/protobuf/unittest.pb.h>
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
using ::testing::ElementsAre;
using ::testing::SizeIs;

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
    RETURN_IF_ERROR(ToProto(proto, json, options));
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
  m.set_string_value("foo");
  m.add_repeated_bool_value(true);
  m.add_repeated_bool_value(false);

  EXPECT_THAT(
      ToJson(m),
      IsOkAndHolds(
          R"({"stringValue":"foo","messageValue":{},"repeatedBoolValue":[true,false]})"));

  JsonPrintOptions options;
  options.add_whitespace = true;
  // Note: whitespace here is significant.
  EXPECT_THAT(ToJson(m, options), IsOkAndHolds(R"({
 "stringValue": "foo",
 "messageValue": {},
 "repeatedBoolValue": [
  true,
  false
 ]
}
)"));
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

  EXPECT_THAT(
      ToJson(protobuf_unittest::TestAllTypes(), options),
      IsOkAndHolds(
          R"({"optionalInt32":0,"optionalInt64":"0","optionalUint32":0,)"
          R"("optionalUint64":"0","optionalSint32":0,"optionalSint64":"0","optionalFixed32":0,)"
          R"("optionalFixed64":"0","optionalSfixed32":0,"optionalSfixed64":"0",)"
          R"("optionalFloat":0,"optionalDouble":0,"optionalBool":false,"optionalString":"",)"
          R"("optionalBytes":"","optionalgroup":null,"optionalNestedEnum":"FOO","optionalForeignEnum":"FOREIGN_FOO",)"
          R"("optionalImportEnum":"IMPORT_FOO","optionalStringPiece":"","optionalCord":"",)"
          R"("repeatedInt32":[],"repeatedInt64":[],"repeatedUint32":[],"repeatedUint64":[],)"
          R"("repeatedSint32":[],"repeatedSint64":[],"repeatedFixed32":[],"repeatedFixed64":[],)"
          R"("repeatedSfixed32":[],"repeatedSfixed64":[],"repeatedFloat":[],"repeatedDouble":[],)"
          R"("repeatedBool":[],"repeatedString":[],"repeatedBytes":[],"repeatedgroup":[],)"
          R"("repeatedNestedMessage":[],"repeatedForeignMessage":[],"repeatedImportMessage":[],)"
          R"("repeatedNestedEnum":[],"repeatedForeignEnum":[],"repeatedImportEnum":[],)"
          R"("repeatedStringPiece":[],"repeatedCord":[],"repeatedLazyMessage":[],"defaultInt32":41,)"
          R"("defaultInt64":"42","defaultUint32":43,"defaultUint64":"44","defaultSint32":-45,)"
          R"("defaultSint64":"46","defaultFixed32":47,"defaultFixed64":"48","defaultSfixed32":49,)"
          R"("defaultSfixed64":"-50","defaultFloat":51.5,"defaultDouble":52000,"defaultBool":true,)"
          R"("defaultString":"hello","defaultBytes":"d29ybGQ=","defaultNestedEnum":"BAR",)"
          R"("defaultForeignEnum":"FOREIGN_BAR","defaultImportEnum":"IMPORT_BAR",)"
          R"("defaultStringPiece":"abc","defaultCord":"123"})"));

  // The ESF parser actually gets this wrong, and serializes floats whose
  // default value is non-finite as 0. We make sure to reproduce this bug.
  EXPECT_THAT(
      ToJson(protobuf_unittest::TestExtremeDefaultValues(), options),
      IsOkAndHolds(
          R"({"escapedBytes":"XDAwMFwwMDFcMDA3XDAxMFwwMTRcblxyXHRcMDEzXFxcJ1wiXDM3Ng==")"
          R"(,"largeUint32":4294967295,"largeUint64":"18446744073709551615",)"
          R"("smallInt32":-2147483647,"smallInt64":"-9223372036854775807")"
          R"(,"reallySmallInt32":-2147483648,"reallySmallInt64":"-9223372036854775808",)"
          R"("utf8String":"ሴ","zeroFloat":0,"oneFloat":1,"smallFloat":1.5,)"
          R"("negativeOneFloat":-1,"negativeFloat":-1.5,"largeFloat":2e+08,)"
          R"("smallNegativeFloat":-8e-28,"infDouble":0,"negInfDouble":0)"
          R"(,"nanDouble":0,"infFloat":0,"negInfFloat":0,"nanFloat":0)"
          R"(,"cppTrigraph":"? ? ?? ?? ??? ??/ ??-","stringWithZero":"hel\u0000lo")"
          R"(,"bytesWithZero":"d29yXDAwMGxk","stringPieceWithZero":"ab\u0000c")"
          R"(,"cordWithZero":"12\u00003","replacementString":"${unknown}"})"));
}

TEST_P(JsonTest, TestPreserveProtoFieldNames) {
  TestMessage m;
  m.mutable_message_value();

  JsonPrintOptions options;
  options.preserve_proto_field_names = true;
  EXPECT_THAT(ToJson(m, options), IsOkAndHolds("{\"message_value\":{}}"));

}

TEST_P(JsonTest, Camels) {
  protobuf_unittest::TestCamelCaseFieldNames m;
  m.set_stringfield("sTRINGfIELD");

  EXPECT_THAT(ToJson(m), IsOkAndHolds(R"({"StringField":"sTRINGfIELD"})"));
}

TEST_P(JsonTest, EvilString) {
  auto m = ToProto<TestMessage>(R"json(
    {"string_value": ")json"
                                "\n\r\b\f\1\2\3"
                                "\"}");
  ASSERT_OK(m);
  EXPECT_EQ(m->string_value(), "\n\r\b\f\1\2\3");
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
  EXPECT_THAT(parsed->repeated_enum_value(),
              ElementsAre(proto3::FOO, proto3::BAR));
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

TEST_P(JsonTest, TestPrintProto2EnumAsIntWithDefaultValue) {
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

TEST_P(JsonTest, WebSafeBytes) {
  auto m = ToProto<TestMessage>(R"json({
      "bytesValue": "-_"
  })json");
  ASSERT_OK(m);

  EXPECT_EQ(m->bytes_value(), "\xfb");
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
      "doubleValue": -55.3,
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
        {},
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
  EXPECT_EQ(m->double_value(),
            -55.3);  // This value is intentionally not a nice
                     // round number in base 2, so its floating point
                     // representation has many digits at the end, which
                     // printing back to JSON must handle well.
  EXPECT_EQ(m->string_value(), "foo bar baz");
  EXPECT_EQ(m->enum_value(), proto3::EnumType::BAR);
  EXPECT_EQ(m->message_value().value(), 2048);

  EXPECT_THAT(m->repeated_bool_value(), ElementsAre(true));
  EXPECT_THAT(m->repeated_int32_value(), ElementsAre(0, -42));
  EXPECT_THAT(m->repeated_uint64_value(), ElementsAre(1, 2));
  EXPECT_THAT(m->repeated_double_value(), ElementsAre(1.5, -2));
  EXPECT_THAT(m->repeated_string_value(), ElementsAre("foo", "bar ", ""));
  EXPECT_THAT(m->repeated_enum_value(), ElementsAre(proto3::BAR, proto3::FOO));

  ASSERT_THAT(m->repeated_message_value(), SizeIs(3));
  EXPECT_EQ(m->repeated_message_value(0).value(), 40);
  EXPECT_EQ(m->repeated_message_value(1).value(), 0);
  EXPECT_EQ(m->repeated_message_value(2).value(), 96);

  EXPECT_THAT(
      ToJson(*m),
      IsOkAndHolds(
          R"({"boolValue":true,"int32Value":1234567891,"int64Value":"-5302428716536692736",)"
          R"("uint32Value":42,"uint64Value":"530242871653669","floatValue":3.4e+38,)"
          R"("doubleValue":-55.3,"stringValue":"foo bar baz","enumValue":"BAR",)"
          R"("messageValue":{"value":2048},"repeatedBoolValue":[true],"repeatedInt32Value":[0,-42])"
          R"(,"repeatedUint64Value":["1","2"],"repeatedDoubleValue":[1.5,-2],)"
          R"("repeatedStringValue":["foo","bar ",""],"repeatedEnumValue":["BAR","FOO"],)"
          R"("repeatedMessageValue":[{"value":40},{},{"value":96}]})"));
}

TEST_P(JsonTest, CurseOfAtob) {
  auto m = ToProto<TestMessage>(R"json(
    {
      repeatedBoolValue: ["0", "1", "false", "true", "f", "t", "no", "yes", "n", "y"]
    }
  )json");
  ASSERT_OK(m);
  EXPECT_THAT(m->repeated_bool_value(),
              ElementsAre(false, true, false, true, false, true, false, true,
                          false, true));
}

TEST_P(JsonTest, FloatPrecision) {
  google::protobuf::Value v;
  v.mutable_list_value()->add_values()->set_number_value(0.9900000095367432);
  v.mutable_list_value()->add_values()->set_number_value(0.8799999952316284);

  EXPECT_THAT(ToJson(v),
              IsOkAndHolds("[0.99000000953674316,0.87999999523162842]"));
}

TEST_P(JsonTest, ParseLegacySingleRepeatedField) {
  auto m = ToProto<TestMessage>(R"json({
    "repeatedInt32Value": 1997,
    "repeatedStringValue": "oh no",
    "repeatedEnumValue": "BAR",
    "repeatedMessageValue": {"value": -1}
  })json");
  ASSERT_OK(m);

  EXPECT_THAT(m->repeated_int32_value(), ElementsAre(1997));
  EXPECT_THAT(m->repeated_string_value(), ElementsAre("oh no"));
  EXPECT_THAT(m->repeated_enum_value(), ElementsAre(proto3::EnumType::BAR));

  ASSERT_THAT(m->repeated_message_value(), SizeIs(1));
  EXPECT_EQ(m->repeated_message_value(0).value(), -1);

  EXPECT_THAT(ToJson(*m),
              IsOkAndHolds(R"({"repeatedInt32Value":[1997],)"
                           R"("repeatedStringValue":["oh no"],)"
                           R"("repeatedEnumValue":["BAR"],)"
                           R"("repeatedMessageValue":[{"value":-1}]})"));
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

TEST_P(JsonTest, RepeatedMapKey) {
  EXPECT_THAT(ToProto<TestMap>(R"json({
    "string_map": {
      "twiceKey": 0,
      "twiceKey": 1
    }
  })json"), StatusIs(util::StatusCode::kInvalidArgument));
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

TEST_P(JsonTest, ParseOverOneof) {
  TestOneof m;
  m.set_oneof_string_value("foo");
  ASSERT_OK(ToProto(m, R"json({
    "oneofInt32Value": 5,
  })json"));
  EXPECT_EQ(m.oneof_int32_value(), 5);
}

TEST_P(JsonTest, RepeatedSingularKeys) {
  auto m = ToProto<TestMessage>(R"json({
    "int32Value": 1,
    "int32Value": 2
  })json");
  EXPECT_OK(m);
  EXPECT_EQ(m->int32_value(), 2);
}

TEST_P(JsonTest, RepeatedRepeatedKeys) {
  auto m = ToProto<TestMessage>(R"json({
    "repeatedInt32Value": [1],
    "repeatedInt32Value": [2, 3]
  })json");
  EXPECT_OK(m);
  EXPECT_THAT(m->repeated_int32_value(), ElementsAre(1, 2, 3));
}

TEST_P(JsonTest, RepeatedOneofKeys) {
  EXPECT_THAT(ToProto<TestOneof>(R"json({
    "oneofInt32Value": 1,
    "oneofStringValue": "foo"
  })json"),
              StatusIs(util::StatusCode::kInvalidArgument));
}

TEST_P(JsonTest, TestParseIgnoreUnknownFields) {
  JsonParseOptions options;
  options.ignore_unknown_fields = true;
  EXPECT_OK(ToProto<TestMessage>(R"({"unknownName":0})", options));

  TestMessage m;
  m.GetReflection()->MutableUnknownFields(&m)->AddFixed32(9001, 9001);
  m.GetReflection()->MutableUnknownFields(&m)->AddFixed64(9001, 9001);
  m.GetReflection()->MutableUnknownFields(&m)->AddVarint(9001, 9001);
  m.GetReflection()->MutableUnknownFields(&m)->AddLengthDelimited(9001, "9001");
  EXPECT_THAT(ToJson(m), IsOkAndHolds("{}"));
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
  EXPECT_THAT(generated.repeated_int32_value(), ElementsAre(1, 2));

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

TEST_P(JsonTest, TestParsingBrokenAny) {
  auto m = ToProto<TestAny>(R"json(
    {
      "value": {}
    }
  )json");
  ASSERT_OK(m);
  EXPECT_EQ(m->value().type_url(), "");
  EXPECT_EQ(m->value().value(), "");

  EXPECT_THAT(ToProto<TestAny>(R"json(
    {
      "value": {
        "type_url": "garbage",
        "value": "bW9yZSBnYXJiYWdl"
      }
    }
  )json"),
              StatusIs(util::StatusCode::kInvalidArgument));
}

TEST_P(JsonTest, TestFlatList) {
  auto m = ToProto<TestMessage>(R"json(
    {
      "repeatedInt32Value": [[[5]], [6]]
    }
  )json");
  ASSERT_OK(m);
  EXPECT_THAT(m->repeated_int32_value(), ElementsAre(5, 6));

  // The above flatteing behavior is suppressed for google::protobuf::ListValue.
  auto m2 = ToProto<google::protobuf::Value>(R"json(
    {
      "repeatedInt32Value": [[[5]], [6]]
    }
  )json");
  ASSERT_OK(m2);
  auto fields = m2->struct_value().fields();
  auto list = fields["repeatedInt32Value"].list_value();
  EXPECT_EQ(list.values(0)
                .list_value()
                .values(0)
                .list_value()
                .values(0)
                .number_value(),
            5);
  EXPECT_EQ(list.values(1).list_value().values(0).number_value(), 6);
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

  auto m2 = ToProto<TestWrapper>(R"json(
    {
      "boolValue": { "value": true },
      "int32Value": { "value": 42 },
      "stringValue": { "value": "ieieo" },
    }
  )json");
  ASSERT_OK(m2);

  EXPECT_TRUE(m2->bool_value().value());
  EXPECT_EQ(m2->int32_value().value(), 42);
  EXPECT_EQ(m2->string_value().value(), "ieieo");
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

TEST_P(JsonTest, TestHugeBareString) {
  auto m = ToProto<TestMessage>(R"json({
    "int64Value": 6009652459062546621
  })json");
  ASSERT_OK(m);
  EXPECT_EQ(m->int64_value(), 6009652459062546621);
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

TEST_P(JsonTest, TestParsingEnumLowercase) {
  JsonParseOptions options;
  options.case_insensitive_enum_parsing = true;
  auto m =
      ToProto<TestMessage>(R"json({"enum_value": "TLSv1_2"})json", options);
  ASSERT_OK(m);
  EXPECT_THAT(m->enum_value(), proto3::TLSv1_2);
}

TEST_P(JsonTest, TestParsingEnumIgnoreCase) {
  TestMessage m;
  m.set_enum_value(proto3::FOO);

  JsonParseOptions options;
  options.case_insensitive_enum_parsing = true;
  ASSERT_OK(ToProto(m, R"json({"enum_value":"bar"})json", options));
  EXPECT_EQ(m.enum_value(), proto3::BAR);
}

// Parsing does NOT work like MergeFrom: existing repeated field values are
// clobbered, not appended to.
TEST_P(JsonTest, TestOverwriteRepeated) {
  TestMessage m;
  m.add_repeated_int32_value(5);

  ASSERT_OK(ToProto(m, R"json({"repeated_int32_value": [1, 2, 3]})json"));
  EXPECT_THAT(m.repeated_int32_value(), ElementsAre(1, 2, 3));
}


TEST_P(JsonTest, TestDuration) {
  auto m = ToProto<proto3::TestDuration>(R"json(
    {
      "value": "123456.789s",
      "repeated_value": ["0.1s", "999s"]
    }
  )json");
  ASSERT_OK(m);

  EXPECT_EQ(m->value().seconds(), 123456);
  EXPECT_EQ(m->value().nanos(), 789000000);

  EXPECT_THAT(m->repeated_value(), SizeIs(2));
  EXPECT_EQ(m->repeated_value(0).seconds(), 0);
  EXPECT_EQ(m->repeated_value(0).nanos(), 100000000);
  EXPECT_EQ(m->repeated_value(1).seconds(), 999);
  EXPECT_EQ(m->repeated_value(1).nanos(), 0);

  EXPECT_THAT(
      ToJson(*m),
      IsOkAndHolds(
          R"({"value":"123456.789s","repeatedValue":["0.100s","999s"]})"));

  auto m2 = ToProto<proto3::TestDuration>(R"json(
    {
      "value": {"seconds": 4, "nanos": 5},
    }
  )json");
  ASSERT_OK(m2);

  EXPECT_EQ(m2->value().seconds(), 4);
  EXPECT_EQ(m2->value().nanos(), 5);

  // Negative duration with zero seconds.
  auto m3 = ToProto<proto3::TestDuration>(R"json(
    {
      "value": {"nanos": -5},
    }
  )json");
  ASSERT_OK(m3);
  EXPECT_EQ(m3->value().seconds(), 0);
  EXPECT_EQ(m3->value().nanos(), -5);
  EXPECT_THAT(ToJson(m3->value()), IsOkAndHolds("\"-0.000000005s\""));

  // Negative duration with zero nanos.
  auto m4 = ToProto<proto3::TestDuration>(R"json(
    {
      "value": {"seconds": -5},
    }
  )json");
  ASSERT_OK(m4);
  EXPECT_EQ(m4->value().seconds(), -5);
  EXPECT_EQ(m4->value().nanos(), 0);
  EXPECT_THAT(ToJson(m4->value()), IsOkAndHolds("\"-5s\""));

  // Parse "0.5s" as a JSON string.
  auto m5 = ToProto<proto3::TestDuration>(R"json(
    {
      "value": "0.5s",
    }
  )json");
  ASSERT_OK(m5);
  EXPECT_EQ(m5->value().seconds(), 0);
  EXPECT_EQ(m5->value().nanos(), 500000000);
  EXPECT_THAT(ToJson(m5->value()), IsOkAndHolds("\"0.500s\""));
}

// These tests are not exhaustive; tests in //third_party/protobuf/conformance
// are more comprehensive.
TEST_P(JsonTest, TestTimestamp) {
  auto m = ToProto<proto3::TestTimestamp>(R"json(
    {
      "value": "1996-02-27T12:00:00Z",
      "repeated_value": ["9999-12-31T23:59:59Z"]
    }
  )json");
  ASSERT_OK(m);

  EXPECT_EQ(m->value().seconds(), 825422400);
  EXPECT_EQ(m->value().nanos(), 0);
  EXPECT_THAT(m->repeated_value(), SizeIs(1));
  EXPECT_EQ(m->repeated_value(0).seconds(), 253402300799);
  EXPECT_EQ(m->repeated_value(0).nanos(), 0);

  EXPECT_THAT(
      ToJson(*m),
      IsOkAndHolds(
          R"({"value":"1996-02-27T12:00:00Z","repeatedValue":["9999-12-31T23:59:59Z"]})"));

  auto m2 = ToProto<proto3::TestTimestamp>(R"json(
    {
      "value": {"seconds": 4, "nanos": 5},
    }
  )json");
  ASSERT_OK(m2);

  EXPECT_EQ(m2->value().seconds(), 4);
  EXPECT_EQ(m2->value().nanos(), 5);
}

// This test case comes from Envoy's tests. They like to parse a Value out of
// YAML, turn it into JSON, and then parse it as a different proto. This means
// we must be extremely careful with integer fields, because they need to
// round-trip through doubles. This happens all over Envoy. :(
TEST_P(JsonTest, TestEnvoyRoundTrip) {
  auto m = ToProto<google::protobuf::Value>(R"json(
    {
      "value": {"seconds": 1234567891, "nanos": 234000000},
    }
  )json");
  ASSERT_OK(m);

  auto j = ToJson(*m);
  ASSERT_OK(j);

  auto m2 = ToProto<proto3::TestTimestamp>(*j);
  ASSERT_OK(m2);

  EXPECT_EQ(m2->value().seconds(), 1234567891);
  EXPECT_EQ(m2->value().nanos(), 234000000);
}

TEST_P(JsonTest, TestFieldMask) {
  auto m = ToProto<proto3::TestFieldMask>(R"json(
    {
      "value": "foo,bar.bazBaz"
    }
  )json");
  ASSERT_OK(m);

  EXPECT_THAT(m->value().paths(), ElementsAre("foo", "bar.baz_baz"));
  EXPECT_THAT(ToJson(*m), IsOkAndHolds(R"({"value":"foo,bar.bazBaz"})"));

  auto m2 = ToProto<proto3::TestFieldMask>(R"json(
    {
      "value": {
        "paths": ["yep.really"]
      },
    }
  )json");
  ASSERT_OK(m2);

  EXPECT_THAT(m2->value().paths(), ElementsAre("yep.really"));
}

TEST_P(JsonTest, TestLegalNullsInArray) {
  auto m = ToProto<proto3::TestNullValue>(R"json({
    "repeatedNullValue": [null]
  })json");
  ASSERT_OK(m);

  EXPECT_THAT(m->repeated_null_value(),
              ElementsAre(google::protobuf::NULL_VALUE));

  auto m2 = ToProto<proto3::TestValue>(R"json({
    "repeatedValue": [null]
  })json");
  ASSERT_OK(m2);

  ASSERT_THAT(m2->repeated_value(), SizeIs(1));
  EXPECT_TRUE(m2->repeated_value(0).has_null_value());

  m2->Clear();
  m2->mutable_repeated_value();  // Materialize an empty singular Value.
  m2->add_repeated_value();
  m2->add_repeated_value()->set_string_value("solitude");
  m2->add_repeated_value();
  EXPECT_THAT(ToJson(*m2), IsOkAndHolds(R"({"repeatedValue":["solitude"]})"));
}

TEST_P(JsonTest, ListList) {
  auto m = ToProto<proto3::TestListValue>(R"json({
    "repeated_value": [["ayy", "lmao"]]
  })json");
  ASSERT_OK(m);

  EXPECT_EQ(m->repeated_value(0).values(0).string_value(), "ayy");
  EXPECT_EQ(m->repeated_value(0).values(1).string_value(), "lmao");

  m = ToProto<proto3::TestListValue>(R"json({
    "repeated_value": [{
      "values": ["ayy", "lmao"]
    }]
  })json");
  ASSERT_OK(m);

  EXPECT_EQ(m->repeated_value(0).values(0).string_value(), "ayy");
  EXPECT_EQ(m->repeated_value(0).values(1).string_value(), "lmao");
}

TEST_P(JsonTest, HtmlEscape) {
  TestMessage m;
  m.set_string_value("</script>");
  EXPECT_THAT(ToJson(m),
              IsOkAndHolds(R"({"stringValue":"\u003c/script\u003e"})"));
}

}  // namespace
}  // namespace util
}  // namespace protobuf
}  // namespace google
