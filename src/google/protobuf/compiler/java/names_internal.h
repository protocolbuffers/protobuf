// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_INTERNAL_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_INTERNAL_H__

#include <string>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator_lite.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// Joins two package segments into a single package name with a dot separator,
// unless either of the segments is empty, in which case no separator is added.
inline std::string JoinPackage(absl::string_view a, absl::string_view b) {
  if (a.empty()) {
    return std::string(b);
  } else if (b.empty()) {
    return std::string(a);
  } else {
    return absl::StrCat(a, ".", b);
  }
}

inline std::string DefaultJavaPackage(const FileDescriptor* file) {
  if (file->options().has_java_package()) {
    return file->options().java_package();
  } else {
    return JoinPackage(google::protobuf::internal::IsOss() ? "" : "com.google.protos",
                       file->package());
  }
}

// The package name to use for a file that is being compiled as proto2-API.
// If the file is declared as proto1-API, this may involve using the alternate
// package name.
inline std::string Proto2DefaultJavaPackage(const FileDescriptor* file) {

  return DefaultJavaPackage(file);
}

// Converts a Java package name to a directory name.
inline std::string PackageToDir(std::string package_name) {
  std::string package_dir = absl::StrReplaceAll(package_name, {{".", "/"}});
  if (!package_dir.empty()) absl::StrAppend(&package_dir, "/");
  return package_dir;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_NAMES_INTERNAL_H__
