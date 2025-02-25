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
#include "google/protobuf/descriptor.h"

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
        "'experimental-codegen=enabled' to '--rust_opt'.");
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

  auto mapping_arg = absl::c_find_if(
      args, [](auto& arg) { return arg.first == "crate_mapping"; });
  if (mapping_arg != args.end()) {
    opts.mapping_file_path = mapping_arg->second;
  }

  auto strip_nonfunctional_codegen_arg = absl::c_find_if(args, [](auto& arg) {
    return arg.first == "experimental_strip_nonfunctional_codegen";
  });
  if (strip_nonfunctional_codegen_arg != args.end()) {
    opts.strip_nonfunctional_codegen = true;
  }

  auto generated_entry_point_rs_file_name_arg =
      absl::c_find_if(args, [](auto& arg) {
        return arg.first == "generated_entry_point_rs_file_name";
      });
  if (generated_entry_point_rs_file_name_arg != args.end()) {
    opts.generated_entry_point_rs_file_name =
        generated_entry_point_rs_file_name_arg->second;
  }

  return opts;
}

bool IsInCurrentlyGeneratingCrate(Context& ctx, const FileDescriptor& file) {
  return ctx.generator_context().is_file_in_current_crate(file);
}

bool IsInCurrentlyGeneratingCrate(Context& ctx, const Descriptor& message) {
  return IsInCurrentlyGeneratingCrate(ctx, *message.file());
}

bool IsInCurrentlyGeneratingCrate(Context& ctx, const EnumDescriptor& enum_) {
  return IsInCurrentlyGeneratingCrate(ctx, *enum_.file());
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
