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
#include "upb/mem/alloc.h"
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

  if (upb_AllocationCount_IsAvailable()) {
    auto RunJsonScenario = [&]() -> bool {
      upb_Arena* local_arena = upb_Arena_New();
      if (!local_arena) return false;

      upb_test_Box* local_box = upb_test_Box_new(local_arena);
      if (!local_box) {
        upb_Arena_Free(local_arena);
        return false;
      }

      upb::Status local_status;
      bool local_ok = upb_JsonDecode(
          json.data(), json.size(), UPB_UPCAST(local_box), m.ptr(),
          defpool.ptr(), options, local_arena, local_status.ptr());
      if (!local_ok) {
        upb_Arena_Free(local_arena);
        return false;
      }

      size_t local_sz =
          upb_JsonEncode(UPB_UPCAST(local_box), m.ptr(), defpool.ptr(), options,
                         nullptr, 0, local_status.ptr());
      if (local_sz == (size_t)-1 || !local_status.ok()) {
        upb_Arena_Free(local_arena);
        return false;
      }

      char* local_json_buf = (char*)upb_Arena_Malloc(local_arena, local_sz + 1);
      if (!local_json_buf) {
        upb_Arena_Free(local_arena);
        return false;
      }

      size_t local_written =
          upb_JsonEncode(UPB_UPCAST(local_box), m.ptr(), defpool.ptr(), options,
                         local_json_buf, local_sz + 1, local_status.ptr());
      if (local_written == (size_t)-1 || !local_status.ok()) {
        upb_Arena_Free(local_arena);
        return false;
      }

      upb_Arena_Free(local_arena);
      return true;
    };

    upb_AllocationCount_Reset();
    if (RunJsonScenario()) {
      size_t total_allocations = upb_AllocationCount_Get();
      for (size_t i = 0; i < total_allocations; ++i) {
        upb_AllocationCount_Reset();
        upb_AllocationCount_FailOn(i);
        bool success_with_fail = RunJsonScenario();
        EXPECT_FALSE(success_with_fail)
            << "Fuzzed JSON scenario unexpectedly succeeded when allocation "
            << "number " << i << " was failed, with "
            << upb_AllocationCount_Get() << " total.";
      }
    }
    upb_AllocationCount_Reset();
  }
}
FUZZ_TEST(FuzzTest, DecodeEncodeArbitraryJson);

TEST(FuzzTest, UnclosedObjectKey) { DecodeEncodeArbitraryJson("{\" "); }

TEST(FuzzTest, MalformedExponent) {
  DecodeEncodeArbitraryJson(R"({"val":0XE$})");
}

}  // namespace
