// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/allowlists/allowlists.h"

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/allowlists/allowlist.h"

namespace google {
namespace protobuf {
namespace compiler {

// NOTE: Allowlists in this file are not accepting new entries unless otherwise
// specified.

static constexpr auto kWeakImports = internal::MakeAllowlist({
// Intentionally left blank.
});

bool IsWeakImportFile(absl::string_view file) {
  return kWeakImports.Allows(file);
}
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
