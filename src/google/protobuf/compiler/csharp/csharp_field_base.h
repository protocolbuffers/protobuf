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

#ifndef GOOGLE_PROTOBUF_COMPILER_CSHARP_FIELD_BASE_H__
#define GOOGLE_PROTOBUF_COMPILER_CSHARP_FIELD_BASE_H__

#include <string>
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/csharp/csharp_source_generator_base.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

class Writer;

class FieldGeneratorBase : public SourceGeneratorBase {
 public:
  FieldGeneratorBase(const FieldDescriptor* descriptor, int fieldOrdinal);
  ~FieldGeneratorBase();

  virtual void GenerateMembers(Writer* writer) = 0;
  virtual void GenerateBuilderMembers(Writer* writer) = 0;
  virtual void GenerateMergingCode(Writer* writer) = 0;
  virtual void GenerateBuildingCode(Writer* writer) = 0;
  virtual void GenerateParsingCode(Writer* writer) = 0;
  virtual void GenerateSerializationCode(Writer* writer) = 0;
  virtual void GenerateSerializedSizeCode(Writer* writer) = 0;

  virtual void WriteHash(Writer* writer) = 0;
  virtual void WriteEquals(Writer* writer) = 0;
  virtual void WriteToString(Writer* writer) = 0;

 protected:
  const FieldDescriptor* descriptor_;
  const int fieldOrdinal_;

  void AddDeprecatedFlag(Writer* writer);
  void AddNullCheck(Writer* writer);
  void AddNullCheck(Writer* writer, const std::string& name);

  void AddPublicMemberAttributes(Writer* writer);

  std::string property_name();
  std::string name();
  std::string type_name();
  bool has_default_value();
  bool is_nullable_type();
  std::string default_value();
  std::string number();
  std::string message_or_group();
  std::string capitalized_type_name();
  std::string field_ordinal();

 private:
  std::string GetStringDefaultValueInternal();
  std::string GetBytesDefaultValueInternal();

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldGeneratorBase);
};

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_CSHARP_FIELD_BASE_H__

