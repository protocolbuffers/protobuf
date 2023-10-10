// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC nor the names of its
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

#include "upb/json/encode.h"

#include <cstddef>
#include <string>

#include "google/protobuf/struct.upb.h"
#include <gtest/gtest.h>
#include "upb/base/status.hpp"
#include "upb/json/test.upb.h"
#include "upb/json/test.upbdefs.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"

static std::string JsonEncode(const upb_test_Box* msg, int options) {
  upb::Arena a;
  upb::Status status;
  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_Box_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  size_t json_size = upb_JsonEncode(msg, m.ptr(), defpool.ptr(), options,
                                    nullptr, 0, status.ptr());
  char* json_buf = (char*)upb_Arena_Malloc(a.ptr(), json_size + 1);

  size_t size = upb_JsonEncode(msg, m.ptr(), defpool.ptr(), options, json_buf,
                               json_size + 1, status.ptr());
  EXPECT_EQ(size, json_size);
  return std::string(json_buf, json_size);
}

// Encode a single optional enum.
TEST(JsonTest, EncodeEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_set_first_tag(foo, upb_test_Z_BAR);

  EXPECT_EQ(R"({"firstTag":"Z_BAR"})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"firstTag":1})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Encode a single optional negative enum.
TEST(JsonTest, EncodeNegativeEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_set_last_tag(foo, upb_test_Z_BAZ);

  EXPECT_EQ(R"({"lastTag":"Z_BAZ"})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"lastTag":-2})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Encode a single repeated enum.
TEST(JsonTest, EncodeRepeatedEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  upb_test_Box_add_more_tags(foo, upb_test_Z_BAT, a.ptr());

  EXPECT_EQ(R"({"moreTags":["Z_BAT"]})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"moreTags":[13]})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

// Special case: encode null enum.
TEST(JsonTest, EncodeNullEnum) {
  upb::Arena a;

  upb_test_Box* foo = upb_test_Box_new(a.ptr());
  google_protobuf_Value_set_null_value(upb_test_Box_mutable_val(foo, a.ptr()),
                                       google_protobuf_NULL_VALUE);

  EXPECT_EQ(R"({"val":null})", JsonEncode(foo, 0));
  EXPECT_EQ(R"({"val":null})",
            JsonEncode(foo, upb_JsonEncode_FormatEnumsAsIntegers));
}

TEST(JsonTest, EncodeConflictJsonName) {
  upb::Arena a;
  upb_test_Box* box = upb_test_Box_new(a.ptr());
  upb_test_Box_set_value(box, 2);
  EXPECT_EQ(R"({"old_value":2})", JsonEncode(box, 0));

  upb_test_Box* new_box = upb_test_Box_new(a.ptr());
  upb_test_Box_set_new_value(new_box, 2);
  EXPECT_EQ(R"({"value":2})", JsonEncode(new_box, 0));
}
