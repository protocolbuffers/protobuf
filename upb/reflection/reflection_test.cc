// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.upb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "google/protobuf/unittest.upbdefs.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.h"
#include "upb/reflection/def.hpp"
#include "upb/reflection/internal/def_pool.h"
#include "upb/test/parse_text_proto.h"

namespace upb_test {
namespace {

using ::testing::HasSubstr;
using ::testing::NotNull;

google_protobuf_FileDescriptorProto* ToUpbDescriptorSet(
    const google::protobuf::FileDescriptorProto& proto, upb::Arena& arena) {
  std::string serialized;
  (void)proto.SerializeToString(&serialized);
  return google_protobuf_FileDescriptorProto_parse(serialized.data(), serialized.size(),
                                          arena.ptr());
}

absl::StatusOr<upb::DefPool> LoadDescriptorSetFromProto(
    const google::protobuf::FileDescriptorSet& set) {
  upb::Arena arena;
  upb::DefPool defpool;
  upb::Status status;
  for (const auto& file : set.file()) {
    google_protobuf_FileDescriptorProto* upb_proto = ToUpbDescriptorSet(file, arena);
    ABSL_CHECK(upb_proto);
    upb::FileDefPtr file_def = defpool.AddFile(upb_proto, &status);
    if (!file_def) return absl::InternalError(status.error_message());
  }
  return defpool;
}

absl::StatusOr<upb::DefPool> LoadDescriptorProto(absl::string_view proto_text) {
  google::protobuf::FileDescriptorProto proto = ParseTextProtoOrDie(proto_text);
  google::protobuf::FileDescriptorSet set;
  *set.add_file() = proto;
  return LoadDescriptorSetFromProto(set);
}

TEST(ReflectionTest, EmptyFileWithoutExtensionsLoads) {
  absl::StatusOr<upb::DefPool> pool = LoadDescriptorProto(
      R"pb(
        syntax: "proto3" name: "empty.proto"
      )pb");
  ASSERT_TRUE(pool.ok()) << pool.status();
}

TEST(ReflectionTest, OpenEnumWithNonZeroDefault) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              syntax: "proto3"
                              name: "F"
                              enum_type {
                                name: "BadEnum"
                                value { name: "v1" number: 1 }
                              }
                            )pb")
                            .status();
  EXPECT_EQ(std::string_view(status.message()),
            "for open enums, the first value must be zero (BadEnum)");
}

TEST(ReflectionTest, FailedAddFileDoesNotRetainExtensions) {
  // Add a valid message with one available extension number.
  google::protobuf::FileDescriptorProto base;
  base.set_name("base.proto");
  base.set_package("base");
  base.set_syntax("proto2");
  auto* target = base.add_message_type();
  target->set_name("Target");
  auto* range = target->add_extension_range();
  range->set_start(100);
  range->set_end(101);

  // This file is rejected because both extensions use that number.
  google::protobuf::FileDescriptorProto duplicate;
  duplicate.set_name("duplicate.proto");
  duplicate.set_package("duplicate");
  duplicate.set_syntax("proto2");
  duplicate.add_dependency("base.proto");
  for (const char* name : {"first", "second"}) {
    auto* extension = duplicate.add_extension();
    extension->set_name(name);
    extension->set_extendee(".base.Target");
    extension->set_number(100);
    extension->set_label(google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL);
    extension->set_type(google::protobuf::FieldDescriptorProto::TYPE_INT32);
  }

  upb::Arena arena;
  upb::DefPool pool;
  upb::Status status;
  ASSERT_TRUE(pool.AddFile(ToUpbDescriptorSet(base, arena), &status));
  status.Clear();
  EXPECT_FALSE(pool.AddFile(ToUpbDescriptorSet(duplicate, arena), &status));

  upb::MessageDefPtr message = pool.FindMessageByName("base.Target");
  ASSERT_TRUE(message);
  // A failed addition must not leave extensions in the pool.
  size_t count;
  const upb_FieldDef** extensions =
      upb_DefPool_GetAllExtensions(pool.ptr(), message.ptr(), &count);
  EXPECT_EQ(count, 0);
  upb_gfree(extensions);
}

TEST(ReflectionTest, EnumDefault) {
  upb::DefPool pool = LoadDescriptorProto(
                          R"pb(
                            syntax: "proto2"
                            name: "F"
                            enum_type {
                              name: "FooEnum"
                              value { name: "v1" number: 1 }
                            }
                          )pb")
                          .value();
  upb::EnumDefPtr e = pool.FindEnumByName("FooEnum");
  EXPECT_EQ(e.default_value(), 1);
}

TEST(ReflectionTest, ImplicitPresenceWithDefault) {
  absl::Status status =
      LoadDescriptorProto(
          R"pb(
            syntax: "editions"
            edition: EDITION_2023
            name: "F"
            message_type {
              name: "FooMessage"
              field {
                name: "f1"
                number: 1
                type: TYPE_INT32
                default_value: "1"
                options { features { field_presence: IMPLICIT } }
              }
            }
          )pb")
          .status();
  EXPECT_EQ(std::string_view(status.message()),
            "fields with implicit presence cannot have explicit defaults "
            "(FooMessage.f1)");
}

TEST(ReflectionTest, RepeatedFieldWithDefault) {
  absl::Status status =
      LoadDescriptorProto(
          R"pb(
            syntax: "proto2"
            name: "F"
            message_type {
              name: "FooMessage"
              field {
                name: "f1"
                number: 1
                label: LABEL_REPEATED
                type: TYPE_STRING
                default_value: "abc"
              }
            }
          )pb")
          .status();
  EXPECT_EQ(std::string_view(status.message()),
            "repeated fields cannot have explicit defaults (FooMessage.f1)");
}

TEST(ReflectionTest, StringAndBytesDefaults) {
  absl::StatusOr<upb::DefPool> pool = LoadDescriptorProto(
      R"pb(
        syntax: "proto2"
        name: "F"
        message_type {
          name: "FooMessage"
          field {
            name: "str"
            number: 1
            label: LABEL_OPTIONAL
            type: TYPE_STRING
            default_value: "anon"
          }
          field {
            name: "bytes_raw"
            number: 2
            label: LABEL_OPTIONAL
            type: TYPE_BYTES
            default_value: "raw"
          }
          field {
            name: "bytes_escaped"
            number: 3
            label: LABEL_OPTIONAL
            type: TYPE_BYTES
            default_value: "a\\000\\001b"
          }
          field {
            name: "no_default"
            number: 4
            label: LABEL_OPTIONAL
            type: TYPE_STRING
          }
        }
      )pb");
  ASSERT_TRUE(pool.ok()) << pool.status();
  upb::MessageDefPtr m = pool->FindMessageByName("FooMessage");
  ASSERT_TRUE(m);

  // Every default must be null-terminated just past its last byte, including
  // bytes defaults without escapes, which fill their whole allocation.
  auto expect_default = [&](const char* name, std::string_view expected) {
    SCOPED_TRACE(name);
    upb::FieldDefPtr f = m.FindFieldByName(name);
    ASSERT_TRUE(f);
    upb_StringView sv = f.default_value().str_val;
    ASSERT_NE(sv.data, nullptr);
    EXPECT_EQ(std::string_view(sv.data, sv.size), expected);
    EXPECT_EQ(sv.data[sv.size], '\0');
  };
  expect_default("str", "anon");
  expect_default("bytes_raw", "raw");
  expect_default("bytes_escaped", std::string_view("a\0\001b", 4));
  expect_default("no_default", "");
}

TEST(ReflectionTest, ImplicitPresenceWithNonZeroDefaultEnum) {
  absl::Status status =
      LoadDescriptorProto(
          R"pb(
            syntax: "editions"
            edition: EDITION_2023
            name: "F"
            enum_type {
              name: "FooEnum"
              value { name: "v1" number: 1 }
              options { features { enum_type: CLOSED } }
            }
            message_type {
              name: "FooMessage"
              field {
                name: "f1"
                number: 1
                type: TYPE_ENUM
                type_name: "FooEnum"
                options { features { field_presence: IMPLICIT } }
              }
            }
          )pb")
          .status();
  EXPECT_EQ(std::string_view(status.message()),
            "Implicit presence field (FooMessage.f1) cannot use an enum type "
            "with a non-zero default (FooEnum)");
}

TEST(ReflectionTest, EditionWithoutSyntax) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              edition: EDITION_2023
                            )pb")
                            .status();
  EXPECT_EQ(
      status.message(),
      R"(Setting edition requires that syntax="editions", but syntax is "")");
}

TEST(ReflectionTest, EditionWithWrongSyntax) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              edition: EDITION_2023 syntax: "proto2"
                            )pb")
                            .status();
  EXPECT_EQ(
      status.message(),
      R"(Setting edition requires that syntax="editions", but syntax is "proto2")");
}

