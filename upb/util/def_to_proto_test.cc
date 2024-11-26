// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/util/def_to_proto.h"

#include <cstddef>
#include <memory>
#include <string>

#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.upb.h"
#include "google/protobuf/descriptor.upbdefs.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/util/message_differencer.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb/test/parse_text_proto.h"
#include "upb/util/def_to_proto_editions_test.upbdefs.h"
#include "upb/util/def_to_proto_test.h"
#include "upb/util/def_to_proto_test.upbdefs.h"

namespace upb_test {

// Loads and retrieves a descriptor for `msgdef` into the given `pool`.
const google::protobuf::Descriptor* AddMessageDescriptor(
    upb::MessageDefPtr msgdef, google::protobuf::DescriptorPool* pool) {
  upb::Arena tmp_arena;
  upb::FileDefPtr file = msgdef.file();
  google_protobuf_FileDescriptorProto* upb_proto =
      upb_FileDef_ToProto(file.ptr(), tmp_arena.ptr());
  size_t size;
  const char* buf = google_protobuf_FileDescriptorProto_serialize(
      upb_proto, tmp_arena.ptr(), &size);
  google::protobuf::FileDescriptorProto google_proto;
  google_proto.ParseFromArray(buf, size);
  const google::protobuf::FileDescriptor* file_desc =
      pool->BuildFile(google_proto);
  EXPECT_TRUE(file_desc != nullptr);
  return pool->FindMessageTypeByName(msgdef.full_name());
}

// Converts a upb `msg` (with type `msgdef`) into a protobuf Message object from
// the given factory and descriptor.
std::unique_ptr<google::protobuf::Message> ToProto(
    const upb_Message* msg, const upb_MessageDef* msgdef,
    const google::protobuf::Descriptor* desc,
    google::protobuf::MessageFactory* factory) {
  upb::Arena arena;
  EXPECT_TRUE(desc != nullptr);
  std::unique_ptr<google::protobuf::Message> google_msg(
      factory->GetPrototype(desc)->New());
  char* buf;
  size_t size;
  upb_EncodeStatus status = upb_Encode(msg, upb_MessageDef_MiniTable(msgdef), 0,
                                       arena.ptr(), &buf, &size);
  EXPECT_EQ(status, kUpb_EncodeStatus_Ok);
  google_msg->ParseFromArray(buf, size);
  return google_msg;
}

// A gtest matcher that verifies that a proto is equal to `proto`.  Both `proto`
// and `arg` must be messages of type `msgdef_func` (a .upbdefs.h function that
// loads a known msgdef into the given defpool).
MATCHER_P2(EqualsUpbProto, proto, msgdef_func,
           negation ? "are not equal" : "are equal") {
  upb::DefPool defpool;
  google::protobuf::DescriptorPool pool;
  google::protobuf::DynamicMessageFactory factory;
  upb::MessageDefPtr msgdef(msgdef_func(defpool.ptr()));
  EXPECT_TRUE(msgdef.ptr() != nullptr);
  const google::protobuf::Descriptor* desc =
      AddMessageDescriptor(msgdef, &pool);
  EXPECT_TRUE(desc != nullptr);
  std::unique_ptr<google::protobuf::Message> m1(
      ToProto(UPB_UPCAST(proto), msgdef.ptr(), desc, &factory));
  std::unique_ptr<google::protobuf::Message> m2(
      ToProto((upb_Message*)arg, msgdef.ptr(), desc, &factory));
  std::string differences;
  google::protobuf::util::MessageDifferencer differencer;
  differencer.ReportDifferencesToString(&differences);
  bool eq = differencer.Compare(*m2, *m1);
  if (!eq) {
    *result_listener << differences;
  }
  return eq;
}

// Verifies that the given upb FileDef can be converted to a proto that matches
// `proto`.
void CheckFile(const upb::FileDefPtr file,
               const google_protobuf_FileDescriptorProto* proto) {
  upb::Arena arena;
  google_protobuf_FileDescriptorProto* proto2 =
      upb_FileDef_ToProto(file.ptr(), arena.ptr());
  ASSERT_THAT(
      proto,
      EqualsUpbProto(proto2, google_protobuf_FileDescriptorProto_getmsgdef));
}

// Verifies that upb/util/def_to_proto_test.proto can round-trip:
//   serialized descriptor -> upb def -> serialized descriptor
TEST(DefToProto, Test) {
  upb::Arena arena;
  upb::DefPool defpool;
  upb_StringView test_file_desc =
      upb_util_def_to_proto_test_proto_upbdefinit.descriptor;
  const auto* file_desc = google_protobuf_FileDescriptorProto_parse(
      test_file_desc.data, test_file_desc.size, arena.ptr());

  upb::MessageDefPtr msgdef(pkg_Message_getmsgdef(defpool.ptr()));
  upb::FileDefPtr file = msgdef.file();
  CheckFile(file, file_desc);
}

// Verifies that editions don't leak out legacy feature APIs (e.g. TYPE_GROUP
// and LABEL_REQUIRED):
//   serialized descriptor -> upb def -> serialized descriptor
TEST(DefToProto, TestEditionsLegacyFeatures) {
  upb::Arena arena;
  upb::DefPool defpool;
  upb_StringView test_file_desc =
      upb_util_def_to_proto_editions_test_proto_upbdefinit
          .descriptor;
  const auto* file = google_protobuf_FileDescriptorProto_parse(
      test_file_desc.data, test_file_desc.size, arena.ptr());

  size_t size;
  const auto* messages = google_protobuf_FileDescriptorProto_message_type(file, &size);
  ASSERT_EQ(size, 1);
  const auto* fields = google_protobuf_DescriptorProto_field(messages[0], &size);
  ASSERT_EQ(size, 2);
  EXPECT_EQ(google_protobuf_FieldDescriptorProto_label(fields[0]),
            google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL);
  EXPECT_EQ(google_protobuf_FieldDescriptorProto_type(fields[1]),
            google_protobuf_FieldDescriptorProto_TYPE_MESSAGE);
}

// Like the previous test, but uses a message layout built at runtime.
TEST(DefToProto, TestRuntimeReflection) {
  upb::Arena arena;
  upb::DefPool defpool;
  upb_StringView test_file_desc =
      upb_util_def_to_proto_test_proto_upbdefinit.descriptor;
  const auto* file_desc = google_protobuf_FileDescriptorProto_parse(
      test_file_desc.data, test_file_desc.size, arena.ptr());

  _upb_DefPool_LoadDefInitEx(
      defpool.ptr(),
      &upb_util_def_to_proto_test_proto_upbdefinit, true);
  upb::FileDefPtr file = defpool.FindFileByName(
      upb_util_def_to_proto_test_proto_upbdefinit.filename);
  CheckFile(file, file_desc);
}

// Fuzz test regressions.

TEST(FuzzTest, EmptyPackage) {
  RoundTripDescriptor(ParseTextProtoOrDie(R"pb(file { package: "" })pb"));
}

TEST(FuzzTest, EmptyName) {
  RoundTripDescriptor(ParseTextProtoOrDie(R"pb(file { name: "" })pb"));
}

TEST(FuzzTest, EmptyPackage2) {
  RoundTripDescriptor(
      ParseTextProtoOrDie(R"pb(file { name: "n" package: "" })pb"));
}

TEST(FuzzTest, FileNameEmbeddedNull) {
  RoundTripDescriptor(ParseTextProtoOrDie(R"pb(file { name: "\000" })pb"));
}

TEST(FuzzTest, DuplicateOneofIndex) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "F"
             message_type {
               name: "M"
               oneof_decl { name: "O" }
               field { name: "f1" number: 1 type: TYPE_INT32 oneof_index: 0 }
               field { name: "f2" number: 1 type: TYPE_INT32 oneof_index: 0 }
             }
           })pb"));
}

