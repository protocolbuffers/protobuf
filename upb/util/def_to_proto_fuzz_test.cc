
#include <string>

#include "google/protobuf/descriptor.proto.h"
#include "gtest/gtest.h"
#include "testing/fuzzing/fuzztest.h"
#include "upb/util/def_to_proto_test.h"

namespace upb_test {

FUZZ_TEST(FuzzTest, RoundTripDescriptor)
    .WithDomains(
        ::fuzztest::Arbitrary<google::protobuf::FileDescriptorSet>().WithProtobufField(
            "file",
            ::fuzztest::Arbitrary<google::protobuf::FileDescriptorProto>()
                // upb_FileDef_ToProto() does not attempt to preserve
                // source_code_info.
                .WithFieldUnset("source_code_info")
                .WithProtobufField(
                    "service",
                    ::fuzztest::Arbitrary<google::protobuf::ServiceDescriptorProto>()
                        // streams are google3-only, and we do not currently
                        // attempt to preserve them.
                        .WithFieldUnset("stream"))));

}  // namespace upb_test
