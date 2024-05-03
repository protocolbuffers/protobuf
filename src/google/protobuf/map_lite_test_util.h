// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_MAP_LITE_TEST_UTIL_H__
#define GOOGLE_PROTOBUF_MAP_LITE_TEST_UTIL_H__

#include "google/protobuf/map_lite_unittest.pb.h"

namespace google {
namespace protobuf {

class MapLiteTestUtil {
 public:
  // Set every field in the TestMapLite message to a unique value.
  static void SetMapFields(protobuf_unittest::TestMapLite* message);

  // Set every field in the TestArenaMapLite message to a unique value.
  static void SetArenaMapFields(protobuf_unittest::TestArenaMapLite* message);

  // Set every field in the message to a default value.
  static void SetMapFieldsInitialized(protobuf_unittest::TestMapLite* message);

  // Modify all the map fields of the message (which should already have been
  // initialized with SetMapFields()).
  static void ModifyMapFields(protobuf_unittest::TestMapLite* message);

  // Check that all fields have the values that they should have after
  // SetMapFields() is called.
  static void ExpectMapFieldsSet(const protobuf_unittest::TestMapLite& message);

  // Check that all fields have the values that they should have after
  // SetMapFields() is called for TestArenaMapLite.
  static void ExpectArenaMapFieldsSet(
      const protobuf_unittest::TestArenaMapLite& message);

  // Check that all fields have the values that they should have after
  // SetMapFieldsInitialized() is called.
  static void ExpectMapFieldsSetInitialized(
      const protobuf_unittest::TestMapLite& message);

  // Expect that the message is modified as would be expected from
  // ModifyMapFields().
  static void ExpectMapFieldsModified(
      const protobuf_unittest::TestMapLite& message);

  // Check that all fields are empty.
  static void ExpectClear(const protobuf_unittest::TestMapLite& message);
};

}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_MAP_LITE_TEST_UTIL_H__
