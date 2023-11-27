// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
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
