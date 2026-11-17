// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstring>
#include <string>

#include <gtest/gtest.h>
#include "upb/base/status.hpp"
#include "upb/base/string_view.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb/reflection/descriptor_bootstrap.h"
#include "upb/reflection/message.h"

namespace {

google_protobuf_FieldDescriptorProto* AddField(
    google_protobuf_DescriptorProto* msg, upb::Arena& arena, const char* name,
    int number, google_protobuf_FieldDescriptorProto_Type type) {
  google_protobuf_FieldDescriptorProto* field =
      google_protobuf_DescriptorProto_add_field(msg, arena.ptr());
  EXPECT_NE(field, nullptr);
  google_protobuf_FieldDescriptorProto_set_name(
      field, upb_StringView_FromString(name));
  google_protobuf_FieldDescriptorProto_set_number(field, number);
  google_protobuf_FieldDescriptorProto_set_label(
      field, google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL);
  google_protobuf_FieldDescriptorProto_set_type(field, type);
  return field;
}

google_protobuf_FileDescriptorProto* MapEntryFile(upb::Arena& arena,
                                                   int key_number,
                                                   int value_number) {
  google_protobuf_FileDescriptorProto* file =
      google_protobuf_FileDescriptorProto_new(arena.ptr());
  EXPECT_NE(file, nullptr);
  google_protobuf_FileDescriptorProto_set_name(
      file, upb_StringView_FromString("map_entry.proto"));
  google_protobuf_FileDescriptorProto_set_syntax(
      file, upb_StringView_FromString("proto2"));

  google_protobuf_DescriptorProto* msg =
      google_protobuf_FileDescriptorProto_add_message_type(file, arena.ptr());
  EXPECT_NE(msg, nullptr);
  google_protobuf_DescriptorProto_set_name(msg,
                                           upb_StringView_FromString("M"));
  google_protobuf_MessageOptions* opts =
      google_protobuf_DescriptorProto_mutable_options(msg, arena.ptr());
  EXPECT_NE(opts, nullptr);
  google_protobuf_MessageOptions_set_map_entry(opts, true);

  AddField(msg, arena, "key", key_number,
           google_protobuf_FieldDescriptorProto_TYPE_STRING);
  AddField(msg, arena, "value", value_number,
           google_protobuf_FieldDescriptorProto_TYPE_UINT32);
  return file;
}

void CheckJsonRoundTrip(const upb_FileDef* file, upb::DefPool& pool,
                        const char* json) {
  const upb_MessageDef* mdef = upb_FileDef_TopLevelMessage(file, 0);
  ASSERT_NE(mdef, nullptr);
  upb::Arena arena;
  upb_Message* msg =
      upb_Message_New(upb_MessageDef_MiniTable(mdef), arena.ptr());
  ASSERT_NE(msg, nullptr);

  upb::Status status;
  ASSERT_TRUE(upb_JsonDecode(json, strlen(json), msg, mdef, pool.ptr(), 0,
                             arena.ptr(), status.ptr()))
      << status.error_message();

  char out[64];
  size_t size = upb_JsonEncode(msg, mdef, pool.ptr(), 0, out, sizeof(out),
                               status.ptr());
  ASSERT_TRUE(status.ok()) << status.error_message();
  EXPECT_EQ(std::string(out, size), json);
}

void ExpectRejected(int key_number, int value_number, const char* json,
                    const char* error) {
  upb::Arena arena;
  upb::DefPool pool;
  upb::Status status;
  const upb_FileDef* file = upb_DefPool_AddFile(
      pool.ptr(), MapEntryFile(arena, key_number, value_number), status.ptr());
  if (file != nullptr) {
    CheckJsonRoundTrip(file, pool, json);
  }
  ASSERT_EQ(file, nullptr);
  EXPECT_NE(std::string(status.error_message()).find(error), std::string::npos)
      << status.error_message();
}

TEST(MapEntryDefTest, CanonicalEntryRoundTrips) {
  upb::Arena arena;
  upb::DefPool pool;
  upb::Status status;
  const upb_FileDef* file = upb_DefPool_AddFile(
      pool.ptr(), MapEntryFile(arena, 1, 2), status.ptr());
  ASSERT_NE(file, nullptr) << status.error_message();
  CheckJsonRoundTrip(file, pool, R"({"key":"x"})");
}

TEST(MapEntryDefTest, RejectsMissingKeyField) {
  ExpectRejected(3, 2, R"({"key":"x"})", "map_entry key must be field 1");
}

TEST(MapEntryDefTest, RejectsMissingValueField) {
  ExpectRejected(1, 3, R"({"value":7})", "map_entry value must be field 2");
}

}  // namespace
