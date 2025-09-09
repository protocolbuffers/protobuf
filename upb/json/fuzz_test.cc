// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <cstring>
#include <string_view>

#include <gtest/gtest.h>
#include "testing/fuzzing/fuzztest.h"
#include "upb/base/status.hpp"
#include "upb/base/upcast.h"
#include "upb/json/decode.h"
#include "upb/json/encode.h"
#include "upb/json/test.upb.h"
#include "upb/json/test.upbdefs.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"

namespace {

void DecodeEncodeArbitraryJson(std::string_view json) {
  upb::Arena arena;
  upb::Status status;

  // Copy the input string to the heap. This helps asan reproduce issues that
  // don't reproduce when passing static memory pointers. See b/309107518.
  auto* json_heap = new char[json.size()];
  memcpy(json_heap, json.data(), json.size());

  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_Box_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  upb_test_Box* box = upb_test_Box_new(arena.ptr());
  int options = 0;
  bool ok = upb_JsonDecode(json_heap, json.size(), UPB_UPCAST(box), m.ptr(),
                           defpool.ptr(), options, arena.ptr(), status.ptr());
  delete[] json_heap;
  if (!ok) return;

  size_t size = upb_JsonEncode(UPB_UPCAST(box), m.ptr(), defpool.ptr(), options,
                               nullptr, 0, status.ptr());
  char* json_buf = (char*)upb_Arena_Malloc(arena.ptr(), size + 1);

  size_t written = upb_JsonEncode(UPB_UPCAST(box), m.ptr(), defpool.ptr(),
                                  options, json_buf, size + 1, status.ptr());
  EXPECT_EQ(written, size);
}
FUZZ_TEST(FuzzTest, DecodeEncodeArbitraryJson);

TEST(FuzzTest, UnclosedObjectKey) { DecodeEncodeArbitraryJson("{\" "); }

TEST(FuzzTest, MalformedExponent) {
  DecodeEncodeArbitraryJson(R"({"val":0XE$})");
}

}  // namespace
