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

#include <sstream>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/strutil.h>

#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_message_field.h>
#include <google/protobuf/compiler/csharp/csharp_writer.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

MessageFieldGenerator::MessageFieldGenerator(const FieldDescriptor* descriptor,
                                             int fieldOrdinal)
    : FieldGeneratorBase(descriptor, fieldOrdinal) {
}

MessageFieldGenerator::~MessageFieldGenerator() {

}

void MessageFieldGenerator::GenerateMembers(Writer* writer) {
  writer->WriteLine("private bool has$0$;", property_name());
  writer->WriteLine("private $0$ $1$_;", type_name(), name());
  AddDeprecatedFlag(writer);
  writer->WriteLine("public bool Has$0$ {", property_name());
  writer->WriteLine("  get { return has$0$; }", property_name());
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public $0$ $1$ {", type_name(), property_name());
  writer->WriteLine("  get { return $0$_ ?? $1$; }", name(), default_value());
  writer->WriteLine("}");
}

void MessageFieldGenerator::GenerateBuilderMembers(Writer* writer) {
  AddDeprecatedFlag(writer);
  writer->WriteLine("public bool Has$0$ {", property_name());
  writer->WriteLine(" get { return result.has$0$; }", property_name());
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public $0$ $1$ {", type_name(), property_name());
  writer->WriteLine("  get { return result.$0$; }", property_name());
  writer->WriteLine("  set { Set$0$(value); }", property_name());
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Set$0$($1$ value) {", property_name(),
                    type_name());
  AddNullCheck(writer);
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.has$0$ = true;", property_name());
  writer->WriteLine("  result.$0$_ = value;", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Set$0$($1$.Builder builderForValue) {",
                    property_name(), type_name());
  AddNullCheck(writer, "builderForValue");
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.has$0$ = true;", property_name());
  writer->WriteLine("  result.$0$_ = builderForValue.Build();", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Merge$0$($1$ value) {", property_name(),
                    type_name());
  AddNullCheck(writer);
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  if (result.has$0$ &&", property_name());
  writer->WriteLine("      result.$0$_ != $1$) {", name(), default_value());
  writer->WriteLine(
      "      result.$0$_ = $1$.CreateBuilder(result.$0$_).MergeFrom(value).BuildPartial();",
      name(), type_name());
  writer->WriteLine("  } else {");
  writer->WriteLine("    result.$0$_ = value;", name());
  writer->WriteLine("  }");
  writer->WriteLine("  result.has$0$ = true;", property_name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Clear$0$() {", property_name());
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.has$0$ = false;", property_name());
  writer->WriteLine("  result.$0$_ = null;", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
}

void MessageFieldGenerator::GenerateMergingCode(Writer* writer) {
  writer->WriteLine("if (other.Has$0$) {", property_name());
  writer->WriteLine("  Merge$0$(other.$0$);", property_name());
  writer->WriteLine("}");
}

void MessageFieldGenerator::GenerateBuildingCode(Writer* writer) {
  // Nothing to do for singular fields
}

void MessageFieldGenerator::GenerateParsingCode(Writer* writer) {
  writer->WriteLine("$0$.Builder subBuilder = $0$.CreateBuilder();",
                    type_name());
  writer->WriteLine("if (result.has$0$) {", property_name());
  writer->WriteLine("  subBuilder.MergeFrom($0$);", property_name());
  writer->WriteLine("}");

  if (descriptor_->type() == FieldDescriptor::TYPE_GROUP) {
    writer->WriteLine("input.ReadGroup($0$, subBuilder, extensionRegistry);",
                      number());
  } else {
    writer->WriteLine("input.ReadMessage(subBuilder, extensionRegistry);");
  }
  writer->WriteLine("$0$ = subBuilder.BuildPartial();", property_name());
}

void MessageFieldGenerator::GenerateSerializationCode(Writer* writer) {
  writer->WriteLine("if (has$0$) {", property_name());
  writer->WriteLine("  output.Write$0$($1$, field_names[$3$], $2$);",
                    message_or_group(), number(), property_name(),
                    field_ordinal());
  writer->WriteLine("}");
}

void MessageFieldGenerator::GenerateSerializedSizeCode(Writer* writer) {
  writer->WriteLine("if (has$0$) {", property_name());
  writer->WriteLine("  size += pb::CodedOutputStream.Compute$0$Size($1$, $2$);",
                    message_or_group(), number(), property_name());
  writer->WriteLine("}");
}

void MessageFieldGenerator::WriteHash(Writer* writer) {
  writer->WriteLine("if (has$0$) hash ^= $1$_.GetHashCode();", property_name(),
                    name());
}
void MessageFieldGenerator::WriteEquals(Writer* writer) {
  writer->WriteLine(
      "if (has$0$ != other.has$0$ || (has$0$ && !$1$_.Equals(other.$1$_))) return false;",
      property_name(), name());
}
void MessageFieldGenerator::WriteToString(Writer* writer) {
  writer->WriteLine("PrintField(\"$2$\", has$0$, $1$_, writer);",
                    property_name(), name(), GetFieldName(descriptor_));
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
