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
#include "upb/def.h"
#include "upb/descriptor/reader.h"
#include "upb/handlers.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb_test.h"
#include "upb/upb.h"

static void TestSymbolTable(const char *descriptor_file) {
  upb::SymbolTable *s = upb::SymbolTable::New(&s);
  upb::Status status;
  if (!upb::LoadDescriptorFileIntoSymtab(s, descriptor_file, &status)) {
    std::cerr << "Couldn't load descriptor: " << status.GetString();
    exit(1);
  }
  const upb::MessageDef *md = s->LookupMessage("A", &md);
  ASSERT(md);

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
  return 0;
}

}
