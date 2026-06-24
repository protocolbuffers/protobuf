// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/json/decode.h"

#include <string>
#include <vector>

#include "google/protobuf/struct.upb.h"
#include <gtest/gtest.h>
#include "upb/base/status.hpp"
#include "upb/base/upcast.h"
#include "upb/json/test.upb.h"
#include "upb/json/test.upbdefs.h"
#include "upb/mem/arena.h"
#include "upb/mem/arena.hpp"
#include "upb/reflection/def.hpp"

static upb_test_Box* JsonDecode(const char* json, upb_Arena* a) {
  upb::Status status;
  upb::DefPool defpool;
  upb::MessageDefPtr m(upb_test_Box_getmsgdef(defpool.ptr()));
  EXPECT_TRUE(m.ptr() != nullptr);

  upb_test_Box* box = upb_test_Box_new(a);
  int options = 0;
  bool ok = upb_JsonDecode(json, strlen(json), UPB_UPCAST(box), m.ptr(),
                           defpool.ptr(), options, a, status.ptr());
  return ok ? box : nullptr;
}

struct FloatTest {
  const std::string json;
  float f;
};

static const std::vector<FloatTest> FloatTestsPass = {
    {R"({"f": 0})", 0},
    {R"({"f": 1})", 1},
    {R"({"f": 1.000000})", 1},
    {R"({"f": 1.5e1})", 15},
    {R"({"f": 15e-1})", 1.5},
    {R"({"f": -3.5})", -3.5},
    {R"({"f": 3.402823e38})", 3.402823e38},
    {R"({"f": -3.402823e38})", -3.402823e38},
    {R"({"f": 340282346638528859811704183484516925440.0})",
     340282346638528859811704183484516925440.0},
    {R"({"f": -340282346638528859811704183484516925440.0})",
     -340282346638528859811704183484516925440.0},
};

static const std::vector<FloatTest> FloatTestsFail = {
    {R"({"f": 1z})", 0},
    {R"({"f": 3.4028236e+38})", 0},
    {R"({"f": -3.4028236e+38})", 0},
};

// Decode some floats.
TEST(JsonTest, DecodeFloats) {
  upb::Arena a;

  for (const auto& test : FloatTestsPass) {
    upb_test_Box* box = JsonDecode(test.json.c_str(), a.ptr());
    EXPECT_NE(box, nullptr);
    float f = upb_test_Box_f(box);
    EXPECT_EQ(f, test.f);
  }

  for (const auto& test : FloatTestsFail) {
    upb_test_Box* box = JsonDecode(test.json.c_str(), a.ptr());
    EXPECT_EQ(box, nullptr);
  }
}

TEST(JsonTest, DecodeConflictJsonName) {
  upb::Arena a;
  std::string json_string = R"({"value": 2})";
  upb_test_Box* box = JsonDecode(json_string.c_str(), a.ptr());
  EXPECT_EQ(2, upb_test_Box_new_value(box));
  EXPECT_EQ(0, upb_test_Box_value(box));
}

TEST(JsonTest, RejectsBadTrailingCharacters) {
  upb::Arena a;
  std::string json_string = R"({}abc)";
  upb_test_Box* box = JsonDecode(json_string.c_str(), a.ptr());
  EXPECT_EQ(box, nullptr);
}

TEST(JsonTest, AcceptsTrailingWhitespace) {
  upb::Arena a;
  std::string json_string = "{} \n \r\n \t\t";
  upb_test_Box* box = JsonDecode(json_string.c_str(), a.ptr());
  EXPECT_NE(box, nullptr);
}

// Regression: jsondec_base64_tablelookup() previously indexed a 256-byte
// signed-char table with table[(unsigned)ch], which let C integer
// promotion sign-extend bytes >= 0x80 into negative ints, producing
// OOB reads ~4 GiB past the table. Decoding a bytes-typed field whose
// JSON string contains high-bit-set bytes (e.g. the UTF-8 encoding of
// \u0080 = 0xC2 0x80) must fail gracefully without OOB-reading.
TEST(JsonTest, RejectsBase64WithHighBitBytes) {
  upb::Arena a;
  std::string json_string = R"({"data":"\u0080\u0080\u0080\u0080"})";
  upb_test_Box* box = JsonDecode(json_string.c_str(), a.ptr());
  EXPECT_EQ(box, nullptr);
}

#include "upb/mem/alloc.h"

struct FailAfterAlloc {
  upb_alloc alloc;
  int limit;
  int count;
};

static void* FailAfterAllocFunc(upb_alloc* alloc, void* ptr, size_t oldsize,
                                size_t size, size_t* actual_size) {
  FailAfterAlloc* self = reinterpret_cast<FailAfterAlloc*>(alloc);
  if (size > 0) {
    if (self->count >= self->limit) {
      return nullptr;
    }
    self->count++;
  }
  return upb_alloc_global.func(alloc, ptr, oldsize, size, actual_size);
}

static bool TryDecodeWithLimit(const char* json, int limit) {
  FailAfterAlloc allocator = {{&FailAfterAllocFunc}, limit, 0};
  upb_Arena* a = upb_Arena_Init(nullptr, 0, &allocator.alloc);
  if (!a) {
    return false;
  }
  upb::Status status;
  upb::DefPool defpool;
  upb_test_Box* box = upb_test_Box_new(a);
  bool ok = false;
  if (box) {
    upb::MessageDefPtr m(upb_test_Box_getmsgdef(defpool.ptr()));
    int options = 0;
    ok = upb_JsonDecode(json, strlen(json), UPB_UPCAST(box), m.ptr(),
                        defpool.ptr(), options, a, status.ptr());
  }
  upb_Arena_Free(a);
  return ok;
}

static void TestJsonAllocationFailure(const char* json) {
  int limit = 0;
  bool success = false;
  for (; limit < 1000; ++limit) {
    if (TryDecodeWithLimit(json, limit)) {
      success = true;
      break;
    }
  }
  EXPECT_TRUE(success) << "JSON failed to decode even with 1000 allocations: "
                       << json;

  for (int i = 0; i < limit; ++i) {
    TryDecodeWithLimit(json, i);
  }
}

TEST(JsonTest, AllocationFailureFieldMask) {
  TestJsonAllocationFailure(R"({"mask_val": "foo,bar,baz"})");
}

TEST(JsonTest, AllocationFailureListValue) {
  TestJsonAllocationFailure(R"({"val": [1, 2, 3]})");
}

TEST(JsonTest, AllocationFailureStruct) {
  TestJsonAllocationFailure(R"({"val": {"a": 1, "b": 2}})");
}

TEST(JsonTest, AllocationFailureAny) {
  TestJsonAllocationFailure(
      R"({"any_val": {"@type": "type.googleapis.com/google.protobuf.Value", "value": "foo"}})");
}
