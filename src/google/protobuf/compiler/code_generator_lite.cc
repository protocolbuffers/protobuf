// Protocol Buffers - Google's data interchange format
// Copyright 2008-2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/code_generator_lite.h"

#include <string>
#include <utility>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"

namespace google {
namespace protobuf {
namespace compiler {

// Parses a set of comma-delimited name/value pairs.
void ParseGeneratorParameter(
    absl::string_view text,
    std::vector<std::pair<std::string, std::string> >* output) {
  std::vector<absl::string_view> parts =
      absl::StrSplit(text, ',', absl::SkipEmpty());

  for (absl::string_view part : parts) {
    auto equals_pos = part.find_first_of('=');
    if (equals_pos == absl::string_view::npos) {
      output->emplace_back(part, "");
    } else {
      output->emplace_back(part.substr(0, equals_pos),
                           part.substr(equals_pos + 1));
    }
  }
}

// Strips ".proto" or ".protodevel" from the end of a filename.
std::string StripProto(absl::string_view filename) {
  if (absl::EndsWith(filename, ".protodevel")) {
    return std::string(absl::StripSuffix(filename, ".protodevel"));
  } else {
    return std::string(absl::StripSuffix(filename, ".proto"));
  }
}

bool IsKnownFeatureProto(absl::string_view filename) {
  if (filename == "google/protobuf/cpp_features.proto" ||
      filename == "google/protobuf/java_features.proto") {
    return true;
  }
  return false;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
