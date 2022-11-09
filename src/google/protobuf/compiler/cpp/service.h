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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_SERVICE_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_SERVICE_H__

#include <string>

#include "google/protobuf/descriptor.h"
#include "absl/container/flat_hash_map.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
class ServiceGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  ServiceGenerator(
      const ServiceDescriptor* descriptor,
      const absl::flat_hash_map<absl::string_view, std::string>& vars,
      const Options& options)
      : descriptor_(descriptor), options_(&options), vars_(vars) {
    vars_["classname"] = descriptor_->name();
    vars_["full_name"] = descriptor_->full_name();
  }

  ServiceGenerator(const ServiceGenerator&) = delete;
  ServiceGenerator& operator=(const ServiceGenerator&) = delete;
  ServiceGenerator(ServiceGenerator&&) = delete;
  ServiceGenerator& operator=(ServiceGenerator&&) = delete;

  ~ServiceGenerator() = default;

  // Generate the class definitions for the service's interface and the
  // stub implementation.
  void GenerateDeclarations(io::Printer* printer);

  // Generate implementations of everything declared by
  // GenerateDeclarations().
  void GenerateImplementation(io::Printer* printer);

 private:
  enum RequestOrResponse { kRequest, kResponse };
  enum VirtualOrNot { kVirtual, kNonVirtual };

  // Prints signatures for all methods in the
  void GenerateMethodSignatures(VirtualOrNot virtual_or_not,
                                io::Printer* printer);

  // Generate the default implementations of the service methods, which
  // produce a "not implemented" error.
  void GenerateNotImplementedMethods(io::Printer* printer);

  // Generate the CallMethod() method of the service.
  void GenerateCallMethod(io::Printer* printer);

  // Generate the Get{Request,Response}Prototype() methods.
  void GenerateGetPrototype(RequestOrResponse which, io::Printer* printer);

  // Generate the cases in CallMethod().
  void GenerateCallMethodCases(io::Printer* printer);

  // Generate the stub's implementations of the service methods.
  void GenerateStubMethods(io::Printer* printer);

  const ServiceDescriptor* descriptor_;
  const Options* options_;
  absl::flat_hash_map<absl::string_view, std::string> vars_;

  int index_in_metadata_;

  friend class FileGenerator;
};
}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_SERVICE_H__
