// Copyright 2007 Google Inc.  All rights reserved.
// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "net/proto2/contrib/parse_proto/parse_text_proto.h"
#include "google/protobuf/unittest_eagerly_verified_lazy.pb.h"
#include "google/protobuf/unittest_import_proto3_eagerly_verified_lazy.pb.h"
#include "google/protobuf/unittest_mset_eagerly_verified_lazy.pb.h"
#include "google/protobuf/unittest_mset_wire_format_eagerly_verified_lazy.pb.h"
#include "net/proto2/util/public:proto_file_parser.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/flags/flag.h"
#include "absl/random/random.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/lazy_field.h"
#include "google/protobuf/parse_context.h"
#include "google/protobuf/reflection_visit_fields.h"
#include "google/protobuf/test_util.h"
#include "google/protobuf/wire_format_lite.h"

#define MESSAGE_TEST_NAME MessageTestEagerlyVerifiedLazy
#define MESSAGE_FACTORY_TEST_NAME MessageFactoryTestEagerlyVerifiedLazy
#define UNITTEST_PACKAGE_NAME "proto2_unittest_eagerly_verified_lazy"
#define UNITTEST ::proto2_unittest_eagerly_verified_lazy
#define UNITTEST_IMPORT ::proto2_unittest_import_eagerly_verified_lazy

// Must include after the above macros.
// clang-format off
#include "google/protobuf/message_unittest.inc"
#include "google/protobuf/message_unittest_legacy_apis.inc"
// clang-format on

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::google::protobuf::contrib::parse_proto::ParseTextProtoOrDie;
using UNITTEST::NestedTestAllTypes;
using UNITTEST::TestAllTypes;
using UNITTEST::TestChildExtension;
using UNITTEST::TestEagerMaybeLazy;
using UNITTEST::TestMessageSetExtension1;
using UNITTEST::TestNestedChildExtension;
using UNITTEST::TestVerify;

const LazyField* GetLazyField(const Message& message, absl::string_view name) {
  const Descriptor* desc = message.GetDescriptor();
  const FieldDescriptor* field = desc->FindFieldByName(name);
  const Reflection* reflection = message.GetReflection();
  return reflection->GetLazyField(message, field);
}

TEST(MESSAGE_TEST_NAME, ForcedLazyMustBeEffective) {
  TestAllTypes p;
  TestUtil::SetAllFields(&p);

  int lazy_field_count = 0;
  ReflectionVisit::VisitFields(
      p,
      [&](auto info) {
        if constexpr (!info.is_map) {
          if constexpr (info.cpp_type == FieldDescriptor::CPPTYPE_MESSAGE &&
                        !info.is_repeated && !info.is_extension) {
            EXPECT_TRUE(info.field->type() == FieldDescriptor::TYPE_GROUP ||
                        info.is_lazy)
                << info.field->full_name();

            if (info.is_lazy) ++lazy_field_count;
          }
        }
      },
      FieldMask::kMessage);
  EXPECT_EQ(lazy_field_count, 6);
}

TEST(MESSAGE_TEST_NAME, DynamicMessage) {
  NestedTestAllTypes o;
  TestUtil::SetAllFields(o.mutable_payload());

  DynamicMessageFactory factory;
  std::unique_ptr<Message> dynamic_message;
  dynamic_message.reset(factory.GetPrototype(o.GetDescriptor())->New());
  dynamic_message->ParseFromString(o.SerializeAsCord());

  const FieldDescriptor* payload_field =
      dynamic_message->GetDescriptor()->FindFieldByName("payload");
  const auto& payload = dynamic_message->GetReflection()->GetMessage(
      *dynamic_message, payload_field);

  TestUtil::ReflectionTester reflection_tester(payload_field->message_type());
  reflection_tester.ExpectAllFieldsSetViaReflection(payload);
}