TEST(FuzzTest, NanValue) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             enum_type {
               value {
                 number: 0
                 options { uninterpreted_option { double_value: nan } }
               }
             }
           })pb"));
}

TEST(FuzzTest, EnumValueEmbeddedNull) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             enum_type {
               name: "f"
               value { name: "\000" number: 0 }
             }
           })pb"));
}

TEST(FuzzTest, EnumValueNoNumber) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             enum_type {
               name: "f"
               value { name: "abc" }
             }
           })pb"));
}

TEST(FuzzTest, DefaultWithUnterminatedHex) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             message_type {
               name: "A"
               field {
                 name: "f"
                 number: 1
                 label: LABEL_OPTIONAL
                 type: TYPE_BYTES
                 default_value: "\\x"
               }
             }
           })pb"));
}

TEST(FuzzTest, DefaultWithValidHexEscape) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             message_type {
               name: "A"
               field {
                 name: "f"
                 number: 1
                 label: LABEL_OPTIONAL
                 type: TYPE_BYTES
                 default_value: "\\x03"
               }
             }
           })pb"));
}

TEST(FuzzTest, DefaultWithValidHexEscapePrintable) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "\035"
             message_type {
               name: "A"
               field {
                 name: "f"
                 number: 1
                 label: LABEL_OPTIONAL
                 type: TYPE_BYTES
                 default_value: "\\x23"  # 0x32 = '#'
               }
             }
           })pb"));
}

TEST(FuzzTest, PackageStartsWithNumber) {
  RoundTripDescriptor(
      ParseTextProtoOrDie(R"pb(file { name: "" package: "0" })pb"));
}

TEST(FuzzTest, RoundTripDescriptorRegression) {
  RoundTripDescriptor(ParseTextProtoOrDie(R"pb(file {
                                                 name: ""
                                                 message_type {
                                                   name: "A"
                                                   field {
                                                     name: "B"
                                                     number: 1
                                                     type: TYPE_BYTES
                                                     default_value: "\007"
                                                   }
                                                 }
                                               })pb"));
}

// Multiple oneof fields which have the same name.
TEST(FuzzTest, RoundTripDescriptorRegressionOneofSameName) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             name: "N"
             package: ""
             message_type {
               name: "b"
               field { name: "W" number: 1 type: TYPE_BYTES oneof_index: 0 }
               field { name: "W" number: 17 type: TYPE_UINT32 oneof_index: 0 }
               oneof_decl { name: "k" }
             }
           })pb"));
}

TEST(FuzzTest, NegativeOneofIndex) {
  RoundTripDescriptor(ParseTextProtoOrDie(
      R"pb(file {
             message_type {
               name: "A"
               field { name: "A" number: 0 type_name: "" oneof_index: -1 }
             }
           }
      )pb"));
}

}  // namespace upb_test
