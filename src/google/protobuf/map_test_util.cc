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

#include <google/protobuf/map_test_util.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/testing/googletest.h>
#include <gtest/gtest.h>

namespace google {
namespace protobuf {

void MapTestUtil::SetMapFields(unittest::TestMap* message) {
  // Add first element.
  (*message->mutable_map_int32_int32())[0] = 0;
  (*message->mutable_map_int64_int64())[0] = 0;
  (*message->mutable_map_uint32_uint32())[0] = 0;
  (*message->mutable_map_uint64_uint64())[0] = 0;
  (*message->mutable_map_sint32_sint32())[0] = 0;
  (*message->mutable_map_sint64_sint64())[0] = 0;
  (*message->mutable_map_fixed32_fixed32())[0] = 0;
  (*message->mutable_map_fixed64_fixed64())[0] = 0;
  (*message->mutable_map_sfixed32_sfixed32())[0] = 0;
  (*message->mutable_map_sfixed64_sfixed64())[0] = 0;
  (*message->mutable_map_int32_float())[0] = 0.0;
  (*message->mutable_map_int32_double())[0] = 0.0;
  (*message->mutable_map_bool_bool())[0] = false;
  (*message->mutable_map_string_string())["0"] = "0";
  (*message->mutable_map_int32_bytes())[0] = "0";
  (*message->mutable_map_int32_enum())[0] =
      unittest::MAP_ENUM_BAR;
  (*message->mutable_map_int32_foreign_message())[0].set_c(0);

  // Add second element
  (*message->mutable_map_int32_int32())[1] = 1;
  (*message->mutable_map_int64_int64())[1] = 1;
  (*message->mutable_map_uint32_uint32())[1] = 1;
  (*message->mutable_map_uint64_uint64())[1] = 1;
  (*message->mutable_map_sint32_sint32())[1] = 1;
  (*message->mutable_map_sint64_sint64())[1] = 1;
  (*message->mutable_map_fixed32_fixed32())[1] = 1;
  (*message->mutable_map_fixed64_fixed64())[1] = 1;
  (*message->mutable_map_sfixed32_sfixed32())[1] = 1;
  (*message->mutable_map_sfixed64_sfixed64())[1] = 1;
  (*message->mutable_map_int32_float())[1] = 1.0;
  (*message->mutable_map_int32_double())[1] = 1.0;
  (*message->mutable_map_bool_bool())[1] = true;
  (*message->mutable_map_string_string())["1"] = "1";
  (*message->mutable_map_int32_bytes())[1] = "1";
  (*message->mutable_map_int32_enum())[1] =
      unittest::MAP_ENUM_BAZ;
  (*message->mutable_map_int32_foreign_message())[1].set_c(1);
}

void MapTestUtil::SetMapFieldsInitialized(unittest::TestMap* message) {
  // Add first element using bracket operator, which should assign default
  // value automatically.
  (*message->mutable_map_int32_int32())[0];
  (*message->mutable_map_int64_int64())[0];
  (*message->mutable_map_uint32_uint32())[0];
  (*message->mutable_map_uint64_uint64())[0];
  (*message->mutable_map_sint32_sint32())[0];
  (*message->mutable_map_sint64_sint64())[0];
  (*message->mutable_map_fixed32_fixed32())[0];
  (*message->mutable_map_fixed64_fixed64())[0];
  (*message->mutable_map_sfixed32_sfixed32())[0];
  (*message->mutable_map_sfixed64_sfixed64())[0];
  (*message->mutable_map_int32_float())[0];
  (*message->mutable_map_int32_double())[0];
  (*message->mutable_map_bool_bool())[0];
  (*message->mutable_map_string_string())["0"];
  (*message->mutable_map_int32_bytes())[0];
  (*message->mutable_map_int32_enum())[0];
  (*message->mutable_map_int32_foreign_message())[0];
}

void MapTestUtil::ModifyMapFields(unittest::TestMap* message) {
  (*message->mutable_map_int32_int32())[1] = 2;
  (*message->mutable_map_int64_int64())[1] = 2;
  (*message->mutable_map_uint32_uint32())[1] = 2;
  (*message->mutable_map_uint64_uint64())[1] = 2;
  (*message->mutable_map_sint32_sint32())[1] = 2;
  (*message->mutable_map_sint64_sint64())[1] = 2;
  (*message->mutable_map_fixed32_fixed32())[1] = 2;
  (*message->mutable_map_fixed64_fixed64())[1] = 2;
  (*message->mutable_map_sfixed32_sfixed32())[1] = 2;
  (*message->mutable_map_sfixed64_sfixed64())[1] = 2;
  (*message->mutable_map_int32_float())[1] = 2.0;
  (*message->mutable_map_int32_double())[1] = 2.0;
  (*message->mutable_map_bool_bool())[1] = false;
  (*message->mutable_map_string_string())["1"] = "2";
  (*message->mutable_map_int32_bytes())[1] = "2";
  (*message->mutable_map_int32_enum())[1] =
      unittest::MAP_ENUM_FOO;
  (*message->mutable_map_int32_foreign_message())[1].set_c(2);
}

void MapTestUtil::ExpectClear(const unittest::TestMap& message) {
  EXPECT_EQ(0, message.map_int32_int32().size());
  EXPECT_EQ(0, message.map_int64_int64().size());
  EXPECT_EQ(0, message.map_uint32_uint32().size());
  EXPECT_EQ(0, message.map_uint64_uint64().size());
  EXPECT_EQ(0, message.map_sint32_sint32().size());
  EXPECT_EQ(0, message.map_sint64_sint64().size());
  EXPECT_EQ(0, message.map_fixed32_fixed32().size());
  EXPECT_EQ(0, message.map_fixed64_fixed64().size());
  EXPECT_EQ(0, message.map_sfixed32_sfixed32().size());
  EXPECT_EQ(0, message.map_sfixed64_sfixed64().size());
  EXPECT_EQ(0, message.map_int32_float().size());
  EXPECT_EQ(0, message.map_int32_double().size());
  EXPECT_EQ(0, message.map_bool_bool().size());
  EXPECT_EQ(0, message.map_string_string().size());
  EXPECT_EQ(0, message.map_int32_bytes().size());
  EXPECT_EQ(0, message.map_int32_enum().size());
  EXPECT_EQ(0, message.map_int32_foreign_message().size());
}

void MapTestUtil::ExpectMapFieldsSet(const unittest::TestMap& message) {
  EXPECT_EQ(2, message.map_int32_int32().size());
  EXPECT_EQ(2, message.map_int64_int64().size());
  EXPECT_EQ(2, message.map_uint32_uint32().size());
  EXPECT_EQ(2, message.map_uint64_uint64().size());
  EXPECT_EQ(2, message.map_sint32_sint32().size());
  EXPECT_EQ(2, message.map_sint64_sint64().size());
  EXPECT_EQ(2, message.map_fixed32_fixed32().size());
  EXPECT_EQ(2, message.map_fixed64_fixed64().size());
  EXPECT_EQ(2, message.map_sfixed32_sfixed32().size());
  EXPECT_EQ(2, message.map_sfixed64_sfixed64().size());
  EXPECT_EQ(2, message.map_int32_float().size());
  EXPECT_EQ(2, message.map_int32_double().size());
  EXPECT_EQ(2, message.map_bool_bool().size());
  EXPECT_EQ(2, message.map_string_string().size());
  EXPECT_EQ(2, message.map_int32_bytes().size());
  EXPECT_EQ(2, message.map_int32_enum().size());
  EXPECT_EQ(2, message.map_int32_foreign_message().size());

  EXPECT_EQ(0, message.map_int32_int32().at(0));
  EXPECT_EQ(0, message.map_int64_int64().at(0));
  EXPECT_EQ(0, message.map_uint32_uint32().at(0));
  EXPECT_EQ(0, message.map_uint64_uint64().at(0));
  EXPECT_EQ(0, message.map_sint32_sint32().at(0));
  EXPECT_EQ(0, message.map_sint64_sint64().at(0));
  EXPECT_EQ(0, message.map_fixed32_fixed32().at(0));
  EXPECT_EQ(0, message.map_fixed64_fixed64().at(0));
  EXPECT_EQ(0, message.map_sfixed32_sfixed32().at(0));
  EXPECT_EQ(0, message.map_sfixed64_sfixed64().at(0));
  EXPECT_EQ(0, message.map_int32_float().at(0));
  EXPECT_EQ(0, message.map_int32_double().at(0));
  EXPECT_EQ(false, message.map_bool_bool().at(0));
  EXPECT_EQ("0", message.map_string_string().at("0"));
  EXPECT_EQ("0", message.map_int32_bytes().at(0));
  EXPECT_EQ(unittest::MAP_ENUM_BAR, message.map_int32_enum().at(0));
  EXPECT_EQ(0, message.map_int32_foreign_message().at(0).c());

  EXPECT_EQ(1, message.map_int32_int32().at(1));
  EXPECT_EQ(1, message.map_int64_int64().at(1));
  EXPECT_EQ(1, message.map_uint32_uint32().at(1));
  EXPECT_EQ(1, message.map_uint64_uint64().at(1));
  EXPECT_EQ(1, message.map_sint32_sint32().at(1));
  EXPECT_EQ(1, message.map_sint64_sint64().at(1));
  EXPECT_EQ(1, message.map_fixed32_fixed32().at(1));
  EXPECT_EQ(1, message.map_fixed64_fixed64().at(1));
  EXPECT_EQ(1, message.map_sfixed32_sfixed32().at(1));
  EXPECT_EQ(1, message.map_sfixed64_sfixed64().at(1));
  EXPECT_EQ(1, message.map_int32_float().at(1));
  EXPECT_EQ(1, message.map_int32_double().at(1));
  EXPECT_EQ(true, message.map_bool_bool().at(1));
  EXPECT_EQ("1", message.map_string_string().at("1"));
  EXPECT_EQ("1", message.map_int32_bytes().at(1));
  EXPECT_EQ(unittest::MAP_ENUM_BAZ, message.map_int32_enum().at(1));
  EXPECT_EQ(1, message.map_int32_foreign_message().at(1).c());
}

void MapTestUtil::ExpectMapFieldsSetInitialized(
    const unittest::TestMap& message) {
  EXPECT_EQ(1, message.map_int32_int32().size());
  EXPECT_EQ(1, message.map_int64_int64().size());
  EXPECT_EQ(1, message.map_uint32_uint32().size());
  EXPECT_EQ(1, message.map_uint64_uint64().size());
  EXPECT_EQ(1, message.map_sint32_sint32().size());
  EXPECT_EQ(1, message.map_sint64_sint64().size());
  EXPECT_EQ(1, message.map_fixed32_fixed32().size());
  EXPECT_EQ(1, message.map_fixed64_fixed64().size());
  EXPECT_EQ(1, message.map_sfixed32_sfixed32().size());
  EXPECT_EQ(1, message.map_sfixed64_sfixed64().size());
  EXPECT_EQ(1, message.map_int32_float().size());
  EXPECT_EQ(1, message.map_int32_double().size());
  EXPECT_EQ(1, message.map_bool_bool().size());
  EXPECT_EQ(1, message.map_string_string().size());
  EXPECT_EQ(1, message.map_int32_bytes().size());
  EXPECT_EQ(1, message.map_int32_enum().size());
  EXPECT_EQ(1, message.map_int32_foreign_message().size());

  EXPECT_EQ(0, message.map_int32_int32().at(0));
  EXPECT_EQ(0, message.map_int64_int64().at(0));
  EXPECT_EQ(0, message.map_uint32_uint32().at(0));
  EXPECT_EQ(0, message.map_uint64_uint64().at(0));
  EXPECT_EQ(0, message.map_sint32_sint32().at(0));
  EXPECT_EQ(0, message.map_sint64_sint64().at(0));
  EXPECT_EQ(0, message.map_fixed32_fixed32().at(0));
  EXPECT_EQ(0, message.map_fixed64_fixed64().at(0));
  EXPECT_EQ(0, message.map_sfixed32_sfixed32().at(0));
  EXPECT_EQ(0, message.map_sfixed64_sfixed64().at(0));
  EXPECT_EQ(0, message.map_int32_float().at(0));
  EXPECT_EQ(0, message.map_int32_double().at(0));
  EXPECT_EQ(false, message.map_bool_bool().at(0));
  EXPECT_EQ("", message.map_string_string().at("0"));
  EXPECT_EQ("", message.map_int32_bytes().at(0));
  EXPECT_EQ(unittest::MAP_ENUM_FOO, message.map_int32_enum().at(0));
  EXPECT_EQ(0, message.map_int32_foreign_message().at(0).ByteSize());
}

void MapTestUtil::ExpectMapFieldsModified(
    const unittest::TestMap& message) {
  // ModifyMapFields only sets the second element of each field.  In addition to
  // verifying this, we also verify that the first element and size were *not*
  // modified.
  EXPECT_EQ(2, message.map_int32_int32().size());
  EXPECT_EQ(2, message.map_int64_int64().size());
  EXPECT_EQ(2, message.map_uint32_uint32().size());
  EXPECT_EQ(2, message.map_uint64_uint64().size());
  EXPECT_EQ(2, message.map_sint32_sint32().size());
  EXPECT_EQ(2, message.map_sint64_sint64().size());
  EXPECT_EQ(2, message.map_fixed32_fixed32().size());
  EXPECT_EQ(2, message.map_fixed64_fixed64().size());
  EXPECT_EQ(2, message.map_sfixed32_sfixed32().size());
  EXPECT_EQ(2, message.map_sfixed64_sfixed64().size());
  EXPECT_EQ(2, message.map_int32_float().size());
  EXPECT_EQ(2, message.map_int32_double().size());
  EXPECT_EQ(2, message.map_bool_bool().size());
  EXPECT_EQ(2, message.map_string_string().size());
  EXPECT_EQ(2, message.map_int32_bytes().size());
  EXPECT_EQ(2, message.map_int32_enum().size());
  EXPECT_EQ(2, message.map_int32_foreign_message().size());

  EXPECT_EQ(0, message.map_int32_int32().at(0));
  EXPECT_EQ(0, message.map_int64_int64().at(0));
  EXPECT_EQ(0, message.map_uint32_uint32().at(0));
  EXPECT_EQ(0, message.map_uint64_uint64().at(0));
  EXPECT_EQ(0, message.map_sint32_sint32().at(0));
  EXPECT_EQ(0, message.map_sint64_sint64().at(0));
  EXPECT_EQ(0, message.map_fixed32_fixed32().at(0));
  EXPECT_EQ(0, message.map_fixed64_fixed64().at(0));
  EXPECT_EQ(0, message.map_sfixed32_sfixed32().at(0));
  EXPECT_EQ(0, message.map_sfixed64_sfixed64().at(0));
  EXPECT_EQ(0, message.map_int32_float().at(0));
  EXPECT_EQ(0, message.map_int32_double().at(0));
  EXPECT_EQ(false, message.map_bool_bool().at(0));
  EXPECT_EQ("0", message.map_string_string().at("0"));
  EXPECT_EQ("0", message.map_int32_bytes().at(0));
  EXPECT_EQ(unittest::MAP_ENUM_BAR, message.map_int32_enum().at(0));
  EXPECT_EQ(0, message.map_int32_foreign_message().at(0).c());

  // Actually verify the second (modified) elements now.
  EXPECT_EQ(2, message.map_int32_int32().at(1));
  EXPECT_EQ(2, message.map_int64_int64().at(1));
  EXPECT_EQ(2, message.map_uint32_uint32().at(1));
  EXPECT_EQ(2, message.map_uint64_uint64().at(1));
  EXPECT_EQ(2, message.map_sint32_sint32().at(1));
  EXPECT_EQ(2, message.map_sint64_sint64().at(1));
  EXPECT_EQ(2, message.map_fixed32_fixed32().at(1));
  EXPECT_EQ(2, message.map_fixed64_fixed64().at(1));
  EXPECT_EQ(2, message.map_sfixed32_sfixed32().at(1));
  EXPECT_EQ(2, message.map_sfixed64_sfixed64().at(1));
  EXPECT_EQ(2, message.map_int32_float().at(1));
  EXPECT_EQ(2, message.map_int32_double().at(1));
  EXPECT_EQ(false, message.map_bool_bool().at(1));
  EXPECT_EQ("2", message.map_string_string().at("1"));
  EXPECT_EQ("2", message.map_int32_bytes().at(1));
  EXPECT_EQ(unittest::MAP_ENUM_FOO, message.map_int32_enum().at(1));
  EXPECT_EQ(2, message.map_int32_foreign_message().at(1).c());
}

void MapTestUtil::ExpectMapsSize(
    const unittest::TestMap& message, int size) {
  const Descriptor* descriptor = message.GetDescriptor();

  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_int32_int32")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_int64_int64")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_uint32_uint32")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_uint64_uint64")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_sint32_sint32")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_sint64_sint64")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_fixed32_fixed32")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_fixed64_fixed64")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_sfixed32_sfixed32")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_sfixed64_sfixed64")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_int32_float")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_int32_double")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_bool_bool")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_string_string")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_int32_bytes")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
    message, descriptor->FindFieldByName("map_int32_foreign_message")));
}

