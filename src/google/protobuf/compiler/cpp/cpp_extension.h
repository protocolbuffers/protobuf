// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.
// http://code.google.com/p/protobuf/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_EXTENSION_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_EXTENSION_H__

#include <string>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {
  class FieldDescriptor;       // descriptor.h
  namespace io {
    class Printer;             // printer.h
  }
}

namespace protobuf {
namespace compiler {
namespace cpp {

// Generates code for an extension, which may be within the scope of some
// message or may be at file scope.  This is much simpler than FieldGenerator
// since extensions are just simple identifiers with interesting types.
class ExtensionGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  explicit ExtensionGenerator(const FieldDescriptor* descriptor,
                              const string& dllexport_decl);
  ~ExtensionGenerator();

  // Header stuff.
  void GenerateDeclaration(io::Printer* printer);

  // Source file stuff.
  void GenerateDefinition(io::Printer* printer);

 private:
  const FieldDescriptor* descriptor_;
  string type_traits_;
  string dllexport_decl_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ExtensionGenerator);
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__
