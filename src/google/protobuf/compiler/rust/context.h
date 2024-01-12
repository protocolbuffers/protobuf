// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_CONTEXT_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_CONTEXT_H__

#include <algorithm>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {
// Marks which kernel the Rust codegen should generate code for.
enum class Kernel {
  kUpb,
  kCpp,
};

inline absl::string_view KernelRsName(Kernel kernel) {
  switch (kernel) {
    case Kernel::kUpb:
      return "upb";
    case Kernel::kCpp:
      return "cpp";
    default:
      ABSL_LOG(FATAL) << "Unknown kernel type: " << static_cast<int>(kernel);
      return "";
  }
}

// Global options for a codegen invocation.
struct Options {
  Kernel kernel;
  std::string mapping_file_path;

  static absl::StatusOr<Options> Parse(absl::string_view param);
};

class RustGeneratorContext {
 public:
  explicit RustGeneratorContext(
      const std::vector<const FileDescriptor*>* files_in_current_crate,
      const absl::flat_hash_map<std::string, std::string>*
          import_path_to_crate_name)
      : files_in_current_crate_(*files_in_current_crate),
        import_path_to_crate_name_(*import_path_to_crate_name) {}

  const FileDescriptor& primary_file() const {
    return *files_in_current_crate_.front();
  }

  bool is_file_in_current_crate(const FileDescriptor& f) const {
    return std::find(files_in_current_crate_.begin(),
                     files_in_current_crate_.end(),
                     &f) != files_in_current_crate_.end();
  }

  absl::string_view ImportPathToCrateName(absl::string_view import_path) const {
    return import_path_to_crate_name_.at(import_path);
  }

 private:
  const std::vector<const FileDescriptor*>& files_in_current_crate_;
  const absl::flat_hash_map<std::string, std::string>&
      import_path_to_crate_name_;
};

// A context for generating a particular kind of definition.
class Context {
 public:
  Context(const Options* opts,
          const RustGeneratorContext* rust_generator_context,
          io::Printer* printer)
      : opts_(opts),
        rust_generator_context_(rust_generator_context),
        printer_(printer) {}

  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;
  Context(Context&&) = default;
  Context& operator=(Context&&) = default;

  const Options& opts() const { return *opts_; }
  const RustGeneratorContext& generator_context() const {
    return *rust_generator_context_;
  }

  bool is_cpp() const { return opts_->kernel == Kernel::kCpp; }
  bool is_upb() const { return opts_->kernel == Kernel::kUpb; }

  // NOTE: prefer ctx.Emit() over ctx.printer().Emit();
  io::Printer& printer() const { return *printer_; }

  Context WithPrinter(io::Printer* printer) const {
    return Context(opts_, rust_generator_context_, printer);
  }

  // Forwards to Emit(), which will likely be called all the time.
  void Emit(absl::string_view format,
            io::Printer::SourceLocation loc =
                io::Printer::SourceLocation::current()) const {
    printer_->Emit(format, loc);
  }
  void Emit(absl::Span<const io::Printer::Sub> vars, absl::string_view format,
            io::Printer::SourceLocation loc =
                io::Printer::SourceLocation::current()) const {
    printer_->Emit(vars, format, loc);
  }

 private:
  const Options* opts_;
  const RustGeneratorContext* rust_generator_context_;
  io::Printer* printer_;
};

bool IsInCurrentlyGeneratingCrate(Context& ctx, const FileDescriptor& file);
bool IsInCurrentlyGeneratingCrate(Context& ctx, const Descriptor& message);
bool IsInCurrentlyGeneratingCrate(Context& ctx, const EnumDescriptor& enum_);

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_CONTEXT_H__
