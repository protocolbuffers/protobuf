// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_UPB_UPB_GENERATOR_REFLECTION_CONTEXT_H__
#define GOOGLE_UPB_UPB_GENERATOR_REFLECTION_CONTEXT_H__

#include <string>

#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace upb {
namespace generator {
namespace reflection {

struct Options {
  std::string dllexport_decl;
};

class Context {
 public:
  explicit Context(const Options& options,
                   google::protobuf::io::ZeroCopyOutputStream* stream)
      : options_(options), printer_(stream) {}

  // A few convenience wrappers so that users can write `ctx.Emit()` instead of
  // `ctx.printer().Emit()`.
  void Emit(absl::Span<const google::protobuf::io::Printer::Sub> vars,
            absl::string_view format,
            google::protobuf::io::Printer::SourceLocation loc =
                google::protobuf::io::Printer::SourceLocation::current()) {
    printer_.Emit(vars, format, loc);
  }

  void Emit(absl::string_view format,
            google::protobuf::io::Printer::SourceLocation loc =
                google::protobuf::io::Printer::SourceLocation::current()) {
    printer_.Emit(format, loc);
  }

  const Options& options() const { return options_; }

 private:
  const Options& options_;
  google::protobuf::io::Printer printer_;
};

}  // namespace reflection
}  // namespace generator
}  // namespace upb

#endif  // GOOGLE_UPB_UPB_GENERATOR_REFLECTION_CONTEXT_H__
