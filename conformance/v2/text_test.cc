#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "conformance/v2/binary_wireformat.h"
#include "conformance/v2/global_test_environment.h"
#include "conformance/v2/matchers.h"
#include "conformance/v2/params.h"
#include "conformance/v2/testee.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"
#include "google/protobuf/test_textproto.h"

namespace google {
namespace protobuf {
namespace {

using TextTest = ConformanceTest;

TEST_F(TextTest, ValidNonMessage) {
  EXPECT_THAT(RequiredTest("ValidNonMessage")
                  .ParseBinary(TestAllTypesEdition2023::descriptor(),
                               Wire(VarintField(1, 99)))
                  .SerializeBinary(),
              ParsedPayload(EqualsProto(R"pb(optional_int32: 99)pb")));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