TEST(ReflectionTest, SyntaxEditionsWithNoEdition) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              syntax: "editions"
                            )pb")
                            .status();
  EXPECT_EQ(status.message(),
            R"(File has syntax="editions", but no edition is specified)");
}

TEST(ReflectionTest, InvalidSyntax) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              syntax: "abc123"
                            )pb")
                            .status();
  EXPECT_EQ(status.message(), R"(Invalid syntax 'abc123')");
}

TEST(ReflectionTest, ExplicitFeatureOnProto2File) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              syntax: "proto2"
                              options { features { field_presence: EXPLICIT } }
                            )pb")
                            .status();
  EXPECT_EQ(status.message(), R"(Features can only be specified for editions)");
}

TEST(ReflectionTest, ExplicitFeatureOnProto2Message) {
  absl::Status status =
      LoadDescriptorProto(
          R"pb(
            syntax: "proto2"
            message_type {
              name: "M"
              options { features { field_presence: EXPLICIT } }
            }
          )pb")
          .status();
  EXPECT_EQ(status.message(), R"(Features can only be specified for editions)");
}

TEST(ReflectionTest, ExplicitFeatureOnProto2Enum) {
  absl::Status status =
      LoadDescriptorProto(
          R"pb(
            syntax: "proto2"
            enum_type {
              name: "E"
              options { features { field_presence: EXPLICIT } }
            }
          )pb")
          .status();
  EXPECT_EQ(status.message(), R"(Features can only be specified for editions)");
}

TEST(ReflectionTest, ExplicitFeatureOnProto2EnumValue) {
  absl::Status status =
      LoadDescriptorProto(
          R"pb(
            syntax: "proto2"
            enum_type {
              name: "E"
              value {
                name: "V"
                options { features { field_presence: EXPLICIT } }
              }
            }
          )pb")
          .status();
  EXPECT_EQ(status.message(), R"(Features can only be specified for editions)");
}

TEST(ReflectionTest, TooManyRequiredFieldsFailGracefully) {
  const auto make_desc = [](int n) {
    std::vector<std::string> fields;
    for (int i = 1; i <= n; ++i) {
      fields.push_back(absl::StrFormat(R"pb(
                                         field {
                                           name: "f%d"
                                           number: %d
                                           type: TYPE_INT32
                                           label: LABEL_REQUIRED
                                         })pb",
                                       i, i));
    }
    return absl::StrFormat(
        R"pb(
          syntax: "proto2"
          name: "F"
          message_type { name: "FooMessage" %s }
        )pb",
        absl::StrJoin(fields, "\n"));
  };
  // 63 required fields is ok.
  upb::DefPool good = LoadDescriptorProto(make_desc(63)).value();
  auto m = good.FindMessageByName("FooMessage");
  auto f = m.FindFieldByNumber(63);
  EXPECT_STREQ(f.full_name(), "FooMessage.f63");

  // 64 is too much.
  EXPECT_THAT(LoadDescriptorProto(make_desc(64)).status().message(),
              HasSubstr("Too many required fields"));
}

#define STRING_AND_SIZE(string) string, strlen(string)

TEST(ReflectionTest, FindMethodByName) {
  upb::Arena arena;
  upb::DefPool defpool;
  upb::Status status;
  ASSERT_TRUE(_upb_DefPool_LoadDefInit(
      defpool.ptr(), &google_protobuf_unittest_proto_upbdefinit));
  const upb_ServiceDef* service_def = upb_DefPool_FindServiceByName(
      defpool.ptr(), "proto2_unittest.TestService");
  ASSERT_THAT(service_def, NotNull());
  EXPECT_STREQ(upb_ServiceDef_Name(service_def), "TestService");
  EXPECT_STREQ(upb_ServiceDef_FullName(service_def),
               "proto2_unittest.TestService");
  EXPECT_EQ(upb_DefPool_FindServiceByNameWithSize(
                defpool.ptr(), STRING_AND_SIZE("proto2_unittest.TestService")),
            service_def);
  const upb_MethodDef* method_def =
      upb_ServiceDef_FindMethodByName(service_def, "Bar");
  ASSERT_THAT(method_def, NotNull());
  EXPECT_STREQ(upb_MethodDef_Name(method_def), "Bar");
  EXPECT_STREQ(upb_MethodDef_FullName(method_def),
               "proto2_unittest.TestService.Bar");
  EXPECT_EQ(upb_ServiceDef_FindMethodByNameWithSize(service_def,
                                                    STRING_AND_SIZE("Bar")),
            method_def);
}

TEST(ReflectionTest, FindEnumByName) {
  upb::Arena arena;
  upb::DefPool defpool;
  upb::Status status;
  ASSERT_TRUE(_upb_DefPool_LoadDefInit(
      defpool.ptr(), &google_protobuf_unittest_proto_upbdefinit));
  const upb_EnumDef* enum_def = upb_DefPool_FindEnumByName(
      defpool.ptr(), "proto2_unittest.TestAllTypes.NestedEnum");
  ASSERT_THAT(enum_def, NotNull());
  EXPECT_STREQ(upb_EnumDef_Name(enum_def), "NestedEnum");
  EXPECT_STREQ(upb_EnumDef_FullName(enum_def),
               "proto2_unittest.TestAllTypes.NestedEnum");
  EXPECT_EQ(upb_DefPool_FindEnumByNameWithSize(
                defpool.ptr(),
                STRING_AND_SIZE("proto2_unittest.TestAllTypes.NestedEnum")),
            enum_def);
}

TEST(ReflectionTest, FindEnumValueByName) {
  upb::Arena arena;
  upb::DefPool defpool;
  upb::Status status;
  ASSERT_TRUE(_upb_DefPool_LoadDefInit(
      defpool.ptr(), &google_protobuf_unittest_proto_upbdefinit));
  const upb_EnumValueDef* enum_value_def = upb_DefPool_FindEnumValueByName(
      defpool.ptr(), "proto2_unittest.TestAllTypes.BAR");
  ASSERT_THAT(enum_value_def, NotNull());
  EXPECT_STREQ(upb_EnumValueDef_Name(enum_value_def), "BAR");
  EXPECT_STREQ(upb_EnumValueDef_FullName(enum_value_def),
               "proto2_unittest.TestAllTypes.BAR");
  EXPECT_EQ(
      upb_DefPool_FindEnumValueByNameWithSize(
          defpool.ptr(), STRING_AND_SIZE("proto2_unittest.TestAllTypes.BAR")),
      enum_value_def);
  const upb_EnumDef* enum_def = upb_DefPool_FindEnumByName(
      defpool.ptr(), "proto2_unittest.TestAllTypes.NestedEnum");
  ASSERT_THAT(enum_def, NotNull());
  EXPECT_EQ(upb_EnumDef_FindValueByName(enum_def, "BAR"), enum_value_def);
  EXPECT_EQ(
      upb_EnumDef_FindValueByNameWithSize(enum_def, STRING_AND_SIZE("BAR")),
      enum_value_def);
}

TEST(ReflectionTest, NegativePublicDependencyIndex) {
  // Verify that a negative public_dependency index is rejected rather than
  // causing an out-of-bounds read when the index is later used to access the
  // deps array.
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              name: "test.proto" public_dependency: -1
                            )pb")
                            .status();
  EXPECT_FALSE(status.ok());
  EXPECT_THAT(std::string(status.message()), HasSubstr("out of range"));
}

TEST(ReflectionTest, RepeatedRequiredField) {
  // A repeated field marked LEGACY_REQUIRED is rejected rather than producing a
  // field that is both required and repeated.  Such a field has no presence,
  // which violates the invariant that every required field has presence and
  // would cause an out-of-bounds read when upb_FieldPath_ToText() formats the
  // missing-required-field path.  protoc and the C++ DescriptorPool reject this
  // for the same reason.
  absl::Status status =
      LoadDescriptorProto(
          R"pb(
            name: "test.proto"
            syntax: "editions"
            edition: EDITION_2023
            message_type {
              name: "M"
              field {
                name: "r"
                number: 1
                label: LABEL_REPEATED
                type: TYPE_INT32
                options { features { field_presence: LEGACY_REQUIRED } }
              }
            }
          )pb")
          .status();
  EXPECT_FALSE(status.ok());
  EXPECT_THAT(std::string(status.message()), HasSubstr("cannot be required"));
}

TEST(ReflectionTest, NegativeWeakDependencyIndex) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              name: "test.proto" weak_dependency: -1
                            )pb")
                            .status();
  EXPECT_FALSE(status.ok());
  EXPECT_THAT(std::string(status.message()), HasSubstr("out of range"));
}

TEST(ReflectionTest, ZeroPublicDependencyIndexWithNoDeps) {
  // Index 0 is out of range when there are no dependencies (dep_count=0).
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              name: "test.proto" public_dependency: 0
                            )pb")
                            .status();
  EXPECT_FALSE(status.ok());
  EXPECT_THAT(std::string(status.message()), HasSubstr("out of range"));
}