std::vector<const Message*> MapTestUtil::GetMapEntries(
    const unittest::TestMap& message, int index) {
  const Descriptor* descriptor = message.GetDescriptor();
  std::vector<const Message*> result;

  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_int32_int32"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_int64_int64"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_uint32_uint32"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_uint64_uint64"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_sint32_sint32"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_sint64_sint64"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_fixed32_fixed32"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_fixed64_fixed64"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_sfixed32_sfixed32"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_sfixed64_sfixed64"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_int32_float"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_int32_double"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_bool_bool"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_string_string"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_int32_bytes"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_int32_enum"), index));
  result.push_back(&message.GetReflection()->GetRepeatedMessage(
    message, descriptor->FindFieldByName("map_int32_foreign_message"), index));

  return result;
}

std::vector<const Message*> MapTestUtil::GetMapEntriesFromRelease(
    unittest::TestMap* message) {
  const Descriptor* descriptor = message->GetDescriptor();
  std::vector<const Message*> result;

  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_int32_int32")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_int64_int64")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_uint32_uint32")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_uint64_uint64")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_sint32_sint32")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_sint64_sint64")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_fixed32_fixed32")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_fixed64_fixed64")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_sfixed32_sfixed32")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_sfixed64_sfixed64")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_int32_float")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_int32_double")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_bool_bool")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_string_string")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_int32_bytes")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_int32_enum")));
  result.push_back(message->GetReflection()->ReleaseLast(
    message, descriptor->FindFieldByName("map_int32_foreign_message")));

  return result;
}

