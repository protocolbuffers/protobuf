// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <memory>
#include <string>
#include <vector>

#include <google/protobuf/test_util.h>
#include <google/protobuf/unittest.pb.h>
#include <google/protobuf/unittest_proto3_arena.pb.h>
#include <google/protobuf/unittest_proto3_optional.pb.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>
#include <google/protobuf/stubs/strutil.h>

using proto3_arena_unittest::ForeignMessage;
using proto3_arena_unittest::TestAllTypes;

namespace google {
namespace protobuf {

namespace internal {

class Proto3ArenaTestHelper {
 public:
  template <typename T>
  static Arena* GetOwningArena(const T& msg) {
    return msg.GetOwningArena();
  }
};

}  // namespace internal

namespace {
// We selectively set/check a few representative fields rather than all fields
// as this test is only expected to cover the basics of arena support.
void SetAllFields(TestAllTypes* m) {
  m->set_optional_int32(100);
  m->set_optional_string("asdf");
  m->set_optional_bytes("jkl;");
  m->mutable_optional_nested_message()->set_bb(42);
  m->mutable_optional_foreign_message()->set_c(43);
  m->set_optional_nested_enum(proto3_arena_unittest::TestAllTypes::BAZ);
  m->set_optional_foreign_enum(proto3_arena_unittest::FOREIGN_BAZ);
  m->mutable_optional_lazy_message()->set_bb(45);
  m->add_repeated_int32(100);
  m->add_repeated_string("asdf");
  m->add_repeated_bytes("jkl;");
  m->add_repeated_nested_message()->set_bb(46);
  m->add_repeated_foreign_message()->set_c(47);
  m->add_repeated_nested_enum(proto3_arena_unittest::TestAllTypes::BAZ);
  m->add_repeated_foreign_enum(proto3_arena_unittest::FOREIGN_BAZ);
  m->add_repeated_lazy_message()->set_bb(49);

  m->set_oneof_uint32(1);
  m->mutable_oneof_nested_message()->set_bb(50);
  m->set_oneof_string("test");  // only this one remains set
}

void ExpectAllFieldsSet(const TestAllTypes& m) {
  EXPECT_EQ(100, m.optional_int32());
  EXPECT_EQ("asdf", m.optional_string());
  EXPECT_EQ("jkl;", m.optional_bytes());
  EXPECT_EQ(true, m.has_optional_nested_message());
  EXPECT_EQ(42, m.optional_nested_message().bb());
  EXPECT_EQ(true, m.has_optional_foreign_message());
  EXPECT_EQ(43, m.optional_foreign_message().c());
  EXPECT_EQ(proto3_arena_unittest::TestAllTypes::BAZ, m.optional_nested_enum());
  EXPECT_EQ(proto3_arena_unittest::FOREIGN_BAZ, m.optional_foreign_enum());
  EXPECT_EQ(true, m.has_optional_lazy_message());
  EXPECT_EQ(45, m.optional_lazy_message().bb());

  EXPECT_EQ(1, m.repeated_int32_size());
  EXPECT_EQ(100, m.repeated_int32(0));
  EXPECT_EQ(1, m.repeated_string_size());
  EXPECT_EQ("asdf", m.repeated_string(0));
  EXPECT_EQ(1, m.repeated_bytes_size());
  EXPECT_EQ("jkl;", m.repeated_bytes(0));
  EXPECT_EQ(1, m.repeated_nested_message_size());
  EXPECT_EQ(46, m.repeated_nested_message(0).bb());
  EXPECT_EQ(1, m.repeated_foreign_message_size());
  EXPECT_EQ(47, m.repeated_foreign_message(0).c());
  EXPECT_EQ(1, m.repeated_nested_enum_size());
  EXPECT_EQ(proto3_arena_unittest::TestAllTypes::BAZ,
            m.repeated_nested_enum(0));
  EXPECT_EQ(1, m.repeated_foreign_enum_size());
  EXPECT_EQ(proto3_arena_unittest::FOREIGN_BAZ, m.repeated_foreign_enum(0));
  EXPECT_EQ(1, m.repeated_lazy_message_size());
  EXPECT_EQ(49, m.repeated_lazy_message(0).bb());

  EXPECT_EQ(proto3_arena_unittest::TestAllTypes::kOneofString,
            m.oneof_field_case());
  EXPECT_EQ("test", m.oneof_string());
}

// In this file we only test some basic functionalities of arena support in
// proto3 and expect the arena support to be fully tested in proto2 unittests
// because proto3 shares most code with proto2.

TEST(Proto3ArenaTest, Parsing) {
  TestAllTypes original;
  SetAllFields(&original);

  Arena arena;
  TestAllTypes* arena_message = Arena::CreateMessage<TestAllTypes>(&arena);
  arena_message->ParseFromString(original.SerializeAsString());
  ExpectAllFieldsSet(*arena_message);
}

TEST(Proto3ArenaTest, UnknownFields) {
  TestAllTypes original;
  SetAllFields(&original);

  Arena arena;
  TestAllTypes* arena_message = Arena::CreateMessage<TestAllTypes>(&arena);
  arena_message->ParseFromString(original.SerializeAsString());
  ExpectAllFieldsSet(*arena_message);

  // In proto3 we can still get a pointer to the UnknownFieldSet through
  // reflection API.
  UnknownFieldSet* unknown_fields =
      arena_message->GetReflection()->MutableUnknownFields(arena_message);
  // We can modify this UnknownFieldSet.
  unknown_fields->AddVarint(1, 2);
  // And the unknown fields should be changed.
  ASSERT_NE(original.ByteSizeLong(), arena_message->ByteSizeLong());
  ASSERT_FALSE(
      arena_message->GetReflection()->GetUnknownFields(*arena_message).empty());
}

TEST(Proto3ArenaTest, GetArena) {
  Arena arena;

  // Tests arena-allocated message and submessages.
  auto* arena_message1 = Arena::CreateMessage<TestAllTypes>(&arena);
  auto* arena_submessage1 = arena_message1->mutable_optional_foreign_message();
  auto* arena_repeated_submessage1 =
      arena_message1->add_repeated_foreign_message();
  EXPECT_EQ(&arena, arena_message1->GetArena());
  EXPECT_EQ(&arena,
            internal::Proto3ArenaTestHelper::GetOwningArena(*arena_message1));
  EXPECT_EQ(&arena, arena_submessage1->GetArena());
  EXPECT_EQ(&arena, arena_repeated_submessage1->GetArena());

  // Tests attached heap-allocated messages.
  auto* arena_message2 = Arena::CreateMessage<TestAllTypes>(&arena);
  arena_message2->set_allocated_optional_foreign_message(new ForeignMessage());
  arena_message2->mutable_repeated_foreign_message()->AddAllocated(
      new ForeignMessage());
  const auto& submessage2 = arena_message2->optional_foreign_message();
  const auto& repeated_submessage2 =
      arena_message2->repeated_foreign_message(0);
  EXPECT_EQ(nullptr, submessage2.GetArena());
  EXPECT_EQ(&arena,
            internal::Proto3ArenaTestHelper::GetOwningArena(submessage2));
  EXPECT_EQ(nullptr, repeated_submessage2.GetArena());
  EXPECT_EQ(&arena, internal::Proto3ArenaTestHelper::GetOwningArena(
                        repeated_submessage2));

  // Tests message created by Arena::Create.
  auto* arena_message3 = Arena::Create<TestAllTypes>(&arena);
  EXPECT_EQ(nullptr, arena_message3->GetArena());
  EXPECT_EQ(&arena,
            internal::Proto3ArenaTestHelper::GetOwningArena(*arena_message3));
}

TEST(Proto3ArenaTest, GetArenaWithUnknown) {
  Arena arena;

  // Tests arena-allocated message and submessages.
  auto* arena_message1 = Arena::CreateMessage<TestAllTypes>(&arena);
  arena_message1->GetReflection()->MutableUnknownFields(arena_message1);
  auto* arena_submessage1 = arena_message1->mutable_optional_foreign_message();
  arena_submessage1->GetReflection()->MutableUnknownFields(arena_submessage1);
  auto* arena_repeated_submessage1 =
      arena_message1->add_repeated_foreign_message();
  arena_repeated_submessage1->GetReflection()->MutableUnknownFields(
      arena_repeated_submessage1);
  EXPECT_EQ(&arena, arena_message1->GetArena());
  EXPECT_EQ(&arena,
            internal::Proto3ArenaTestHelper::GetOwningArena(*arena_message1));
  EXPECT_EQ(&arena, arena_submessage1->GetArena());
  EXPECT_EQ(&arena, arena_repeated_submessage1->GetArena());

  // Tests attached heap-allocated messages.
  auto* arena_message2 = Arena::CreateMessage<TestAllTypes>(&arena);
  arena_message2->set_allocated_optional_foreign_message(new ForeignMessage());
  arena_message2->mutable_repeated_foreign_message()->AddAllocated(
      new ForeignMessage());
  auto* submessage2 = arena_message2->mutable_optional_foreign_message();
  submessage2->GetReflection()->MutableUnknownFields(submessage2);
  auto* repeated_submessage2 =
      arena_message2->mutable_repeated_foreign_message(0);
  repeated_submessage2->GetReflection()->MutableUnknownFields(
      repeated_submessage2);
  EXPECT_EQ(nullptr, submessage2->GetArena());
  EXPECT_EQ(&arena,
            internal::Proto3ArenaTestHelper::GetOwningArena(*submessage2));
  EXPECT_EQ(nullptr, repeated_submessage2->GetArena());
  EXPECT_EQ(&arena, internal::Proto3ArenaTestHelper::GetOwningArena(
                        *repeated_submessage2));
}

TEST(Proto3ArenaTest, Swap) {
  Arena arena1;
  Arena arena2;

  // Test Swap().
  TestAllTypes* arena1_message = Arena::CreateMessage<TestAllTypes>(&arena1);
  TestAllTypes* arena2_message = Arena::CreateMessage<TestAllTypes>(&arena2);
  arena1_message->Swap(arena2_message);
  EXPECT_EQ(&arena1, arena1_message->GetArena());
  EXPECT_EQ(&arena2, arena2_message->GetArena());
}

TEST(Proto3ArenaTest, SetAllocatedMessage) {
  Arena arena;
  TestAllTypes* arena_message = Arena::CreateMessage<TestAllTypes>(&arena);
  TestAllTypes::NestedMessage* nested = new TestAllTypes::NestedMessage;
  nested->set_bb(118);
  arena_message->set_allocated_optional_nested_message(nested);
  EXPECT_EQ(118, arena_message->optional_nested_message().bb());
}

TEST(Proto3ArenaTest, ReleaseMessage) {
  Arena arena;
  TestAllTypes* arena_message = Arena::CreateMessage<TestAllTypes>(&arena);
  arena_message->mutable_optional_nested_message()->set_bb(118);
  std::unique_ptr<TestAllTypes::NestedMessage> nested(
      arena_message->release_optional_nested_message());
  EXPECT_EQ(118, nested->bb());
}

TEST(Proto3ArenaTest, MessageFieldClear) {
  // GitHub issue #310: https://github.com/protocolbuffers/protobuf/issues/310
  Arena arena;
  TestAllTypes* arena_message = Arena::CreateMessage<TestAllTypes>(&arena);
  arena_message->mutable_optional_nested_message()->set_bb(118);
  // This should not crash, but prior to the bugfix, it tried to use `operator
  // delete` the nested message (which is on the arena):
  arena_message->Clear();
}

TEST(Proto3ArenaTest, MessageFieldClearViaReflection) {
  Arena arena;
  TestAllTypes* message = Arena::CreateMessage<TestAllTypes>(&arena);
  const Reflection* r = message->GetReflection();
  const Descriptor* d = message->GetDescriptor();
  const FieldDescriptor* msg_field =
      d->FindFieldByName("optional_nested_message");

  message->mutable_optional_nested_message()->set_bb(1);
  r->ClearField(message, msg_field);
  EXPECT_FALSE(message->has_optional_nested_message());
  EXPECT_EQ(0, message->optional_nested_message().bb());
}

TEST(Proto3OptionalTest, OptionalFields) {
  protobuf_unittest::TestProto3Optional msg;
  EXPECT_FALSE(msg.has_optional_int32());
  msg.set_optional_int32(0);
  EXPECT_TRUE(msg.has_optional_int32());

  std::string serialized;
  msg.SerializeToString(&serialized);
  EXPECT_GT(serialized.size(), 0);

  msg.clear_optional_int32();
  EXPECT_FALSE(msg.has_optional_int32());
  msg.SerializeToString(&serialized);
  EXPECT_EQ(serialized.size(), 0);
}

TEST(Proto3OptionalTest, OptionalFieldDescriptor) {
  const Descriptor* d = protobuf_unittest::TestProto3Optional::descriptor();

  for (int i = 0; i < d->field_count(); i++) {
    const FieldDescriptor* f = d->field(i);
    if (HasPrefixString(f->name(), "singular")) {
      EXPECT_FALSE(f->has_optional_keyword()) << f->full_name();
      EXPECT_FALSE(f->has_presence()) << f->full_name();
      EXPECT_FALSE(f->containing_oneof()) << f->full_name();
    } else {
      EXPECT_TRUE(f->has_optional_keyword()) << f->full_name();
      EXPECT_TRUE(f->has_presence()) << f->full_name();
      EXPECT_TRUE(f->containing_oneof()) << f->full_name();
    }
  }
}

TEST(Proto3OptionalTest, OptionalField) {
  protobuf_unittest::TestProto3Optional msg;
  EXPECT_FALSE(msg.has_optional_int32());
  msg.set_optional_int32(0);
  EXPECT_TRUE(msg.has_optional_int32());

  std::string serialized;
  msg.SerializeToString(&serialized);
  EXPECT_GT(serialized.size(), 0);

  msg.clear_optional_int32();
  EXPECT_FALSE(msg.has_optional_int32());
  msg.SerializeToString(&serialized);
  EXPECT_EQ(serialized.size(), 0);
}

TEST(Proto3OptionalTest, OptionalFieldReflection) {
  // Tests that oneof reflection works on synthetic oneofs.
  //
  // We test this more deeply elsewhere by parsing/serializing TextFormat (which
  // doesn't treat synthetic oneofs specially, so reflects over them normally).
  protobuf_unittest::TestProto3Optional msg;
  const google::protobuf::Descriptor* d = msg.GetDescriptor();
  const google::protobuf::Reflection* r = msg.GetReflection();
  const google::protobuf::FieldDescriptor* f = d->FindFieldByName("optional_int32");
  const google::protobuf::OneofDescriptor* o = d->FindOneofByName("_optional_int32");
  GOOGLE_CHECK(f);
  GOOGLE_CHECK(o);
  EXPECT_TRUE(o->is_synthetic());

  EXPECT_FALSE(r->HasField(msg, f));
  EXPECT_FALSE(r->HasOneof(msg, o));
  EXPECT_TRUE(r->GetOneofFieldDescriptor(msg, o) == nullptr);

  r->SetInt32(&msg, f, 123);
  EXPECT_EQ(123, msg.optional_int32());
  EXPECT_EQ(123, r->GetInt32(msg, f));
  EXPECT_TRUE(r->HasField(msg, f));
  EXPECT_TRUE(r->HasOneof(msg, o));
  EXPECT_EQ(f, r->GetOneofFieldDescriptor(msg, o));

  std::vector<const FieldDescriptor*> fields;
  r->ListFields(msg, &fields);
  EXPECT_EQ(1, fields.size());
  EXPECT_EQ(f, fields[0]);

  r->ClearOneof(&msg, o);
  EXPECT_FALSE(r->HasField(msg, f));
  EXPECT_FALSE(r->HasOneof(msg, o));
  EXPECT_TRUE(r->GetOneofFieldDescriptor(msg, o) == nullptr);

  msg.set_optional_int32(123);
  EXPECT_EQ(123, r->GetInt32(msg, f));
  EXPECT_TRUE(r->HasField(msg, f));
  EXPECT_TRUE(r->HasOneof(msg, o));
  EXPECT_EQ(f, r->GetOneofFieldDescriptor(msg, o));

  r->ClearOneof(&msg, o);
  EXPECT_FALSE(r->HasField(msg, f));
  EXPECT_FALSE(r->HasOneof(msg, o));
  EXPECT_TRUE(r->GetOneofFieldDescriptor(msg, o) == nullptr);
}

// It's a regression test for b/160665543.
TEST(Proto3OptionalTest, ClearNonOptionalMessageField) {
  protobuf_unittest::TestProto3OptionalMessage msg;
  msg.mutable_nested_message();
  const google::protobuf::Descriptor* d = msg.GetDescriptor();
  const google::protobuf::Reflection* r = msg.GetReflection();
  const google::protobuf::FieldDescriptor* f = d->FindFieldByName("nested_message");
  r->ClearField(&msg, f);
}

TEST(Proto3OptionalTest, ClearOptionalMessageField) {
  protobuf_unittest::TestProto3OptionalMessage msg;
  msg.mutable_optional_nested_message();
  const google::protobuf::Descriptor* d = msg.GetDescriptor();
  const google::protobuf::Reflection* r = msg.GetReflection();
  const google::protobuf::FieldDescriptor* f =
      d->FindFieldByName("optional_nested_message");
  r->ClearField(&msg, f);
}

TEST(Proto3OptionalTest, SwapNonOptionalMessageField) {
  protobuf_unittest::TestProto3OptionalMessage msg1;
  protobuf_unittest::TestProto3OptionalMessage msg2;
  msg1.mutable_nested_message();
  const google::protobuf::Descriptor* d = msg1.GetDescriptor();
  const google::protobuf::Reflection* r = msg1.GetReflection();
  const google::protobuf::FieldDescriptor* f = d->FindFieldByName("nested_message");
  r->SwapFields(&msg1, &msg2, {f});
}

TEST(Proto3OptionalTest, SwapOptionalMessageField) {
  protobuf_unittest::TestProto3OptionalMessage msg1;
  protobuf_unittest::TestProto3OptionalMessage msg2;
  msg1.mutable_optional_nested_message();
  const google::protobuf::Descriptor* d = msg1.GetDescriptor();
  const google::protobuf::Reflection* r = msg1.GetReflection();
  const google::protobuf::FieldDescriptor* f =
      d->FindFieldByName("optional_nested_message");
  r->SwapFields(&msg1, &msg2, {f});
}

void SetAllFieldsZero(protobuf_unittest::TestProto3Optional* msg) {
  msg->set_optional_int32(0);
  msg->set_optional_int64(0);
  msg->set_optional_uint32(0);
  msg->set_optional_uint64(0);
  msg->set_optional_sint32(0);
  msg->set_optional_sint64(0);
  msg->set_optional_fixed32(0);
  msg->set_optional_fixed64(0);
  msg->set_optional_sfixed32(0);
  msg->set_optional_sfixed64(0);
  msg->set_optional_float(0);
  msg->set_optional_double(0);
  msg->set_optional_bool(false);
  msg->set_optional_string("");
  msg->set_optional_bytes("");
  msg->mutable_optional_nested_message();
  msg->mutable_lazy_nested_message();
  msg->set_optional_nested_enum(
      protobuf_unittest::TestProto3Optional::UNSPECIFIED);
}

void SetAllFieldsNonZero(protobuf_unittest::TestProto3Optional* msg) {
  msg->set_optional_int32(101);
  msg->set_optional_int64(102);
  msg->set_optional_uint32(103);
  msg->set_optional_uint64(104);
  msg->set_optional_sint32(105);
  msg->set_optional_sint64(106);
  msg->set_optional_fixed32(107);
  msg->set_optional_fixed64(108);
  msg->set_optional_sfixed32(109);
  msg->set_optional_sfixed64(110);
  msg->set_optional_float(111);
  msg->set_optional_double(112);
  msg->set_optional_bool(true);
  msg->set_optional_string("abc");
  msg->set_optional_bytes("def");
  msg->mutable_optional_nested_message();
  msg->mutable_lazy_nested_message();
  msg->set_optional_nested_enum(protobuf_unittest::TestProto3Optional::BAZ);
}

void TestAllFieldsZero(const protobuf_unittest::TestProto3Optional& msg) {
  EXPECT_EQ(0, msg.optional_int32());
  EXPECT_EQ(0, msg.optional_int64());
  EXPECT_EQ(0, msg.optional_uint32());
  EXPECT_EQ(0, msg.optional_uint64());
  EXPECT_EQ(0, msg.optional_sint32());
  EXPECT_EQ(0, msg.optional_sint64());
  EXPECT_EQ(0, msg.optional_fixed32());
  EXPECT_EQ(0, msg.optional_fixed64());
  EXPECT_EQ(0, msg.optional_sfixed32());
  EXPECT_EQ(0, msg.optional_sfixed64());
  EXPECT_EQ(0, msg.optional_float());
  EXPECT_EQ(0, msg.optional_double());
  EXPECT_EQ(0, msg.optional_bool());
  EXPECT_EQ("", msg.optional_string());
  EXPECT_EQ("", msg.optional_bytes());
  EXPECT_EQ(protobuf_unittest::TestProto3Optional::UNSPECIFIED,
            msg.optional_nested_enum());

  const Reflection* r = msg.GetReflection();
  const Descriptor* d = msg.GetDescriptor();
  EXPECT_EQ("", r->GetString(msg, d->FindFieldByName("optional_string")));
}

void TestAllFieldsNonZero(const protobuf_unittest::TestProto3Optional& msg) {
  EXPECT_EQ(101, msg.optional_int32());
  EXPECT_EQ(102, msg.optional_int64());
  EXPECT_EQ(103, msg.optional_uint32());
  EXPECT_EQ(104, msg.optional_uint64());
  EXPECT_EQ(105, msg.optional_sint32());
  EXPECT_EQ(106, msg.optional_sint64());
  EXPECT_EQ(107, msg.optional_fixed32());
  EXPECT_EQ(108, msg.optional_fixed64());
  EXPECT_EQ(109, msg.optional_sfixed32());
  EXPECT_EQ(110, msg.optional_sfixed64());
  EXPECT_EQ(111, msg.optional_float());
  EXPECT_EQ(112, msg.optional_double());
  EXPECT_EQ(true, msg.optional_bool());
  EXPECT_EQ("abc", msg.optional_string());
  EXPECT_EQ("def", msg.optional_bytes());
  EXPECT_EQ(protobuf_unittest::TestProto3Optional::BAZ,
            msg.optional_nested_enum());
}

void TestAllFieldsSet(const protobuf_unittest::TestProto3Optional& msg,
                      bool set) {
  EXPECT_EQ(set, msg.has_optional_int32());
  EXPECT_EQ(set, msg.has_optional_int64());
  EXPECT_EQ(set, msg.has_optional_uint32());
  EXPECT_EQ(set, msg.has_optional_uint64());
  EXPECT_EQ(set, msg.has_optional_sint32());
  EXPECT_EQ(set, msg.has_optional_sint64());
  EXPECT_EQ(set, msg.has_optional_fixed32());
  EXPECT_EQ(set, msg.has_optional_fixed64());
  EXPECT_EQ(set, msg.has_optional_sfixed32());
  EXPECT_EQ(set, msg.has_optional_sfixed64());
  EXPECT_EQ(set, msg.has_optional_float());
  EXPECT_EQ(set, msg.has_optional_double());
  EXPECT_EQ(set, msg.has_optional_bool());
  EXPECT_EQ(set, msg.has_optional_string());
  EXPECT_EQ(set, msg.has_optional_bytes());
  EXPECT_EQ(set, msg.has_optional_nested_message());
  EXPECT_EQ(set, msg.has_lazy_nested_message());
  EXPECT_EQ(set, msg.has_optional_nested_enum());
}

TEST(Proto3OptionalTest, BinaryRoundTrip) {
  protobuf_unittest::TestProto3Optional msg;
  TestAllFieldsSet(msg, false);
  SetAllFieldsZero(&msg);
  TestAllFieldsZero(msg);
  TestAllFieldsSet(msg, true);

  protobuf_unittest::TestProto3Optional msg2;
  std::string serialized;
  msg.SerializeToString(&serialized);
  EXPECT_TRUE(msg2.ParseFromString(serialized));
  TestAllFieldsZero(msg2);
  TestAllFieldsSet(msg2, true);
}

TEST(Proto3OptionalTest, TextFormatRoundTripZeros) {
  protobuf_unittest::TestProto3Optional msg;
  SetAllFieldsZero(&msg);

  protobuf_unittest::TestProto3Optional msg2;
  std::string text;
  EXPECT_TRUE(TextFormat::PrintToString(msg, &text));
  EXPECT_TRUE(TextFormat::ParseFromString(text, &msg2));
  TestAllFieldsSet(msg2, true);
  TestAllFieldsZero(msg2);
}

TEST(Proto3OptionalTest, TextFormatRoundTripNonZeros) {
  protobuf_unittest::TestProto3Optional msg;
  SetAllFieldsNonZero(&msg);

  protobuf_unittest::TestProto3Optional msg2;
  std::string text;
  EXPECT_TRUE(TextFormat::PrintToString(msg, &text));
  EXPECT_TRUE(TextFormat::ParseFromString(text, &msg2));
  TestAllFieldsSet(msg2, true);
  TestAllFieldsNonZero(msg2);
}

TEST(Proto3OptionalTest, SwapRoundTripZero) {
  protobuf_unittest::TestProto3Optional msg;
  SetAllFieldsZero(&msg);
  TestAllFieldsSet(msg, true);

  protobuf_unittest::TestProto3Optional msg2;
  msg.Swap(&msg2);
  TestAllFieldsSet(msg2, true);
  TestAllFieldsZero(msg2);
}

TEST(Proto3OptionalTest, SwapRoundTripNonZero) {
  protobuf_unittest::TestProto3Optional msg;
  SetAllFieldsNonZero(&msg);
  TestAllFieldsSet(msg, true);

  protobuf_unittest::TestProto3Optional msg2;
  msg.Swap(&msg2);
  TestAllFieldsSet(msg2, true);
  TestAllFieldsNonZero(msg2);
}

TEST(Proto3OptionalTest, ReflectiveSwapRoundTrip) {
  protobuf_unittest::TestProto3Optional msg;
  SetAllFieldsZero(&msg);
  TestAllFieldsSet(msg, true);

  protobuf_unittest::TestProto3Optional msg2;
  msg2.GetReflection()->Swap(&msg, &msg2);
  TestAllFieldsSet(msg2, true);
  TestAllFieldsZero(msg2);
}

TEST(Proto3OptionalTest, PlainFields) {
  const Descriptor* d = TestAllTypes::descriptor();

  EXPECT_FALSE(d->FindFieldByName("optional_int32")->has_presence());
  EXPECT_TRUE(d->FindFieldByName("oneof_nested_message")->has_presence());
}

}  // namespace
}  // namespace protobuf
}  // namespace google