TEST(MESSAGE_TEST_NAME, ParseProto3Submessage) {
  UNITTEST::TestProto3SubmessageTestData o, p;

  auto* d = o.mutable_child()->mutable_proto3_import_message();
  d->set_optional_value("100");
  d->add_repeated_value("201");
  d->add_repeated_value("202");

  auto serialized = o.SerializeAsCord();

  // No UTF-8 validation for byte fields.
  EXPECT_TRUE(p.ParseFromString(serialized));

  UNITTEST::TestProto3Submessage q;
  // Must pass UTF-8 validation.
  EXPECT_TRUE(q.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, ParseProto3SubmessageBadUtf8) {
  UNITTEST::TestProto3SubmessageTestData o, p;
  constexpr int kDepth = 5;
  TestUtil::SetAllFields(o.mutable_proto2_all_types());
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  TestUtil::SetAllFields(child->mutable_proto2_all_types());
  auto* d = child->mutable_proto3_import_message();

  // "\xFF" is invalid UTF-8 encoding.
  std::vector<std::string> data = {
      "\xFF",  "hello", "hello", "hello", "hello", "hello",
      "hello", "hello", "hello", "hello", "hello", "hello",
  };
  absl::BitGen bitgen;
  std::shuffle(data.begin(), data.end(), bitgen);

  // One of the following mutation fails UTF-8 validation.
  d->set_optional_value(data[0]);
  d->set_optional_string_piece(data[1]);
  d->set_optional_cord(data[2]);
  d->add_repeated_value(data[3]);
  d->add_repeated_value(data[4]);
  d->add_repeated_string_piece(data[5]);
  d->add_repeated_string_piece(data[6]);
  d->add_repeated_cord(data[7]);
  d->add_repeated_cord(data[8]);
  d->set_oneof_value(data[9]);
  auto* mv = d->add_map_value();
  mv->set_key(data[10]);
  mv->set_value(data[11]);

  auto serialized = o.SerializeAsCord();

  // No UTF-8 validation for byte fields.
  EXPECT_TRUE(p.ParseFromString(serialized));

  UNITTEST::TestProto3Submessage q;
  // Must fail UTF-8 validation.
  EXPECT_FALSE(q.ParseFromString(serialized));
}

constexpr char kChildTag = static_cast<char>(
    WireFormatLite::MakeTag(1, WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
constexpr char kPayloadTag = static_cast<char>(
    WireFormatLite::MakeTag(2, WireFormatLite::WIRETYPE_LENGTH_DELIMITED));
constexpr char kInt32Tag = static_cast<char>(
    WireFormatLite::MakeTag(1, WireFormatLite::WIRETYPE_VARINT));
constexpr char kUint32Tag = static_cast<char>(
    WireFormatLite::MakeTag(3, WireFormatLite::WIRETYPE_VARINT));

TEST(MESSAGE_TEST_NAME, NegativeInt32) {
  const char encoded[] = {
      kChildTag, 10,   kChildTag, 8,    kPayloadTag, 6,
      kInt32Tag, 0xFF, 0xFF,      0xFF, 0xFF,        0x0F,
  };

  const char expected[] = {
      kChildTag, 15,   kChildTag, 13,   kPayloadTag, 11,
      kInt32Tag, 0xFF, 0xFF,      0xFF, 0xFF,        0xFF,
      0xFF,      0xFF, 0xFF,      0xFF, 0x01,
  };

  NestedTestAllTypes p;
  ASSERT_TRUE(p.ParseFromString(absl::string_view(encoded, sizeof(encoded))));
  EXPECT_EQ(p.SerializeAsString(),
            absl::string_view(expected, sizeof(expected)));
}

TEST(MESSAGE_TEST_NAME, NegativeUint32) {
  const char expected[] = {
      kChildTag,  10,   kChildTag, 8,    kPayloadTag, 6,
      kUint32Tag, 0xFF, 0xFF,      0xFF, 0xFF,        0x0F,
  };

  NestedTestAllTypes p;
  p.mutable_child()->mutable_child()->mutable_payload()->set_optional_uint32(
      -1);
  EXPECT_EQ(p.SerializeAsString(),
            absl::string_view(expected, sizeof(expected)));
}

template <typename Proto>
bool IsMsgUnparsed(const Proto& p) {
  const FieldDescriptor* field =
      p.GetDescriptor()->FindFieldByName("optional_nested");
  const LazyField* lazy = p.GetReflection()->GetLazyField(p, field);
  return lazy->HasUnparsed();
}

TEST(MESSAGE_TEST_NAME, Uint32) {
  TestVerify original, parsed;
  TestVerify::Nested* nested = original.mutable_optional_nested();
  nested->set_optional_uint32_1(-1);
  nested->add_packed_uint32_2(-1);
  nested->add_packed_uint32_2(-1);
  nested->set_optional_uint32_4(-1);
  nested->set_optional_uint32_5(-1);
  nested->set_optional_uint32_7(-1);
  nested->set_optional_uint32_8(-1);
  nested->set_required_uint32_30(-1);
  nested->set_required_uint32_63(-1);
  ASSERT_TRUE(
      parsed.ParsePartialFromString(original.SerializePartialAsString()));

  EXPECT_TRUE(IsMsgUnparsed(parsed));
}

TEST(MESSAGE_TEST_NAME, Uint32Packed) {
  TestVerify original, parsed;
  TestVerify::Nested* nested = original.mutable_optional_nested();
  nested->add_packed_uint32_2(-1);
  nested->add_packed_uint32_2(-1);
  ASSERT_TRUE(
      parsed.ParsePartialFromString(original.SerializePartialAsString()));

  EXPECT_TRUE(IsMsgUnparsed(parsed));
}

TEST(MESSAGE_TEST_NAME, Int32LargeFieldNumber) {
  auto make_message = []() {
    UNITTEST::TestVerifyBigFieldNumberUint32 message;
    UNITTEST::TestVerifyBigFieldNumberUint32::Nested* nested =
        message.mutable_optional_nested();
    nested->set_optional_uint32_2(-1);
    nested->set_optional_uint32_63(-1);
    return message;
  };

  {
    UNITTEST::TestVerifyBigFieldNumberUint32 original = make_message();
    original.mutable_optional_nested()->set_optional_uint32_1(-1);
    UNITTEST::TestVerifyBigFieldNumber parsed;
    ASSERT_TRUE(parsed.ParseFromString(original.SerializeAsString()));

    EXPECT_FALSE(IsMsgUnparsed(parsed));
  }
  {
    UNITTEST::TestVerifyBigFieldNumberUint32 original = make_message();
    original.mutable_optional_nested()->set_optional_uint32_65(-1);
    UNITTEST::TestVerifyBigFieldNumber parsed;
    ASSERT_TRUE(parsed.ParseFromString(original.SerializeAsString()));

    EXPECT_FALSE(IsMsgUnparsed(parsed));
  }
  {
    UNITTEST::TestVerifyBigFieldNumberUint32 original = make_message();
    original.mutable_optional_nested()->set_optional_uint32_1000(-1);
    UNITTEST::TestVerifyBigFieldNumber parsed;
    ASSERT_TRUE(parsed.ParseFromString(original.SerializeAsString()));

    EXPECT_FALSE(IsMsgUnparsed(parsed));
  }
  {
    UNITTEST::TestVerifyBigFieldNumberUint32 original = make_message();
    original.mutable_optional_nested()->set_optional_uint32_5000(-1);
    UNITTEST::TestVerifyBigFieldNumber parsed;
    ASSERT_TRUE(parsed.ParseFromString(original.SerializeAsString()));

    EXPECT_FALSE(IsMsgUnparsed(parsed));
  }
}

TEST(MESSAGE_TEST_NAME, MissingRequired) {
  TestVerify original, parsed;
  TestVerify::Nested* nested = original.mutable_optional_nested();
  nested->set_required_uint32_30(1);
  nested->set_required_uint32_63(1);

  EXPECT_FALSE(parsed.ParseFromString(original.SerializePartialAsString()));
  EXPECT_FALSE(IsMsgUnparsed(parsed));
}

TEST(MESSAGE_TEST_NAME, NotMissingRequired) {
  TestVerify original, parsed, direct_parsed;
  TestVerify::Nested* nested = original.mutable_optional_nested();
  nested->set_required_uint32_30(1);
  nested->set_required_uint32_63(1);
  nested->set_required_int32_64(1);
  std::string data = original.SerializePartialAsString();

  ASSERT_TRUE(parsed.ParseFromString(data));

  const char* ptr;
  internal::ParseContext ctx(100, false, &ptr, data);
  ctx.SetCheckRequiredFields(true);
  (void)ctx.set_lazy_parse_mode(
      internal::ParseContext::LazyParseMode::kEagerVerify);
  ptr = direct_parsed._InternalParse(ptr, &ctx);

  EXPECT_NE(ptr, nullptr);
  EXPECT_TRUE(ctx.EndedAtLimit());
  EXPECT_FALSE(ctx.missing_required_fields());
}

TEST(MESSAGE_TEST_NAME, IsInitializedWithoutParsing) {
  TestVerify original, parsed;
  TestVerify::Nested* nested = original.mutable_optional_nested();
  nested->set_required_uint32_30(1);
  nested->set_required_uint32_63(1);
  nested->set_required_int32_64(1);
  std::string data = original.SerializePartialAsString();

  ASSERT_TRUE(parsed.ParsePartialFromString(data));
  ASSERT_TRUE(parsed.IsInitialized());

  EXPECT_TRUE(IsMsgUnparsed(parsed));
}

TEST(MESSAGE_TEST_NAME, ExplicitLazyExceedRecursionLimit) {
  UNITTEST::NestedTestAllTypes original, parsed;
  original.mutable_lazy_child()
      ->mutable_child()
      ->mutable_payload()
      ->set_optional_int32(-1);
  std::string serialized;
  ASSERT_TRUE(original.SerializeToString(&serialized));

  io::ArrayInputStream array_stream(serialized.data(), serialized.size());
  io::CodedInputStream input_stream(&array_stream);
  input_stream.SetRecursionLimit(2);
  EXPECT_FALSE(parsed.ParseFromCodedStream(&input_stream));

  EXPECT_NE(parsed.lazy_child().child().payload().optional_int32(), -1);
}

TEST(MESSAGE_TEST_NAME, ExplicitLazyBadLengthDelimitedSize) {
  std::string serialized;
  uint32_t tag = internal::WireFormatLite::MakeTag(
      1, internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
  ASSERT_LT(tag, INT8_MAX);
  serialized.push_back(tag);
  serialized.push_back(6);
  serialized.push_back(tag);
  serialized.append(5, 0xff);

  UNITTEST::TestLazyMessage parsed;
  EXPECT_FALSE(parsed.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, NestedLazyRecursionLimit) {
  UNITTEST::NestedTestAllTypes original, parsed;
  original.mutable_lazy_child()
      ->mutable_lazy_child()
      ->mutable_lazy_child()
      ->mutable_payload()
      ->set_optional_int32(-1);
  std::string serialized;
  ASSERT_TRUE(original.SerializeToString(&serialized));
  ASSERT_TRUE(parsed.ParseFromString(serialized));

  io::ArrayInputStream array_stream(serialized.data(), serialized.size());
  io::CodedInputStream input_stream(&array_stream);
  input_stream.SetRecursionLimit(2);
  EXPECT_FALSE(parsed.ParseFromCodedStream(&input_stream));
  EXPECT_TRUE(parsed.has_lazy_child());
  EXPECT_TRUE(parsed.lazy_child().has_lazy_child());
  EXPECT_TRUE(parsed.lazy_child().lazy_child().has_lazy_child());
  EXPECT_FALSE(parsed.lazy_child().lazy_child().lazy_child().has_payload());
}

TEST(MESSAGE_TEST_NAME, UnparsedEmpty) {
  const char encoded[] = {'\042', 100};
  UNITTEST::NestedTestAllTypes message;

  EXPECT_FALSE(
      message.ParseFromString(absl::string_view(encoded, sizeof(encoded))));
  EXPECT_TRUE(message.has_lazy_child());
  EXPECT_EQ(message.lazy_child().ByteSizeLong(), 0);
}

TEST(MESSAGE_TEST_NAME, DefaultInstanceByteSizeLong) {
  EXPECT_EQ(UNITTEST::NestedTestAllTypes::default_instance().ByteSizeLong(), 0);
  EXPECT_EQ(UNITTEST::NestedTestAllTypes::default_instance().GetCachedSize(),
            0);
}

TEST(MESSAGE_TEST_NAME, ParseFailNonCanonicalZeroTag) {
  const char encoded[] = {"\n\x3\x80\0\0"};
  UNITTEST::NestedTestAllTypes parsed;
  EXPECT_FALSE(parsed.ParsePartialFromString(
      absl::string_view{encoded, sizeof(encoded) - 1}));
}

TEST(MESSAGE_TEST_NAME, ParseFailNonCanonicalZeroField) {
  const char encoded[] = {"\012\x6\205\0\0\0\0\0"};
  UNITTEST::NestedTestAllTypes parsed;
  EXPECT_FALSE(parsed.ParsePartialFromString(
      absl::string_view{encoded, sizeof(encoded) - 1}));
}

TEST(MESSAGE_TEST_NAME, NestedExplicitLazyExceedRecursionLimit) {
  UNITTEST::NestedTestAllTypes original, parsed;
  original.mutable_lazy_child()
      ->mutable_child()
      ->mutable_lazy_child()
      ->mutable_child()
      ->mutable_payload()
      ->set_optional_int32(-1);
  std::string serialized;
  EXPECT_TRUE(original.SerializeToString(&serialized));

  io::ArrayInputStream array_stream(serialized.data(), serialized.size());
  io::CodedInputStream input_stream(&array_stream);
  input_stream.SetRecursionLimit(4);
  EXPECT_FALSE(parsed.ParseFromCodedStream(&input_stream));

  EXPECT_NE(parsed.lazy_child()
                .child()
                .lazy_child()
                .child()
                .payload()
                .optional_int32(),
            -1);
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfSubmessageTruncated) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 5;
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  TestUtil::SetAllFields(child->mutable_payload());

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  EXPECT_TRUE(p.ParseFromString(serialized));

  constexpr int kMaxTruncate = 50;
  ASSERT_GT(serialized.size(), kMaxTruncate);

  for (int i = 1; i < kMaxTruncate; i += 3) {
    EXPECT_FALSE(
        p.ParseFromString(serialized.substr(0, serialized.size() - i)));
  }
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfWireMalformed) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 5;
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  child->mutable_payload()->set_optional_int32(-1);

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  EXPECT_TRUE(p.ParseFromString(serialized));

  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfOneofWireMalformed) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 5;
  auto* child = o.mutable_child();
  for (int i = 0; i < kDepth; i++) {
    child = child->mutable_child();
  }
  child->mutable_payload()->mutable_oneof_nested_message()->set_bb(-1);

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  EXPECT_TRUE(p.ParseFromString(serialized));

  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfExtensionWireMalformed) {
  UNITTEST::TestChildExtension o, p;
  auto* m = o.mutable_optional_extension()->MutableExtension(
      UNITTEST::optional_nested_message_extension);

  m->set_bb(-1);

  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  EXPECT_TRUE(p.ParseFromString(serialized));

  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfGroupFieldMalformed) {
  UNITTEST::TestMutualRecursionA original, parsed;
  original.mutable_bb()
      ->mutable_a()
      ->mutable_subgroup()
      ->mutable_sub_message()
      ->mutable_b()
      ->set_optional_int32(-1);

  std::string data;
  ASSERT_TRUE(original.SerializeToString(&data));
  ASSERT_TRUE(parsed.ParseFromString(data));
  data[data.size() - 2] = 0xFF;

  EXPECT_FALSE(parsed.ParseFromString(data));
}

TEST(MESSAGE_TEST_NAME, ParseFailsIfRepeatedGroupFieldMalformed) {
  UNITTEST::TestMutualRecursionA original, parsed;
  original.mutable_bb()
      ->mutable_a()
      ->add_subgroupr()
      ->mutable_payload()
      ->set_optional_int64(-1);

  std::string data;
  ASSERT_TRUE(original.SerializeToString(&data));
  ASSERT_TRUE(parsed.ParseFromString(data));
  data[data.size() - 2] = 0xFF;

  EXPECT_FALSE(parsed.ParseFromString(data));
}

TEST(MESSAGE_TEST_NAME, UninitializedAndMalformed) {
  UNITTEST::TestRequiredForeign o, p1, p2;
  o.mutable_optional_message()->set_a(-1);

  std::string serialized;
  EXPECT_TRUE(o.SerializePartialToString(&serialized));

  EXPECT_TRUE(p1.ParsePartialFromString(serialized));
  EXPECT_FALSE(p1.IsInitialized());

  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p2.ParseFromString(serialized));
  EXPECT_FALSE(p2.IsInitialized());
}

TEST(MESSAGE_TEST_NAME, ParseStrictlyBoundedStream) {
  UNITTEST::NestedTestAllTypes o, p;
  constexpr int kDepth = 2;
  o = InitNestedProto(kDepth);
  TestUtil::SetAllFields(o.mutable_child()->mutable_payload());
  o.mutable_child()->mutable_child()->mutable_payload()->set_optional_string(
      std::string(1024, 'a'));

  std::string data;
  EXPECT_TRUE(o.SerializeToString(&data));

  TestUtil::BoundedArrayInputStream stream(data.data(), data.size());
  EXPECT_TRUE(p.ParseFromBoundedZeroCopyStream(&stream, data.size()));
  TestUtil::ExpectAllFieldsSet(p.child().payload());
}

void TouchLazy(UNITTEST::NestedTestAllTypes* msg);
void TouchLazy(UNITTEST::TestAllTypes* msg);
void TouchLazy(UNITTEST::TestAllTypes::NestedMessage* msg) {}

void TouchLazy(UNITTEST::TestAllTypes* msg) {
  if (msg->has_optional_lazy_message()) {
    TouchLazy(msg->mutable_optional_lazy_message());
  }
  if (msg->has_optional_unverified_lazy_message()) {
    TouchLazy(msg->mutable_optional_unverified_lazy_message());
  }
  for (auto& child : *msg->mutable_repeated_lazy_message()) {
    TouchLazy(&child);
  }
}

void TouchLazy(UNITTEST::NestedTestAllTypes* msg) {
  if (msg->has_child()) TouchLazy(msg->mutable_child());
  if (msg->has_payload()) TouchLazy(msg->mutable_payload());
  for (auto& child : *msg->mutable_repeated_child()) {
    TouchLazy(&child);
  }
  if (msg->has_lazy_child()) TouchLazy(msg->mutable_lazy_child());
  if (msg->has_eager_child()) TouchLazy(msg->mutable_eager_child());
}

TEST(MESSAGE_TEST_NAME, SuccessAfterParsingFailure) {
  UNITTEST::NestedTestAllTypes o, p, q;
  constexpr int kDepth = 5;
  o = InitNestedProto(kDepth);
  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  EXPECT_TRUE(p.ParseFromString(serialized));

  serialized[serialized.size() - 1] = 0xFF;
  EXPECT_FALSE(p.ParseFromString(serialized));

  TouchLazy(&p);
  EXPECT_TRUE(q.ParseFromString(p.SerializeAsString()));
}

TEST(MESSAGE_TEST_NAME, ExceedRecursionLimit) {
  UNITTEST::NestedTestAllTypes o, p;
  const int kDepth = io::CodedInputStream::GetDefaultRecursionLimit() + 10;
  o = InitNestedProto(kDepth);
  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  EXPECT_FALSE(p.ParseFromString(serialized));
}

TEST(MESSAGE_TEST_NAME, SupportCustomRecursionLimitRead) {
  UNITTEST::NestedTestAllTypes o, p;
  const int kDepth = io::CodedInputStream::GetDefaultRecursionLimit() + 10;
  o = InitNestedProto(kDepth);
  std::string serialized;
  EXPECT_TRUE(o.SerializeToString(&serialized));

  io::ArrayInputStream raw_input(serialized.data(), serialized.size());
  io::CodedInputStream input(&raw_input);
  input.SetRecursionLimit(kDepth + 10);
  EXPECT_TRUE(p.ParseFromCodedStream(&input));

  EXPECT_EQ(p.child().payload().optional_int32(), 0);
  EXPECT_EQ(p.child().child().payload().optional_int32(), 1);

  std::string result;
  EXPECT_TRUE(p.SerializeToString(&result));
}
}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
