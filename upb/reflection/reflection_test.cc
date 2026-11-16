// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <climits>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.upb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "google/protobuf/unittest.upbdefs.h"
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/mem/alloc.h"
#include "upb/mem/arena.hpp"
#include "upb/message/array.h"
#include "upb/message/map.h"
#include "upb/message/message.h"
#include "upb/message/value.h"
#include "upb/mini_table/message.h"
#include "upb/reflection/def.h"
#include "upb/reflection/def.hpp"
#include "upb/reflection/internal/def_pool.h"
#include "upb/reflection/message.h"
#include "upb/test/parse_text_proto.h"
#include "upb/wire/decode.h"
#include "upb/wire/encode.h"

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
  std::string json_name;
};

static absl::StatusOr<upb::DefPool> LoadEnumDescriptorWithValues(
    absl::Span<const TestEnumValueSpec> values) {
  google::protobuf::FileDescriptorProto file_proto;
  file_proto.set_name("test.proto");
  file_proto.set_syntax("editions");
  file_proto.set_edition(google::protobuf::EDITION_2026);

  google::protobuf::EnumDescriptorProto* enum_proto = file_proto.add_enum_type();
  enum_proto->set_name("TestEnum");

  for (const TestEnumValueSpec& v : values) {
    google::protobuf::EnumValueDescriptorProto* val_proto = enum_proto->add_value();
    val_proto->set_name(v.name);
    val_proto->set_number(v.number);
    if (!v.json_name.empty()) {
      SetCustomJsonOption(val_proto->mutable_options(), v.json_name);
    }
  }

  google::protobuf::FileDescriptorSet set;
  *set.add_file() = file_proto;
  return LoadDescriptorSetFromProto(set);
}

