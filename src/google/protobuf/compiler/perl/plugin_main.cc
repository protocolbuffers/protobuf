// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <cstddef>
#include <iostream>
#include <string>

#include "google/protobuf/compiler/perl/generator.h"
#include "upb/mem/arena.h"

#ifdef GOOGLE3
#include "google/protobuf/compiler/perl/plugin.upb.h"
#else
#include "google/protobuf/compiler/plugin.upb.h"
#endif

int main(int argc, char** argv) {
  upb_Arena* arena = upb_Arena_New();

  std::string input;
  char buffer[4096];
  while (std::cin.read(buffer, sizeof(buffer))) {
    input.append(buffer, std::cin.gcount());
  }
  input.append(buffer, std::cin.gcount());

  proto2_compiler_CodeGeneratorRequest* request =
      proto2_compiler_CodeGeneratorRequest_parse(input.data(), input.length(),
                                                 arena);

  if (!request) {
    std::cerr << "Failed to parse CodeGeneratorRequest\n";
    upb_Arena_Free(arena);
    return 1;
  }

  google::protobuf::compiler::perl::PerlCodeGenerator generator(request, arena);
  proto2_compiler_CodeGeneratorResponse* response = generator.Generate();

  size_t size;
  char* buf =
      proto2_compiler_CodeGeneratorResponse_serialize(response, arena, &size);
  std::cout.write(buf, size);

  upb_Arena_Free(arena);
  return 0;
}
