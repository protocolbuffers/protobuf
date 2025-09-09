// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/versions.h"

#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/plugin.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace internal {
Version ParseProtobufVersion(absl::string_view version) {
  ABSL_CHECK(!version.empty()) << "version cannot be empty.";
  Version result;
  std::vector<std::string> parts = absl::StrSplit(version, '-');
  ABSL_CHECK(parts.size() <= 2)
      << "version cannot have more than one suffix annotated by \"-\".";
  if (parts.size() == 2) result.set_suffix(absl::StrCat("-", parts[1]));
  parts = absl::StrSplit(parts[0], '.');
  ABSL_CHECK(parts.size() == 3)
      << "version string must provide major, minor and micro numbers.";
  result.set_major(std::stoi(parts[0]));
  result.set_minor(std::stoi(parts[1]));
  result.set_patch(std::stoi(parts[2]));
  return result;
}
}  // namespace internal

const Version& GetProtobufCPPVersion(bool oss_runtime) {
  absl::string_view version = PROTOBUF_CPP_VERSION_STRING;
  // Heap-allocated versions to avoid re-parsing version strings
  static const Version* cpp_version =
      new Version(internal::ParseProtobufVersion(version));
  return *cpp_version;
}

const Version& GetProtobufJavaVersion(bool oss_runtime) {
  absl::string_view version = PROTOBUF_JAVA_VERSION_STRING;
  static const Version* java_version =
      new Version(internal::ParseProtobufVersion(version));
  return *java_version;
}

const Version& GetProtobufPythonVersion(bool oss_runtime) {
  absl::string_view version = PROTOBUF_PYTHON_VERSION_STRING;
  static const Version* python_version =
      new Version(internal::ParseProtobufVersion(version));
  return *python_version;
}
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
