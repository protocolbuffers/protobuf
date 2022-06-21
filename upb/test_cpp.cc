// Copyright (c) 2009-2021, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Google LLC nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/*
 * Tests for C++ wrappers.
 */

#include <stdio.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include "google/protobuf/timestamp.upb.h"
#include "google/protobuf/timestamp.upbdefs.h"
#include "gtest/gtest.h"
#include "upb/def.h"
#include "upb/def.hpp"
#include "upb/json_decode.h"
#include "upb/json_encode.h"
#include "upb/test_cpp.upb.h"
#include "upb/test_cpp.upbdefs.h"
#include "upb/upb.h"

// Must be last.
#include "upb/port_def.inc"

TEST(Cpp, Iteration) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(defpool.ptr()));

  // Test range-based for on both fields and oneofs (with the iterator adaptor).
  int field_count = 0;
  for (auto field : md.fields()) {
    UPB_UNUSED(field);
    field_count++;
  }
  EXPECT_EQ(field_count, md.field_count());

  int oneof_count = 0;
  for (auto oneof : md.oneofs()) {
    UPB_UNUSED(oneof);
    oneof_count++;
  }
  EXPECT_EQ(oneof_count, md.oneof_count());
}

TEST(Cpp, Arena) {
  int n = 100000;

  struct Decrementer {
    Decrementer(int* _p) : p(_p) {}
    ~Decrementer() { (*p)--; }
    int* p;
  };

  {
    upb::Arena arena;
    for (int i = 0; i < n; i++) {
      arena.Own(new Decrementer(&n));

      // Intersperse allocation and ensure we can write to it.
      int* val = static_cast<int*>(upb_Arena_Malloc(arena.ptr(), sizeof(int)));
      *val = i;
    }

    // Test a large allocation.
    upb_Arena_Malloc(arena.ptr(), 1000000);
  }
  EXPECT_EQ(0, n);

  {
    // Test fuse.
    upb::Arena arena1;
    upb::Arena arena2;

    arena1.Fuse(arena2);

    upb_Arena_Malloc(arena1.ptr(), 10000);
    upb_Arena_Malloc(arena2.ptr(), 10000);
  }
}

TEST(Cpp, InlinedArena) {
  int n = 100000;

  struct Decrementer {
    Decrementer(int* _p) : p(_p) {}
    ~Decrementer() { (*p)--; }
    int* p;
  };

  {
    upb::InlinedArena<1024> arena;
    for (int i = 0; i < n; i++) {
      arena.Own(new Decrementer(&n));

      // Intersperse allocation and ensure we can write to it.
      int* val = static_cast<int*>(upb_Arena_Malloc(arena.ptr(), sizeof(int)));
      *val = i;
    }

    // Test a large allocation.
    upb_Arena_Malloc(arena.ptr(), 1000000);
  }
  EXPECT_EQ(0, n);
}

TEST(Cpp, Default) {
  upb::DefPool defpool;
  upb::Arena arena;
  upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(defpool.ptr()));
  upb_test_TestMessage* msg = upb_test_TestMessage_new(arena.ptr());
  size_t size = upb_JsonEncode(msg, md.ptr(), NULL, 0, NULL, 0, NULL);
  EXPECT_EQ(2, size);  // "{}"
}

TEST(Cpp, JsonNull) {
  upb::DefPool defpool;
  upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(defpool.ptr()));
  upb::FieldDefPtr i32_f = md.FindFieldByName("i32");
  upb::FieldDefPtr str_f = md.FindFieldByName("str");
  ASSERT_TRUE(i32_f);
  ASSERT_TRUE(str_f);
  EXPECT_EQ(5, i32_f.default_value().int32_val);
  EXPECT_EQ(0, strcmp(str_f.default_value().str_val.data, "abc"));
  EXPECT_EQ(3, str_f.default_value().str_val.size);
}

TEST(Cpp, TimestampEncoder) {
  upb::DefPool defpool;
  upb::Arena arena;
  upb::MessageDefPtr md(google_protobuf_Timestamp_getmsgdef(defpool.ptr()));
  google_protobuf_Timestamp* timestamp_upb =
      google_protobuf_Timestamp_new(arena.ptr());
  google_protobuf_Timestamp* timestamp_upb_decoded =
      google_protobuf_Timestamp_new(arena.ptr());

  long timestamps[] = {
      253402300799,  // 9999-12-31T23:59:59Z
      1641006000,    // 2022-01-01T03:00:00Z
      0,             // 1970-01-01T00:00:00Z
      -31525200,     // 1969-01-01T03:00:00Z
      -2208988800,   // 1900-01-01T00:00:00Z
      -62135596800,  // 0000-01-01T00:00:00Z
  };

  for (long timestamp : timestamps) {
    google_protobuf_Timestamp_set_seconds(timestamp_upb, timestamp);

    char json[128];
    size_t size = upb_JsonEncode(timestamp_upb, md.ptr(), NULL, 0, json,
                                 sizeof(json), NULL);
    bool result = upb_JsonDecode(json, size, timestamp_upb_decoded, md.ptr(),
                                 NULL, 0, arena.ptr(), NULL);
    const long timestamp_decoded =
        google_protobuf_Timestamp_seconds(timestamp_upb_decoded);

    ASSERT_TRUE(result);
    EXPECT_EQ(timestamp, timestamp_decoded);
  }
}
