
#include <string>
#include <string_view>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.upb.h"
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "upb/base/status.hpp"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb/test/parse_text_proto.h"

namespace upb_test {
namespace {

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

}  // namespace
}  // namespace upb_test
