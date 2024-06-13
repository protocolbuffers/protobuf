// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/retention.h"

#include <memory>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/log/absl_check.h"
#include "absl/log/die_if_null.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/parser.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/text_format.h"


namespace google {
namespace protobuf {
namespace compiler {
namespace {

MATCHER_P(EqualsProto, msg, "") {
  return msg.DebugString() == arg.DebugString();
}

class FakeErrorCollector : public io::ErrorCollector {
 public:
  FakeErrorCollector() = default;
  ~FakeErrorCollector() override = default;

  void RecordError(int line, io::ColumnNumber column,
                   absl::string_view message) override {
    ABSL_CHECK(false) << line << ":" << column << ": " << message;
  }

  void RecordWarning(int line, io::ColumnNumber column,
                     absl::string_view message) override {
    ABSL_CHECK(false) << line << ":" << column << ": " << message;
  }
};
class RetentionStripTest : public testing::Test {
 protected:
  void SetUp() override {
    FileDescriptorProto descriptor_proto_descriptor;
    FileDescriptorSet::descriptor()->file()->CopyTo(
        &descriptor_proto_descriptor);
    pool_.BuildFile(descriptor_proto_descriptor);
  }

  const FileDescriptor* ParseSchema(absl::string_view contents,
                                    absl::string_view file_name = "foo.proto") {
    std::string proto_file = absl::Substitute(
        R"schema(
          syntax = "proto2";

          package google.protobuf.internal;

          import "$0";

          $1
        )schema",
        FileDescriptorSet::descriptor()->file()->name(), contents);
    io::ArrayInputStream input_stream(proto_file.data(),
                                      static_cast<int>(proto_file.size()));
    FakeErrorCollector error_collector;
    io::Tokenizer tokenizer(&input_stream, &error_collector);
    Parser parser;
    parser.RecordErrorsTo(&error_collector);
    FileDescriptorProto file_descriptor;
    ABSL_CHECK(parser.Parse(&tokenizer, &file_descriptor));
    file_descriptor.set_name(file_name);

    return pool_.BuildFile(file_descriptor);
  }

  template <typename ProtoType>
  ProtoType BuildDynamicProto(absl::string_view data) {
    // We use a dynamic message to generate the expected options proto. This
    // lets us parse the custom options in text format.
    const Descriptor* file_options_descriptor =
        pool_.FindMessageTypeByName(ProtoType().GetTypeName());
    DynamicMessageFactory factory;
    std::unique_ptr<Message> dynamic_message(
        factory.GetPrototype(file_options_descriptor)->New());
    ABSL_CHECK(TextFormat::ParseFromString(data, dynamic_message.get()));
    ProtoType ret;
    ABSL_CHECK(ret.ParseFromString(dynamic_message->SerializeAsString()));
    return ret;
  }

  DescriptorPool pool_;
};

TEST_F(RetentionStripTest, StripSourceRetentionFileOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      option (source_retention_option) = 123;
      option (options) = {
        i1: 123
        i2: 456
        c { s: "abc" }
        rc { s: "abc" }
      };
      option (repeated_options) = {
        i1: 111 i2: 222
      };

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.FileOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })schema");

  FileOptions expected_options = BuildDynamicProto<FileOptions>(
      R"pb(
        [google.protobuf.internal.options] {
          i2: 456
          c {}
          rc {}
        }
        [google.protobuf.internal.repeated_options] { i2: 222 })pb");

  FileDescriptorProto stripped_file = StripSourceRetentionOptions(*file);