TEST(ReflectionTest, ZeroWeakDependencyIndexWithNoDeps) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              name: "test.proto" weak_dependency: 0
                            )pb")
                            .status();
  EXPECT_FALSE(status.ok());
  EXPECT_THAT(std::string(status.message()), HasSubstr("out of range"));
}

// Sets (pb.enumvalue.json).string option (extension field 998) on
// EnumValueOptions.
static void SetCustomJsonOption(google::protobuf::EnumValueOptions* options,
                                absl::string_view json_name) {
  // Wire format for JsonEnumValueOptions { string string = 1 }:
  // Field 1 (string, length-delimited): tag = (1 << 3) | 2 = 0x0a.
  std::string payload;
  payload.push_back('\x0a');
  payload.push_back(static_cast<char>(json_name.size()));
  payload.append(json_name);
  // Field 998 is extension (pb.enumvalue.json) on EnumValueOptions.
  options->mutable_unknown_fields()->AddLengthDelimited(998, payload);
}

struct TestEnumValueSpec {
  std::string name;
  int number;
  bool has_json_name;
  std::string json_name;

  TestEnumValueSpec(const char* name, int number, const char* json_name)
      : name(name),
        number(number),
        has_json_name(json_name != nullptr),
        json_name(json_name != nullptr ? json_name : "") {}

  static TestEnumValueSpec WithJson(const char* name, int number,
                                    absl::string_view json_name) {
    TestEnumValueSpec spec(name, number, nullptr);
    spec.has_json_name = true;
    spec.json_name.assign(json_name.data(), json_name.size());
    return spec;
  }
};

static void SwapEnumValues(TestEnumValueSpec& a, TestEnumValueSpec& b) {
  TestEnumValueSpec tmp = a;
  a = b;
  b = tmp;
}

static google::protobuf::FileDescriptorProto MakeEnumDescriptorWithValues(
    absl::Span<const TestEnumValueSpec> values, bool allow_alias = false) {
  google::protobuf::FileDescriptorProto file_proto;
  file_proto.set_name("test.proto");
  file_proto.set_syntax("editions");
  file_proto.set_edition(google::protobuf::EDITION_2026);

  google::protobuf::EnumDescriptorProto* enum_proto = file_proto.add_enum_type();
  enum_proto->set_name("TestEnum");
  if (allow_alias) enum_proto->mutable_options()->set_allow_alias(true);

  for (const TestEnumValueSpec& v : values) {
    google::protobuf::EnumValueDescriptorProto* val_proto = enum_proto->add_value();
    val_proto->set_name(v.name);
    val_proto->set_number(v.number);
    if (v.has_json_name) {
      SetCustomJsonOption(val_proto->mutable_options(), v.json_name);
    }
  }
  return file_proto;
}

static absl::StatusOr<upb::DefPool> LoadEnumDescriptorWithValues(
    absl::Span<const TestEnumValueSpec> values, bool allow_alias = false) {
  google::protobuf::FileDescriptorSet set;
  *set.add_file() = MakeEnumDescriptorWithValues(values, allow_alias);
  return LoadDescriptorSetFromProto(set);
}

TEST(ReflectionTest, EnumCustomJsonNameConflictDifferentNumberFails) {
  // Two enum values with DIFFERENT numbers (1 and 2) sharing the same custom
  // JSON name must fail descriptor validation.
  absl::Status status =
      LoadEnumDescriptorWithValues({
                                       {"VAL_ZERO", 0, nullptr},
                                       {"VAL_A", 1, "custom_name"},
                                       {"VAL_B", 2, "custom_name"},
                                   })
          .status();
  EXPECT_FALSE(status.ok());
  EXPECT_THAT(std::string(status.message()),
              HasSubstr("duplicate custom json_name (custom_name) in enum"));
}

TEST(ReflectionTest, EnumCustomJsonNameAliasedSameNumberSucceeds) {
  // Two aliased enum values with the SAME number (1 and 1) sharing the same
  // custom JSON name must succeed descriptor validation.
  absl::Status status = LoadEnumDescriptorWithValues(
                            {
                                {"VAL_ZERO", 0, nullptr},
                                {"VAL_A", 1, "custom_name"},
                                {"VAL_B", 1, "custom_name"},
                            },
                            true)
                            .status();
  EXPECT_TRUE(status.ok()) << status.message();
}

TEST(ReflectionTest, EnumCustomJsonNameConflictsWithDeclaredNameInEitherOrder) {
  for (bool reverse : {false, true}) {
    for (const char* second_json :
         {static_cast<const char*>(nullptr), "other"}) {
      SCOPED_TRACE(reverse);
      SCOPED_TRACE(second_json ? second_json : "unset");
      TestEnumValueSpec values[] = {{"VAL_ZERO", 0, nullptr},
                                    {"VAL_A", 1, "VAL_B"},
                                    {"VAL_B", 2, second_json}};
      if (reverse) SwapEnumValues(values[1], values[2]);

      google::protobuf::DescriptorPool cpp_pool;
      EXPECT_EQ(cpp_pool.BuildFile(MakeEnumDescriptorWithValues(values)),
                nullptr);
      absl::Status status = LoadEnumDescriptorWithValues(values).status();
      EXPECT_FALSE(status.ok());
      EXPECT_THAT(std::string(status.message()),
                  HasSubstr("duplicate custom json_name (VAL_B) in enum"));
    }
  }
}

TEST(ReflectionTest, EnumCustomJsonNameAliasKeepsDeclaredNameInEitherOrder) {
  for (bool reverse : {false, true}) {
    SCOPED_TRACE(reverse);
    // Both aliases must explicitly specify the same custom string, even when
    // that string is the declared name of one alias.
    TestEnumValueSpec values[] = {
        {"VAL_ZERO", 0, nullptr}, {"VAL_A", 1, "VAL_B"}, {"VAL_B", 1, "VAL_B"}};
    if (reverse) SwapEnumValues(values[1], values[2]);
    const int b_index = reverse ? 1 : 2;

    google::protobuf::DescriptorPool cpp_pool;
    const auto* cpp_file =
        cpp_pool.BuildFile(MakeEnumDescriptorWithValues(values, true));
    ASSERT_NE(cpp_file, nullptr);
    const auto* cpp_enum = cpp_file->enum_type(0);
    EXPECT_EQ(cpp_enum->FindValueByName("VAL_B"), cpp_enum->value(b_index));
    EXPECT_EQ(cpp_enum->FindValueByNumber(1), cpp_enum->value(1));

    auto pool = LoadEnumDescriptorWithValues(values, true);
    ASSERT_TRUE(pool.ok()) << pool.status();
    const upb_EnumDef* e = upb_DefPool_FindEnumByName(pool->ptr(), "TestEnum");
    ASSERT_NE(e, nullptr);
    const upb_EnumValueDef* b = upb_EnumDef_Value(e, b_index);
    EXPECT_EQ(upb_EnumDef_FindValueByName(e, "VAL_B"), b);
    EXPECT_EQ(upb_EnumDef_FindValueByNameWithSize(e, "VAL_B_suffix", 5), b);
    EXPECT_EQ(upb_EnumDef_FindByJsonName(e, "VAL_B"), b);
    EXPECT_EQ(upb_EnumDef_FindValueByNumber(e, 1), upb_EnumDef_Value(e, 1));
    EXPECT_EQ(upb_EnumDef_FindValueByName(e, "VAL_A"),
              upb_EnumDef_Value(e, reverse ? 2 : 1));
    EXPECT_EQ(upb_EnumDef_ValueCount(e), 3);
  }
}

TEST(ReflectionTest, EnumCustomJsonNameAliasesRequireMatchingOptions) {
  for (bool reverse : {false, true}) {
    for (const char* second_json :
         {static_cast<const char*>(nullptr), "other"}) {
      SCOPED_TRACE(reverse);
      SCOPED_TRACE(second_json ? second_json : "unset");
      TestEnumValueSpec values[] = {{"VAL_ZERO", 0, nullptr},
                                    {"VAL_A", 1, "VAL_B"},
                                    {"VAL_B", 1, second_json}};
      if (reverse) SwapEnumValues(values[1], values[2]);

      google::protobuf::DescriptorPool cpp_pool;
      EXPECT_EQ(cpp_pool.BuildFile(MakeEnumDescriptorWithValues(values, true)),
                nullptr);
      absl::Status status = LoadEnumDescriptorWithValues(values, true).status();
      EXPECT_FALSE(status.ok());
      EXPECT_THAT(std::string(status.message()),
                  HasSubstr("must have the same custom JSON string"));
    }
  }
}

