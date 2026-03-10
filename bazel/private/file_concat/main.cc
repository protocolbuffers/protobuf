// Copyright (c) 2020-2025, Google LLC
// All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <fstream>
#include <iostream>
#include <string>

namespace {

constexpr size_t kBufferSize = 4096;  // 4kB

// Return codes.
constexpr int kOk = 0;
constexpr int kUsageError = 1;
constexpr int kIOError = 2;

}  // namespace

int main(int argc, const char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <output> <inputs...>" << std::endl;
    return kUsageError;
  }

  std::string output_path(argv[1]);
  std::ofstream output(output_path, std::ofstream::binary);
  if (!output) {
    std::cerr << "Could not open output file " << output_path << std::endl;
    return kIOError;
  }

  for (int i = 2; i < argc; i++) {
    std::string input_path(argv[i]);
    std::ifstream input(input_path, std::ifstream::binary);
    if (!input) {
      std::cerr << "Could not open input file " << output_path << std::endl;
      return kIOError;
    }

    char buffer[kBufferSize];
    while (input) {
      if (!input.read(buffer, kBufferSize) && !input.eof()) {
        std::cerr << "Error reading from " << input_path << std::endl;
        return kIOError;
      }
      if (!output.write(buffer, input.gcount())) {
        std::cerr << "Error writing to " << output_path << std::endl;
        return kIOError;
      }
    }
  }

  return kOk;
}