  EXPECT_THAT(StripSourceRetentionOptions(*file).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripLocalSourceRetentionOptions(*file),
              EqualsProto(expected_options));
}

TEST_F(RetentionStripTest, StripSourceRetentionProtoFileOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      option (source_retention_option) = 123;
      option (options) = {
        i1: 123
        i2: 456
        c { s: "abc" }
        rc { s: "abc" }
      };
      option (repeated_options) = {
        i1: 111 i2: 222
      };

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.FileOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      }
  )schema");

  FileDescriptorProto proto;
  file->CopyTo(&proto);

  ASSERT_THAT(proto.options(), EqualsProto(BuildDynamicProto<FileOptions>(R"pb(
                [google.protobuf.internal.source_retention_option]: 123
                [google.protobuf.internal.options] {
                  i1: 123
                  i2: 456
                  c { s: "abc" }
                  rc { s: "abc" }
                }
                [google.protobuf.internal.repeated_options] { i1: 111 i2: 222 })pb")));

  StripSourceRetentionOptions(*file->pool(), proto);

  EXPECT_THAT(proto.options(), EqualsProto(BuildDynamicProto<FileOptions>(R"pb(
                [google.protobuf.internal.options] {
                  i2: 456
                  c {}
                  rc {}
                }
                [google.protobuf.internal.repeated_options] { i2: 222 })pb")));
}

TEST_F(RetentionStripTest, StripSourceRetentionMessageOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      message TestMessage {
        option (source_retention_option) = 123;
        option (options) = {
          i1: 123
          i2: 456
          c { s: "abc" }
          rc { s: "abc" }
        };
        option (repeated_options) = {
          i1: 111 i2: 222
        };
      }

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.MessageOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })schema");

  MessageOptions expected_options = BuildDynamicProto<MessageOptions>(
      R"pb(
        [google.protobuf.internal.options] {
          i2: 456
          c {}
          rc {}
        }
        [google.protobuf.internal.repeated_options] { i2: 222 })pb");
  const Descriptor* message =
      ABSL_DIE_IF_NULL(file->FindMessageTypeByName("TestMessage"));

  EXPECT_THAT(StripSourceRetentionOptions(*file).message_type(0).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*message).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripLocalSourceRetentionOptions(*message),
              EqualsProto(expected_options));
}

TEST_F(RetentionStripTest, StripSourceRetentionEnumOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      enum TestEnum {
        option (source_retention_option) = 123;
        option (options) = {
          i1: 123
          i2: 456
          c { s: "abc" }
          rc { s: "abc" }
        };
        option (repeated_options) = {
          i1: 111 i2: 222
        };
        VALUE1 = 0;
      }

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.EnumOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })schema");

  EnumOptions expected_options = BuildDynamicProto<EnumOptions>(
      R"pb(
        [google.protobuf.internal.options] {
          i2: 456
          c {}
          rc {}
        }
        [google.protobuf.internal.repeated_options] { i2: 222 })pb");
  const EnumDescriptor* enm =
      ABSL_DIE_IF_NULL(file->FindEnumTypeByName("TestEnum"));

  EXPECT_THAT(StripSourceRetentionOptions(*file).enum_type(0).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*enm).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripLocalSourceRetentionOptions(*enm),
              EqualsProto(expected_options));
}

TEST_F(RetentionStripTest, StripSourceRetentionEnumValueOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      enum TestEnum {
        VALUE1 = 0 [(source_retention_option) = 123, (options) = {
          i1: 123
          i2: 456
          c { s: "abc" }
          rc { s: "abc" }
        }, (repeated_options) = {
          i1: 111 i2: 222
        }];
      }

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.EnumValueOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })schema");

  EnumValueOptions expected_options = BuildDynamicProto<EnumValueOptions>(
      R"pb(
        [google.protobuf.internal.options] {
          i2: 456
          c {}
          rc {}
        }
        [google.protobuf.internal.repeated_options] { i2: 222 })pb");
  const EnumDescriptor* enm =
      ABSL_DIE_IF_NULL(file->FindEnumTypeByName("TestEnum"));
  const EnumValueDescriptor* value = ABSL_DIE_IF_NULL(enm->value(0));

  EXPECT_THAT(
      StripSourceRetentionOptions(*file).enum_type(0).value(0).options(),
      EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*enm).value(0).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripLocalSourceRetentionOptions(*value),
              EqualsProto(expected_options));
}

TEST_F(RetentionStripTest, StripSourceRetentionFieldOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      message TestMessage {
        optional string test_field = 1 [(source_retention_option) = 123, (options) = {
          i1: 123
          i2: 456
          c { s: "abc" }
          rc { s: "abc" }
        }, (repeated_options) = {
          i1: 111 i2: 222
        }];
      }

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.FieldOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })schema");

  FieldOptions expected_options = BuildDynamicProto<FieldOptions>(
      R"pb(
        [google.protobuf.internal.options] {
          i2: 456
          c {}
          rc {}
        }
        [google.protobuf.internal.repeated_options] { i2: 222 })pb");
  const Descriptor* message =
      ABSL_DIE_IF_NULL(file->FindMessageTypeByName("TestMessage"));
  const FieldDescriptor* field =
      ABSL_DIE_IF_NULL(message->FindFieldByName("test_field"));

  EXPECT_THAT(
      StripSourceRetentionOptions(*file).message_type(0).field(0).options(),
      EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*message).field(0).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*field).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripLocalSourceRetentionOptions(*field),
              EqualsProto(expected_options));
}