TEST(ReflectionTest, EnumCustomJsonNameAliasesPreserveJsonOnlyLookup) {
  for (bool reverse : {false, true}) {
    for (const char* json_name : {"custom_name", ""}) {
      SCOPED_TRACE(reverse);
      SCOPED_TRACE(json_name);
      TestEnumValueSpec values[] = {{"VAL_ZERO", 0, nullptr},
                                    {"VAL_A", 1, json_name},
                                    {"VAL_B", 1, json_name}};
      if (reverse) SwapEnumValues(values[1], values[2]);
      google::protobuf::DescriptorPool cpp_pool;
      ASSERT_NE(cpp_pool.BuildFile(MakeEnumDescriptorWithValues(values, true)),
                nullptr);

      auto pool = LoadEnumDescriptorWithValues(values, true);
      ASSERT_TRUE(pool.ok()) << pool.status();
      const upb_EnumDef* e =
          upb_DefPool_FindEnumByName(pool->ptr(), "TestEnum");
      ASSERT_NE(e, nullptr);
      EXPECT_EQ(upb_EnumDef_FindValueByName(e, json_name), nullptr);
      EXPECT_EQ(
          upb_EnumDef_FindValueByNameWithSize(e, json_name, strlen(json_name)),
          nullptr);
      EXPECT_EQ(upb_EnumDef_FindByJsonName(e, json_name),
                upb_EnumDef_Value(e, 1));
      EXPECT_EQ(upb_EnumDef_FindValueByNumber(e, 1), upb_EnumDef_Value(e, 1));
      EXPECT_EQ(upb_EnumDef_FindValueByName(e, "VAL_A"),
                upb_EnumDef_Value(e, reverse ? 2 : 1));
      EXPECT_EQ(upb_EnumDef_FindValueByName(e, "VAL_B"),
                upb_EnumDef_Value(e, reverse ? 1 : 2));
    }
  }
}

TEST(ReflectionTest, EnumCustomJsonNameEmptyOptionDiffersFromUnset) {
  for (bool reverse : {false, true}) {
    SCOPED_TRACE(reverse);
    TestEnumValueSpec values[] = {
        {"VAL_ZERO", 0, nullptr}, {"VAL_A", 1, ""}, {"VAL_B", 1, nullptr}};
    if (reverse) SwapEnumValues(values[1], values[2]);
    google::protobuf::DescriptorPool cpp_pool;
    EXPECT_EQ(cpp_pool.BuildFile(MakeEnumDescriptorWithValues(values, true)),
              nullptr);
    absl::Status status = LoadEnumDescriptorWithValues(values, true).status();
    EXPECT_THAT(std::string(status.message()),
                HasSubstr("must have the same custom JSON string"));
  }
}

TEST(ReflectionTest, EnumCustomJsonNameEqualToOwnNameKeepsDeclaredLookup) {
  const TestEnumValueSpec values[] = {{"VAL_ZERO", 0, nullptr},
                                      {"VAL_A", 1, "VAL_A"}};
  google::protobuf::DescriptorPool cpp_pool;
  ASSERT_NE(cpp_pool.BuildFile(MakeEnumDescriptorWithValues(values)), nullptr);
  auto pool = LoadEnumDescriptorWithValues(values);
  ASSERT_TRUE(pool.ok()) << pool.status();
  const upb_EnumDef* e = upb_DefPool_FindEnumByName(pool->ptr(), "TestEnum");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(upb_EnumDef_FindValueByName(e, "VAL_A"), upb_EnumDef_Value(e, 1));
  EXPECT_EQ(upb_EnumDef_FindByJsonName(e, "VAL_A"), upb_EnumDef_Value(e, 1));
}

TEST(ReflectionTest, EnumCustomJsonNameWithEmbeddedNulIsRejected) {
  // upb exposes upb_EnumValueDef_JsonName() as a NUL-terminated C string, so
  // embedded NUL in (pb.enumvalue.json).string must be rejected before
  // computing or indexing the C-string form (matching C++ DescriptorPool).
  const absl::string_view kNulCases[] = {
      absl::string_view("\0", 1),
      absl::string_view("foo\0bar", 7),
      absl::string_view("VAL_A\0", 6),
      absl::string_view("VAL_B\0extra", 11),
  };
  for (absl::string_view bad_json : kNulCases) {
    SCOPED_TRACE(bad_json.size());
    const TestEnumValueSpec values[] = {
        {"VAL_ZERO", 0, nullptr},
        TestEnumValueSpec::WithJson("VAL_A", 1, bad_json),
    };
    const auto proto = MakeEnumDescriptorWithValues(values);
    const google::protobuf::EnumValueOptions& opts =
        proto.enum_type(0).value(1).options();
    std::string opts_wire;
    ASSERT_TRUE(opts.SerializeToString(&opts_wire));
    std::string expected_payload;
    expected_payload.push_back('\x0a');
    expected_payload.push_back(static_cast<char>(bad_json.size()));
    expected_payload.append(bad_json.data(), bad_json.size());
    // Field 998, wire type 2 (LEN): tag = (998 << 3) | 2 = 7986 -> 0xb2 0x3e.
    std::string expected_wire;
    expected_wire.append("\xb2\x3e", 2);
    expected_wire.push_back(static_cast<char>(expected_payload.size()));
    expected_wire.append(expected_payload);
    ASSERT_EQ(opts_wire, expected_wire);

    class RecordingErrorCollector
        : public google::protobuf::DescriptorPool::ErrorCollector {
     public:
      void RecordError(absl::string_view, absl::string_view,
                       const google::protobuf::Message*, ErrorLocation,
                       absl::string_view message) override {
        errors.push_back(std::string(message));
      }
      std::vector<std::string> errors;
    } collector;
    google::protobuf::DescriptorPool cpp_pool;
    EXPECT_EQ(cpp_pool.BuildFileCollectingErrors(proto, &collector), nullptr);
    EXPECT_THAT(absl::StrJoin(collector.errors, "\n"),
                HasSubstr("embedded null characters"));

    absl::Status status = LoadEnumDescriptorWithValues(values).status();
    EXPECT_FALSE(status.ok());
    EXPECT_THAT(std::string(status.message()), HasSubstr("cannot contain NUL"));
  }

  // Embedded NUL after a prefix matching another declared value or custom name
  // must be rejected as an embedded NUL rather than truncating to a prefix
  // collision, in either order and for both distinct and aliased numbers.
  for (bool reverse : {false, true}) {
    SCOPED_TRACE(reverse);
    TestEnumValueSpec vs_declared[] = {
        {"VAL_ZERO", 0, nullptr},
        TestEnumValueSpec::WithJson("VAL_A", 1,
                                    absl::string_view("VAL_B\0extra", 11)),
        {"VAL_B", 2, nullptr},
    };
    if (reverse) SwapEnumValues(vs_declared[1], vs_declared[2]);
    absl::Status status = LoadEnumDescriptorWithValues(vs_declared).status();
    EXPECT_FALSE(status.ok());
    EXPECT_THAT(std::string(status.message()), HasSubstr("cannot contain NUL"));

    TestEnumValueSpec vs_distinct[] = {
        {"VAL_ZERO", 0, nullptr},
        TestEnumValueSpec::WithJson("VAL_A", 1,
                                    absl::string_view("foo\0a", 5)),
        TestEnumValueSpec::WithJson("VAL_B", 2,
                                    absl::string_view("foo\0b", 5)),
    };
    if (reverse) SwapEnumValues(vs_distinct[1], vs_distinct[2]);
    status = LoadEnumDescriptorWithValues(vs_distinct).status();
    EXPECT_FALSE(status.ok());
    EXPECT_THAT(std::string(status.message()), HasSubstr("cannot contain NUL"));

    TestEnumValueSpec vs_alias[] = {
        {"VAL_ZERO", 0, nullptr},
        TestEnumValueSpec::WithJson("VAL_A", 1,
                                    absl::string_view("foo\0bar", 7)),
        TestEnumValueSpec::WithJson("VAL_B", 1,
                                    absl::string_view("foo\0bar", 7)),
    };
    if (reverse) SwapEnumValues(vs_alias[1], vs_alias[2]);
    status = LoadEnumDescriptorWithValues(vs_alias, true).status();
    EXPECT_FALSE(status.ok());
    EXPECT_THAT(std::string(status.message()), HasSubstr("cannot contain NUL"));
  }

  // Failed embedded-NUL load must roll back cleanly and leave the pool usable,
  // and length-aware lookup on a valid custom JSON name must not match a
  // NUL-suffixed query.
  upb::Arena arena;
  upb::DefPool pool;
  upb::Status status;
  const google::protobuf::FileDescriptorProto existing =
      ParseTextProtoOrDie(R"pb(
        name: "existing.proto"
        syntax: "proto2"
        message_type { name: "ExistingMessage" }
      )pb");
  ASSERT_TRUE(pool.AddFile(ToUpbDescriptorSet(existing, arena), &status));
  const auto* existing_message =
      pool.FindMessageByName("ExistingMessage").ptr();
  ASSERT_NE(existing_message, nullptr);

  const TestEnumValueSpec bad_values[] = {
      {"VAL_ZERO", 0, nullptr},
      TestEnumValueSpec::WithJson("VAL_A", 1,
                                  absl::string_view("foo\0bar", 7)),
  };
  ASSERT_FALSE(pool.AddFile(
      ToUpbDescriptorSet(MakeEnumDescriptorWithValues(bad_values), arena),
      &status));
  EXPECT_THAT(status.error_message(), HasSubstr("cannot contain NUL"));
  EXPECT_FALSE(pool.FindFileByName("test.proto"));
  EXPECT_FALSE(pool.FindEnumByName("TestEnum"));
  EXPECT_EQ(upb_DefPool_FindEnumValueByName(pool.ptr(), "VAL_ZERO"), nullptr);
  EXPECT_EQ(upb_DefPool_FindEnumValueByName(pool.ptr(), "VAL_A"), nullptr);
  EXPECT_EQ(pool.FindMessageByName("ExistingMessage").ptr(), existing_message);

  const TestEnumValueSpec valid_values[] = {
      {"VAL_ZERO", 0, nullptr},
      {"VAL_A", 1, "foo"},
  };
  status.Clear();
  ASSERT_TRUE(pool.AddFile(
      ToUpbDescriptorSet(MakeEnumDescriptorWithValues(valid_values), arena),
      &status))
      << status.error_message();
  const upb_EnumDef* e = pool.FindEnumByName("TestEnum").ptr();
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(upb_EnumDef_FindByJsonNameWithSize(e, "foo", 3),
            upb_EnumDef_Value(e, 1));
  EXPECT_EQ(upb_EnumDef_FindByJsonNameWithSize(e, "foo\0bar", 7), nullptr);
}

