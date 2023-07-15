// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#include "google/protobuf/descriptor_visitor.h"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {
namespace {

using ::testing::Contains;
using ::testing::IsSupersetOf;
using ::testing::Not;

constexpr absl::string_view kUnittestProtoFile =
    "google/protobuf/unittest.proto";

TEST(VisitDescriptorsTest, SingleTypeNoProto) {
  const FileDescriptor& file =
      *protobuf_unittest::TestAllTypes::GetDescriptor()->file();
  std::vector<std::string> descriptors;
  VisitDescriptors(file, [&](const Descriptor& descriptor) {
    descriptors.push_back(descriptor.full_name());
  });
  EXPECT_THAT(descriptors,
              IsSupersetOf({"protobuf_unittest.TestAllTypes",
                            "protobuf_unittest.TestAllTypes.NestedMessage"}));
}

TEST(VisitDescriptorsTest, SingleTypeWithProto) {
  const FileDescriptor& file =
      *protobuf_unittest::TestAllTypes::GetDescriptor()->file();
  FileDescriptorProto proto;
  file.CopyTo(&proto);
  std::vector<std::string> descriptors;
  VisitDescriptors(
      file, proto,
      [&](const Descriptor& descriptor, const DescriptorProto& proto) {
        descriptors.push_back(descriptor.full_name());
        EXPECT_EQ(descriptor.name(), proto.name());
      });
  EXPECT_THAT(descriptors,
              IsSupersetOf({"protobuf_unittest.TestAllTypes",
                            "protobuf_unittest.TestAllTypes.NestedMessage"}));
}

TEST(VisitDescriptorsTest, SingleTypeMutableProto) {
  const FileDescriptor& file =
      *protobuf_unittest::TestAllTypes::GetDescriptor()->file();
  FileDescriptorProto proto;
  file.CopyTo(&proto);
  std::vector<std::string> descriptors;
  VisitDescriptors(file, proto,
                   [&](const Descriptor& descriptor, DescriptorProto& proto) {
                     descriptors.push_back(descriptor.full_name());
                     EXPECT_EQ(descriptor.name(), proto.name());
                     proto.set_name("<redacted>");
                   });
  EXPECT_THAT(descriptors,
              IsSupersetOf({"protobuf_unittest.TestAllTypes",
                            "protobuf_unittest.TestAllTypes.NestedMessage"}));
  EXPECT_EQ(proto.message_type(0).name(), "<redacted>");
}

TEST(VisitDescriptorsTest, AllTypesDeduce) {
  const FileDescriptor& file =
      *protobuf_unittest::TestAllTypes::GetDescriptor()->file();
  std::vector<std::string> descriptors;
  VisitDescriptors(file, [&](const auto& descriptor) {
    descriptors.push_back(descriptor.name());
  });
  EXPECT_THAT(descriptors, Contains(kUnittestProtoFile));
  EXPECT_THAT(descriptors, IsSupersetOf({"TestAllTypes", "TestSparseEnum",
                                         "SPARSE_C", "optional_int32",
                                         "oneof_nested_message", "oneof_field",
                                         "optional_nested_message_extension"}));
}

TEST(VisitDescriptorsTest, AllTypesDeduceSelective) {
  const FileDescriptor& file =
      *protobuf_unittest::TestAllTypes::GetDescriptor()->file();
  std::vector<std::string> descriptors;
  VisitDescriptors(
      file,
      // Only select on descriptors with a full_name method.
      [&](const auto& descriptor)
          -> std::enable_if_t<
              !std::is_void<decltype(descriptor.full_name())>::value> {
        descriptors.push_back(descriptor.full_name());
      });
  // FileDescriptor doesn't have a full_name method.
  EXPECT_THAT(descriptors, Not(Contains(kUnittestProtoFile)));
  EXPECT_THAT(descriptors,
              IsSupersetOf(
                  {"protobuf_unittest.TestAllTypes",
                   "protobuf_unittest.TestSparseEnum", "protobuf_unittest.SPARSE_C",
                   "protobuf_unittest.TestAllTypes.optional_int32",
                   "protobuf_unittest.TestAllTypes.oneof_nested_message",
                   "protobuf_unittest.TestAllTypes.oneof_field",
                   "protobuf_unittest.optional_nested_message_extension"}));
}

void TestHandle(const Descriptor& message, const DescriptorProto& proto,
                std::vector<std::string>* result) {
  if (result != nullptr) result->push_back(message.full_name());
  EXPECT_EQ(message.name(), proto.name());
}
void TestHandle(const EnumDescriptor& enm, const EnumDescriptorProto& proto,
                std::vector<std::string>* result) {
  if (result != nullptr) result->push_back(enm.full_name());
  EXPECT_EQ(enm.name(), proto.name());
}
TEST(VisitDescriptorsTest, AllTypesDeduceDelegate) {
  const FileDescriptor& file =
      *protobuf_unittest::TestAllTypes::GetDescriptor()->file();
  FileDescriptorProto proto;
  file.CopyTo(&proto);
  std::vector<std::string> descriptors;

  VisitDescriptors(file, proto,
                   [&](const auto& descriptor, const auto& proto)
                       -> decltype(TestHandle(descriptor, proto, nullptr)) {
                     TestHandle(descriptor, proto, &descriptors);
                   });

  EXPECT_THAT(descriptors, IsSupersetOf({"protobuf_unittest.TestAllTypes",
                                         "protobuf_unittest.TestSparseEnum"}));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