MapTestUtil::MapReflectionTester::MapReflectionTester(
    const Descriptor* base_descriptor)
  : base_descriptor_(base_descriptor) {
  const DescriptorPool* pool = base_descriptor->file()->pool();

  map_enum_foo_ = pool->FindEnumValueByName("protobuf_unittest.MAP_ENUM_FOO");
  map_enum_bar_ = pool->FindEnumValueByName("protobuf_unittest.MAP_ENUM_BAR");
  map_enum_baz_ = pool->FindEnumValueByName("protobuf_unittest.MAP_ENUM_BAZ");

  foreign_c_ = pool->FindFieldByName(
      "protobuf_unittest.ForeignMessage.c");
  map_int32_int32_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32Int32Entry.key");
  map_int32_int32_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32Int32Entry.value");
  map_int64_int64_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt64Int64Entry.key");
  map_int64_int64_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt64Int64Entry.value");
  map_uint32_uint32_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapUint32Uint32Entry.key");
  map_uint32_uint32_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapUint32Uint32Entry.value");
  map_uint64_uint64_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapUint64Uint64Entry.key");
  map_uint64_uint64_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapUint64Uint64Entry.value");
  map_sint32_sint32_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapSint32Sint32Entry.key");
  map_sint32_sint32_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapSint32Sint32Entry.value");
  map_sint64_sint64_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapSint64Sint64Entry.key");
  map_sint64_sint64_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapSint64Sint64Entry.value");
  map_fixed32_fixed32_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapFixed32Fixed32Entry.key");
  map_fixed32_fixed32_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapFixed32Fixed32Entry.value");
  map_fixed64_fixed64_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapFixed64Fixed64Entry.key");
  map_fixed64_fixed64_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapFixed64Fixed64Entry.value");
  map_sfixed32_sfixed32_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapSfixed32Sfixed32Entry.key");
  map_sfixed32_sfixed32_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapSfixed32Sfixed32Entry.value");
  map_sfixed64_sfixed64_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapSfixed64Sfixed64Entry.key");
  map_sfixed64_sfixed64_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapSfixed64Sfixed64Entry.value");
  map_int32_float_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32FloatEntry.key");
  map_int32_float_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32FloatEntry.value");
  map_int32_double_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32DoubleEntry.key");
  map_int32_double_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32DoubleEntry.value");
  map_bool_bool_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapBoolBoolEntry.key");
  map_bool_bool_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapBoolBoolEntry.value");
  map_string_string_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapStringStringEntry.key");
  map_string_string_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapStringStringEntry.value");
  map_int32_bytes_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32BytesEntry.key");
  map_int32_bytes_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32BytesEntry.value");
  map_int32_enum_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32EnumEntry.key");
  map_int32_enum_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32EnumEntry.value");
  map_int32_foreign_message_key_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32ForeignMessageEntry.key");
  map_int32_foreign_message_val_ = pool->FindFieldByName(
      "protobuf_unittest.TestMap.MapInt32ForeignMessageEntry.value");

  EXPECT_FALSE(map_enum_foo_ == NULL);
  EXPECT_FALSE(map_enum_bar_ == NULL);
  EXPECT_FALSE(map_enum_baz_ == NULL);
  EXPECT_FALSE(map_int32_int32_key_ == NULL);
  EXPECT_FALSE(map_int32_int32_val_ == NULL);
  EXPECT_FALSE(map_int64_int64_key_ == NULL);
  EXPECT_FALSE(map_int64_int64_val_ == NULL);
  EXPECT_FALSE(map_uint32_uint32_key_ == NULL);
  EXPECT_FALSE(map_uint32_uint32_val_ == NULL);
  EXPECT_FALSE(map_uint64_uint64_key_ == NULL);
  EXPECT_FALSE(map_uint64_uint64_val_ == NULL);
  EXPECT_FALSE(map_sint32_sint32_key_ == NULL);
  EXPECT_FALSE(map_sint32_sint32_val_ == NULL);
  EXPECT_FALSE(map_sint64_sint64_key_ == NULL);
  EXPECT_FALSE(map_sint64_sint64_val_ == NULL);
  EXPECT_FALSE(map_fixed32_fixed32_key_ == NULL);
  EXPECT_FALSE(map_fixed32_fixed32_val_ == NULL);
  EXPECT_FALSE(map_fixed64_fixed64_key_ == NULL);
  EXPECT_FALSE(map_fixed64_fixed64_val_ == NULL);
  EXPECT_FALSE(map_sfixed32_sfixed32_key_ == NULL);
  EXPECT_FALSE(map_sfixed32_sfixed32_val_ == NULL);
  EXPECT_FALSE(map_sfixed64_sfixed64_key_ == NULL);
  EXPECT_FALSE(map_sfixed64_sfixed64_val_ == NULL);
  EXPECT_FALSE(map_int32_float_key_ == NULL);
  EXPECT_FALSE(map_int32_float_val_ == NULL);
  EXPECT_FALSE(map_int32_double_key_ == NULL);
  EXPECT_FALSE(map_int32_double_val_ == NULL);
  EXPECT_FALSE(map_bool_bool_key_ == NULL);
  EXPECT_FALSE(map_bool_bool_val_ == NULL);
  EXPECT_FALSE(map_string_string_key_ == NULL);
  EXPECT_FALSE(map_string_string_val_ == NULL);
  EXPECT_FALSE(map_int32_bytes_key_ == NULL);
  EXPECT_FALSE(map_int32_bytes_val_ == NULL);
  EXPECT_FALSE(map_int32_enum_key_ == NULL);
  EXPECT_FALSE(map_int32_enum_val_ == NULL);
  EXPECT_FALSE(map_int32_foreign_message_key_ == NULL);
  EXPECT_FALSE(map_int32_foreign_message_val_ == NULL);
}

