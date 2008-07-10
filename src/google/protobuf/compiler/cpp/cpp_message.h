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

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__

#include <string>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/cpp/cpp_field.h>

namespace google {
namespace protobuf {
  namespace io {
    class Printer;             // printer.h
  }
}

namespace protobuf {
namespace compiler {
namespace cpp {

class EnumGenerator;           // enum.h
class ExtensionGenerator;      // extension.h

class MessageGenerator {
 public:
  // See generator.cc for the meaning of dllexport_decl.
  explicit MessageGenerator(const Descriptor* descriptor,
                            const string& dllexport_decl);
  ~MessageGenerator();

  // Header stuff.

  // Generate foward declarations for this class and all its nested types.
  void GenerateForwardDeclaration(io::Printer* printer);

  // Generate definitions of all nested enums (must come before class
  // definitions because those classes use the enums definitions).
  void GenerateEnumDefinitions(io::Printer* printer);

  // Generate definitions for this class and all its nested types.
  void GenerateClassDefinition(io::Printer* printer);

  // Generate definitions of inline methods (placed at the end of the header
  // file).
  void GenerateInlineMethods(io::Printer* printer);

  // Source file stuff.

  // Generate code which declares all the global descriptor pointers which
  // will be initialized by the methods below.
  void GenerateDescriptorDeclarations(io::Printer* printer);

  // Generate code that initializes the global variable storing the message's
  // descriptor.
  void GenerateDescriptorInitializer(io::Printer* printer, int index);

  // Generate all non-inline methods for this class.
  void GenerateClassMethods(io::Printer* printer);

 private:
  // Generate declarations and definitions of accessors for fields.
  void GenerateFieldAccessorDeclarations(io::Printer* printer);
  void GenerateFieldAccessorDefinitions(io::Printer* printer);

  // Generate the field offsets array.
  void GenerateOffsets(io::Printer* printer);

  // Generate constructors and destructor.
  void GenerateStructors(io::Printer* printer);

  // Generate the member initializer list for the constructors. The member
  // initializer list is shared between the default constructor and the copy
  // constructor.
  void GenerateInitializerList(io::Printer* printer);

  // Generate standard Message methods.
  void GenerateClear(io::Printer* printer);
  void GenerateMergeFromCodedStream(io::Printer* printer);
  void GenerateSerializeWithCachedSizes(io::Printer* printer);
  void GenerateByteSize(io::Printer* printer);
  void GenerateMergeFrom(io::Printer* printer);
  void GenerateCopyFrom(io::Printer* printer);
  void GenerateIsInitialized(io::Printer* printer);

  // Helpers for GenerateSerializeWithCachedSizes().
  void GenerateSerializeOneField(io::Printer* printer,
                                 const FieldDescriptor* field);
  void GenerateSerializeOneExtensionRange(
      io::Printer* printer, const Descriptor::ExtensionRange* range);

  const Descriptor* descriptor_;
  string classname_;
  string dllexport_decl_;
  FieldGeneratorMap field_generators_;
  scoped_array<scoped_ptr<MessageGenerator> > nested_generators_;
  scoped_array<scoped_ptr<EnumGenerator> > enum_generators_;
  scoped_array<scoped_ptr<ExtensionGenerator> > extension_generators_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MessageGenerator);
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_MESSAGE_H__