TEST(ReflectionTest, EnumAliasesWithoutCustomJsonNamesKeepDeclaredLookup) {
  const TestEnumValueSpec values[] = {
      {"VAL_ZERO", 0, nullptr}, {"VAL_A", 1, nullptr}, {"VAL_B", 1, nullptr}};
  google::protobuf::DescriptorPool cpp_pool;
  ASSERT_NE(cpp_pool.BuildFile(MakeEnumDescriptorWithValues(values, true)),
            nullptr);
  auto pool = LoadEnumDescriptorWithValues(values, true);
  ASSERT_TRUE(pool.ok()) << pool.status();
  const upb_EnumDef* e = upb_DefPool_FindEnumByName(pool->ptr(), "TestEnum");
  ASSERT_NE(e, nullptr);
  EXPECT_EQ(upb_EnumDef_FindValueByName(e, "VAL_A"), upb_EnumDef_Value(e, 1));
  EXPECT_EQ(upb_EnumDef_FindValueByName(e, "VAL_B"), upb_EnumDef_Value(e, 2));
  EXPECT_EQ(upb_EnumDef_FindValueByNumber(e, 1), upb_EnumDef_Value(e, 1));
}

TEST(ReflectionTest, EnumDuplicateNumbersRequireAllowAliasInEitherOrder) {
  for (bool explicit_false : {false, true}) {
    for (bool reverse : {false, true}) {
      SCOPED_TRACE(explicit_false ? "allow_alias=false" : "allow_alias unset");
      SCOPED_TRACE(reverse);
      TestEnumValueSpec values[] = {{"VAL_ZERO", 0, nullptr},
                                    {"VAL_A", 1, nullptr},
                                    {"VAL_B", 1, nullptr}};
      if (reverse) SwapEnumValues(values[1], values[2]);
      auto proto = MakeEnumDescriptorWithValues(values);
      if (explicit_false) {
        proto.mutable_enum_type(0)->mutable_options()->set_allow_alias(false);
      }
      ASSERT_EQ(proto.enum_type(0).options().has_allow_alias(), explicit_false);

      google::protobuf::DescriptorPool cpp_pool;
      EXPECT_EQ(cpp_pool.BuildFile(proto), nullptr);
      upb::Arena arena;
      upb::DefPool pool;
      upb::Status status;
      EXPECT_FALSE(pool.AddFile(ToUpbDescriptorSet(proto, arena), &status));
      EXPECT_THAT(status.error_message(),
                  HasSubstr("option allow_alias = true;"));
    }
  }
}

TEST(ReflectionTest, EnumCustomJsonNameAliasesRequireAllowAliasInEitherOrder) {
  for (bool explicit_false : {false, true}) {
    for (bool reverse : {false, true}) {
      for (const char* second_json :
           {static_cast<const char*>(nullptr), "VAL_B"}) {
        SCOPED_TRACE(explicit_false ? "allow_alias=false"
                                    : "allow_alias unset");
        SCOPED_TRACE(reverse);
        SCOPED_TRACE(second_json ? second_json : "unset");
        TestEnumValueSpec values[] = {{"VAL_ZERO", 0, nullptr},
                                      {"VAL_A", 1, "VAL_B"},
                                      {"VAL_B", 1, second_json}};
        if (reverse) SwapEnumValues(values[1], values[2]);
        auto proto = MakeEnumDescriptorWithValues(values);
        if (explicit_false) {
          proto.mutable_enum_type(0)->mutable_options()->set_allow_alias(false);
        }
        ASSERT_EQ(proto.enum_type(0).options().has_allow_alias(),
                  explicit_false);

        google::protobuf::DescriptorPool cpp_pool;
        EXPECT_EQ(cpp_pool.BuildFile(proto), nullptr);
        upb::Arena arena;
        upb::DefPool pool;
        upb::Status status;
        EXPECT_FALSE(pool.AddFile(ToUpbDescriptorSet(proto, arena), &status));
        EXPECT_THAT(status.error_message(),
                    HasSubstr("option allow_alias = true;"));
      }
    }
  }
}

TEST(ReflectionTest, EnumDuplicateNumberFailureLeavesPoolUsable) {
  for (bool explicit_false : {false, true}) {
    for (bool reverse : {false, true}) {
      for (const char* json_name :
           {static_cast<const char*>(nullptr), "VAL_B"}) {
        SCOPED_TRACE(explicit_false ? "allow_alias=false"
                                    : "allow_alias unset");
        SCOPED_TRACE(reverse);
        SCOPED_TRACE(json_name ? json_name : "unset");
        upb::Arena arena;
        upb::DefPool pool;
        upb::Status status;
        const google::protobuf::FileDescriptorProto existing =
            ParseTextProtoOrDie(R"pb(
              name: "existing.proto"
              syntax: "proto2"
              message_type { name: "ExistingMessage" }
            )pb");
        ASSERT_TRUE(pool.AddFile(ToUpbDescriptorSet(existing, arena), &status));
        const auto* existing_message =
            pool.FindMessageByName("ExistingMessage").ptr();
        ASSERT_NE(existing_message, nullptr);

        TestEnumValueSpec values[] = {{"VAL_ZERO", 0, nullptr},
                                      {"VAL_A", 1, json_name},
                                      {"VAL_B", 1, json_name}};
        if (reverse) SwapEnumValues(values[1], values[2]);
        auto proto = MakeEnumDescriptorWithValues(values);
        if (explicit_false) {
          proto.mutable_enum_type(0)->mutable_options()->set_allow_alias(false);
        }
        ASSERT_FALSE(pool.AddFile(ToUpbDescriptorSet(proto, arena), &status));
        EXPECT_THAT(status.error_message(),
                    HasSubstr("option allow_alias = true;"));
        EXPECT_FALSE(pool.FindFileByName("test.proto"));
        EXPECT_FALSE(pool.FindEnumByName("TestEnum"));
        for (const auto& value : values) {
          EXPECT_EQ(
              upb_DefPool_FindEnumValueByName(pool.ptr(), value.name.c_str()),
              nullptr);
        }
        EXPECT_EQ(pool.FindMessageByName("ExistingMessage").ptr(),
                  existing_message);

        // Reuse the same file and symbol names after the failed-add rollback.
        proto.mutable_enum_type(0)->mutable_options()->set_allow_alias(true);
        status.Clear();
        ASSERT_TRUE(pool.AddFile(ToUpbDescriptorSet(proto, arena), &status))
            << status.error_message();
        EXPECT_TRUE(status.ok());
        EXPECT_TRUE(pool.FindFileByName("test.proto"));
        const auto* e = pool.FindEnumByName("TestEnum").ptr();
        ASSERT_NE(e, nullptr);
        EXPECT_EQ(upb_EnumDef_ValueCount(e), 3);
        for (int i = 0; i < 3; ++i) {
          EXPECT_EQ(upb_EnumDef_FindValueByName(e, values[i].name.c_str()),
                    upb_EnumDef_Value(e, i));
          EXPECT_EQ(upb_DefPool_FindEnumValueByName(pool.ptr(),
                                                    values[i].name.c_str()),
                    upb_EnumDef_Value(e, i));
        }
        EXPECT_EQ(upb_EnumDef_FindValueByNumber(e, 1), upb_EnumDef_Value(e, 1));
        if (json_name) {
          EXPECT_EQ(upb_EnumDef_FindByJsonName(e, json_name),
                    upb_EnumDef_FindValueByName(e, "VAL_B"));
        }
        EXPECT_EQ(pool.FindMessageByName("ExistingMessage").ptr(),
                  existing_message);
      }
    }
  }
}

