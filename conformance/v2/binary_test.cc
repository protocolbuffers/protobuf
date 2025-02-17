#include <tuple>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/v2/binary_wireformat.h"
#include "conformance/v2/global_test_environment.h"
#include "conformance/v2/matchers.h"
#include "conformance/v2/naming.h"
#include "conformance/v2/params.h"
#include "conformance/v2/testee.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace {

using ::testing::NotNull;

using DelimitedFieldTest = ConformanceTest;

// TODO: maybe switch to a builder pattern for the testee?
// EXPECT_THAT(TestBuilder("ValidNonMessage", Testee::Strictness::kRequired)
//    .ParseBinary(
//        TestAllTypesEdition2023::descriptor(), Wire(VarintField(1, 99))))
//    .SerializeBinary(),
//  EqualsBinary(R"pb(optional_int32: 99)pb"));

TEST_F(DelimitedFieldTest, ValidNonMessage) {
  EXPECT_THAT(RequiredTest("ValidNonMessage")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               Wire(VarintField(1, 99)))
                  .SerializeBinary(),
              ParsedPayload(EqualsProto(R"pb(optional_int32: 99)pb")));
}

TEST_F(DelimitedFieldTest, ValidNonMessage2) {
  EXPECT_THAT(RequiredTest("ValidNonMessage2")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               LengthPrefixedField(18, VarintField(1, 88)))
                  .SerializeBinary(),
              ParsedPayload(EqualsProto(R"pb(optional_int32: 98)pb")));
}

TEST_F(DelimitedFieldTest, ValidNonMessage3) {
  EXPECT_THAT(RequiredTest("ValidNonMessage3")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               LengthPrefixedField(18, VarintField(1, 87)))
                  .SerializeBinary(),
              ParsedPayload(EqualsProto(R"pb(optional_int32: 99)pb")));
}

TEST_F(DelimitedFieldTest, ValidNonMessage4) {
  EXPECT_THAT(RequiredTest("ValidNonMessage4")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               LengthPrefixedField(18, VarintField(1, 666)))
                  .SerializeBinary(),
              ParsedPayload(EqualsProto(R"pb(optional_int32: 99)pb")));
}

TEST_F(DelimitedFieldTest, ValidLengthPrefixedField) {
  EXPECT_THAT(
      RequiredTest("ValidLengthPrefixedField")
          .ParseBinary(TestAllTypesEdition2023::descriptor(),
                       LengthPrefixedField(18, VarintField(1, 99)))
          .SerializeBinary(),
      ParsedPayload(EqualsProto(R"pb(optional_nested_message { a: 99 })pb")));
}

using BinaryTest = ConformanceTest;

TEST_F(BinaryTest, Failing) {
  EXPECT_THAT(RequiredTest("Failing")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               VarintField(1, 99))
                  .SerializeBinary(),
              ParsedPayload(EqualsProto(R"pb(optional_int32: 98)pb")));
}

class LengthDelimitedFieldTest
    : public ConformanceTest,
      public testing::WithParamInterface<std::tuple<
          const Descriptor*, std::pair<FieldDescriptor::Type, int>>> {};

TEST_P(LengthDelimitedFieldTest,
       PrematureEofInDelimitedDataForKnownNonRepeatedValue) {
  const Descriptor* message = std::get<0>(GetParam());
  FieldDescriptor::Type type = std::get<1>(GetParam()).first;
  int field = std::get<1>(GetParam()).second;
  ASSERT_THAT(message->FindFieldByNumber(field), NotNull());
  ASSERT_EQ(message->FindFieldByNumber(field)->type(), type);

  EXPECT_THAT(
      RequiredTest(
          absl::StrCat("PrematureEofInDelimitedDataForKnownNonRepeatedValue.",
                       absl::AsciiStrToUpper(FieldDescriptor::TypeName(type))))
          .ParseBinary(message,
                       Wire(Tag(field, WireType::kLengthPrefixed), Varint(1)))
          .SerializeBinary(),
      IsParseError());
}

INSTANTIATE_TEST_SUITE_P(
    LengthDelimitedFieldTest, LengthDelimitedFieldTest,
    testing::Combine(
        CommonTestDescriptors(),
        testing::Values(std::make_pair(FieldDescriptor::TYPE_MESSAGE, 18),
                        std::make_pair(FieldDescriptor::TYPE_STRING, 14),
                        std::make_pair(FieldDescriptor::TYPE_BYTES, 15))),
    [](const testing::TestParamInfo<LengthDelimitedFieldTest::ParamType>&
           info) {
      return absl::StrCat(
          GetEditionIdentifier(*std::get<0>(info.param)), "_",
          FieldDescriptor::TypeName(std::get<1>(info.param).first));
    });

}  // namespace
}  // namespace protobuf
}  // namespace google
