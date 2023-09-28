// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/versions.h"

#include <string>
#include <vector>

#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/plugin.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
Version GetProtobufVersion(absl::string_view version) {
  Version result;
  std::vector<std::string> parts = absl::StrSplit(version, '-');
  if (parts.size() == 2) result.set_suffix(parts[1]);
  parts = absl::StrSplit(parts[0], '.');
  if (parts.size() == 3) result.set_patch(std::stoi(parts[2]));
  result.set_major(std::stoi(parts[0]));
  result.set_minor(std::stoi(parts[1]));
  return result;
}
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