TEST(ReflectionTest, EnumDuplicateDeclaredNameIsStillRejected) {
  for (int number : {1, 2}) {
    SCOPED_TRACE(number);
    const TestEnumValueSpec values[] = {{"VAL_ZERO", 0, nullptr},
                                        {"VAL_A", 1, nullptr},
                                        {"VAL_A", number, nullptr}};
    google::protobuf::DescriptorPool cpp_pool;
    EXPECT_EQ(cpp_pool.BuildFile(MakeEnumDescriptorWithValues(values, true)),
              nullptr);
    absl::Status status = LoadEnumDescriptorWithValues(values, true).status();
    EXPECT_FALSE(status.ok());
    EXPECT_THAT(std::string(status.message()), HasSubstr("duplicate symbol"));
  }
}

TEST(ReflectionTest, LegacyJsonFieldConflictsKeepsCorrectNumberMapping) {
  // With deprecated_legacy_json_field_conflicts, two fields may share a
  // json_name. Each field number must still map to its own FieldDef.
  upb::DefPool pool =
      LoadDescriptorProto(
          R"pb(
            syntax: "proto2"
            name: "F"
            message_type { name: "Sub" }
            message_type {
              name: "M"
              options { deprecated_legacy_json_field_conflicts: true }
              field {
                name: "a"
                number: 1
                label: LABEL_OPTIONAL
                type: TYPE_MESSAGE
                type_name: ".Sub"
                json_name: "j"
              }
              field {
                name: "b"
                number: 2
                label: LABEL_OPTIONAL
                type: TYPE_FIXED64
                json_name: "j"
              }
            }
          )pb")
          .value();
  upb::MessageDefPtr m = pool.FindMessageByName("M");
  ASSERT_TRUE(m);
  upb::FieldDefPtr a = m.FindFieldByNumber(1);
  upb::FieldDefPtr b = m.FindFieldByNumber(2);
  ASSERT_TRUE(a);
  ASSERT_TRUE(b);
  EXPECT_STREQ(a.name(), "a");
  EXPECT_EQ(a.number(), 1);
  EXPECT_STREQ(b.name(), "b");
  EXPECT_EQ(b.number(), 2);
  EXPECT_EQ(b.type(), kUpb_FieldType_Fixed64);
}

TEST(ReflectionTest, JsonFieldConflictsRejectedWithoutLegacyOption) {
  absl::Status status = LoadDescriptorProto(
                            R"pb(
                              syntax: "proto2"
                              name: "F"
                              message_type {
                                name: "M"
                                field {
                                  name: "a"
                                  number: 1
                                  label: LABEL_OPTIONAL
                                  type: TYPE_FIXED64
                                  json_name: "j"
                                }
                                field {
                                  name: "b"
                                  number: 2
                                  label: LABEL_OPTIONAL
                                  type: TYPE_FIXED64
                                  json_name: "j"
                                }
                              }
                            )pb")
                            .status();
  EXPECT_THAT(std::string(status.message()),
              HasSubstr("duplicate json_name (j)"));
}

TEST(ReflectionTest, DefPoolFindMethodsRespectSize) {
  upb::DefPool pool = LoadDescriptorProto(R"pb(
                        syntax: "proto2"
                        name: "test.proto"
                        package: "pkg"
                        message_type {
                          name: "TestMessage"
                          extension_range { start: 1 end: 1000 }
                        }
                        enum_type {
                          name: "TestEnum"
                          value { name: "VAL" number: 1 }
                        }
                        extension {
                          name: "test_ext"
                          number: 100
                          label: LABEL_OPTIONAL
                          type: TYPE_INT32
                          extendee: ".pkg.TestMessage"
                        }
                      )pb")
                          .value();

  // Test FindMessageByName
  {
    absl::string_view full_name = "pkg.TestMessage";
    EXPECT_TRUE(pool.FindMessageByName(full_name));
    EXPECT_FALSE(
        pool.FindMessageByName(full_name.substr(0, 12)));  // "pkg.TestMess"
  }

  // Test FindEnumByName
  {
    absl::string_view full_name = "pkg.TestEnum";
    EXPECT_TRUE(pool.FindEnumByName(full_name));
    EXPECT_FALSE(pool.FindEnumByName(full_name.substr(0, 10)));  // "pkg.TestEn"
  }

  // Test FindFileByName
  {
    absl::string_view full_name = "test.proto";
    EXPECT_TRUE(pool.FindFileByName(full_name));
    EXPECT_FALSE(pool.FindFileByName(full_name.substr(0, 7)));  // "test.pr"
  }

  // Test FindExtensionByName
  {
    absl::string_view full_name = "pkg.test_ext";
    EXPECT_TRUE(pool.FindExtensionByName(full_name));
    EXPECT_FALSE(
        pool.FindExtensionByName(full_name.substr(0, 11)));  // "pkg.test_ex"
  }
}

TEST(ReflectionTest, FileDefResolvesDiamondDag) {
  google::protobuf::FileDescriptorSet set;
  for (absl::string_view name : {"f_0.proto", "priv.proto"}) {
    set.add_file()->set_name(name);
  }
  for (int k = 1; k <= 32; ++k) {
    for (char c : {'a', 'b'}) {
      google::protobuf::FileDescriptorProto* f = set.add_file();
      f->set_name(absl::StrFormat("%c_%d.proto", c, k));
      for (char p : absl::string_view(k == 1 ? "f" : "ab")) {
        f->add_public_dependency(f->dependency_size());
        f->add_dependency(absl::StrFormat("%c_%d.proto", p, k - 1));
      }
      if (k == 32 && c == 'a') f->add_dependency("priv.proto");
    }
  }
  absl::StatusOr<upb::DefPool> pool = LoadDescriptorSetFromProto(set);
  ASSERT_TRUE(pool.ok()) << pool.status();
  upb::FileDefPtr top = pool->FindFileByName("a_32.proto");
  ASSERT_TRUE(top);
  EXPECT_TRUE(top.resolves("f_0.proto"));
  EXPECT_FALSE(top.resolves("priv.proto"));
}

TEST(ReflectionTest, LoadDefInitDeepChain) {
  constexpr int kLen = 15000;
  std::vector<std::string> names(kLen), ser(kLen);
  std::vector<std::array<_upb_DefPool_Init*, 2>> dep_ptrs(kLen);
  std::vector<_upb_DefPool_Init> inits(kLen);
  for (int i = 0; i < kLen; ++i) {
    names[i] = absl::StrFormat("f_%d.proto", i);
    google::protobuf::FileDescriptorProto proto;
    proto.set_name(names[i]);
    if (i > 0) {
      proto.add_dependency(names[i - 1]);
      dep_ptrs[i] = {&inits[i - 1], nullptr};
    }
    ser[i] = proto.SerializeAsString();
    inits[i] = {i > 0 ? dep_ptrs[i].data() : nullptr, nullptr, names[i].c_str(),
                upb_StringView_FromDataAndSize(ser[i].data(), ser[i].size())};
  }
  upb::DefPool pool;
  ASSERT_TRUE(_upb_DefPool_LoadDefInitEx(pool.ptr(), &inits.back(), true));
  EXPECT_TRUE(pool.FindFileByName("f_0.proto"));
  EXPECT_TRUE(pool.FindFileByName("f_14999.proto"));

  _upb_DefPool_Init bad_dep = {nullptr, nullptr, "bad.proto",
                               upb_StringView_FromDataAndSize("\xff", 1)};
  _upb_DefPool_Init* bad_deps[] = {&bad_dep, nullptr};
  _upb_DefPool_Init bad_top = {
      bad_deps, nullptr, "bad_top.proto",
      upb_StringView_FromDataAndSize(ser[0].data(), ser[0].size())};
  EXPECT_FALSE(_upb_DefPool_LoadDefInitEx(pool.ptr(), &bad_top, true));
}

absl::Status LoadMapEntry(std::string_view entry_body) {
  return LoadDescriptorProto(absl::StrFormat(
                                 R"(
               syntax: "proto2" name: "m.proto"
               message_type { name: "Sub" }
               enum_type { name: "ZeroEnum" value { name: "Z" number: 0 } }
               enum_type {
                 name: "NonZeroEnum"
                 value { name: "ONE" number: 1 }
                 value { name: "ZERO" number: 0 }
               }
               message_type {
                 name: "Outer"
                 field {
                   name: "m" number: 1 label: LABEL_REPEATED
                   type: TYPE_MESSAGE type_name: ".Outer.Entry"
                 }
                 nested_type { name: "Entry" options { map_entry: true } %s }
               }
             )",
                                 entry_body))
      .status();
}

