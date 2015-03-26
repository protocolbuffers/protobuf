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

#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_repeated_message_field.h>
#include <google/protobuf/compiler/csharp/csharp_writer.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

RepeatedMessageFieldGenerator::RepeatedMessageFieldGenerator(
    const FieldDescriptor* descriptor, int fieldOrdinal)
    : FieldGeneratorBase(descriptor, fieldOrdinal) {
}

RepeatedMessageFieldGenerator::~RepeatedMessageFieldGenerator() {

}

void RepeatedMessageFieldGenerator::GenerateMembers(Writer* writer) {
  writer->WriteLine(
      "private pbc::PopsicleList<$0$> $1$_ = new pbc::PopsicleList<$0$>();",
      type_name(), name());
  AddDeprecatedFlag(writer);
  writer->WriteLine("public scg::IList<$0$> $1$List {", type_name(),
                    property_name());
  writer->WriteLine("  get { return $0$_; }", name());
  writer->WriteLine("}");

  // TODO(jonskeet): Redundant API calls? Possibly - include for portability though. Maybe create an option.
  AddDeprecatedFlag(writer);
  writer->WriteLine("public int $0$Count {", property_name());
  writer->WriteLine("  get { return $0$_.Count; }", name());
  writer->WriteLine("}");

  AddDeprecatedFlag(writer);
  writer->WriteLine("public $0$ Get$1$(int index) {", type_name(),
                    property_name());
  writer->WriteLine("  return $0$_[index];", name());
  writer->WriteLine("}");
}

void RepeatedMessageFieldGenerator::GenerateBuilderMembers(Writer* writer) {
  // Note:  We can return the original list here, because we make it unmodifiable when we build
  // We return it via IPopsicleList so that collection initializers work more pleasantly.
  AddDeprecatedFlag(writer);
  writer->WriteLine("public pbc::IPopsicleList<$0$> $1$List {", type_name(),
                    property_name());
  writer->WriteLine("  get { return PrepareBuilder().$0$_; }", name());
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public int $0$Count {", property_name());
  writer->WriteLine("  get { return result.$0$Count; }", property_name());
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public $0$ Get$1$(int index) {", type_name(),
                    property_name());
  writer->WriteLine("  return result.Get$0$(index);", property_name());
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Set$0$(int index, $1$ value) {",
                    property_name(), type_name());
  AddNullCheck(writer);
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.$0$_[index] = value;", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  // Extra overload for builder (just on messages)
  AddDeprecatedFlag(writer);
  writer->WriteLine(
      "public Builder Set$0$(int index, $1$.Builder builderForValue) {",
      property_name(), type_name());
  AddNullCheck(writer, "builderForValue");
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.$0$_[index] = builderForValue.Build();", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Add$0$($1$ value) {", property_name(),
                    type_name());
  AddNullCheck(writer);
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.$0$_.Add(value);", name(), type_name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  // Extra overload for builder (just on messages)
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Add$0$($1$.Builder builderForValue) {",
                    property_name(), type_name());
  AddNullCheck(writer, "builderForValue");
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.$0$_.Add(builderForValue.Build());", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine(
      "public Builder AddRange$0$(scg::IEnumerable<$1$> values) {",
      property_name(), type_name());
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.$0$_.Add(values);", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Clear$0$() {", property_name());
  writer->WriteLine("  PrepareBuilder();");
  writer->WriteLine("  result.$0$_.Clear();", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
}

void RepeatedMessageFieldGenerator::GenerateMergingCode(Writer* writer) {
  writer->WriteLine("if (other.$0$_.Count != 0) {", name());
  writer->WriteLine("  result.$0$_.Add(other.$0$_);", name());
  writer->WriteLine("}");
}

void RepeatedMessageFieldGenerator::GenerateBuildingCode(Writer* writer) {
  writer->WriteLine("$0$_.MakeReadOnly();", name());
}

void RepeatedMessageFieldGenerator::GenerateParsingCode(Writer* writer) {
  writer->WriteLine(
      "input.Read$0$Array(tag, field_name, result.$1$_, $2$.DefaultInstance, extensionRegistry);",
      message_or_group(), name(), type_name());
}

void RepeatedMessageFieldGenerator::GenerateSerializationCode(Writer* writer) {
  writer->WriteLine("if ($0$_.Count > 0) {", name());
  writer->Indent();
  writer->WriteLine("output.Write$0$Array($1$, field_names[$3$], $2$_);",
                    message_or_group(), number(), name(), field_ordinal());
  writer->Outdent();
  writer->WriteLine("}");
}

void RepeatedMessageFieldGenerator::GenerateSerializedSizeCode(Writer* writer) {
  writer->WriteLine("foreach ($0$ element in $1$List) {", type_name(),
                    property_name());
  writer->WriteLine(
      "  size += pb::CodedOutputStream.Compute$0$Size($1$, element);",
      message_or_group(), number());
  writer->WriteLine("}");
}

void RepeatedMessageFieldGenerator::WriteHash(Writer* writer) {
  writer->WriteLine("foreach($0$ i in $1$_)", type_name(), name());
  writer->WriteLine("  hash ^= i.GetHashCode();");
}
void RepeatedMessageFieldGenerator::WriteEquals(Writer* writer) {
  writer->WriteLine("if($0$_.Count != other.$0$_.Count) return false;", name());
  writer->WriteLine("for(int ix=0; ix < $0$_.Count; ix++)", name());
  writer->WriteLine("  if(!$0$_[ix].Equals(other.$0$_[ix])) return false;",
                    name());
}
void RepeatedMessageFieldGenerator::WriteToString(Writer* writer) {
  writer->WriteLine("PrintField(\"$0$\", $1$_, writer);",
                    GetFieldName(descriptor_), name());
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
