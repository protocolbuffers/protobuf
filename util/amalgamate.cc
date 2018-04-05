// Copyright 2018 Google Inc.  All rights reserved.
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

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>

using std::cout;
using std::string;

int main(int argc, char* argv[]) {
  string prefix("#include ");
  std::unordered_set<string> includes;
  for (int i = 1; i < argc; ++i) {
    cout << '\n';
    cout << "//////////////////////////////////////////////////////////////\n";
    cout << "// " << argv[i] << '\n';
    cout << "//////////////////////////////////////////////////////////////\n";
    cout << '\n';
    std::ifstream f(argv[i]);
    string line;
    while (std::getline(f, line)) {
      if (line.compare(0, prefix.size(), prefix) == 0) {
        if (includes.find(line) != includes.end()) {
          continue;
        }
        includes.insert(line);
      }
      cout << line << '\n';
    }
  }
  return 0;
}