TEST(ReflectionTest, ValidateMapEntry) {
  for (std::string_view valid_entry : {
           "field { name: 'key' number: 1 type: TYPE_INT32 } "
           "field { name: 'value' number: 2 type: TYPE_STRING }",
           "field { name: 'key' number: 1 type: TYPE_STRING } "
           "field { name: 'value' number: 2 type: TYPE_MESSAGE "
           "type_name: '.Sub' }",
           "field { name: 'key' number: 1 type: TYPE_BOOL } "
           "field { name: 'value' number: 2 type: TYPE_ENUM "
           "type_name: '.ZeroEnum' }",
       }) {
    SCOPED_TRACE(valid_entry);
    EXPECT_TRUE(LoadMapEntry(valid_entry).ok());
  }

  // Editions DELIMITED encoding applies to normal submessage fields, not map
  // entries or message-typed map values.
  absl::StatusOr<upb::DefPool> delimited_pool = LoadDescriptorProto(R"pb(
    syntax: "editions"
    edition: EDITION_2023
    name: "delimited_map.proto"
    options { features { message_encoding: DELIMITED } }
    message_type { name: "Sub" }
    message_type {
      name: "Outer"
      field { name: "sub" number: 1 type: TYPE_MESSAGE type_name: ".Sub" }
      field {
        name: "m"
        number: 2
        label: LABEL_REPEATED
        type: TYPE_MESSAGE
        type_name: ".Outer.Entry"
      }
      nested_type {
        name: "Entry"
        options { map_entry: true }
        field { name: "key" number: 1 type: TYPE_STRING }
        field { name: "value" number: 2 type: TYPE_MESSAGE type_name: ".Sub" }
      }
    }
  )pb");
  ASSERT_TRUE(delimited_pool.ok()) << delimited_pool.status();
  EXPECT_EQ(
      delimited_pool->FindMessageByName("Outer").FindFieldByNumber(1).type(),
      kUpb_FieldType_Group);
  EXPECT_EQ(delimited_pool->FindMessageByName("Outer.Entry").map_value().type(),
            kUpb_FieldType_Message);

  struct InvalidCase {
    std::string_view entry_body;
    std::string_view expected_error;
  };
  constexpr InvalidCase kInvalidCases[] = {
      {"field { name: 'key' number: 1 type: TYPE_INT32 }",
       "must have exactly 2 fields"},
      {"field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 2 type: TYPE_INT32 } "
       "field { name: 'extra' number: 3 type: TYPE_INT32 }",
       "must have exactly 2 fields"},
      {"field { name: 'value' number: 2 type: TYPE_INT64 } "
       "field { name: 'key' number: 1 type: TYPE_MESSAGE type_name: '.Sub' }",
       "first field must be optional key = 1"},
      {"field { name: 'k' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "first field must be optional key = 1"},
      {"field { name: 'key' number: 1 label: LABEL_REQUIRED type: TYPE_INT32 } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "first field must be optional key = 1"},
      {"field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 3 type: TYPE_INT32 }",
       "second field must be optional value = 2"},
      {"field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'v' number: 2 type: TYPE_INT32 }",
       "second field must be optional value = 2"},
      {"field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 2 label: LABEL_REPEATED "
       "type: TYPE_INT32 }",
       "second field must be optional value = 2"},
      {"oneof_decl { name: 'o' } "
       "field { name: 'key' number: 1 type: TYPE_INT32 oneof_index: 0 } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "oneofs"},
      {"extension_range { start: 5 end: 9 } "
       "field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "extension ranges"},
      {"nested_type { name: 'N' } "
       "field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "nested types"},
      {"field { name: 'key' number: 1 type: TYPE_FLOAT } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "invalid key type"},
      {"field { name: 'key' number: 1 type: TYPE_DOUBLE } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "invalid key type"},
      {"field { name: 'key' number: 1 type: TYPE_BYTES } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "invalid key type"},
      {"field { name: 'key' number: 1 type: TYPE_ENUM type_name: '.ZeroEnum' } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "invalid key type"},
      {"field { name: 'key' number: 1 type: TYPE_MESSAGE type_name: '.Sub' } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "invalid key type"},
      {"field { name: 'key' number: 1 type: TYPE_GROUP type_name: '.Sub' } "
       "field { name: 'value' number: 2 type: TYPE_INT32 }",
       "invalid key type"},
      {"field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 2 type: TYPE_GROUP type_name: '.Sub' }",
       "value cannot be a group"},
      {"field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 2 type: TYPE_MESSAGE "
       "type_name: '.Outer.Entry' }",
       "value cannot be another map entry"},
      {"field { name: 'key' number: 1 type: TYPE_INT32 } "
       "field { name: 'value' number: 2 type: TYPE_ENUM "
       "type_name: '.NonZeroEnum' }",
       "must define 0 as the first value"},
  };
  for (const InvalidCase& test_case : kInvalidCases) {
    SCOPED_TRACE(test_case.entry_body);
    absl::Status status = LoadMapEntry(test_case.entry_body);
    EXPECT_FALSE(status.ok());
    EXPECT_THAT(status.message(), HasSubstr(test_case.expected_error));
  }
}

TEST(ReflectionTest, TooManyExtensionsFailGracefully) {
  const auto make_file = [](int top_level_exts, int nested_exts) {
    google::protobuf::FileDescriptorProto file;
    file.set_name("F");
    file.set_syntax("proto2");
    google::protobuf::DescriptorProto* msg = file.add_message_type();
    msg->set_name("M");
    google::protobuf::DescriptorProto::ExtensionRange* range = msg->add_extension_range();
    range->set_start(1);
    range->set_end(kUpb_MaxFieldNumber + 1);

    int num = 1;
    const auto add_ext = [&](google::protobuf::FieldDescriptorProto* ext) {
      ext->set_name(absl::StrCat("e", num));
      ext->set_extendee(".M");
      ext->set_number(num++);
      ext->set_label(google::protobuf::FieldDescriptorProto::LABEL_OPTIONAL);
      ext->set_type(google::protobuf::FieldDescriptorProto::TYPE_INT32);
    };
    file.mutable_extension()->Reserve(top_level_exts);
    for (int i = 0; i < top_level_exts; ++i) add_ext(file.add_extension());
    msg->mutable_extension()->Reserve(nested_exts);
    for (int i = 0; i < nested_exts; ++i) add_ext(msg->add_extension());
    return file;
  };

  // UINT16_MAX extensions is ok.
  {
    upb::Arena arena;
    upb::DefPool pool;
    upb::Status status;
    ASSERT_TRUE(pool.AddFile(
        ToUpbDescriptorSet(make_file(UINT16_MAX - 1, 1), arena), &status))
        << status.error_message();
    EXPECT_TRUE(pool.FindExtensionByName("M.e65535"));
  }

  // Exceeding UINT16_MAX extensions is rejected.
  for (int extra = 1; extra <= 2; ++extra) {
    upb::Arena arena;
    upb::DefPool pool;
    upb::Status status;
    EXPECT_FALSE(pool.AddFile(
        ToUpbDescriptorSet(make_file(UINT16_MAX, extra), arena), &status));
    EXPECT_THAT(status.error_message(),
                HasSubstr("Extensions in file exceed the limit of 65535"));
  }
}

TEST(ReflectionTest, ValidateWellKnownTypes) {
  absl::StatusOr<upb::DefPool> valid_pool = LoadDescriptorProto(R"(
    syntax: "proto3" name: "wkt.proto" package: "google.protobuf"
    enum_type { name: "NullValue" value { name: "NULL_VALUE" number: 0 } }
    message_type { name: "Any"
      field { name: "type_url" number: 1 label: LABEL_OPTIONAL type: TYPE_STRING }
      field { name: "value" number: 2 label: LABEL_OPTIONAL type: TYPE_BYTES } }
    message_type { name: "FieldMask"
      field { name: "paths" number: 1 label: LABEL_REPEATED type: TYPE_STRING } }
    message_type { name: "Duration"
      field { name: "seconds" number: 1 label: LABEL_OPTIONAL type: TYPE_INT64 }
      field { name: "nanos" number: 2 label: LABEL_OPTIONAL type: TYPE_INT32 } }
    message_type { name: "Timestamp"
      field { name: "seconds" number: 1 label: LABEL_OPTIONAL type: TYPE_INT64 }
      field { name: "nanos" number: 2 label: LABEL_OPTIONAL type: TYPE_INT32 } }
    message_type { name: "DoubleValue"
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_DOUBLE } }
    message_type { name: "FloatValue"
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_FLOAT } }
    message_type { name: "Int64Value"
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_INT64 } }
    message_type { name: "UInt64Value"
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_UINT64 } }
    message_type { name: "Int32Value"
      oneof_decl { name: "_value" }
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32
              oneof_index: 0 proto3_optional: true } }
    message_type { name: "UInt32Value"
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_UINT32 } }
    message_type { name: "BoolValue"
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_BOOL } }
    message_type { name: "StringValue"
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_STRING } }
    message_type { name: "BytesValue"
      field { name: "value" number: 1 label: LABEL_OPTIONAL type: TYPE_BYTES } }
    message_type { name: "Struct"
      field { name: "fields" number: 1 label: LABEL_REPEATED type: TYPE_MESSAGE
              type_name: ".google.protobuf.Struct.FieldsEntry" }
      nested_type { name: "FieldsEntry" options { map_entry: true }
        field { name: "key" number: 1 label: LABEL_OPTIONAL type: TYPE_STRING }
        field { name: "value" number: 2 label: LABEL_OPTIONAL type: TYPE_MESSAGE
                type_name: ".google.protobuf.Value" } } }
    message_type { name: "Value" oneof_decl { name: "kind" }
      field { name: "null_value" number: 1 label: LABEL_OPTIONAL type: TYPE_ENUM
              type_name: ".google.protobuf.NullValue" oneof_index: 0 }
      field { name: "number_value" number: 2 label: LABEL_OPTIONAL
              type: TYPE_DOUBLE oneof_index: 0 }
      field { name: "string_value" number: 3 label: LABEL_OPTIONAL
              type: TYPE_STRING oneof_index: 0 }
      field { name: "bool_value" number: 4 label: LABEL_OPTIONAL type: TYPE_BOOL
              oneof_index: 0 }
      field { name: "struct_value" number: 5 label: LABEL_OPTIONAL
              type: TYPE_MESSAGE type_name: ".google.protobuf.Struct"
              oneof_index: 0 }
      field { name: "list_value" number: 6 label: LABEL_OPTIONAL
              type: TYPE_MESSAGE type_name: ".google.protobuf.ListValue"
              oneof_index: 0 } }
    message_type { name: "ListValue"
      field { name: "values" number: 1 label: LABEL_REPEATED type: TYPE_MESSAGE
              type_name: ".google.protobuf.Value" } }
  )");
  ASSERT_TRUE(valid_pool.ok()) << valid_pool.status();

  const struct {
    const char* name;
    upb_WellKnown expected;
  } kExpected[] = {
      {"google.protobuf.Any", kUpb_WellKnown_Any},
      {"google.protobuf.FieldMask", kUpb_WellKnown_FieldMask},
      {"google.protobuf.Duration", kUpb_WellKnown_Duration},
      {"google.protobuf.Timestamp", kUpb_WellKnown_Timestamp},
      {"google.protobuf.DoubleValue", kUpb_WellKnown_DoubleValue},
      {"google.protobuf.FloatValue", kUpb_WellKnown_FloatValue},
      {"google.protobuf.Int64Value", kUpb_WellKnown_Int64Value},
      {"google.protobuf.UInt64Value", kUpb_WellKnown_UInt64Value},
      {"google.protobuf.Int32Value", kUpb_WellKnown_Int32Value},
      {"google.protobuf.UInt32Value", kUpb_WellKnown_UInt32Value},
      {"google.protobuf.BoolValue", kUpb_WellKnown_BoolValue},
      {"google.protobuf.StringValue", kUpb_WellKnown_StringValue},
      {"google.protobuf.BytesValue", kUpb_WellKnown_BytesValue},
      {"google.protobuf.Value", kUpb_WellKnown_Value},
      {"google.protobuf.ListValue", kUpb_WellKnown_ListValue},
      {"google.protobuf.Struct", kUpb_WellKnown_Struct},
  };
  for (const auto& tc : kExpected) {
    SCOPED_TRACE(tc.name);
    EXPECT_EQ(valid_pool->FindMessageByName(tc.name).wellknowntype(),
              tc.expected);
  }

  const struct {
    const char* name;
    std::string_view file_body;
  } kNonCanonicalCases[] = {
      {"google.protobuf.Any",
       "package: 'google.protobuf' message_type { name: 'Any' "
       "field { name: 'type_url' number: 1 label: LABEL_OPTIONAL "
       "type: TYPE_STRING } }"},
      {"google.protobuf.Any",
       "package: 'google.protobuf' message_type { name: 'Any' "
       "options { map_entry: true } "
       "field { name: 'key' number: 1 label: LABEL_OPTIONAL "
       "type: TYPE_STRING } "
       "field { name: 'value' number: 2 label: LABEL_OPTIONAL "
       "type: TYPE_BYTES } }"},
      {"google.protobuf.FieldMask",
       "package: 'google.protobuf' message_type { name: 'FieldMask' "
       "field { name: 'paths' number: 1 label: LABEL_OPTIONAL "
       "type: TYPE_STRING } }"},
      {"google.protobuf.Timestamp",
       "package: 'google.protobuf' message_type { name: 'Timestamp' "
       "field { name: 'seconds' number: 1 label: LABEL_OPTIONAL "
       "type: TYPE_SINT64 } "
       "field { name: 'nanos' number: 2 label: LABEL_OPTIONAL "
       "type: TYPE_INT32 } }"},
      {"google.protobuf.Duration",
       "package: 'google.protobuf' message_type { name: 'Duration' "
       "oneof_decl { name: 'o' } "
       "field { name: 'seconds' number: 1 label: LABEL_OPTIONAL "
       "type: TYPE_INT64 oneof_index: 0 } "
       "field { name: 'nanos' number: 2 label: LABEL_OPTIONAL "
       "type: TYPE_INT32 } }"},
      {"google.protobuf.Int32Value",
       "package: 'google.protobuf' message_type { name: 'Int32Value' "
       "field { name: 'value' number: 1 label: LABEL_REQUIRED "
       "type: TYPE_INT32 } }"},
      {"google.protobuf.Struct",
       "package: 'google.protobuf' message_type { name: 'Value' } "
       "message_type { name: 'Struct' "
       "field { name: 'wrong_name' number: 1 label: LABEL_REPEATED "
       "type: TYPE_MESSAGE type_name: '.google.protobuf.Struct.E' } "
       "nested_type { name: 'E' options { map_entry: true } "
       "field { name: 'key' number: 1 label: LABEL_OPTIONAL "
       "type: TYPE_STRING } "
       "field { name: 'value' number: 2 label: LABEL_OPTIONAL "
       "type: TYPE_MESSAGE type_name: '.google.protobuf.Value' } } }"},
      {"google.protobuf.Struct",
       "package: 'google.protobuf' message_type { name: 'Value' } "
       "message_type { name: 'Struct' "
       "field { name: 'fields' number: 1 label: LABEL_OPTIONAL "
       "type: TYPE_MESSAGE type_name: '.google.protobuf.Value' } }"},
      {"google.protobuf.ListValue",
       "package: 'google.protobuf' message_type { name: 'Sub' } "
       "message_type { name: 'ListValue' "
       "field { name: 'values' number: 1 label: LABEL_REPEATED "
       "type: TYPE_MESSAGE type_name: '.google.protobuf.Sub' } }"},
      {"google.protobuf.Value",
       "package: 'google.protobuf' "
       "enum_type { name: 'NullValue' value { name: 'N' number: 0 } } "
       "message_type { name: 'ListValue' } "
       "message_type { name: 'Value' oneof_decl { name: 'kind' } "
       "field { name: 'a' number: 1 label: LABEL_OPTIONAL type: TYPE_ENUM "
       "type_name: '.google.protobuf.NullValue' oneof_index: 0 } "
       "field { name: 'b' number: 2 label: LABEL_OPTIONAL type: TYPE_DOUBLE "
       "oneof_index: 0 } "
       "field { name: 'c' number: 3 label: LABEL_OPTIONAL type: TYPE_STRING "
       "oneof_index: 0 } "
       "field { name: 'd' number: 4 label: LABEL_OPTIONAL type: TYPE_BOOL "
       "oneof_index: 0 } "
       "field { name: 'e' number: 5 label: LABEL_OPTIONAL type: TYPE_MESSAGE "
       "type_name: '.google.protobuf.Value' oneof_index: 0 } "
       "field { name: 'f' number: 6 label: LABEL_OPTIONAL type: TYPE_MESSAGE "
       "type_name: '.google.protobuf.ListValue' oneof_index: 0 } }"},
      {"google.protobuf.BoolValue",
       "package: 'google' message_type { name: 'protobuf' "
       "nested_type { name: 'BoolValue' "
       "field { name: 'value' number: 1 label: LABEL_OPTIONAL "
       "type: TYPE_BOOL } } }"},
  };
  for (const auto& tc : kNonCanonicalCases) {
    SCOPED_TRACE(tc.file_body);
    absl::StatusOr<upb::DefPool> pool = LoadDescriptorProto(
        absl::StrFormat("syntax: 'proto2' name: 'f.proto' %s", tc.file_body));
    ASSERT_TRUE(pool.ok()) << pool.status();
    EXPECT_EQ(pool->FindMessageByName(tc.name).wellknowntype(),
              kUpb_WellKnown_Unspecified);
  }
}

}  // namespace
}  // namespace upb_test
