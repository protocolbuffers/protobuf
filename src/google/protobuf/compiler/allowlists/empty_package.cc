// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/allowlists/allowlist.h"
#include "google/protobuf/compiler/allowlists/allowlists.h"

namespace google {
namespace protobuf {
namespace compiler {
// NOTE: Allowlists in this file are not accepting new entries unless otherwise
// specified.

static constexpr auto kEmptyPackage = internal::MakeAllowlist(
    {
// Intentionally left blank.
    },
    internal::AllowlistFlags::kAllowAllInOss);

static constexpr auto kTradeFedProtos = internal::MakeAllowlist({
// Intentionally left blank.
});

static constexpr auto k3pCriuProtos = internal::MakeAllowlist({
// Intentionally left blank.
});

bool IsEmptyPackageFile(absl::string_view file) {
  return kEmptyPackage.Allows(file) || kTradeFedProtos.Allows(file) ||
         k3pCriuProtos.Allows(file);
}
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
