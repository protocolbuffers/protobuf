/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Tests for C++ wrappers.
 */

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <set>
#include "upb/def.h"
#include "upb/descriptor/reader.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb_test.h"
#include "upb/upb.h"

template <class T>
void AssertInsert(T* const container, const typename T::value_type& val) {
  bool inserted = container->insert(val).second;
  ASSERT(inserted);
}

static void TestCasts1() {
  const upb::MessageDef* md = upb::MessageDef::New(&md);
  const upb::Def* def = md->Upcast();
  const upb::MessageDef* md2 = upb::down_cast<const upb::MessageDef*>(def);
  const upb::MessageDef* md3 = upb::dyn_cast<const upb::MessageDef*>(def);

  ASSERT(md == md2);
  ASSERT(md == md3);

  const upb::EnumDef* ed = upb::dyn_cast<const upb::EnumDef*>(def);
  ASSERT(!ed);

  md->Unref(&md);
}

static void TestCasts2() {
  // Test non-const -> const cast.
  upb::MessageDef* md = upb::MessageDef::New(&md);
  upb::Def* def = md->Upcast();
  const upb::MessageDef* const_md = upb::down_cast<const upb::MessageDef*>(def);
  ASSERT(const_md == md);
  md->Unref(&md);
}

static void TestSymbolTable(const char *descriptor_file) {
  upb::SymbolTable *s = upb::SymbolTable::New(&s);
  upb::Status status;
  if (!upb::LoadDescriptorFileIntoSymtab(s, descriptor_file, &status)) {
    std::cerr << "Couldn't load descriptor: " << status.GetString();
    exit(1);
  }
  const upb::MessageDef* md = s->LookupMessage("C", &md);
  ASSERT(md);

  // We want a def that satisfies this to test iteration.
  ASSERT(md->field_count() > 1);

#ifdef UPB_CXX11
  // Test range-based for.
  std::set<const upb::FieldDef*> fielddefs;
  for (const upb::FieldDef* f : *md) {
    AssertInsert(&fielddefs, f);
    ASSERT(f->containing_type() == md);
  }
  ASSERT(fielddefs.size() == md->field_count());
#endif

  s->Unref(&s);
  md->Unref(&md);
}

extern "C" {

int run_tests(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: test_cpp <descriptor file>\n");
    return 1;
  }
  TestSymbolTable(argv[1]);
  TestCasts1();
  TestCasts2();
  return 0;
}

}
