/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/util/def_to_proto.h"

#include "gmock/gmock.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor.upbdefs.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "upb/def.hpp"
#include "upb/upb.hpp"
#include "upb/util/def_to_proto_test.upbdefs.h"

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
      ToProto(proto, msgdef.ptr(), desc, &factory));
  std::unique_ptr<google::protobuf::Message> m2(
      ToProto(arg, msgdef.ptr(), desc, &factory));
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
