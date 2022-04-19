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

#include <google/protobuf/descriptor.h>
#include <google/protobuf/map_test_util_impl.h>
#include <google/protobuf/message.h>

namespace google {
namespace protobuf {

class MapTestUtil {
 public:
  // Set every field in the TestMap message to a unique value.
  static void SetMapFields(UNITTEST::TestMap* message);

  // Set every field in the TestArenaMap message to a unique value.
  static void SetArenaMapFields(UNITTEST::TestArenaMap* message);

  // Set every field in the message to a default value.
  static void SetMapFieldsInitialized(UNITTEST::TestMap* message);

  // Modify all the map fields of the message (which should already have been
  // initialized with SetMapFields()).
  static void ModifyMapFields(UNITTEST::TestMap* message);

  // Check that all fields have the values that they should have after
  // SetMapFields() is called.
  static void ExpectMapFieldsSet(const UNITTEST::TestMap& message);

  // Check that all fields have the values that they should have after
  // SetMapFields() is called for TestArenaMap.
  static void ExpectArenaMapFieldsSet(const UNITTEST::TestArenaMap& message);

  // Check that all fields have the values that they should have after
  // SetMapFieldsInitialized() is called.
  static void ExpectMapFieldsSetInitialized(const UNITTEST::TestMap& message);

  // Expect that the message is modified as would be expected from
  // ModifyMapFields().
  static void ExpectMapFieldsModified(const UNITTEST::TestMap& message);

  // Check that all fields are empty.
  static void ExpectClear(const UNITTEST::TestMap& message);

  // Check that all map fields have the given size.
  static void ExpectMapsSize(const UNITTEST::TestMap& message, int size);

  // Get pointers of map entries at given index.
  static std::vector<const Message*> GetMapEntries(
      const UNITTEST::TestMap& message, int index);

  // Get pointers of map entries from release.
  static std::vector<const Message*> GetMapEntriesFromRelease(
      UNITTEST::TestMap* message);
};

inline void MapTestUtil::SetMapFields(UNITTEST::TestMap* message) {
  MapTestUtilImpl::SetMapFields<UNITTEST::MapEnum, UNITTEST::MAP_ENUM_BAR,
                                UNITTEST::MAP_ENUM_BAZ>(message);
}

inline void MapTestUtil::SetArenaMapFields(UNITTEST::TestArenaMap* message) {
  MapTestUtilImpl::SetArenaMapFields<UNITTEST::MapEnum, UNITTEST::MAP_ENUM_BAR,
                                     UNITTEST::MAP_ENUM_BAZ>(message);
}

inline void MapTestUtil::SetMapFieldsInitialized(UNITTEST::TestMap* message) {
  MapTestUtilImpl::SetMapFieldsInitialized(message);
}

inline void MapTestUtil::ModifyMapFields(UNITTEST::TestMap* message) {
  MapTestUtilImpl::ModifyMapFields<UNITTEST::MapEnum, UNITTEST::MAP_ENUM_FOO>(
      message);
}

inline void MapTestUtil::ExpectClear(const UNITTEST::TestMap& message) {
  MapTestUtilImpl::ExpectClear(message);
}

inline void MapTestUtil::ExpectMapFieldsSet(const UNITTEST::TestMap& message) {
  MapTestUtilImpl::ExpectMapFieldsSet<UNITTEST::MapEnum, UNITTEST::MAP_ENUM_BAR,
                                      UNITTEST::MAP_ENUM_BAZ>(message);
}

inline void MapTestUtil::ExpectArenaMapFieldsSet(
    const UNITTEST::TestArenaMap& message) {
  MapTestUtilImpl::ExpectArenaMapFieldsSet<
      UNITTEST::MapEnum, UNITTEST::MAP_ENUM_BAR, UNITTEST::MAP_ENUM_BAZ>(
      message);
}

inline void MapTestUtil::ExpectMapFieldsSetInitialized(
    const UNITTEST::TestMap& message) {
  MapTestUtilImpl::ExpectMapFieldsSetInitialized<UNITTEST::MapEnum,
                                                 UNITTEST::MAP_ENUM_FOO>(
      message);
}

inline void MapTestUtil::ExpectMapFieldsModified(
    const UNITTEST::TestMap& message) {
  MapTestUtilImpl::ExpectMapFieldsModified<
      UNITTEST::MapEnum, UNITTEST::MAP_ENUM_BAR, UNITTEST::MAP_ENUM_FOO>(
      message);
}

inline void MapTestUtil::ExpectMapsSize(const UNITTEST::TestMap& message,
                                        int size) {
  const Descriptor* descriptor = message.GetDescriptor();

  EXPECT_EQ(size, message.GetReflection()->FieldSize(
                      message, descriptor->FindFieldByName("map_int32_int32")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
                      message, descriptor->FindFieldByName("map_int64_int64")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_uint32_uint32")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_uint64_uint64")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_sint32_sint32")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_sint64_sint64")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_fixed32_fixed32")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_fixed64_fixed64")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_sfixed32_sfixed32")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_sfixed64_sfixed64")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
                      message, descriptor->FindFieldByName("map_int32_float")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_int32_double")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
                      message, descriptor->FindFieldByName("map_bool_bool")));
  EXPECT_EQ(size,
            message.GetReflection()->FieldSize(
                message, descriptor->FindFieldByName("map_string_string")));
  EXPECT_EQ(size, message.GetReflection()->FieldSize(
                      message, descriptor->FindFieldByName("map_int32_bytes")));
  EXPECT_EQ(
      size,
      message.GetReflection()->FieldSize(
          message, descriptor->FindFieldByName("map_int32_foreign_message")));
}

inline std::vector<const Message*> MapTestUtil::GetMapEntries(
    const UNITTEST::TestMap& message, int index) {
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
      message, descriptor->FindFieldByName("map_int32_foreign_message"),
      index));

  return result;
}

inline std::vector<const Message*> MapTestUtil::GetMapEntriesFromRelease(
    UNITTEST::TestMap* message) {
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

}  // namespace protobuf
}  // namespace google
