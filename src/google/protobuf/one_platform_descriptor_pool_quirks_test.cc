// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/one_platform_descriptor_pool_quirks.h"

#include <optional>
#include <string_view>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace {

using ::absl_testing::IsOk;
using ::testing::IsNull;
using ::testing::NotNull;

inline constexpr std::string_view kScopedEnumsFileDescriptorName =
    "one_platform_descriptor_pool_quirks.proto";
inline constexpr std::string_view kScopedEnumsFileDescriptorPackage =
    "one_platform_descriptor_pool_quirks";

google::protobuf::FileDescriptorProto GetScopedEnumsFileDescriptor(
    std::string_view syntax, std::optional<google::protobuf::Edition> edition) {
  google::protobuf::FileDescriptorProto file_descriptor_proto;
  file_descriptor_proto.set_name(kScopedEnumsFileDescriptorName);
  file_descriptor_proto.set_package(kScopedEnumsFileDescriptorPackage);
  file_descriptor_proto.set_syntax(syntax);
  if (edition.has_value()) {
    file_descriptor_proto.set_edition(*edition);
  }
  auto* enum_descriptor1 = file_descriptor_proto.add_enum_type();
  enum_descriptor1->set_name("Enum1");
  {
    auto* enum_value_descriptor1 = enum_descriptor1->add_value();
    enum_value_descriptor1->set_name("FOO");
    enum_value_descriptor1->set_number(0);
    auto* enum_value_descriptor2 = enum_descriptor1->add_value();
    enum_value_descriptor2->set_name("UNIQUE1_FOO");
    enum_value_descriptor2->set_number(1);
  }
  auto* enum_descriptor2 = file_descriptor_proto.add_enum_type();
  enum_descriptor2->set_name("Enum2");
  {
    auto* enum_value_descriptor1 = enum_descriptor2->add_value();
    enum_value_descriptor1->set_name("UNIQUE2_FOO");
    enum_value_descriptor1->set_number(0);
    auto* enum_value_descriptor2 = enum_descriptor2->add_value();
    enum_value_descriptor2->set_name("FOO");
    enum_value_descriptor2->set_number(1);
  }
  auto* message_descriptor = file_descriptor_proto.add_message_type();
  message_descriptor->set_name("Message");
  *message_descriptor->add_enum_type() = *enum_descriptor1;
  *message_descriptor->add_enum_type() = *enum_descriptor2;
  return file_descriptor_proto;
}

template <typename T>
void CheckFindEnumValueByNameAbsent(const T* parent) {
  EXPECT_THAT(parent->FindEnumValueByName("FOO"), IsNull());
  EXPECT_THAT(parent->FindEnumValueByName("UNIQUE1_FOO"), IsNull());
  EXPECT_THAT(parent->FindEnumValueByName("UNIQUE2_FOO"), IsNull());
}

void CheckFindEnumValueByNameAbsent(const google::protobuf::DescriptorPool* parent) {
  EXPECT_THAT(parent->FindEnumValueByName(
                  absl::StrCat(kScopedEnumsFileDescriptorPackage, ".FOO")),
              IsNull());
  EXPECT_THAT(parent->FindEnumValueByName(absl::StrCat(
                  kScopedEnumsFileDescriptorPackage, ".UNIQUE1_FOO")),
              IsNull());
  EXPECT_THAT(parent->FindEnumValueByName(absl::StrCat(
                  kScopedEnumsFileDescriptorPackage, ".UNIQUE2_FOO")),
              IsNull());
  EXPECT_THAT(parent->FindEnumValueByName(absl::StrCat(
                  kScopedEnumsFileDescriptorPackage, ".Message.FOO")),
              IsNull());
  EXPECT_THAT(parent->FindEnumValueByName(absl::StrCat(
                  kScopedEnumsFileDescriptorPackage, ".Message.UNIQUE1_FOO")),
              IsNull());
  EXPECT_THAT(parent->FindEnumValueByName(absl::StrCat(
                  kScopedEnumsFileDescriptorPackage, ".Message.UNIQUE2_FOO")),
              IsNull());
}

template <typename T>
void CheckFindEnumValueByNamePresent(const T* parent) {
  const auto* enum1_descriptor = parent->FindEnumTypeByName("Enum1");
  ASSERT_THAT(enum1_descriptor, NotNull());
  const auto* enum2_descriptor = parent->FindEnumTypeByName("Enum2");
  ASSERT_THAT(enum2_descriptor, NotNull());

  const google::protobuf::EnumValueDescriptor* enum_value_descriptor;

  enum_value_descriptor = enum1_descriptor->FindValueByName("FOO");
  ASSERT_THAT(enum_value_descriptor, NotNull());
  EXPECT_EQ(enum_value_descriptor->name(), "FOO");
  EXPECT_EQ(enum_value_descriptor->number(), 0);

  enum_value_descriptor = enum1_descriptor->FindValueByName("UNIQUE1_FOO");
  ASSERT_THAT(enum_value_descriptor, NotNull());
  EXPECT_EQ(enum_value_descriptor->name(), "UNIQUE1_FOO");
  EXPECT_EQ(enum_value_descriptor->number(), 1);

  enum_value_descriptor = enum2_descriptor->FindValueByName("UNIQUE2_FOO");
  ASSERT_THAT(enum_value_descriptor, NotNull());
  EXPECT_EQ(enum_value_descriptor->name(), "UNIQUE2_FOO");
  EXPECT_EQ(enum_value_descriptor->number(), 0);

  enum_value_descriptor = enum2_descriptor->FindValueByName("FOO");
  ASSERT_THAT(enum_value_descriptor, NotNull());
  EXPECT_EQ(enum_value_descriptor->name(), "FOO");
  EXPECT_EQ(enum_value_descriptor->number(), 1);
}

