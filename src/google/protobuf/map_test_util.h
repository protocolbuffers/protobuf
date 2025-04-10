// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MAP_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_MAP_TEST_UTIL_H__

#include <string>

#include <gtest/gtest.h>
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace google {
namespace protobuf {

class MapTestUtil {
 public:
  template <typename T>
  using MapEnum = std::decay_t<decltype(T().map_int32_enum().at(0))>;

  template <typename T>
  static auto MapEnum_FOO() -> decltype(MapEnum<T>::MAP_ENUM_FOO) {
    return MapEnum<T>::MAP_ENUM_FOO;
  }
  template <typename T>
  static auto MapEnum_FOO() -> decltype(MapEnum<T>::MAP_ENUM_FOO_LITE) {
    return MapEnum<T>::MAP_ENUM_FOO_LITE;
  }

  template <typename T>
  static auto MapEnum_BAR() -> decltype(MapEnum<T>::MAP_ENUM_BAR) {
    return MapEnum<T>::MAP_ENUM_BAR;
  }
  template <typename T>
  static auto MapEnum_BAR() -> decltype(MapEnum<T>::MAP_ENUM_BAR_LITE) {
    return MapEnum<T>::MAP_ENUM_BAR_LITE;
  }

  template <typename T>
  static auto MapEnum_BAZ() -> decltype(MapEnum<T>::MAP_ENUM_BAZ) {
    return MapEnum<T>::MAP_ENUM_BAZ;
  }
  template <typename T>
  static auto MapEnum_BAZ() -> decltype(MapEnum<T>::MAP_ENUM_BAZ_LITE) {
    return MapEnum<T>::MAP_ENUM_BAZ_LITE;
  }

  // Set every field in the TestMap message to a unique value.
  template <typename TestMap>
  static void SetMapFields(TestMap* message);

  // Set every field in the TestArenaMap message to a unique value.
  template <typename TestArenaMap>
  static void SetArenaMapFields(TestArenaMap* message);

  // Set every field in the message to a default value.
  template <typename TestMap>
  static void SetMapFieldsInitialized(TestMap* message);

  // Modify all the map fields of the message (which should already have been
  // initialized with SetMapFields()).
  template <typename TestMap>
  static void ModifyMapFields(TestMap* message);

  // Check that all fields have the values that they should have after
  // SetMapFields() is called.
  template <typename TestMap>
  static void ExpectMapFieldsSet(const TestMap& message);

  // Check that all fields have the values that they should have after
  // SetMapFields() is called for TestArenaMap.
  template <typename TestArenaMap>
  static void ExpectArenaMapFieldsSet(const TestArenaMap& message);

  // Check that all fields have the values that they should have after
  // SetMapFieldsInitialized() is called.
  template <typename TestMap>
  static void ExpectMapFieldsSetInitialized(const TestMap& message);

  // Expect that the message is modified as would be expected from
  // ModifyMapFields().
  template <typename TestMap>
  static void ExpectMapFieldsModified(const TestMap& message);

  // Check that all fields are empty.
  template <typename TestMap>
  static void ExpectClear(const TestMap& message);

  // Check that all map fields have the given size.
  template <typename TestMap>
  static void ExpectMapsSize(const TestMap& message, int size);

  // Get pointers of map entries at given index.
  template <typename TestMap>
  static std::vector<const Message*> GetMapEntries(const TestMap& message,
                                                   int index);

  // Get pointers of map entries from release.
  template <typename TestMap>
  static std::vector<const Message*> GetMapEntriesFromRelease(TestMap* message);

  static std::string long_string() {
    return "This is a very long string that goes in the heap";
  }
  static std::string long_string_2() {
    return "This is another very long string that goes in the heap";
  }
};

template <typename TestMap>
void MapTestUtil::SetMapFields(TestMap* message) {
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
  (*message->mutable_map_string_string())[long_string()] = long_string();
  (*message->mutable_map_int32_bytes())[0] = long_string();
  (*message->mutable_map_int32_enum())[0] = MapEnum_BAR<TestMap>();
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
  (*message->mutable_map_string_string())[long_string_2()] = long_string_2();
  (*message->mutable_map_int32_bytes())[1] = long_string_2();
  (*message->mutable_map_int32_enum())[1] = MapEnum_BAZ<TestMap>();
  (*message->mutable_map_int32_foreign_message())[1].set_c(1);
}

template <typename TestArenaMap>
void MapTestUtil::SetArenaMapFields(TestArenaMap* message) {
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
  (*message->mutable_map_string_string())[long_string()] = long_string();
  (*message->mutable_map_int32_bytes())[0] = long_string();
  (*message->mutable_map_int32_enum())[0] = MapEnum_BAR<TestArenaMap>();
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
  (*message->mutable_map_string_string())[long_string_2()] = long_string_2();
  (*message->mutable_map_int32_bytes())[1] = long_string_2();
  (*message->mutable_map_int32_enum())[1] = MapEnum_BAZ<TestArenaMap>();
  (*message->mutable_map_int32_foreign_message())[1].set_c(1);
}

template <typename TestMap>
void MapTestUtil::SetMapFieldsInitialized(TestMap* message) {
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
  (*message->mutable_map_string_string())[long_string()];
  (*message->mutable_map_int32_bytes())[0];
  (*message->mutable_map_int32_enum())[0];
  (*message->mutable_map_int32_foreign_message())[0];
}

template <typename TestMap>
void MapTestUtil::ModifyMapFields(TestMap* message) {
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
  (*message->mutable_map_string_string())[long_string_2()] = "2";
  (*message->mutable_map_int32_bytes())[1] = "2";
  (*message->mutable_map_int32_enum())[1] = MapEnum_FOO<TestMap>();
  (*message->mutable_map_int32_foreign_message())[1].set_c(2);
}

template <typename TestMap>
void MapTestUtil::ExpectClear(const TestMap& message) {
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

template <typename TestMap>
void MapTestUtil::ExpectMapFieldsSet(const TestMap& message) {
  ASSERT_EQ(2, message.map_int32_int32().size());
  ASSERT_EQ(2, message.map_int64_int64().size());
  ASSERT_EQ(2, message.map_uint32_uint32().size());
  ASSERT_EQ(2, message.map_uint64_uint64().size());
  ASSERT_EQ(2, message.map_sint32_sint32().size());
  ASSERT_EQ(2, message.map_sint64_sint64().size());
  ASSERT_EQ(2, message.map_fixed32_fixed32().size());
  ASSERT_EQ(2, message.map_fixed64_fixed64().size());
  ASSERT_EQ(2, message.map_sfixed32_sfixed32().size());
  ASSERT_EQ(2, message.map_sfixed64_sfixed64().size());
  ASSERT_EQ(2, message.map_int32_float().size());
  ASSERT_EQ(2, message.map_int32_double().size());
  ASSERT_EQ(2, message.map_bool_bool().size());
  ASSERT_EQ(2, message.map_string_string().size());
  ASSERT_EQ(2, message.map_int32_bytes().size());
  ASSERT_EQ(2, message.map_int32_enum().size());
  ASSERT_EQ(2, message.map_int32_foreign_message().size());

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
  EXPECT_EQ(long_string(), message.map_string_string().at(long_string()));
  EXPECT_EQ(long_string(), message.map_int32_bytes().at(0));
  EXPECT_EQ(MapEnum_BAR<TestMap>(), message.map_int32_enum().at(0));
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
  EXPECT_EQ(long_string_2(), message.map_string_string().at(long_string_2()));
  EXPECT_EQ(long_string_2(), message.map_int32_bytes().at(1));
  EXPECT_EQ(MapEnum_BAZ<TestMap>(), message.map_int32_enum().at(1));
  EXPECT_EQ(1, message.map_int32_foreign_message().at(1).c());
}

template <typename TestArenaMap>
void MapTestUtil::ExpectArenaMapFieldsSet(const TestArenaMap& message) {
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
  EXPECT_EQ(long_string(), message.map_string_string().at(long_string()));
  EXPECT_EQ(long_string(), message.map_int32_bytes().at(0));
  EXPECT_EQ(MapEnum_BAR<TestArenaMap>(), message.map_int32_enum().at(0));
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
  EXPECT_EQ(long_string_2(), message.map_string_string().at(long_string_2()));
  EXPECT_EQ(long_string_2(), message.map_int32_bytes().at(1));
  EXPECT_EQ(MapEnum_BAZ<TestArenaMap>(), message.map_int32_enum().at(1));
  EXPECT_EQ(1, message.map_int32_foreign_message().at(1).c());
}

template <typename TestMap>
void MapTestUtil::ExpectMapFieldsSetInitialized(const TestMap& message) {
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
  EXPECT_EQ("", message.map_string_string().at(long_string()));
  EXPECT_EQ("", message.map_int32_bytes().at(0));
  EXPECT_EQ(MapEnum_FOO<TestMap>(), message.map_int32_enum().at(0));
  EXPECT_EQ(0, message.map_int32_foreign_message().at(0).ByteSizeLong());
}

template <typename TestMap>
void MapTestUtil::ExpectMapFieldsModified(const TestMap& message) {
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
  EXPECT_EQ(long_string(), message.map_string_string().at(long_string()));
  EXPECT_EQ(long_string(), message.map_int32_bytes().at(0));
  EXPECT_EQ(MapEnum_BAR<TestMap>(), message.map_int32_enum().at(0));
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
  EXPECT_EQ("2", message.map_string_string().at(long_string_2()));
  EXPECT_EQ("2", message.map_int32_bytes().at(1));
  EXPECT_EQ(MapEnum_FOO<TestMap>(), message.map_int32_enum().at(1));
  EXPECT_EQ(2, message.map_int32_foreign_message().at(1).c());
}

template <typename TestMap>
void MapTestUtil::ExpectMapsSize(const TestMap& message, int size) {
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

template <typename TestMap>
std::vector<const Message*> MapTestUtil::GetMapEntries(const TestMap& message,
                                                       int index) {
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

template <typename TestMap>
std::vector<const Message*> MapTestUtil::GetMapEntriesFromRelease(
    TestMap* message) {
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

#endif  // GOOGLE_PROTOBUF_MAP_TEST_UTIL_H__
