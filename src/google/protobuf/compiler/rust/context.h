// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
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
  void Emit(absl::string_view format) const { printer_->Emit(format); }
  void Emit(absl::Span<const io::Printer::Sub> vars,
            absl::string_view format) const {
    printer_->Emit(vars, format);
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