TEST(OnePlatformDescriptorPoolQuirks, ScopedEnumsProto2) {
  google::protobuf::DescriptorPool descriptor_pool;
  ASSERT_THAT(OnePlatformDescriptorPoolQuirks::Enable(descriptor_pool), IsOk());
  ASSERT_THAT(descriptor_pool.BuildFile(
                  GetScopedEnumsFileDescriptor("proto2", std::nullopt)),
              NotNull());
  const auto* file_descriptor =
      descriptor_pool.FindFileByName(kScopedEnumsFileDescriptorName);
  ASSERT_THAT(file_descriptor, NotNull());
  const auto* message_descriptor = descriptor_pool.FindMessageTypeByName(
      absl::StrCat(kScopedEnumsFileDescriptorPackage, ".Message"));
  ASSERT_THAT(message_descriptor, NotNull());

  // OnePlatformDescriptorPoolQuirks disables lookup for enum values by name
  // everywhere except for EnumDescriptor::FindValueByName.
  CheckFindEnumValueByNameAbsent(&descriptor_pool);
  CheckFindEnumValueByNameAbsent(file_descriptor);
  CheckFindEnumValueByNameAbsent(message_descriptor);
  CheckFindEnumValueByNamePresent(file_descriptor);
  CheckFindEnumValueByNamePresent(message_descriptor);
}

TEST(OnePlatformDescriptorPoolQuirks, ScopedEnumsProto3) {
  google::protobuf::DescriptorPool descriptor_pool;
  ASSERT_THAT(OnePlatformDescriptorPoolQuirks::Enable(descriptor_pool), IsOk());
  ASSERT_THAT(descriptor_pool.BuildFile(
                  GetScopedEnumsFileDescriptor("proto3", std::nullopt)),
              NotNull());
  const auto* file_descriptor =
      descriptor_pool.FindFileByName(kScopedEnumsFileDescriptorName);
  ASSERT_THAT(file_descriptor, NotNull());
  const auto* message_descriptor = descriptor_pool.FindMessageTypeByName(
      absl::StrCat(kScopedEnumsFileDescriptorPackage, ".Message"));
  ASSERT_THAT(message_descriptor, NotNull());

  // OnePlatformDescriptorPoolQuirks disables lookup for enum values by name
  // everywhere except for EnumDescriptor::FindValueByName.
  CheckFindEnumValueByNameAbsent(&descriptor_pool);
  CheckFindEnumValueByNameAbsent(file_descriptor);
  CheckFindEnumValueByNameAbsent(message_descriptor);
  CheckFindEnumValueByNamePresent(file_descriptor);
  CheckFindEnumValueByNamePresent(message_descriptor);
}

TEST(OnePlatformDescriptorPoolQuirks, ScopedEnumsProtoEditions) {
  google::protobuf::DescriptorPool descriptor_pool;
  ASSERT_THAT(OnePlatformDescriptorPoolQuirks::Enable(descriptor_pool), IsOk());
  ASSERT_THAT(descriptor_pool.BuildFile(GetScopedEnumsFileDescriptor(
                  "editions", google::protobuf::EDITION_2023)),
              NotNull());
  const auto* file_descriptor =
      descriptor_pool.FindFileByName(kScopedEnumsFileDescriptorName);
  ASSERT_THAT(file_descriptor, NotNull());
  const auto* message_descriptor = descriptor_pool.FindMessageTypeByName(
      absl::StrCat(kScopedEnumsFileDescriptorPackage, ".Message"));
  ASSERT_THAT(message_descriptor, NotNull());

  // OnePlatformDescriptorPoolQuirks disables lookup for enum values by name
  // everywhere except for EnumDescriptor::FindValueByName.
  CheckFindEnumValueByNameAbsent(&descriptor_pool);
  CheckFindEnumValueByNameAbsent(file_descriptor);
  CheckFindEnumValueByNameAbsent(message_descriptor);
  CheckFindEnumValueByNamePresent(file_descriptor);
  CheckFindEnumValueByNamePresent(message_descriptor);
}

}  // namespace
}  // namespace protobuf
}  // namespace google