TEST(ReflectionTest, EnumCustomJsonNameConflictDifferentNumberFails) {
  // Two enum values with DIFFERENT numbers (1 and 2) sharing the same custom
  // JSON name must fail descriptor validation.
  absl::Status status =
      LoadEnumDescriptorWithValues({
                                       {"VAL_ZERO", 0, ""},
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
  absl::Status status =
      LoadEnumDescriptorWithValues({
                                       {"VAL_ZERO", 0, ""},
                                       {"VAL_A", 1, "custom_name"},
                                       {"VAL_B", 1, "custom_name"},
                                   })
          .status();
  EXPECT_TRUE(status.ok()) << status.message();
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

upb::DefPool LoadDiscardUnknownTestPool() {
  absl::StatusOr<upb::DefPool> pool = LoadDescriptorProto(R"pb(
    syntax: "proto2"
    name: "discard_unknown_test.proto"
    message_type {
      name: "Node"
      extension_range { start: 1000 end: 2000 }
      field {
        name: "child"
        number: 1
        label: LABEL_OPTIONAL
        type: TYPE_MESSAGE
        type_name: ".Node"
      }
      field {
        name: "children"
        number: 2
        label: LABEL_REPEATED
        type: TYPE_MESSAGE
        type_name: ".Node"
      }
      field {
        name: "node_map"
        number: 3
        label: LABEL_REPEATED
        type: TYPE_MESSAGE
        type_name: ".Node.NodeMapEntry"
      }
      nested_type {
        name: "NodeMapEntry"
        options { map_entry: true }
        field { name: "key" number: 1 label: LABEL_OPTIONAL type: TYPE_INT32 }
        field {
          name: "value"
          number: 2
          label: LABEL_OPTIONAL
          type: TYPE_MESSAGE
          type_name: ".Node"
        }
      }
    }
    extension {
      name: "ext_child"
      number: 1000
      label: LABEL_OPTIONAL
      type: TYPE_MESSAGE
      type_name: ".Node"
      extendee: ".Node"
    }
  )pb");
  ABSL_CHECK_OK(pool);
  return *std::move(pool);
}

[[nodiscard]] bool AddUnknownField(upb_Message* msg, const upb_MiniTable* mt,
                                   upb::Arena& arena) {
  // Unknown varint field 111 = 1.
  static constexpr char kUnknownVarint[] = {'\xF8', '\x06', '\x01'};
  if (upb_Decode(kUnknownVarint, sizeof(kUnknownVarint), msg, mt, nullptr, 0,
                 arena.ptr()) != kUpb_DecodeStatus_Ok) {
    return false;
  }
  return upb_Message_HasUnknown(msg);
}

TEST(ReflectionTest, DiscardUnknownNegativeMaxDepth) {
  upb::DefPool pool = LoadDiscardUnknownTestPool();
  upb::MessageDefPtr m = pool.FindMessageByName("Node");
  ASSERT_TRUE(m);
  const upb_MiniTable* mt = upb_MessageDef_MiniTable(m.ptr());
  upb::Arena arena;

  upb_Message* msg = upb_Message_New(mt, arena.ptr());
  ASSERT_NE(msg, nullptr);
  ASSERT_TRUE(AddUnknownField(msg, mt, arena));

  EXPECT_FALSE(upb_Message_DiscardUnknown(msg, m.ptr(), nullptr, -1));
  EXPECT_TRUE(upb_Message_HasUnknown(msg));

  EXPECT_FALSE(upb_Message_DiscardUnknown(msg, m.ptr(), nullptr, INT_MIN));
  EXPECT_TRUE(upb_Message_HasUnknown(msg));
}

TEST(ReflectionTest, DiscardUnknownZeroMaxDepth) {
  upb::DefPool pool = LoadDiscardUnknownTestPool();
  upb::MessageDefPtr m = pool.FindMessageByName("Node");
  ASSERT_TRUE(m);
  upb::FieldDefPtr child_f = m.FindFieldByName("child");
  upb::FieldDefPtr children_f = m.FindFieldByName("children");
  upb::FieldDefPtr map_f = m.FindFieldByName("node_map");
  ASSERT_TRUE(child_f);
  ASSERT_TRUE(children_f);
  ASSERT_TRUE(map_f);
  const upb_MiniTable* mt = upb_MessageDef_MiniTable(m.ptr());
  upb::Arena arena;

  // Leaf message with empty containers succeeds with maxdepth == 0.
  upb_Message* leaf = upb_Message_New(mt, arena.ptr());
  ASSERT_NE(leaf, nullptr);
  ASSERT_NE(upb_Message_Mutable(leaf, children_f.ptr(), arena.ptr()).array,
            nullptr);
  ASSERT_NE(upb_Message_Mutable(leaf, map_f.ptr(), arena.ptr()).map, nullptr);
  ASSERT_TRUE(AddUnknownField(leaf, mt, arena));

  EXPECT_TRUE(upb_Message_DiscardUnknown(leaf, m.ptr(), nullptr, 0));
  EXPECT_FALSE(upb_Message_HasUnknown(leaf));

  // Message with a child submessage clears root, leaves child untouched, and
  // returns false with maxdepth == 0.
  upb_Message* root = upb_Message_New(mt, arena.ptr());
  ASSERT_NE(root, nullptr);
  ASSERT_TRUE(AddUnknownField(root, mt, arena));
  upb_Message* child =
      upb_Message_Mutable(root, child_f.ptr(), arena.ptr()).msg;
  ASSERT_NE(child, nullptr);
  ASSERT_TRUE(AddUnknownField(child, mt, arena));

  EXPECT_FALSE(upb_Message_DiscardUnknown(root, m.ptr(), nullptr, 0));
  EXPECT_FALSE(upb_Message_HasUnknown(root));
  EXPECT_TRUE(upb_Message_HasUnknown(child));
}

TEST(ReflectionTest, DiscardUnknownMatchesDefaultDecodeDepth) {
  upb::DefPool pool = LoadDiscardUnknownTestPool();
  upb::MessageDefPtr m = pool.FindMessageByName("Node");
  ASSERT_TRUE(m);
  upb::FieldDefPtr child_f = m.FindFieldByName("child");
  ASSERT_TRUE(child_f);
  const upb_MiniTable* mt = upb_MessageDef_MiniTable(m.ptr());
  upb::Arena arena;

  const int kDefaultDepth =
      static_cast<int>(upb_EncodeOptions_GetEffectiveMaxDepth(0));
  ASSERT_EQ(kDefaultDepth, 100);

  // Build root + 100 nested submessages (levels 0..100), each with an unknown
  // field.
  upb_Message* root = upb_Message_New(mt, arena.ptr());
  ASSERT_NE(root, nullptr);
  upb_Message* cur = root;
  for (int i = 0; i <= kDefaultDepth; ++i) {
    ASSERT_TRUE(AddUnknownField(cur, mt, arena));
    if (i < kDefaultDepth) {
      cur = upb_Message_Mutable(cur, child_f.ptr(), arena.ptr()).msg;
      ASSERT_NE(cur, nullptr);
    }
  }

  // Round-trip via default upb_Encode and upb_Decode (options == 0).
  char* buf = nullptr;
  size_t size = 0;
  ASSERT_EQ(upb_Encode(root, mt, 0, arena.ptr(), &buf, &size),
            kUpb_EncodeStatus_Ok);

  upb_Message* decoded = upb_Message_New(mt, arena.ptr());
  ASSERT_NE(decoded, nullptr);
  ASSERT_EQ(upb_Decode(buf, size, decoded, mt, nullptr, 0, arena.ptr()),
            kUpb_DecodeStatus_Ok);

  EXPECT_TRUE(
      upb_Message_DiscardUnknown(decoded, m.ptr(), nullptr, kDefaultDepth));

  const upb_Message* node = decoded;
  for (int i = 0; i <= kDefaultDepth; ++i) {
    EXPECT_FALSE(upb_Message_HasUnknown(node)) << "at depth " << i;
    if (i < kDefaultDepth) {
      node = upb_Message_GetFieldByDef(node, child_f.ptr()).msg_val;
      ASSERT_NE(node, nullptr) << "at depth " << i;
    }
  }
}

TEST(ReflectionTest, DiscardUnknownExceedingMaxDepth) {
  upb::DefPool pool = LoadDiscardUnknownTestPool();
  upb::MessageDefPtr m = pool.FindMessageByName("Node");
  ASSERT_TRUE(m);
  upb::FieldDefPtr child_f = m.FindFieldByName("child");
  ASSERT_TRUE(child_f);
  const upb_MiniTable* mt = upb_MessageDef_MiniTable(m.ptr());
  upb::Arena arena;

  const int kMaxDepth =
      static_cast<int>(upb_EncodeOptions_GetEffectiveMaxDepth(0));
  // Build root + 101 nested submessages (levels 0..101).
  upb_Message* root = upb_Message_New(mt, arena.ptr());
  ASSERT_NE(root, nullptr);
  upb_Message* cur = root;
  for (int i = 0; i <= kMaxDepth + 1; ++i) {
    ASSERT_TRUE(AddUnknownField(cur, mt, arena));
    if (i < kMaxDepth + 1) {
      cur = upb_Message_Mutable(cur, child_f.ptr(), arena.ptr()).msg;
      ASSERT_NE(cur, nullptr);
    }
  }

  EXPECT_FALSE(upb_Message_DiscardUnknown(root, m.ptr(), nullptr, kMaxDepth));

  const upb_Message* node = root;
  for (int i = 0; i <= kMaxDepth + 1; ++i) {
    if (i <= kMaxDepth) {
      EXPECT_FALSE(upb_Message_HasUnknown(node)) << "at depth " << i;
    } else {
      EXPECT_TRUE(upb_Message_HasUnknown(node)) << "at depth " << i;
    }
    if (i < kMaxDepth + 1) {
      node = upb_Message_GetFieldByDef(node, child_f.ptr()).msg_val;
      ASSERT_NE(node, nullptr) << "at depth " << i;
    }
  }
}

TEST(ReflectionTest, DiscardUnknownSubmessageKinds) {
  upb::DefPool pool = LoadDiscardUnknownTestPool();
  upb::MessageDefPtr m = pool.FindMessageByName("Node");
  ASSERT_TRUE(m);
  upb::FieldDefPtr child_f = m.FindFieldByName("child");
  upb::FieldDefPtr children_f = m.FindFieldByName("children");
  upb::FieldDefPtr map_f = m.FindFieldByName("node_map");
  upb::FieldDefPtr ext_f = pool.FindExtensionByName("ext_child");
  ASSERT_TRUE(child_f);
  ASSERT_TRUE(children_f);
  ASSERT_TRUE(map_f);
  ASSERT_TRUE(ext_f);
  const upb_MiniTable* mt = upb_MessageDef_MiniTable(m.ptr());
  upb::Arena arena;

  // Repeated submessages: maxdepth == 1 clears level 1 and returns true.
  {
    upb_Message* root = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(AddUnknownField(root, mt, arena));
    upb_Array* arr =
        upb_Message_Mutable(root, children_f.ptr(), arena.ptr()).array;
    ASSERT_NE(arr, nullptr);
    upb_Message* elem = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(elem, nullptr);
    ASSERT_TRUE(AddUnknownField(elem, mt, arena));
    upb_MessageValue v;
    v.msg_val = elem;
    ASSERT_TRUE(upb_Array_Append(arr, v, arena.ptr()));

    EXPECT_TRUE(upb_Message_DiscardUnknown(root, m.ptr(), nullptr, 1));
    EXPECT_FALSE(upb_Message_HasUnknown(root));
    EXPECT_FALSE(upb_Message_HasUnknown(elem));
  }

  // Repeated submessages exceeding depth: returns false, preserves level-2
  // unknown field, and still clears sibling at level 1.
  {
    upb_Message* root = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(AddUnknownField(root, mt, arena));
    upb_Array* arr =
        upb_Message_Mutable(root, children_f.ptr(), arena.ptr()).array;
    ASSERT_NE(arr, nullptr);

    upb_Message* elem0 = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(elem0, nullptr);
    ASSERT_TRUE(AddUnknownField(elem0, mt, arena));
    upb_Message* grandchild =
        upb_Message_Mutable(elem0, child_f.ptr(), arena.ptr()).msg;
    ASSERT_NE(grandchild, nullptr);
    ASSERT_TRUE(AddUnknownField(grandchild, mt, arena));

    upb_Message* elem1 = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(elem1, nullptr);
    ASSERT_TRUE(AddUnknownField(elem1, mt, arena));

    upb_MessageValue v0, v1;
    v0.msg_val = elem0;
    v1.msg_val = elem1;
    ASSERT_TRUE(upb_Array_Append(arr, v0, arena.ptr()));
    ASSERT_TRUE(upb_Array_Append(arr, v1, arena.ptr()));

    EXPECT_FALSE(upb_Message_DiscardUnknown(root, m.ptr(), nullptr, 1));
    EXPECT_FALSE(upb_Message_HasUnknown(root));
    EXPECT_FALSE(upb_Message_HasUnknown(elem0));
    EXPECT_FALSE(upb_Message_HasUnknown(elem1));
    EXPECT_TRUE(upb_Message_HasUnknown(grandchild));
  }

  // Map message values: maxdepth == 1 clears level 1 and returns true.
  {
    upb_Message* root = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(AddUnknownField(root, mt, arena));
    upb_Map* map = upb_Message_Mutable(root, map_f.ptr(), arena.ptr()).map;
    ASSERT_NE(map, nullptr);
    upb_Message* val_msg = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(val_msg, nullptr);
    ASSERT_TRUE(AddUnknownField(val_msg, mt, arena));
    upb_MessageValue k, v;
    k.int32_val = 1;
    v.msg_val = val_msg;
    ASSERT_TRUE(upb_Map_Set(map, k, v, arena.ptr()));

    EXPECT_TRUE(upb_Message_DiscardUnknown(root, m.ptr(), nullptr, 1));
    EXPECT_FALSE(upb_Message_HasUnknown(root));
    EXPECT_FALSE(upb_Message_HasUnknown(val_msg));
  }

  // Map message values exceeding depth: returns false, preserves level-2
  // unknown field, and still clears sibling entry at level 1.
  {
    upb_Message* root = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(AddUnknownField(root, mt, arena));
    upb_Map* map = upb_Message_Mutable(root, map_f.ptr(), arena.ptr()).map;
    ASSERT_NE(map, nullptr);

    upb_Message* val1 = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(val1, nullptr);
    ASSERT_TRUE(AddUnknownField(val1, mt, arena));
    upb_Message* grandchild =
        upb_Message_Mutable(val1, child_f.ptr(), arena.ptr()).msg;
    ASSERT_NE(grandchild, nullptr);
    ASSERT_TRUE(AddUnknownField(grandchild, mt, arena));

    upb_Message* val2 = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(val2, nullptr);
    ASSERT_TRUE(AddUnknownField(val2, mt, arena));

    upb_MessageValue k1, v1, k2, v2;
    k1.int32_val = 1;
    v1.msg_val = val1;
    k2.int32_val = 2;
    v2.msg_val = val2;
    ASSERT_TRUE(upb_Map_Set(map, k1, v1, arena.ptr()));
    ASSERT_TRUE(upb_Map_Set(map, k2, v2, arena.ptr()));

    EXPECT_FALSE(upb_Message_DiscardUnknown(root, m.ptr(), nullptr, 1));
    EXPECT_FALSE(upb_Message_HasUnknown(root));
    EXPECT_FALSE(upb_Message_HasUnknown(val1));
    EXPECT_FALSE(upb_Message_HasUnknown(val2));
    EXPECT_TRUE(upb_Message_HasUnknown(grandchild));
  }

  // Extension submessage: maxdepth == 0 fails and leaves extension child
  // untouched; maxdepth == 1 clears both root and extension child.
  {
    upb_Message* root = upb_Message_New(mt, arena.ptr());
    ASSERT_NE(root, nullptr);
    ASSERT_TRUE(AddUnknownField(root, mt, arena));
    upb_Message* ext_child =
        upb_Message_Mutable(root, ext_f.ptr(), arena.ptr()).msg;
    ASSERT_NE(ext_child, nullptr);
    ASSERT_TRUE(AddUnknownField(ext_child, mt, arena));

    EXPECT_FALSE(upb_Message_DiscardUnknown(root, m.ptr(), pool.ptr(), 0));
    EXPECT_FALSE(upb_Message_HasUnknown(root));
    EXPECT_TRUE(upb_Message_HasUnknown(ext_child));

    EXPECT_TRUE(upb_Message_DiscardUnknown(root, m.ptr(), pool.ptr(), 1));
    EXPECT_FALSE(upb_Message_HasUnknown(ext_child));
  }
}

}  // namespace
}  // namespace upb_test