TEST_F(RetentionStripTest, StripSourceRetentionExtensionOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      message TestMessage {
        extensions 1;
      }

      extend TestMessage {
        optional string test_field = 1 [(source_retention_option) = 123, (options) = {
          i1: 123
          i2: 456
          c { s: "abc" }
          rc { s: "abc" }
        }, (repeated_options) = {
          i1: 111 i2: 222
        }];
      }

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.FieldOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })schema");

  FieldOptions expected_options = BuildDynamicProto<FieldOptions>(
      R"pb(
        [google.protobuf.internal.options] {
          i2: 456
          c {}
          rc {}
        }
        [google.protobuf.internal.repeated_options] { i2: 222 })pb");
  const FieldDescriptor* field =
      ABSL_DIE_IF_NULL(file->FindExtensionByName("test_field"));

  EXPECT_THAT(StripSourceRetentionOptions(*file).extension(0).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*field).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripLocalSourceRetentionOptions(*field),
              EqualsProto(expected_options));
}

TEST_F(RetentionStripTest, StripSourceRetentionOneofOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      message TestMessage {
        oneof test_oneof {
        option (source_retention_option) = 123;
        option (options) = {
          i1: 123
          i2: 456
          c { s: "abc" }
          rc { s: "abc" }
        };
        option (repeated_options) = {
          i1: 111 i2: 222
        };
          string field1 = 1;
          string field2 = 2;
        };
      }

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.OneofOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })schema");

  OneofOptions expected_options = BuildDynamicProto<OneofOptions>(
      R"pb(
        [google.protobuf.internal.options] {
          i2: 456
          c {}
          rc {}
        }
        [google.protobuf.internal.repeated_options] { i2: 222 })pb");
  const Descriptor* message =
      ABSL_DIE_IF_NULL(file->FindMessageTypeByName("TestMessage"));
  const OneofDescriptor* oneof =
      ABSL_DIE_IF_NULL(message->FindOneofByName("test_oneof"));

  EXPECT_THAT(StripSourceRetentionOptions(*file)
                  .message_type(0)
                  .oneof_decl(0)
                  .options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*message).oneof_decl(0).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*oneof).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripLocalSourceRetentionOptions(*oneof),
              EqualsProto(expected_options));
}

TEST_F(RetentionStripTest, StripSourceRetentionExtensionRangeOptions) {
  const FileDescriptor* file = ParseSchema(R"schema(
      message TestMessage {
        extensions 1 to max [(source_retention_option) = 123, (options) = {
          i1: 123
          i2: 456
          c { s: "abc" }
          rc { s: "abc" }
        }, (repeated_options) = {
          i1: 111 i2: 222
        }];
      }

      message Options {
        optional int32 i1 = 1 [retention = RETENTION_SOURCE];
        optional int32 i2 = 2;
        message ChildMessage {
          optional string s = 1 [retention = RETENTION_SOURCE];
        }
        optional ChildMessage c = 3;
        repeated ChildMessage rc = 4;
      }

      extend google.protobuf.ExtensionRangeOptions {
        optional int32 source_retention_option = 50000 [retention = RETENTION_SOURCE];
        optional Options options = 50001;
        repeated Options repeated_options = 50002;
      })schema");

  ExtensionRangeOptions expected_options =
      BuildDynamicProto<ExtensionRangeOptions>(
          R"pb(
            [google.protobuf.internal.options] {
              i2: 456
              c {}
              rc {}
            }
            [google.protobuf.internal.repeated_options] { i2: 222 })pb");
  const Descriptor* message =
      ABSL_DIE_IF_NULL(file->FindMessageTypeByName("TestMessage"));
  const Descriptor::ExtensionRange* range =
      ABSL_DIE_IF_NULL(message->FindExtensionRangeContainingNumber(2));

  EXPECT_THAT(StripSourceRetentionOptions(*file)
                  .message_type(0)
                  .extension_range(0)
                  .options(),
              EqualsProto(expected_options));
  EXPECT_THAT(
      StripSourceRetentionOptions(*message).extension_range(0).options(),
      EqualsProto(expected_options));
  EXPECT_THAT(StripSourceRetentionOptions(*message, *range).options(),
              EqualsProto(expected_options));
  EXPECT_THAT(StripLocalSourceRetentionOptions(*message, *range),
              EqualsProto(expected_options));
}


}  // namespace
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
