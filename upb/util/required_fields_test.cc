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

#include "upb/util/required_fields.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "absl/strings/string_view.h"
#include "upb/json_decode.h"
#include "upb/reflection/def.hpp"
#include "upb/upb.hpp"
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
  upb_FieldPathEntry* entries;
  EXPECT_EQ(!missing.empty(), upb_util_HasUnsetRequired(
                                  test_msg, m.ptr(), defpool.ptr(), &entries));
  EXPECT_EQ(missing, PathsToText(entries));
  free(entries);

  // Verify that we can pass a NULL pointer to entries when we don't care about
  // them.
  EXPECT_EQ(!missing.empty(),
            upb_util_HasUnsetRequired(test_msg, m.ptr(), defpool.ptr(), NULL));
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
