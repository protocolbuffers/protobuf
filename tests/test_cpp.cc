/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * Tests for C++ wrappers.
 */

#include <stdio.h>
#include <iostream>
#include "upb/def.hpp"
#include "upb/pb/glue.hpp"

static void TestSymbolTable(const char *descriptor_file) {
  upb::SymbolTable *s = upb::SymbolTable::New();
  upb::Status status;
  if (!upb::LoadDescriptorFileIntoSymtab(s, descriptor_file, &status)) {
    std::cerr << "Couldn't load descriptor: " << status;
    exit(1);
  }
  const upb::MessageDef *md = s->LookupMessage("A");
  assert(md);

  s->Unref();
  md->Unref();
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: test_cpp <descriptor file>\n");
    return 1;
  }
  TestSymbolTable(argv[1]);
  return 0;
}
