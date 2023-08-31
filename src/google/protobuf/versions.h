// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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

#ifndef GOOGLE_PROTOBUF_VERSIONS_H_
#define GOOGLE_PROTOBUF_VERSIONS_H_

#include <string>

#include "absl/strings/string_view.h"

// Macros to be used by language generators to insert version information.
// Although "main" is not a normal version suffix, it would be helpful for
// code generators to discern the main version.
//
// Currently, it holds language versions only for C++, Java and Python,
// but can be extended to other Protobuf languages.
//
// Usually, they should be updated automatically by Protobuf release
// process.
#define PROTOBUF_CPP_VERSION 4024000
#define PROTOBUF_CPP_VERSION_SUFFIX "-main"

#define PROTOBUF_JAVA_VERSION 3024000
#define PROTOBUF_JAVA_VERSION_SUFFIX "-main"

#define PROTOBUF_PYTHON_VERSION 4024000
#define PROTOBUF_PYTHON_VERSION_SUFFIX "-main"

namespace google {
namespace protobuf {
namespace internal {

// Gets the version string with version number and suffix.
// For example, "4.24.0-dev". version must be an integer in the form of
// {1 digit major}0{2 digits minor}0{2 digits micro}.
// suffix can be empty or prefixed by "-".
std::string VersionString(int version, absl::string_view suffix);

}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_VERSIONS_H_