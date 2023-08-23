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

#ifndef GOOGLE_PROTOBUF_COMPILER_ALLOWLISTS_ALLOWLISTS_H__
#define GOOGLE_PROTOBUF_COMPILER_ALLOWLISTS_ALLOWLISTS_H__

#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace compiler {

// Returns whether a file can use the `import weak` syntax.
bool IsWeakImportFile(absl::string_view file);

// Returns whether a file can have an empty package.
bool IsEmptyPackageFile(absl::string_view file);

// Returns whether a file can contain a cc_open_enum.
bool IsOpenEnumFile(absl::string_view file);

// Returns whether a message can contain a cc_open_enum.
bool IsOpenEnumMessage(absl::string_view msg);

// Returns whether a file can contain an unused import.
bool IsUnusedImportFile(absl::string_view file);

// Returns whether a file has early access to editions.
bool IsEarlyEditionsFile(absl::string_view file);

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_ALLOWLISTS_COMPILER_ALLOWLISTS_H__
