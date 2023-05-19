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

#ifndef GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSORS_H__
#define GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSORS_H__

#include <memory>

#include "absl/log/absl_check.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

class AccessorGenerator {
 public:
  AccessorGenerator() = default;

  AccessorGenerator(const AccessorGenerator &) = delete;
  AccessorGenerator(AccessorGenerator &&) = delete;
  AccessorGenerator &operator=(const AccessorGenerator &) = delete;
  AccessorGenerator &operator=(AccessorGenerator &&) = delete;

  virtual ~AccessorGenerator();

  // Constructs a generator for the given field.
  //
  // Returns `nullptr` if there is no known generator for this field.
  static std::unique_ptr<AccessorGenerator> For(Context<FieldDescriptor> field);

  void GenerateMsgImpl(Context<FieldDescriptor> field) const {
    InMsgImpl(field);
  }
  void GenerateExternC(Context<FieldDescriptor> field) const {
    InExternC(field);
  }
  void GenerateThunkCc(Context<FieldDescriptor> field) const {
    ABSL_CHECK(field.is_cpp());
    InThunkCc(field);
  }

 private:
  // Note: the virtual functions are duplicated as non-virtual public functions,
  // so that we can customize prologue and epilogue behavior for these
  // functions. For example, consider calling `field.printer.WithVars()` as a
  // prologue to inject variables automatically.

  // Called inside the main inherent `impl Msg {}` block.
  virtual void InMsgImpl(Context<FieldDescriptor> field) const {}

  // Called inside of a message's `extern "C" {}` block.
  virtual void InExternC(Context<FieldDescriptor> field) const {}

  // Called inside of an `extern "C" {}` block in the  `.thunk.cc` file, if such
  // a file is being generated.
  virtual void InThunkCc(Context<FieldDescriptor> field) const {}

  // These static factories are defined in the corresponding implementation
  // files for each implementation of AccessorGenerator.
  static std::unique_ptr<AccessorGenerator> ForSingularScalar(
      Context<FieldDescriptor> field);
  static std::unique_ptr<AccessorGenerator> ForSingularBytes(
      Context<FieldDescriptor> field);
};

inline AccessorGenerator::~AccessorGenerator() = default;

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_RUST_ACCESSORS_ACCESSORS_H__
