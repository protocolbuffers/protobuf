// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

const char output_file[] = "well_known_types_embed.cc";

static bool AsciiIsPrint(unsigned char c) {
  return c >= 32 && c < 127;
}

static char ToDecimalDigit(int num) {
  assert(num < 10);
  return '0' + num;
}

static std::string CEscape(const std::string& str) {
  std::string dest;

  for (size_t i = 0; i < str.size(); ++i) {
    unsigned char ch = str[i];
    switch (ch) {
      case '\n': dest += "\\n"; break;
      case '\r': dest += "\\r"; break;
      case '\t': dest += "\\t"; break;
      case '\"': dest += "\\\""; break;
      case '\\': dest += "\\\\"; break;
      default:
        if (AsciiIsPrint(ch)) {
          dest += ch;
        } else {
          dest += "\\";
          dest += ToDecimalDigit(ch / 64);
          dest += ToDecimalDigit((ch % 64) / 8);
          dest += ToDecimalDigit(ch % 8);
        }
        break;
    }
  }

  return dest;
}

static void AddFile(const char* name, std::basic_ostream<char>* out) {
  std::ifstream in(name);

  if (!in.is_open()) {
    std::cerr << "Couldn't open input file: " << name << "\n";
    std::exit(EXIT_FAILURE);
  }

  // Make canonical name only include the final element.
  for (const char *p = name; *p; p++) {
    if (*p == '/') {
      name = p + 1;
    }
  }

  *out << "{\"" << CEscape(name) << "\",\n";

  for (std::string line; std::getline(in, line); ) {
    *out << "  \"" << CEscape(line) << "\\n\"\n";
  }

  *out << "},\n";
}

int main(int argc, char *argv[]) {
  std::cout << "#include "
               "<google/protobuf/compiler/js/well_known_types_embed.h>\n";
  std::cout << "struct FileToc well_known_types_js[] = {\n";

  for (int i = 1; i < argc; i++) {
    AddFile(argv[i], &std::cout);
  }

  std::cout << "  {NULL, NULL}  // Terminate the list.\n";
  std::cout << "};\n";

  return EXIT_SUCCESS;
}
