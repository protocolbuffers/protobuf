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

#ifndef GOOGLE_PROTOBUF_COMPILER_CPP_STRING_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_CPP_STRING_FIELD_H__

#include <map>
#include <string>
#include <google/protobuf/compiler/cpp/cpp_field.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

class StringFieldGenerator : public FieldGenerator {
 public:
  explicit StringFieldGenerator(const FieldDescriptor* descriptor);
  ~StringFieldGenerator();

  // implements FieldGenerator ---------------------------------------
  void GeneratePrivateMembers(io::Printer* printer) const;
  void GenerateAccessorDeclarations(io::Printer* printer) const;
  void GenerateInlineAccessorDefinitions(io::Printer* printer) const;
  void GenerateNonInlineAccessorDefinitions(io::Printer* printer) const;
  void GenerateClearingCode(io::Printer* printer) const;
  void GenerateMergingCode(io::Printer* printer) const;
  void GenerateInitializer(io::Printer* printer) const;
  void GenerateDestructorCode(io::Printer* printer) const;
  void GenerateMergeFromCodedStream(io::Printer* printer) const;
  void GenerateSerializeWithCachedSizes(io::Printer* printer) const;
  void GenerateByteSize(io::Printer* printer) const;

 private:
  const FieldDescriptor* descriptor_;
  map<string, string> variables_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(StringFieldGenerator);
};

class RepeatedStringFieldGenerator : public FieldGenerator {
 public:
  explicit RepeatedStringFieldGenerator(const FieldDescriptor* descriptor);
  ~RepeatedStringFieldGenerator();

  // implements FieldGenerator ---------------------------------------
  void GeneratePrivateMembers(io::Printer* printer) const;
  void GenerateAccessorDeclarations(io::Printer* printer) const;
  void GenerateInlineAccessorDefinitions(io::Printer* printer) const;
  void GenerateClearingCode(io::Printer* printer) const;
  void GenerateMergingCode(io::Printer* printer) const;
  void GenerateInitializer(io::Printer* printer) const;
  void GenerateMergeFromCodedStream(io::Printer* printer) const;
  void GenerateSerializeWithCachedSizes(io::Printer* printer) const;
  void GenerateByteSize(io::Printer* printer) const;

 private:
  const FieldDescriptor* descriptor_;
  map<string, string> variables_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RepeatedStringFieldGenerator);
};

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_CPP_STRING_FIELD_H__
