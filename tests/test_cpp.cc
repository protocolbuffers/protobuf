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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
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

#include "tests/test_cpp.upbdefs.h"
#include "tests/test_cpp.upb.h"
#include "tests/upb_test.h"
#include "upb/def.h"
#include "upb/def.hpp"
#include "upb/json_decode.h"
#include "upb/json_encode.h"
#include "upb/upb.h"

// Must be last.
#include "upb/port_def.inc"

void TestIteration() {
  upb::SymbolTable symtab;
  upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(symtab.ptr()));

  // Test range-based for on both fields and oneofs (with the iterator adaptor).
  int field_count = 0;
  for (auto field : md.fields()) {
    UPB_UNUSED(field);
    field_count++;
  }
  ASSERT(field_count == md.field_count());

  int oneof_count = 0;
  for (auto oneof : md.oneofs()) {
    UPB_UNUSED(oneof);
    oneof_count++;
  }
  ASSERT(oneof_count == md.oneof_count());
}

void TestArena() {
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
      int* val = static_cast<int*>(upb_arena_malloc(arena.ptr(), sizeof(int)));
      *val = i;
    }

    // Test a large allocation.
    upb_arena_malloc(arena.ptr(), 1000000);
  }
  ASSERT(n == 0);

  {
    // Test fuse.
    upb::Arena arena1;
    upb::Arena arena2;

    arena1.Fuse(arena2);

    upb_arena_malloc(arena1.ptr(), 10000);
    upb_arena_malloc(arena2.ptr(), 10000);
  }
}

void TestInlinedArena() {
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
      int* val = static_cast<int*>(upb_arena_malloc(arena.ptr(), sizeof(int)));
      *val = i;
    }

    // Test a large allocation.
    upb_arena_malloc(arena.ptr(), 1000000);
  }
  ASSERT(n == 0);
}

void TestDefault() {
  upb::SymbolTable symtab;
  upb::Arena arena;
  upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(symtab.ptr()));
  upb_test_TestMessage *msg = upb_test_TestMessage_new(arena.ptr());
  size_t size = upb_json_encode(msg, md.ptr(), NULL, 0, NULL, 0, NULL);
  ASSERT(size == 2);  // "{}"
}

void TestJsonNull() {
  upb::SymbolTable symtab;
  upb::MessageDefPtr md(upb_test_TestMessage_getmsgdef(symtab.ptr()));
  upb::FieldDefPtr i32_f = md.FindFieldByName("i32");
  upb::FieldDefPtr str_f = md.FindFieldByName("str");
  ASSERT(i32_f && str_f);
  ASSERT(i32_f.default_value().int32_val == 5);
  ASSERT(strcmp(str_f.default_value().str_val.data, "abc") == 0);
  ASSERT(str_f.default_value().str_val.size == 3);
}

extern "C" {

int run_tests() {
  TestIteration();
  TestArena();
  TestDefault();

  return 0;
}

}
