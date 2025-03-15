#ifndef GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_PARAMS_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_PARAMS_H__

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "conformance/test_protos/test_messages_edition2023.pb.h"
#include "google/protobuf/descriptor_legacy.h"
#include "editions/golden/test_messages_proto2_editions.pb.h"
#include "editions/golden/test_messages_proto3_editions.pb.h"
#include "google/protobuf/test_messages_proto2.pb.h"
#include "google/protobuf/test_messages_proto3.pb.h"

namespace google {
namespace protobuf {

using protobuf_test_messages::editions::TestAllTypesEdition2023;
using protobuf_test_messages::proto2::TestAllTypesProto2;
using protobuf_test_messages::proto3::TestAllTypesProto3;
using TestAllTypesProto2Editions =
    protobuf_test_messages::editions::proto2::TestAllTypesProto2;
using TestAllTypesProto3Editions =
    protobuf_test_messages::editions::proto3::TestAllTypesProto3;

inline auto NonEditionsDescriptors() {
  return testing::Values(TestAllTypesProto2::descriptor(),
                         TestAllTypesProto3::descriptor());
}

inline auto CommonTestDescriptors() {
  return testing::Values(TestAllTypesProto2::descriptor(),
                         TestAllTypesProto3::descriptor(),
                         TestAllTypesProto2Editions::descriptor(),
                         TestAllTypesProto3Editions::descriptor());
}

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_V2_TEST_PARAMS_H__
