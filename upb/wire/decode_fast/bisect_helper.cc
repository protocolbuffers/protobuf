// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <iostream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "upb/wire/decode_fast/select.h"

ABSL_FLAG(int, function_idx, -1, "The function index to test.");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  if (absl::GetFlag(FLAGS_function_idx) < 0) {
    std::cerr << "Usage: " << argv[0] << " <function_idx>\n";
    return 1;
  }
  std::cout << upb_DecodeFast_GetFunctionName(absl::GetFlag(FLAGS_function_idx))
            << "\n";
  return 0;
}
