// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_EXTENSION_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_EXTENSION_H__

#include <string>

#include "absl/container/flat_hash_map.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
class FieldDescriptor;  // descriptor.h
namespace io {
class Printer;  // printer.h
}
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class MessageSCCAnalyzer;

// Generates code for an extension, which may be within the scope of some
// message or may be at file scope.  This is much simpler than FieldGenerator
// since extensions are just simple identifiers with interesting types.
class PROTOC_EXPORT ExtensionGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  explicit ExtensionGenerator(const FieldDescriptor* descriptor,
                              const Options& options,
                              MessageSCCAnalyzer* scc_analyzer);
  ExtensionGenerator(const ExtensionGenerator&) = delete;
  ExtensionGenerator& operator=(const ExtensionGenerator&) = delete;
  ~ExtensionGenerator();

  // Header stuff.
  void GenerateDeclaration(io::Printer* p) const;

  // Source file stuff.
  void GenerateDefinition(io::Printer* p);

  void GenerateRegistration(io::Printer* p);

  bool IsScoped() const;

 private:
  const FieldDescriptor* descriptor_;
  std::string type_traits_;
  Options options_;
  MessageSCCAnalyzer* scc_analyzer_;

  absl::flat_hash_map<absl::string_view, std::string> variables_;
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__
