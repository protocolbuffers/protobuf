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

// Generates C# code for a given .proto file.

#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_GENERATOR_H__

#include <string>

#include "google/protobuf/compiler/code_generator.h"

#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

// This specifies the current major version of the C# runtime (the Google.Protobuf package).
// Code generated with this compiler is expected to be compatible with a "same or later"
// version of Google.Protobuf within the same major version. So if code is generated when
// the current Google.Protobuf version is 3.22.3, it is compatible with 3.22.3, 3.22.4 and 3.23.0,
// but incompatible with 2.0.0, 3.21.5, 3.33.2, and 4.0.0.
// The minor and patch versions of the runtime are taken from the overall protobuf version,
// but each language has an independent major version for their runtime.
#define CSHARP_RUNTIME_MAJOR_VERSION 3

// CodeGenerator implementation which generates a C# source file and
// header.  If you create your own protocol compiler binary and you want
// it to support C# output, you can do so by registering an instance of this
// CodeGenerator with the CommandLineInterface in your main() function.
class PROTOC_EXPORT Generator : public CodeGenerator {
 public:
  Generator();
  ~Generator();
  bool Generate(
    const FileDescriptor* file,
    const std::string& parameter,
    GeneratorContext* generator_context,
    std::string* error) const override;
  uint64_t GetSupportedFeatures() const override;
};

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_GENERATOR_H__
