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
