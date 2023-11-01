// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/util/required_fields.h"

#include <stdlib.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "upb/base/status.hpp"
#include "upb/json/decode.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"
#include "upb/util/required_fields_test.upb.h"
#include "upb/util/required_fields_test.upbdefs.h"

std::vector<std::string> PathsToText(upb_FieldPathEntry* entry) {
  std::vector<std::string> ret;
  char buf[1024];  // Larger than anything we'll use in this test.
  while (entry->field) {
    upb_FieldPathEntry* before = entry;
    size_t len = upb_FieldPath_ToText(&entry, buf, sizeof(buf));
    EXPECT_LT(len, sizeof(buf));
    assert(len <= sizeof(buf));
    ret.push_back(buf);

    // Ensure that we can have a short buffer and that it will be
    // NULL-terminated.
    char shortbuf[4];
    size_t len2 = upb_FieldPath_ToText(&before, shortbuf, sizeof(shortbuf));
    EXPECT_EQ(len, len2);
    EXPECT_EQ(ret.back().substr(0, sizeof(shortbuf) - 1),
              std::string(shortbuf));
  }
  return ret;
}

void CheckRequired(absl::string_view json,
                   const std::vector<std::string>& missing) {
  upb::Arena arena;
  upb::DefPool defpool;
  upb_util_test_TestRequiredFields* test_msg =
      upb_util_test_TestRequiredFields_new(arena.ptr());
  upb::MessageDefPtr m(
      upb_util_test_TestRequiredFields_getmsgdef(defpool.ptr()));
  upb::Status status;
  EXPECT_TRUE(upb_JsonDecode(json.data(), json.size(), test_msg, m.ptr(),
                             defpool.ptr(), 0, arena.ptr(), status.ptr()))
      << status.error_message();
  upb_FieldPathEntry* entries = nullptr;
  EXPECT_EQ(!missing.empty(), upb_util_HasUnsetRequired(
                                  test_msg, m.ptr(), defpool.ptr(), &entries));
  if (entries) {
    EXPECT_EQ(missing, PathsToText(entries));
    free(entries);
  }

  // Verify that we can pass a NULL pointer to entries when we don't care about
  // them.
  EXPECT_EQ(!missing.empty(), upb_util_HasUnsetRequired(
                                  test_msg, m.ptr(), defpool.ptr(), nullptr));
}

// message HasRequiredField {
//   required int32 required_int32 = 1;
// }
//
// message TestRequiredFields {
//   required EmptyMessage required_message = 1;
//   optional TestRequiredFields optional_message = 2;
//   repeated HasRequiredField repeated_message = 3;
//   map<int32, HasRequiredField> map_int32_message = 4;
// }
TEST(RequiredFieldsTest, TestRequired) {
  CheckRequired(R"json({})json", {"required_message"});
  CheckRequired(R"json({"required_message": {}}")json", {});
  CheckRequired(
      R"json(
      {
        "optional_message": {}
      }
      )json",
      {"required_message", "optional_message.required_message"});

  // Repeated field.
  CheckRequired(
      R"json(
      {
        "optional_message": {
          "repeated_message": [
            {"required_int32": 1},
            {},
            {"required_int32": 2}
          ]
        }
      }
      )json",
      {"required_message", "optional_message.required_message",
       "optional_message.repeated_message[1].required_int32"});

  // Int32 map key.
  CheckRequired(
      R"json(
      {
        "required_message": {},
        "map_int32_message": {
          "1": {"required_int32": 1},
          "5": {},
          "9": {"required_int32": 1}
        }
      }
      )json",
      {"map_int32_message[5].required_int32"});

  // Int64 map key.
  CheckRequired(
      R"json(
      {
        "required_message": {},
        "map_int64_message": {
          "1": {"required_int32": 1},
          "5": {},
          "9": {"required_int32": 1}
        }
      }
      )json",
      {"map_int64_message[5].required_int32"});

  // Uint32 map key.
  CheckRequired(
      R"json(
      {
        "required_message": {},
        "map_uint32_message": {
          "1": {"required_int32": 1},
          "5": {},
          "9": {"required_int32": 1}
        }
      }
      )json",
      {"map_uint32_message[5].required_int32"});

  // Uint64 map key.
  CheckRequired(
      R"json(
      {
        "required_message": {},
        "map_uint64_message": {
          "1": {"required_int32": 1},
          "5": {},
          "9": {"required_int32": 1}
        }
      }
      )json",
      {"map_uint64_message[5].required_int32"});

  // Bool map key.
  CheckRequired(
      R"json(
      {
        "required_message": {},
        "map_bool_message": {
          "false": {"required_int32": 1},
          "true": {}
        }
      }
      )json",
      {"map_bool_message[true].required_int32"});

  // String map key.
  CheckRequired(
      R"json(
      {
        "required_message": {},
        "map_string_message": {
          "abc": {"required_int32": 1},
          "d\"ef": {}
        }
      }
      )json",
      {R"(map_string_message["d\"ef"].required_int32)"});
}
