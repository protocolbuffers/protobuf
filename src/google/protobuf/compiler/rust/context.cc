// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

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
