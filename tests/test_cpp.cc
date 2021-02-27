/*
 *
 * Tests for C++ wrappers.
 */

#include <stdio.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include "tests/test_cpp.upbdefs.h"
#include "tests/upb_test.h"
#include "upb/def.h"
#include "upb/def.hpp"
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
