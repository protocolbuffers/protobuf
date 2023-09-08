// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_CONTEXT_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_CONTEXT_H__

#include "absl/log/absl_log.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
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

  static absl::StatusOr<Options> Parse(absl::string_view param);
};

// A context for generating a particular kind of definition.
// This type acts as an options struct (as in go/totw/173) for most of the
// generator.
//
// `Descriptor` is the type of a descriptor.h class relevant for the current
// context.
template <typename Descriptor>
class Context {
 public:
  Context(const Options* opts, const Descriptor* desc, io::Printer* printer)
      : opts_(opts), desc_(desc), printer_(printer) {}

  Context(const Context&) = default;
  Context& operator=(const Context&) = default;

  const Descriptor& desc() const { return *desc_; }
  const Options& opts() const { return *opts_; }

  bool is_cpp() const { return opts_->kernel == Kernel::kCpp; }
  bool is_upb() const { return opts_->kernel == Kernel::kUpb; }

  // NOTE: prefer ctx.Emit() over ctx.printer().Emit();
  io::Printer& printer() const { return *printer_; }

  // Creates a new context over a different descriptor.
  template <typename D>
  Context<D> WithDesc(const D& desc) const {
    return Context<D>(opts_, &desc, printer_);
  }

  template <typename D>
  Context<D> WithDesc(const D* desc) const {
    return Context<D>(opts_, desc, printer_);
  }

  Context WithPrinter(io::Printer* printer) const {
    return Context(opts_, desc_, printer);
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
  const Descriptor* desc_;
  io::Printer* printer_;
};
}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_CONTEXT_H__
