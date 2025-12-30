
#include <string>
#include <string_view>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.upb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb/test/parse_text_proto.h"

namespace upb_test {
namespace {

using testing::HasSubstr;

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

}  // namespace
}  // namespace upb_test
