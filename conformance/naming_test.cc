#include "naming.h"

#include <gtest/gtest.h>
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using protobuf_test_messages::editions::TestAllTypesEdition2023;
using protobuf_test_messages::proto2::TestAllTypesProto2;
using protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    protobuf_test_messages::editions::proto3::TestAllTypesProto3;

TEST(NamingTest, GetEditionIdentifier) {
  EXPECT_EQ(GetEditionIdentifier(*TestAllTypesProto2::descriptor()), "Proto2");
  EXPECT_EQ(GetEditionIdentifier(*TestAllTypesProto3::descriptor()), "Proto3");
  EXPECT_EQ(GetEditionIdentifier(*TestAllTypesEdition2023::descriptor()),
            "Editions");
  EXPECT_EQ(GetEditionIdentifier(*TestAllTypesProto2Editions::descriptor()),
            "Editions_Proto2");
  EXPECT_EQ(GetEditionIdentifier(*TestAllTypesProto3Editions::descriptor()),
            "Editions_Proto3");
}

TEST(NamingTest, GetFormatIdentifier) {
  EXPECT_EQ(GetFormatIdentifier(::conformance::PROTOBUF), "Protobuf");
  EXPECT_EQ(GetFormatIdentifier(::conformance::JSON), "Json");
  EXPECT_EQ(GetFormatIdentifier(::conformance::TEXT_FORMAT), "TextFormat");
  EXPECT_DEATH(GetFormatIdentifier(::conformance::JSPB), "Unknown wire format");
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
