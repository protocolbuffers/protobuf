// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_VERSIONS_H__
#define GOOGLE_PROTOBUF_COMPILER_VERSIONS_H__

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/plugin.pb.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

// Defines compiler version strings for Protobuf code generators.
//
// When they are suffixed with "-dev", they reflect the version of the next
// release, otherwise the current released version.
//
// Currently, they are embedded into comments at each gencode for public
// Protobuf C++, Java and Python. Further, we will add version strings for rest
// of languages in version.json, and they will be used to validate version
// compatibility between gencode and runtime.
//
// Versions of other plugins should not put versions here since they will not
// caught by Protobuf releases. Plugin owners should define their versions
// separately.
//
// Please avoid changing them manually, as they should be updated automatically
// by Protobuf release process.
#define PROTOBUF_CPP_VERSION_STRING "7.38.0-dev"
#define PROTOBUF_JAVA_VERSION_STRING "4.38.0-dev"
#define PROTOBUF_PYTHON_VERSION_STRING "7.38.0-dev"
#define PROTOBUF_RUST_VERSION_STRING "0.38.0-dev"


namespace google {
namespace protobuf {
namespace compiler {
namespace internal {
// For internal use to parse the Protobuf language version strings.
PROTOC_EXPORT Version ParseProtobufVersion(absl::string_view version);
}  // namespace internal

// Gets the version message according to the version strings defined above.
const Version& GetProtobufCPPVersion(bool oss_runtime);
const Version& GetProtobufJavaVersion(bool oss_runtime);
const Version& GetProtobufPythonVersion(bool oss_runtime);
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
#include "google/protobuf/port_undef.inc"
#endif  // GOOGLE_PROTOBUF_COMPILER_VERSIONS_H__
