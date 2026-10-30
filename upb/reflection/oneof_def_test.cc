// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <gtest/gtest.h>
#include "absl/cleanup/cleanup.h"
#include "absl/strings/string_view.h"
#include "upb/base/status.h"
#include "upb/base/string_view.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.h"
#include "upb/reflection/descriptor_bootstrap.h"
#include "upb/reflection/message.h"

namespace {

TEST(OneofDefTest, ValidOneofMapEntryAndMessageSetAccepted) {
  upb::Arena proto_arena;
  google_protobuf_FileDescriptorProto* file =
      google_protobuf_FileDescriptorProto_new(proto_arena.ptr());
  ASSERT_NE(file, nullptr);
  google_protobuf_FileDescriptorProto_set_name(
      file, upb_StringView_FromString("valid_defs.proto"));
  google_protobuf_FileDescriptorProto_set_syntax(
      file, upb_StringView_FromString("proto2"));

  google_protobuf_DescriptorProto* msg =
      google_protobuf_FileDescriptorProto_add_message_type(file,
                                                           proto_arena.ptr());
  ASSERT_NE(msg, nullptr);
  google_protobuf_DescriptorProto_set_name(
      msg, upb_StringView_FromString("WithOneof"));
  google_protobuf_OneofDescriptorProto* oneof =
      google_protobuf_DescriptorProto_add_oneof_decl(msg, proto_arena.ptr());
  ASSERT_NE(oneof, nullptr);
  google_protobuf_OneofDescriptorProto_set_name(
      oneof, upb_StringView_FromString("choice"));
  google_protobuf_FieldDescriptorProto* f1 =
      google_protobuf_DescriptorProto_add_field(msg, proto_arena.ptr());
  ASSERT_NE(f1, nullptr);
  google_protobuf_FieldDescriptorProto_set_name(
      f1, upb_StringView_FromString("a"));
  google_protobuf_FieldDescriptorProto_set_number(f1, 1);
  google_protobuf_FieldDescriptorProto_set_label(
      f1, google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL);
  google_protobuf_FieldDescriptorProto_set_type(
      f1, google_protobuf_FieldDescriptorProto_TYPE_INT32);
  google_protobuf_FieldDescriptorProto_set_oneof_index(f1, 0);

  google_protobuf_DescriptorProto* entry =
      google_protobuf_FileDescriptorProto_add_message_type(file,
                                                           proto_arena.ptr());
  ASSERT_NE(entry, nullptr);
  google_protobuf_DescriptorProto_set_name(
      entry, upb_StringView_FromString("ValidEntry"));
  google_protobuf_MessageOptions* entry_opts =
      google_protobuf_DescriptorProto_mutable_options(entry, proto_arena.ptr());
  ASSERT_NE(entry_opts, nullptr);
  google_protobuf_MessageOptions_set_map_entry(entry_opts, true);
  google_protobuf_FieldDescriptorProto* k =
      google_protobuf_DescriptorProto_add_field(entry, proto_arena.ptr());
  google_protobuf_FieldDescriptorProto_set_name(
      k, upb_StringView_FromString("key"));
  google_protobuf_FieldDescriptorProto_set_number(k, 1);
  google_protobuf_FieldDescriptorProto_set_label(
      k, google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL);
  google_protobuf_FieldDescriptorProto_set_type(
      k, google_protobuf_FieldDescriptorProto_TYPE_INT32);
  google_protobuf_FieldDescriptorProto* v =
      google_protobuf_DescriptorProto_add_field(entry, proto_arena.ptr());
  google_protobuf_FieldDescriptorProto_set_name(
      v, upb_StringView_FromString("value"));
  google_protobuf_FieldDescriptorProto_set_number(v, 2);
  google_protobuf_FieldDescriptorProto_set_label(
      v, google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL);
  google_protobuf_FieldDescriptorProto_set_type(
      v, google_protobuf_FieldDescriptorProto_TYPE_INT32);

  google_protobuf_DescriptorProto* mset =
      google_protobuf_FileDescriptorProto_add_message_type(file,
                                                           proto_arena.ptr());
  ASSERT_NE(mset, nullptr);
  google_protobuf_DescriptorProto_set_name(
      mset, upb_StringView_FromString("ValidMessageSet"));
  google_protobuf_MessageOptions* mset_opts =
      google_protobuf_DescriptorProto_mutable_options(mset, proto_arena.ptr());
  ASSERT_NE(mset_opts, nullptr);
  google_protobuf_MessageOptions_set_message_set_wire_format(mset_opts, true);
  google_protobuf_DescriptorProto_ExtensionRange* ext =
      google_protobuf_DescriptorProto_add_extension_range(mset,
                                                          proto_arena.ptr());
  google_protobuf_DescriptorProto_ExtensionRange_set_start(ext, 4);
  google_protobuf_DescriptorProto_ExtensionRange_set_end(ext, 100);

  upb_DefPool* pool = upb_DefPool_New();
  ASSERT_NE(pool, nullptr);
  auto cleanup = absl::MakeCleanup([pool] { upb_DefPool_Free(pool); });

  upb_Status status;
  upb_Status_Clear(&status);
  const upb_FileDef* added = upb_DefPool_AddFile(pool, file, &status);
  ASSERT_NE(added, nullptr) << upb_Status_ErrorMessage(&status);

  const upb_MessageDef* md = upb_DefPool_FindMessageByName(pool, "WithOneof");
  ASSERT_NE(md, nullptr);
  ASSERT_EQ(upb_MessageDef_OneofCount(md), 1);
  const upb_OneofDef* od = upb_MessageDef_Oneof(md, 0);
  ASSERT_NE(od, nullptr);

  upb::Arena msg_arena;
  upb_Message* instance =
      upb_Message_New(upb_MessageDef_MiniTable(md), msg_arena.ptr());
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(upb_Message_WhichOneofByDef(instance, od), nullptr);

  const upb_FieldDef* fd = upb_MessageDef_FindFieldByName(md, "a");
  ASSERT_NE(fd, nullptr);
  upb_MessageValue val;
  val.int32_val = 42;
  ASSERT_TRUE(upb_Message_SetFieldByDef(instance, fd, val, msg_arena.ptr()));
  EXPECT_EQ(upb_Message_WhichOneofByDef(instance, od), fd);
}

TEST(OneofDefTest, EmptyOneofInMessageSetRejected) {
  upb::Arena proto_arena;
  google_protobuf_FileDescriptorProto* file =
      google_protobuf_FileDescriptorProto_new(proto_arena.ptr());
  ASSERT_NE(file, nullptr);
  google_protobuf_FileDescriptorProto_set_name(
      file, upb_StringView_FromString("empty_oneof_msgset.proto"));
  google_protobuf_FileDescriptorProto_set_syntax(
      file, upb_StringView_FromString("proto2"));

  google_protobuf_DescriptorProto* mset =
      google_protobuf_FileDescriptorProto_add_message_type(file,
                                                           proto_arena.ptr());
  ASSERT_NE(mset, nullptr);
  google_protobuf_DescriptorProto_set_name(
      mset, upb_StringView_FromString("EmptyOneofMsgSet"));
  google_protobuf_MessageOptions* opts =
      google_protobuf_DescriptorProto_mutable_options(mset, proto_arena.ptr());
  ASSERT_NE(opts, nullptr);
  google_protobuf_MessageOptions_set_message_set_wire_format(opts, true);
  google_protobuf_DescriptorProto_ExtensionRange* ext =
      google_protobuf_DescriptorProto_add_extension_range(mset,
                                                          proto_arena.ptr());
  google_protobuf_DescriptorProto_ExtensionRange_set_start(ext, 4);
  google_protobuf_DescriptorProto_ExtensionRange_set_end(ext, 100);
  google_protobuf_OneofDescriptorProto* oneof =
      google_protobuf_DescriptorProto_add_oneof_decl(mset, proto_arena.ptr());
  ASSERT_NE(oneof, nullptr);
  google_protobuf_OneofDescriptorProto_set_name(
      oneof, upb_StringView_FromString("empty"));

  upb_DefPool* pool = upb_DefPool_New();
  ASSERT_NE(pool, nullptr);
  auto cleanup = absl::MakeCleanup([pool] { upb_DefPool_Free(pool); });

  upb_Status status;
  upb_Status_Clear(&status);
  const upb_FileDef* added = upb_DefPool_AddFile(pool, file, &status);
  if (added != nullptr) {
    const upb_MessageDef* md =
        upb_DefPool_FindMessageByName(pool, "EmptyOneofMsgSet");
    ASSERT_NE(md, nullptr);
    const upb_OneofDef* od = upb_MessageDef_Oneof(md, 0);
    ASSERT_NE(od, nullptr);
    upb::Arena msg_arena;
    upb_Message* instance =
        upb_Message_New(upb_MessageDef_MiniTable(md), msg_arena.ptr());
    ASSERT_NE(instance, nullptr);
    (void)upb_Message_WhichOneofByDef(instance, od);
  }
  EXPECT_EQ(added, nullptr);
  EXPECT_NE(
      absl::string_view(upb_Status_ErrorMessage(&status))
          .find("Oneof must have at least one field"),
      absl::string_view::npos)
      << upb_Status_ErrorMessage(&status);
}

TEST(OneofDefTest, MapEntryWithOneofRejected) {
  upb::Arena proto_arena;
  google_protobuf_FileDescriptorProto* file =
      google_protobuf_FileDescriptorProto_new(proto_arena.ptr());
  ASSERT_NE(file, nullptr);
  google_protobuf_FileDescriptorProto_set_name(
      file, upb_StringView_FromString("map_entry_oneof.proto"));
  google_protobuf_FileDescriptorProto_set_syntax(
      file, upb_StringView_FromString("proto2"));

  google_protobuf_DescriptorProto* entry =
      google_protobuf_FileDescriptorProto_add_message_type(file,
                                                           proto_arena.ptr());
  ASSERT_NE(entry, nullptr);
  google_protobuf_DescriptorProto_set_name(
      entry, upb_StringView_FromString("BadEntry"));
  google_protobuf_MessageOptions* opts =
      google_protobuf_DescriptorProto_mutable_options(entry, proto_arena.ptr());
  ASSERT_NE(opts, nullptr);
  google_protobuf_MessageOptions_set_map_entry(opts, true);
  google_protobuf_OneofDescriptorProto* oneof =
      google_protobuf_DescriptorProto_add_oneof_decl(entry, proto_arena.ptr());
  ASSERT_NE(oneof, nullptr);
  google_protobuf_OneofDescriptorProto_set_name(
      oneof, upb_StringView_FromString("choice"));

  google_protobuf_FieldDescriptorProto* k =
      google_protobuf_DescriptorProto_add_field(entry, proto_arena.ptr());
  google_protobuf_FieldDescriptorProto_set_name(
      k, upb_StringView_FromString("key"));
  google_protobuf_FieldDescriptorProto_set_number(k, 1);
  google_protobuf_FieldDescriptorProto_set_label(
      k, google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL);
  google_protobuf_FieldDescriptorProto_set_type(
      k, google_protobuf_FieldDescriptorProto_TYPE_INT32);
  google_protobuf_FieldDescriptorProto_set_oneof_index(k, 0);

  google_protobuf_FieldDescriptorProto* v =
      google_protobuf_DescriptorProto_add_field(entry, proto_arena.ptr());
  google_protobuf_FieldDescriptorProto_set_name(
      v, upb_StringView_FromString("value"));
  google_protobuf_FieldDescriptorProto_set_number(v, 2);
  google_protobuf_FieldDescriptorProto_set_label(
      v, google_protobuf_FieldDescriptorProto_LABEL_OPTIONAL);
  google_protobuf_FieldDescriptorProto_set_type(
      v, google_protobuf_FieldDescriptorProto_TYPE_INT32);

  upb_DefPool* pool = upb_DefPool_New();
  ASSERT_NE(pool, nullptr);
  auto cleanup = absl::MakeCleanup([pool] { upb_DefPool_Free(pool); });

  upb_Status status;
  upb_Status_Clear(&status);
  const upb_FileDef* added = upb_DefPool_AddFile(pool, file, &status);
  if (added != nullptr) {
    const upb_MessageDef* md = upb_DefPool_FindMessageByName(pool, "BadEntry");
    ASSERT_NE(md, nullptr);
    const upb_OneofDef* od = upb_MessageDef_Oneof(md, 0);
    ASSERT_NE(od, nullptr);
    upb::Arena msg_arena;
    (void)upb_Arena_Malloc(msg_arena.ptr(), 512);
    upb_Message* instance =
        upb_Message_New(upb_MessageDef_MiniTable(md), msg_arena.ptr());
    ASSERT_NE(instance, nullptr);
    (void)upb_Message_WhichOneofByDef(instance, od);
  }
  EXPECT_EQ(added, nullptr);
  EXPECT_NE(
      absl::string_view(upb_Status_ErrorMessage(&status))
          .find("map_entry cannot have oneofs"),
      absl::string_view::npos)
      << upb_Status_ErrorMessage(&status);
}

}  // namespace
