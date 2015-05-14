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
#include <google/protobuf/compiler/csharp/csharp_primitive_field.h>
#include <google/protobuf/compiler/csharp/csharp_writer.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

PrimitiveFieldGenerator::PrimitiveFieldGenerator(
    const FieldDescriptor* descriptor, int fieldOrdinal)
    : FieldGeneratorBase(descriptor, fieldOrdinal) {
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {

}

void PrimitiveFieldGenerator::GenerateMembers(Writer* writer) {
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("private bool has$0$;", property_name());
  }
  writer->WriteLine("private $0$ $1$_$2$;", type_name(), name(),
                    has_default_value() ? " = " + default_value() : "");
  AddDeprecatedFlag(writer);
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("public bool Has$0$ {", property_name());
    writer->WriteLine("  get { return has$0$; }", property_name());
    writer->WriteLine("}");
  }
  AddPublicMemberAttributes(writer);
  writer->WriteLine("public $0$ $1$ {", type_name(), property_name());
  writer->WriteLine("  get { return $0$_; }", name());
  writer->WriteLine("}");
}

void PrimitiveFieldGenerator::GenerateBuilderMembers(Writer* writer) {
  AddDeprecatedFlag(writer);
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("public bool Has$0$ {", property_name());
    writer->WriteLine("  get { return result.has$0$; }", property_name());
    writer->WriteLine("}");
  }
  AddPublicMemberAttributes(writer);
  writer->WriteLine("public $0$ $1$ {", type_name(), property_name());
  writer->WriteLine("  get { return result.$0$; }", property_name());
  writer->WriteLine("  set { Set$0$(value); }", property_name());
  writer->WriteLine("}");
  AddPublicMemberAttributes(writer);
  writer->WriteLine("public Builder Set$0$($1$ value) {", property_name(),
                    type_name());
  AddNullCheck(writer);
  writer->WriteLine("  PrepareBuilder();");
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("  result.has$0$ = true;", property_name());
  }
  writer->WriteLine("  result.$0$_ = value;", name());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  AddDeprecatedFlag(writer);
  writer->WriteLine("public Builder Clear$0$() {", property_name());
  writer->WriteLine("  PrepareBuilder();");
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("  result.has$0$ = false;", property_name());
  }
  writer->WriteLine("  result.$0$_ = $1$;", name(), default_value());
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
}

void PrimitiveFieldGenerator::GenerateMergingCode(Writer* writer) {
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("if (other.Has$0$) {", property_name());
  } else {
    writer->WriteLine("if (other.$0$ != $1$) {", property_name(), default_value());
  }
  writer->WriteLine("  $0$ = other.$0$;", property_name());
  writer->WriteLine("}");
}

void PrimitiveFieldGenerator::GenerateBuildingCode(Writer* writer) {
  // Nothing to do here for primitive types
}

void PrimitiveFieldGenerator::GenerateParsingCode(Writer* writer) {
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("result.has$0$ = input.Read$1$(ref result.$2$_);",
                      property_name(), capitalized_type_name(), name());
  } else {
    writer->WriteLine("input.Read$0$(ref result.$1$_);",
                      capitalized_type_name(), name());
  }
}

void PrimitiveFieldGenerator::GenerateSerializationCode(Writer* writer) {
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("if (has$0$) {", property_name());
  } else {
    writer->WriteLine("if ($0$ != $1$) {", property_name(), default_value());
  }
  writer->WriteLine("  output.Write$0$($1$, field_names[$3$], $2$);",
                    capitalized_type_name(), number(), property_name(),
                    field_ordinal());
  writer->WriteLine("}");
}

void PrimitiveFieldGenerator::GenerateSerializedSizeCode(Writer* writer) {
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("if (has$0$) {", property_name());
  } else {
    writer->WriteLine("if ($0$ != $1$) {", property_name(), default_value());
  }
  writer->WriteLine("  size += pb::CodedOutputStream.Compute$0$Size($1$, $2$);",
                    capitalized_type_name(), number(), property_name());
  writer->WriteLine("}");
}

void PrimitiveFieldGenerator::WriteHash(Writer* writer) {
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("if (has$0$) {", property_name());
  } else {
    writer->WriteLine("if ($0$ != $1$) {", property_name(), default_value());
  }
  writer->WriteLine("  hash ^= $0$_.GetHashCode();", name());
  writer->WriteLine("}");
}
void PrimitiveFieldGenerator::WriteEquals(Writer* writer) {
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine(
        "if (has$0$ != other.has$0$ || (has$0$ && !$1$_.Equals(other.$1$_))) return false;",
        property_name(), name());
  } else {
    writer->WriteLine(
        "if (!$0$_.Equals(other.$0$_)) return false;", name());
  }
}
void PrimitiveFieldGenerator::WriteToString(Writer* writer) {
  if (SupportFieldPresence(descriptor_->file())) {
    writer->WriteLine("PrintField(\"$0$\", has$1$, $2$_, writer);",
                      descriptor_->name(), property_name(), name());
  } else {
    writer->WriteLine("PrintField(\"$0$\", $1$_, writer);",
                      descriptor_->name(), name());
  }
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
