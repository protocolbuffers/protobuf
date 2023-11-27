// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
