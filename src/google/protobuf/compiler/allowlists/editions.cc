// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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

// NOTE: These files have early default access to go/editions.  The protoc flag
// `--experimental_editions` can also be used to enable editions.

static constexpr auto kEarlyEditionsFile = internal::MakeAllowlist(
    {
// Intentionally left blank.
        "google/protobuf/",
    },
    internal::AllowlistFlags::kMatchPrefix);

bool IsEarlyEditionsFile(absl::string_view file) {
  return kEarlyEditionsFile.Allows(file);
}
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
