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

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_MESSAGE_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_MESSAGE_H__

#include <string>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/java/java_field.h>

namespace google {
namespace protobuf {
  namespace io {
    class Printer;             // printer.h
  }
}

namespace protobuf {
namespace compiler {
namespace java {

class MessageGenerator {
 public:
  explicit MessageGenerator(const Descriptor* descriptor);
  ~MessageGenerator();

  // All static variables have to be declared at the top-level of the file
  // so that we can control initialization order, which is important for
  // DescriptorProto bootstrapping to work.
  void GenerateStaticVariables(io::Printer* printer);

  // Generate the class itself.
  void Generate(io::Printer* printer);

 private:
  void GenerateMessageSerializationMethods(io::Printer* printer);
  void GenerateParseFromMethods(io::Printer* printer);
  void GenerateSerializeOneField(io::Printer* printer,
                                 const FieldDescriptor* field);
  void GenerateSerializeOneExtensionRange(
      io::Printer* printer, const Descriptor::ExtensionRange* range);

  void GenerateBuilder(io::Printer* printer);
  void GenerateCommonBuilderMethods(io::Printer* printer);
  void GenerateBuilderParsingMethods(io::Printer* printer);
  void GenerateIsInitialized(io::Printer* printer);

  const Descriptor* descriptor_;
  FieldGeneratorMap field_generators_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(MessageGenerator);
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_MESSAGE_H__
