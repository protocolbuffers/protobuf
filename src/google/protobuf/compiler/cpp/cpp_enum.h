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

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_H__

#include <string>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
  namespace io {
    class Printer;             // printer.h
  }
}

namespace protobuf {
namespace compiler {
namespace cpp {

class EnumGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  explicit EnumGenerator(const EnumDescriptor* descriptor,
                         const string& dllexport_decl);
  ~EnumGenerator();

  // Header stuff.

  // Generate header code defining the enum.  This code should be placed
  // within the enum's package namespace, but NOT within any class, even for
  // nested enums.
  void GenerateDefinition(io::Printer* printer);

  // For enums nested within a message, generate code to import all the enum's
  // symbols (e.g. the enum type name, all its values, etc.) into the class's
  // namespace.  This should be placed inside the class definition in the
  // header.
  void GenerateSymbolImports(io::Printer* printer);

  // Source file stuff.

  // Generate code that initializes the global variable storing the enum's
  // descriptor.
  void GenerateDescriptorInitializer(io::Printer* printer, int index);

  // Generate non-inline methods related to the enum, such as IsValidValue().
  // Goes in the .cc file.
  void GenerateMethods(io::Printer* printer);

 private:
  const EnumDescriptor* descriptor_;
  string classname_;
  string dllexport_decl_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(EnumGenerator);
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_ENUM_H__
