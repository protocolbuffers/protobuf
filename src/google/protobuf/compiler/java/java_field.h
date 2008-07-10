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

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_FIELD_H__

#include <string>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
  namespace io {
    class Printer;             // printer.h
  }
}

namespace protobuf {
namespace compiler {
namespace java {

class FieldGenerator {
 public:
  FieldGenerator() {}
  virtual ~FieldGenerator();

  virtual void GenerateMembers(io::Printer* printer) const = 0;
  virtual void GenerateBuilderMembers(io::Printer* printer) const = 0;
  virtual void GenerateMergingCode(io::Printer* printer) const = 0;
  virtual void GenerateBuildingCode(io::Printer* printer) const = 0;
  virtual void GenerateParsingCode(io::Printer* printer) const = 0;
  virtual void GenerateSerializationCode(io::Printer* printer) const = 0;
  virtual void GenerateSerializedSizeCode(io::Printer* printer) const = 0;

  virtual string GetBoxedType() const = 0;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldGenerator);
};

// Convenience class which constructs FieldGenerators for a Descriptor.
class FieldGeneratorMap {
 public:
  explicit FieldGeneratorMap(const Descriptor* descriptor);
  ~FieldGeneratorMap();

  const FieldGenerator& get(const FieldDescriptor* field) const;
  const FieldGenerator& get_extension(int index) const;

 private:
  const Descriptor* descriptor_;
  scoped_array<scoped_ptr<FieldGenerator> > field_generators_;
  scoped_array<scoped_ptr<FieldGenerator> > extension_generators_;

  static FieldGenerator* MakeGenerator(const FieldDescriptor* field);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldGeneratorMap);
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_FIELD_H__