// Shorthand to get a FieldDescriptor for a field of unittest::TestMap.
const FieldDescriptor* MapTestUtil::MapReflectionTester::F(const string& name) {
  const FieldDescriptor* result = NULL;
  result = base_descriptor_->FindFieldByName(name);
  GOOGLE_CHECK(result != NULL);
  return result;
}

void MapTestUtil::MapReflectionTester::SetMapFieldsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();
  Message* sub_message = NULL;
  Message* sub_foreign_message = NULL;

  // Add first element.
  sub_message = reflection->AddMessage(message, F("map_int32_int32"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_int32_key_, 0);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_int32_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_int64_int64"));
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_int64_int64_key_, 0);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_int64_int64_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_uint32_uint32"));
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_uint32_uint32_key_, 0);
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_uint32_uint32_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_uint64_uint64"));
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_uint64_uint64_key_, 0);
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_uint64_uint64_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_sint32_sint32"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sint32_sint32_key_, 0);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sint32_sint32_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_sint64_sint64"));
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sint64_sint64_key_, 0);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sint64_sint64_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_fixed32_fixed32"));
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_fixed32_fixed32_key_, 0);
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_fixed32_fixed32_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_fixed64_fixed64"));
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_fixed64_fixed64_key_, 0);
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_fixed64_fixed64_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_sfixed32_sfixed32"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sfixed32_sfixed32_key_, 0);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sfixed32_sfixed32_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_sfixed64_sfixed64"));
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sfixed64_sfixed64_key_, 0);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sfixed64_sfixed64_val_, 0);

  sub_message = reflection->AddMessage(message, F("map_int32_float"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_float_key_, 0);
  sub_message->GetReflection()
      ->SetFloat(sub_message, map_int32_float_val_, 0.0);

  sub_message = reflection->AddMessage(message, F("map_int32_double"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_double_key_, 0);
  sub_message->GetReflection()
      ->SetDouble(sub_message, map_int32_double_val_, 0.0);

  sub_message = reflection->AddMessage(message, F("map_bool_bool"));
  sub_message->GetReflection()
      ->SetBool(sub_message, map_bool_bool_key_, false);
  sub_message->GetReflection()
      ->SetBool(sub_message, map_bool_bool_val_, false);

  sub_message = reflection->AddMessage(message, F("map_string_string"));
  sub_message->GetReflection()
      ->SetString(sub_message, map_string_string_key_, "0");
  sub_message->GetReflection()
      ->SetString(sub_message, map_string_string_val_, "0");

  sub_message = reflection->AddMessage(message, F("map_int32_bytes"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_bytes_key_, 0);
  sub_message->GetReflection()
      ->SetString(sub_message, map_int32_bytes_val_, "0");

  sub_message = reflection->AddMessage(message, F("map_int32_enum"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_enum_key_, 0);
  sub_message->GetReflection()
      ->SetEnum(sub_message, map_int32_enum_val_, map_enum_bar_);

  sub_message = reflection
      ->AddMessage(message, F("map_int32_foreign_message"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_foreign_message_key_, 0);
  sub_foreign_message = sub_message->GetReflection()->
      MutableMessage(sub_message, map_int32_foreign_message_val_, NULL);
  sub_foreign_message->GetReflection()->
      SetInt32(sub_foreign_message, foreign_c_, 0);

  // Add second element
  sub_message = reflection->AddMessage(message, F("map_int32_int32"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_int32_key_, 1);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_int32_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_int64_int64"));
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_int64_int64_key_, 1);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_int64_int64_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_uint32_uint32"));
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_uint32_uint32_key_, 1);
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_uint32_uint32_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_uint64_uint64"));
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_uint64_uint64_key_, 1);
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_uint64_uint64_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_sint32_sint32"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sint32_sint32_key_, 1);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sint32_sint32_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_sint64_sint64"));
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sint64_sint64_key_, 1);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sint64_sint64_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_fixed32_fixed32"));
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_fixed32_fixed32_key_, 1);
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_fixed32_fixed32_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_fixed64_fixed64"));
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_fixed64_fixed64_key_, 1);
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_fixed64_fixed64_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_sfixed32_sfixed32"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sfixed32_sfixed32_key_, 1);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sfixed32_sfixed32_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_sfixed64_sfixed64"));
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sfixed64_sfixed64_key_, 1);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sfixed64_sfixed64_val_, 1);

  sub_message = reflection->AddMessage(message, F("map_int32_float"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_float_key_, 1);
  sub_message->GetReflection()
      ->SetFloat(sub_message, map_int32_float_val_, 1.0);

  sub_message = reflection->AddMessage(message, F("map_int32_double"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_double_key_, 1);
  sub_message->GetReflection()
      ->SetDouble(sub_message, map_int32_double_val_, 1.0);

  sub_message = reflection->AddMessage(message, F("map_bool_bool"));
  sub_message->GetReflection()
      ->SetBool(sub_message, map_bool_bool_key_, true);
  sub_message->GetReflection()
      ->SetBool(sub_message, map_bool_bool_val_, true);

  sub_message = reflection->AddMessage(message, F("map_string_string"));
  sub_message->GetReflection()
      ->SetString(sub_message, map_string_string_key_, "1");
  sub_message->GetReflection()
      ->SetString(sub_message, map_string_string_val_, "1");

  sub_message = reflection->AddMessage(message, F("map_int32_bytes"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_bytes_key_, 1);
  sub_message->GetReflection()
      ->SetString(sub_message, map_int32_bytes_val_, "1");

  sub_message = reflection->AddMessage(message, F("map_int32_enum"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_enum_key_, 1);
  sub_message->GetReflection()
      ->SetEnum(sub_message, map_int32_enum_val_, map_enum_baz_);

  sub_message = reflection
      ->AddMessage(message, F("map_int32_foreign_message"));
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_foreign_message_key_, 1);
  sub_foreign_message = sub_message->GetReflection()->
      MutableMessage(sub_message, map_int32_foreign_message_val_, NULL);
  sub_foreign_message->GetReflection()->
      SetInt32(sub_foreign_message, foreign_c_, 1);
}

void MapTestUtil::MapReflectionTester::ClearMapFieldsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();

  reflection->ClearField(message, F("map_int32_int32"));
  reflection->ClearField(message, F("map_int64_int64"));
  reflection->ClearField(message, F("map_uint32_uint32"));
  reflection->ClearField(message, F("map_uint64_uint64"));
  reflection->ClearField(message, F("map_sint32_sint32"));
  reflection->ClearField(message, F("map_sint64_sint64"));
  reflection->ClearField(message, F("map_fixed32_fixed32"));
  reflection->ClearField(message, F("map_fixed64_fixed64"));
  reflection->ClearField(message, F("map_sfixed32_sfixed32"));
  reflection->ClearField(message, F("map_sfixed64_sfixed64"));
  reflection->ClearField(message, F("map_int32_float"));
  reflection->ClearField(message, F("map_int32_double"));
  reflection->ClearField(message, F("map_bool_bool"));
  reflection->ClearField(message, F("map_string_string"));
  reflection->ClearField(message, F("map_int32_bytes"));
  reflection->ClearField(message, F("map_int32_enum"));
  reflection->ClearField(message, F("map_int32_foreign_message"));
}

void MapTestUtil::MapReflectionTester::ModifyMapFieldsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();
  Message* sub_message;
  Message* sub_foreign_message;

  // Find out which one's key is 0.
  int size = reflection->FieldSize(*message, F("map_int32_int32"));
  int target = 0;
  for (int i = 0; i < size; i++) {
    const Message& temp_message = reflection
      ->GetRepeatedMessage(*message, F("map_int32_int32"), i);
    if (temp_message.GetReflection()
          ->GetInt32(temp_message, map_int32_int32_key_) == 1) {
      target = i;
    }
  }

  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_int32_int32"), target);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_int32_int32_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_int64_int64"), target);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_int64_int64_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_uint32_uint32"), target);
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_uint32_uint32_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_uint64_uint64"), target);
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_uint64_uint64_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_sint32_sint32"), target);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sint32_sint32_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_sint64_sint64"), target);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sint64_sint64_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_fixed32_fixed32"), target);
  sub_message->GetReflection()
      ->SetUInt32(sub_message, map_fixed32_fixed32_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_fixed64_fixed64"), target);
  sub_message->GetReflection()
      ->SetUInt64(sub_message, map_fixed64_fixed64_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_sfixed32_sfixed32"), target);
  sub_message->GetReflection()
      ->SetInt32(sub_message, map_sfixed32_sfixed32_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_sfixed64_sfixed64"), target);
  sub_message->GetReflection()
      ->SetInt64(sub_message, map_sfixed64_sfixed64_val_, 2);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_int32_float"), target);
  sub_message->GetReflection()
      ->SetFloat(sub_message, map_int32_float_val_, 2.0);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_int32_double"), target);
  sub_message->GetReflection()
      ->SetDouble(sub_message, map_int32_double_val_, 2.0);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_bool_bool"), target);
  sub_message->GetReflection()
      ->SetBool(sub_message, map_bool_bool_val_, false);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_string_string"), target);
  sub_message->GetReflection()
      ->SetString(sub_message, map_string_string_val_, "2");
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_int32_bytes"), target);
  sub_message->GetReflection()
      ->SetString(sub_message, map_int32_bytes_val_, "2");
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_int32_enum"), target);
  sub_message->GetReflection()
      ->SetEnum(sub_message, map_int32_enum_val_, map_enum_foo_);
  sub_message = reflection
      ->MutableRepeatedMessage(message, F("map_int32_foreign_message"), target);
  sub_foreign_message = sub_message->GetReflection()->
      MutableMessage(sub_message, map_int32_foreign_message_val_, NULL);
  sub_foreign_message->GetReflection()->
      SetInt32(sub_foreign_message, foreign_c_, 2);
}

void MapTestUtil::MapReflectionTester::RemoveLastMapsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();

  vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (int i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;
    reflection->RemoveLast(message, field);
  }
}

void MapTestUtil::MapReflectionTester::ReleaseLastMapsViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();

  vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (int i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;
    if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) continue;

    Message* released = reflection->ReleaseLast(message, field);
    ASSERT_TRUE(released != NULL) << "ReleaseLast returned NULL for: "
                                  << field->name();
    delete released;
  }
}

