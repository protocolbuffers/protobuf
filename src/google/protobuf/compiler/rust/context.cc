// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
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
//     * Neither the name of Google LLC. nor the names of its
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

#include "google/protobuf/compiler/rust/context.h"

#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/code_generator.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
static constexpr std::pair<absl::string_view, absl::string_view> kMagicValue = {
    "experimental-codegen",
    "enabled",
};

absl::StatusOr<Options> Options::Parse(absl::string_view param) {
  std::vector<std::pair<std::string, std::string>> args;
  ParseGeneratorParameter(param, &args);

  bool has_experimental_value = absl::c_any_of(
      args, [](std::pair<absl::string_view, absl::string_view> pair) {
        return pair == kMagicValue;
      });

  if (!has_experimental_value) {
    return absl::InvalidArgumentError(
        "The Rust codegen is highly experimental. Future versions will break "
        "existing code. Use at your own risk. You can opt-in by passing "
        "'experimental-codegen=enabled' to '--rust_out'.");
  }

  Options opts;

  auto kernel_arg =
      absl::c_find_if(args, [](auto& arg) { return arg.first == "kernel"; });
  if (kernel_arg == args.end()) {
    return absl::InvalidArgumentError(
        "Mandatory option `kernel` missing, please specify `cpp` or "
        "`upb`.");
  }

  if (kernel_arg->second == "upb") {
    opts.kernel = Kernel::kUpb;
  } else if (kernel_arg->second == "cpp") {
    opts.kernel = Kernel::kCpp;
  } else {
    return absl::InvalidArgumentError(
        absl::Substitute("Unknown kernel `$0`, please specify `cpp` or "
                         "`upb`.",
                         kernel_arg->second));
  }

  return opts;
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
