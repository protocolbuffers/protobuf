// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
      *proto2_unittest::TestAllTypes::GetDescriptor()->file();
  std::vector<absl::string_view> descriptors;
  VisitDescriptors(file, [&](const Descriptor& descriptor) {
    descriptors.push_back(descriptor.full_name());
  });
  EXPECT_THAT(descriptors,
              IsSupersetOf({"proto2_unittest.TestAllTypes",
                            "proto2_unittest.TestAllTypes.NestedMessage"}));
}

TEST(VisitDescriptorsTest, SingleTypeWithProto) {
  const FileDescriptor& file =
      *proto2_unittest::TestAllTypes::GetDescriptor()->file();
  FileDescriptorProto proto;
  file.CopyTo(&proto);
  std::vector<absl::string_view> descriptors;
  VisitDescriptors(
      file, proto,
      [&](const Descriptor& descriptor, const DescriptorProto& proto) {
        descriptors.push_back(descriptor.full_name());
        EXPECT_EQ(descriptor.name(), proto.name());
      });
  EXPECT_THAT(descriptors,
              IsSupersetOf({"proto2_unittest.TestAllTypes",
                            "proto2_unittest.TestAllTypes.NestedMessage"}));
}

TEST(VisitDescriptorsTest, SingleTypeMutableProto) {
  const FileDescriptor& file =
      *proto2_unittest::TestAllTypes::GetDescriptor()->file();
  FileDescriptorProto proto;
  file.CopyTo(&proto);
  std::vector<absl::string_view> descriptors;
  VisitDescriptors(file, proto,
                   [&](const Descriptor& descriptor, DescriptorProto& proto) {
                     descriptors.push_back(descriptor.full_name());
                     EXPECT_EQ(descriptor.name(), proto.name());
                     proto.set_name("<redacted>");
                   });
  EXPECT_THAT(descriptors,
              IsSupersetOf({"proto2_unittest.TestAllTypes",
                            "proto2_unittest.TestAllTypes.NestedMessage"}));
  EXPECT_EQ(proto.message_type(0).name(), "<redacted>");
}

TEST(VisitDescriptorsTest, AllTypesDeduce) {
  const FileDescriptor& file =
      *proto2_unittest::TestAllTypes::GetDescriptor()->file();
  std::vector<absl::string_view> descriptors;
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
      *proto2_unittest::TestAllTypes::GetDescriptor()->file();
  std::vector<absl::string_view> descriptors;
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
                  {"proto2_unittest.TestAllTypes",
                   "proto2_unittest.TestSparseEnum", "proto2_unittest.SPARSE_C",
                   "proto2_unittest.TestAllTypes.optional_int32",
                   "proto2_unittest.TestAllTypes.oneof_nested_message",
                   "proto2_unittest.TestAllTypes.oneof_field",
                   "proto2_unittest.optional_nested_message_extension"}));
}

void TestHandle(const Descriptor& message, const DescriptorProto& proto,
                std::vector<absl::string_view>* result) {
  if (result != nullptr) result->push_back(message.full_name());
  EXPECT_EQ(message.name(), proto.name());
}
void TestHandle(const EnumDescriptor& enm, const EnumDescriptorProto& proto,
                std::vector<absl::string_view>* result) {
  if (result != nullptr) result->push_back(enm.full_name());
  EXPECT_EQ(enm.name(), proto.name());
}
TEST(VisitDescriptorsTest, AllTypesDeduceDelegate) {
  const FileDescriptor& file =
      *proto2_unittest::TestAllTypes::GetDescriptor()->file();
  FileDescriptorProto proto;
  file.CopyTo(&proto);
  std::vector<absl::string_view> descriptors;

  VisitDescriptors(file, proto,
                   [&](const auto& descriptor, const auto& proto)
                       -> decltype(TestHandle(descriptor, proto, nullptr)) {
                     TestHandle(descriptor, proto, &descriptors);
                   });

  EXPECT_THAT(descriptors, IsSupersetOf({"proto2_unittest.TestAllTypes",
                                         "proto2_unittest.TestSparseEnum"}));
}

}  // namespace
}  // namespace protobuf
}  // namespace google