void MapTestUtil::MapReflectionTester::SwapMapsViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();
  vector<const FieldDescriptor*> output;
  reflection->ListFields(*message, &output);
  for (int i = 0; i < output.size(); ++i) {
    const FieldDescriptor* field = output[i];
    if (!field->is_repeated()) continue;
    reflection->SwapElements(message, field, 0, 1);
  }
}

void MapTestUtil::MapReflectionTester::
    MutableUnknownFieldsOfMapFieldsViaReflection(Message* message) {
  const Reflection* reflection = message->GetReflection();
  Message* sub_message = NULL;

  sub_message = reflection->AddMessage(message, F("map_int32_int32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_int64_int64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_uint32_uint32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_uint64_uint64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_sint32_sint32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_sint64_sint64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_fixed32_fixed32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_fixed64_fixed64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_sfixed32_sfixed32"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_sfixed64_sfixed64"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_int32_float"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_int32_double"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_bool_bool"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_string_string"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_int32_bytes"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_int32_enum"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
  sub_message = reflection->AddMessage(message, F("map_int32_foreign_message"));
  EXPECT_TRUE(sub_message->GetReflection()->MutableUnknownFields(sub_message) !=
              NULL);
}

void MapTestUtil::MapReflectionTester::ExpectMapFieldsSetViaReflection(
    const Message& message) {
  string scratch;
  const Reflection* reflection = message.GetReflection();
  const Message* sub_message;

  // -----------------------------------------------------------------

  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_int32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int64_int64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_uint32_uint32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_uint64_uint64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_sint32_sint32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_sint64_sint64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_fixed32_fixed32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_fixed64_fixed64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_sfixed32_sfixed32")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_sfixed64_sfixed64")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_float")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_double")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_bool_bool")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_string_string")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_bytes")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_enum")));
  ASSERT_EQ(2, reflection->FieldSize(message, F("map_int32_foreign_message")));

  {
    std::map<int32, int32> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_int32_int32"), i);
      int32 key = sub_message->GetReflection()->GetInt32(
          *sub_message, map_int32_int32_key_);
      int32 val = sub_message->GetReflection()->GetInt32(
          *sub_message, map_int32_int32_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int64, int64> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_int64_int64"), i);
      int64 key = sub_message->GetReflection()->GetInt64(
          *sub_message, map_int64_int64_key_);
      int64 val = sub_message->GetReflection()->GetInt64(
          *sub_message, map_int64_int64_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<uint32, uint32> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_uint32_uint32"), i);
      uint32 key = sub_message->GetReflection()->GetUInt32(
          *sub_message, map_uint32_uint32_key_);
      uint32 val = sub_message->GetReflection()->GetUInt32(
          *sub_message, map_uint32_uint32_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<uint64, uint64> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_uint64_uint64"), i);
      uint64 key = sub_message->GetReflection()->GetUInt64(
          *sub_message, map_uint64_uint64_key_);
      uint64 val = sub_message->GetReflection()->GetUInt64(
          *sub_message, map_uint64_uint64_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int32, int32> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_sint32_sint32"), i);
      int32 key = sub_message->GetReflection()->GetInt32(
          *sub_message, map_sint32_sint32_key_);
      int32 val = sub_message->GetReflection()->GetInt32(
          *sub_message, map_sint32_sint32_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int64, int64> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_sint64_sint64"), i);
      int64 key = sub_message->GetReflection()->GetInt64(
          *sub_message, map_sint64_sint64_key_);
      int64 val = sub_message->GetReflection()->GetInt64(
          *sub_message, map_sint64_sint64_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<uint32, uint32> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_fixed32_fixed32"), i);
      uint32 key = sub_message->GetReflection()->GetUInt32(
          *sub_message, map_fixed32_fixed32_key_);
      uint32 val = sub_message->GetReflection()->GetUInt32(
          *sub_message, map_fixed32_fixed32_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<uint64, uint64> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_fixed64_fixed64"), i);
      uint64 key = sub_message->GetReflection()->GetUInt64(
          *sub_message, map_fixed64_fixed64_key_);
      uint64 val = sub_message->GetReflection()->GetUInt64(
          *sub_message, map_fixed64_fixed64_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int32, int32> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message = &reflection->GetRepeatedMessage(
          message, F("map_sfixed32_sfixed32"), i);
      int32 key = sub_message->GetReflection()->GetInt32(
          *sub_message, map_sfixed32_sfixed32_key_);
      int32 val = sub_message->GetReflection()->GetInt32(
          *sub_message, map_sfixed32_sfixed32_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int64, int64> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message = &reflection->GetRepeatedMessage(
          message, F("map_sfixed64_sfixed64"), i);
      int64 key = sub_message->GetReflection()->GetInt64(
          *sub_message, map_sfixed64_sfixed64_key_);
      int64 val = sub_message->GetReflection()->GetInt64(
          *sub_message, map_sfixed64_sfixed64_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int32, float> map;
    map[0] = 0.0;
    map[1] = 1.0;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_int32_float"), i);
      int32  key = sub_message->GetReflection()->GetInt32(
          *sub_message, map_int32_float_key_);
      float val = sub_message->GetReflection()->GetFloat(
          *sub_message, map_int32_float_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int32, double> map;
    map[0] = 0.0;
    map[1] = 1.0;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_int32_double"), i);
      int32  key = sub_message->GetReflection()->GetInt32(
          *sub_message, map_int32_double_key_);
      double val = sub_message->GetReflection()->GetDouble(
          *sub_message, map_int32_double_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<bool, bool> map;
    map[false] = false;
    map[true] = true;
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_bool_bool"), i);
      bool key = sub_message->GetReflection()->GetBool(
          *sub_message, map_bool_bool_key_);
      bool val = sub_message->GetReflection()->GetBool(
          *sub_message, map_bool_bool_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<string, string> map;
    map["0"] = "0";
    map["1"] = "1";
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_string_string"), i);
      string  key = sub_message->GetReflection()->GetString(
          *sub_message, map_string_string_key_);
      string val = sub_message->GetReflection()->GetString(
          *sub_message, map_string_string_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int32, string> map;
    map[0] = "0";
    map[1] = "1";
    for (int i = 0; i < 2; i++) {
      sub_message =
          &reflection->GetRepeatedMessage(message, F("map_int32_bytes"), i);
      int32  key = sub_message->GetReflection()->GetInt32(
          *sub_message, map_int32_bytes_key_);
      string val = sub_message->GetReflection()->GetString(
          *sub_message, map_int32_bytes_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int32, const EnumValueDescriptor*> map;
    map[0] = map_enum_bar_;
    map[1] = map_enum_baz_;
    for (int i = 0; i < 2; i++) {
      sub_message = &reflection->GetRepeatedMessage(
          message, F("map_int32_enum"), i);
      int32 key = sub_message->GetReflection()->GetInt32(
          *sub_message, map_int32_enum_key_);
      const EnumValueDescriptor* val = sub_message->GetReflection()->GetEnum(
          *sub_message, map_int32_enum_val_);
      EXPECT_EQ(map[key], val);
    }
  }
  {
    std::map<int32, int32> map;
    map[0] = 0;
    map[1] = 1;
    for (int i = 0; i < 2; i++) {
      sub_message = &reflection->GetRepeatedMessage(
          message, F("map_int32_foreign_message"), i);
      int32  key = sub_message->GetReflection()->GetInt32(
          *sub_message, map_int32_foreign_message_key_);
      const Message& foreign_message = sub_message->GetReflection()->GetMessage(
          *sub_message, map_int32_foreign_message_val_);
      int32 val = foreign_message.GetReflection()->GetInt32(
          foreign_message, foreign_c_);
      EXPECT_EQ(map[key], val);
    }
  }
}

void MapTestUtil::MapReflectionTester::ExpectClearViaReflection(
    const Message& message) {
  const Reflection* reflection = message.GetReflection();
  // Map fields are empty.
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_int32_int32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_int64_int64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_uint32_uint32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_uint64_uint64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_sint32_sint32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_sint64_sint64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_fixed32_fixed32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_fixed64_fixed64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_sfixed32_sfixed32")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_sfixed64_sfixed64")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_int32_float")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_int32_double")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_bool_bool")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_string_string")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_int32_bytes")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_int32_enum")));
  EXPECT_EQ(0, reflection->FieldSize(message, F("map_int32_foreign_message")));
}

void MapTestUtil::MapReflectionTester::ExpectMapEntryClearViaReflection(
    Message* message) {
  const Reflection* reflection = message->GetReflection();
  const Message* sub_message;

  {
    const FieldDescriptor* descriptor = F("map_int32_int32");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_int32_int32"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_int64_int64");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_int64_int64"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt64(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt64(*sub_message,
                                                        value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_uint32_uint32");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_uint32_uint32"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetUInt32(*sub_message,
                                                         key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetUInt32(*sub_message,
                                                         value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_uint64_uint64");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_uint64_uint64"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetUInt64(*sub_message,
                                                         key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetUInt64(*sub_message,
                                                         value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_sint32_sint32");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_sint32_sint32"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_sint64_sint64");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_sint64_sint64"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt64(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt64(*sub_message,
                                                        value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_fixed32_fixed32");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_fixed32_fixed32"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetUInt32(*sub_message,
                                                         key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetUInt32(*sub_message,
                                                         value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_fixed64_fixed64");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_fixed64_fixed64"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetUInt64(*sub_message,
                                                         key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetUInt64(*sub_message,
                                                         value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_sfixed32_sfixed32");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_sfixed32_sfixed32"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_sfixed64_sfixed64");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_sfixed64_sfixed64"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt64(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt64(*sub_message,
                                                        value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_int32_float");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_int32_float"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetFloat(*sub_message,
                                                        value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_int32_double");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_int32_double"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetDouble(*sub_message,
                                                         value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_bool_bool");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_bool_bool"));
    EXPECT_EQ(false, sub_message->GetReflection()->GetBool(*sub_message,
                                                           key_descriptor));
    EXPECT_EQ(false, sub_message->GetReflection()->GetBool(*sub_message,
                                                           value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_string_string");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_string_string"));
    EXPECT_EQ("", sub_message->GetReflection()->GetString(*sub_message,
                                                          key_descriptor));
    EXPECT_EQ("", sub_message->GetReflection()->GetString(*sub_message,
                                                          value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_int32_bytes");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_int32_bytes"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ("", sub_message->GetReflection()->GetString(*sub_message,
                                                        value_descriptor));
  }
  {
    const FieldDescriptor* descriptor = F("map_int32_enum");
    const FieldDescriptor* key_descriptor =
        descriptor->message_type()->FindFieldByName("key");
    const FieldDescriptor* value_descriptor =
        descriptor->message_type()->FindFieldByName("value");
    sub_message = reflection->AddMessage(message, F("map_int32_enum"));
    EXPECT_EQ(0, sub_message->GetReflection()->GetInt32(*sub_message,
                                                        key_descriptor));
    EXPECT_EQ(0, sub_message->GetReflection()->GetEnum(*sub_message,
                                                        value_descriptor));
  }
  // Map using message as value has been tested in other place. Thus, we don't
  // test it here.
}

}  // namespace protobuf
}  // namespace google
