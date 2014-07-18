// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <map>
#include <string>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/java/java_context.h>
#include <google/protobuf/compiler/java/java_doc_comment.h>
#include <google/protobuf/compiler/java/java_enum_field.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/compiler/java/java_name_resolver.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

void SetEnumVariables(const FieldDescriptor* descriptor,
                      int messageBitIndex,
                      int builderBitIndex,
                      const FieldGeneratorInfo* info,
                      ClassNameResolver* name_resolver,
                      map<string, string>* variables) {
  SetCommonFieldVariables(descriptor, info, variables);

  (*variables)["type"] =
      name_resolver->GetImmutableClassName(descriptor->enum_type());
  (*variables)["mutable_type"] =
      name_resolver->GetMutableClassName(descriptor->enum_type());
  (*variables)["default"] = ImmutableDefaultValue(descriptor, name_resolver);
  (*variables)["tag"] = SimpleItoa(internal::WireFormat::MakeTag(descriptor));
  (*variables)["tag_size"] = SimpleItoa(
      internal::WireFormat::TagSize(descriptor->number(), GetType(descriptor)));
  // TODO(birdo): Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] = descriptor->options().deprecated()
      ? "@java.lang.Deprecated " : "";
  (*variables)["on_changed"] =
      HasDescriptorMethods(descriptor->containing_type()) ? "onChanged();" : "";

  if (SupportFieldPresence(descriptor->file())) {
    // For singular messages and builders, one bit is used for the hasField bit.
    (*variables)["get_has_field_bit_message"] = GenerateGetBit(messageBitIndex);
    (*variables)["get_has_field_bit_builder"] = GenerateGetBit(builderBitIndex);

    // Note that these have a trailing ";".
    (*variables)["set_has_field_bit_message"] =
        GenerateSetBit(messageBitIndex) + ";";
    (*variables)["set_has_field_bit_builder"] =
        GenerateSetBit(builderBitIndex) + ";";
    (*variables)["clear_has_field_bit_builder"] =
        GenerateClearBit(builderBitIndex) + ";";

    (*variables)["is_field_present_message"] = GenerateGetBit(messageBitIndex);
  } else {
    (*variables)["set_has_field_bit_message"] = "";
    (*variables)["set_has_field_bit_builder"] = "";
    (*variables)["clear_has_field_bit_builder"] = "";

    (*variables)["is_field_present_message"] =
          (*variables)["name"] + "_ != " + (*variables)["default"];
  }

  // For repated builders, one bit is used for whether the array is immutable.
  (*variables)["get_mutable_bit_builder"] = GenerateGetBit(builderBitIndex);
  (*variables)["set_mutable_bit_builder"] = GenerateSetBit(builderBitIndex);
  (*variables)["clear_mutable_bit_builder"] = GenerateClearBit(builderBitIndex);

  // For repeated fields, one bit is used for whether the array is immutable
  // in the parsing constructor.
  (*variables)["get_mutable_bit_parser"] =
      GenerateGetBitMutableLocal(builderBitIndex);
  (*variables)["set_mutable_bit_parser"] =
      GenerateSetBitMutableLocal(builderBitIndex);

  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(builderBitIndex);
  (*variables)["set_has_field_bit_to_local"] =
      GenerateSetBitToLocal(messageBitIndex);
}

}  // namespace

// ===================================================================

ImmutableEnumFieldGenerator::
ImmutableEnumFieldGenerator(const FieldDescriptor* descriptor,
                            int messageBitIndex,
                            int builderBitIndex,
                            Context* context)
  : descriptor_(descriptor), messageBitIndex_(messageBitIndex),
    builderBitIndex_(builderBitIndex),
    name_resolver_(context->GetNameResolver()) {
  SetEnumVariables(descriptor, messageBitIndex, builderBitIndex,
                   context->GetFieldGeneratorInfo(descriptor),
                   name_resolver_, &variables_);
}

ImmutableEnumFieldGenerator::~ImmutableEnumFieldGenerator() {}

int ImmutableEnumFieldGenerator::GetNumBitsForMessage() const {
  return 1;
}

int ImmutableEnumFieldGenerator::GetNumBitsForBuilder() const {
  return 1;
}

void ImmutableEnumFieldGenerator::
GenerateInterfaceMembers(io::Printer* printer) const {
  if (SupportFieldPresence(descriptor_->file())) {
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
      "$deprecation$boolean has$capitalized_name$();\n");
  }
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$$type$ get$capitalized_name$();\n");
}

void ImmutableEnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private $type$ $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);
  if (SupportFieldPresence(descriptor_->file())) {
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
      "$deprecation$public boolean has$capitalized_name$() {\n"
      "  return $get_has_field_bit_message$;\n"
      "}\n");
  }
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public $type$ get$capitalized_name$() {\n"
    "  return $name$_;\n"
    "}\n");
}

void ImmutableEnumFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private $type$ $name$_ = $default$;\n");
  if (SupportFieldPresence(descriptor_->file())) {
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
      "$deprecation$public boolean has$capitalized_name$() {\n"
      "  return $get_has_field_bit_builder$;\n"
      "}\n");
  }
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public $type$ get$capitalized_name$() {\n"
    "  return $name$_;\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public Builder set$capitalized_name$($type$ value) {\n"
    "  if (value == null) {\n"
    "    throw new NullPointerException();\n"
    "  }\n"
    "  $set_has_field_bit_builder$\n"
    "  $name$_ = value;\n"
    "  $on_changed$\n"
    "  return this;\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public Builder clear$capitalized_name$() {\n"
    "  $clear_has_field_bit_builder$\n"
    "  $name$_ = $default$;\n"
    "  $on_changed$\n"
    "  return this;\n"
    "}\n");
}

void ImmutableEnumFieldGenerator::
GenerateFieldBuilderInitializationCode(io::Printer* printer)  const {
  // noop for enums
}

void ImmutableEnumFieldGenerator::
GenerateInitializationCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default$;\n");
}

void ImmutableEnumFieldGenerator::
GenerateBuilderClearCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$name$_ = $default$;\n"
    "$clear_has_field_bit_builder$\n");
}

void ImmutableEnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(variables_,
      "if (other.has$capitalized_name$()) {\n"
      "  set$capitalized_name$(other.get$capitalized_name$());\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "if (other.get$capitalized_name$() != $default$) {\n"
      "  set$capitalized_name$(other.get$capitalized_name$());\n"
      "}\n");
  }
}

void ImmutableEnumFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  if (SupportFieldPresence(descriptor_->file())) {
    printer->Print(variables_,
      "if ($get_has_field_bit_from_local$) {\n"
      "  $set_has_field_bit_to_local$;\n"
      "}\n");
  }
  printer->Print(variables_,
    "result.$name$_ = $name$_;\n");
}

void ImmutableEnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "int rawValue = input.readEnum();\n"
    "$type$ value = $type$.valueOf(rawValue);\n"
    "if (value == null) {\n");
  if (UseUnknownFieldSet(descriptor_->containing_type())) {
    printer->Print(variables_,
      "  unknownFields.mergeVarintField($number$, rawValue);\n");
  } else {
    printer->Print(variables_,
      "  unknownFieldsCodedOutput.writeRawVarint32(tag);\n"
      "  unknownFieldsCodedOutput.writeRawVarint32(rawValue);\n");
  }
  printer->Print(variables_,
    "} else {\n"
    "  $set_has_field_bit_message$\n"
    "  $name$_ = value;\n"
    "}\n");
}

void ImmutableEnumFieldGenerator::
GenerateParsingDoneCode(io::Printer* printer) const {
  // noop for enums
}

void ImmutableEnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if ($is_field_present_message$) {\n"
    "  output.writeEnum($number$, $name$_.getNumber());\n"
    "}\n");
}

void ImmutableEnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if ($is_field_present_message$) {\n"
    "  size += com.google.protobuf.CodedOutputStream\n"
    "    .computeEnumSize($number$, $name$_.getNumber());\n"
    "}\n");
}

void ImmutableEnumFieldGenerator::
GenerateEqualsCode(io::Printer* printer) const {
  printer->Print(variables_,
    "result = result &&\n"
    "    (get$capitalized_name$() == other.get$capitalized_name$());\n");
}

void ImmutableEnumFieldGenerator::
GenerateHashCode(io::Printer* printer) const {
  printer->Print(variables_,
    "hash = (37 * hash) + $constant_name$;\n"
    "hash = (53 * hash) + com.google.protobuf.Internal.hashEnum(\n"
    "    get$capitalized_name$());\n");
}

string ImmutableEnumFieldGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->enum_type());
}

// ===================================================================

ImmutableEnumOneofFieldGenerator::
ImmutableEnumOneofFieldGenerator(const FieldDescriptor* descriptor,
                                 int messageBitIndex,
                                 int builderBitIndex,
                                 Context* context)
    : ImmutableEnumFieldGenerator(
          descriptor, messageBitIndex, builderBitIndex, context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutableEnumOneofFieldGenerator::
~ImmutableEnumOneofFieldGenerator() {}

void ImmutableEnumOneofFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  PrintExtraFieldInfo(variables_, printer);
  if (SupportFieldPresence(descriptor_->file())) {
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
      "$deprecation$public boolean has$capitalized_name$() {\n"
      "  return $has_oneof_case_message$;\n"
      "}\n");
  }
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public $type$ get$capitalized_name$() {\n"
    "  if ($has_oneof_case_message$) {\n"
    "    return ($type$) $oneof_name$_;\n"
    "  }\n"
    "  return $default$;\n"
    "}\n");
}

void ImmutableEnumOneofFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  if (SupportFieldPresence(descriptor_->file())) {
    WriteFieldDocComment(printer, descriptor_);
    printer->Print(variables_,
      "$deprecation$public boolean has$capitalized_name$() {\n"
      "  return $has_oneof_case_message$;\n"
      "}\n");
  }
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public $type$ get$capitalized_name$() {\n"
    "  if ($has_oneof_case_message$) {\n"
    "    return ($type$) $oneof_name$_;\n"
    "  }\n"
    "  return $default$;\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public Builder set$capitalized_name$($type$ value) {\n"
    "  if (value == null) {\n"
    "    throw new NullPointerException();\n"
    "  }\n"
    "  $set_oneof_case_message$;\n"
    "  $oneof_name$_ = value;\n"
    "  $on_changed$\n"
    "  return this;\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public Builder clear$capitalized_name$() {\n"
    "  if ($has_oneof_case_message$) {\n"
    "    $clear_oneof_case_message$;\n"
    "    $oneof_name$_ = null;\n"
    "    $on_changed$\n"
    "  }\n"
    "  return this;\n"
    "}\n");
}

void ImmutableEnumOneofFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if ($has_oneof_case_message$) {\n"
    "  result.$oneof_name$_ = $oneof_name$_;\n"
    "}\n");
}

void ImmutableEnumOneofFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "set$capitalized_name$(other.get$capitalized_name$());\n");
}

void ImmutableEnumOneofFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  printer->Print(variables_,
    "int rawValue = input.readEnum();\n"
    "$type$ value = $type$.valueOf(rawValue);\n"
    "if (value == null) {\n");
  if (UseUnknownFieldSet(descriptor_->containing_type())) {
    printer->Print(variables_,
      "  unknownFields.mergeVarintField($number$, rawValue);\n");
  } else {
    printer->Print(variables_,
      "  unknownFieldsCodedOutput.writeRawVarint32(tag);\n"
      "  unknownFieldsCodedOutput.writeRawVarint32(rawValue);\n");
  }
  printer->Print(variables_,
    "} else {\n"
    "  $set_oneof_case_message$;\n"
    "  $oneof_name$_ = value;\n"
    "}\n");
}

void ImmutableEnumOneofFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if ($has_oneof_case_message$) {\n"
    "  output.writeEnum($number$, (($type$) $oneof_name$_).getNumber());\n"
    "}\n");
}

void ImmutableEnumOneofFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if ($has_oneof_case_message$) {\n"
    "  size += com.google.protobuf.CodedOutputStream\n"
    "    .computeEnumSize($number$, (($type$) $oneof_name$_).getNumber());\n"
    "}\n");
}

// ===================================================================

RepeatedImmutableEnumFieldGenerator::
RepeatedImmutableEnumFieldGenerator(const FieldDescriptor* descriptor,
                                    int messageBitIndex,
                                    int builderBitIndex,
                                    Context* context)
  : descriptor_(descriptor), messageBitIndex_(messageBitIndex),
    builderBitIndex_(builderBitIndex), context_(context),
    name_resolver_(context->GetNameResolver()) {
  SetEnumVariables(descriptor, messageBitIndex, builderBitIndex,
                   context->GetFieldGeneratorInfo(descriptor),
                   name_resolver_, &variables_);
}

RepeatedImmutableEnumFieldGenerator::~RepeatedImmutableEnumFieldGenerator() {}

int RepeatedImmutableEnumFieldGenerator::GetNumBitsForMessage() const {
  return 0;
}

int RepeatedImmutableEnumFieldGenerator::GetNumBitsForBuilder() const {
  return 1;
}

void RepeatedImmutableEnumFieldGenerator::
GenerateInterfaceMembers(io::Printer* printer) const {
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$java.util.List<$type$> get$capitalized_name$List();\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$int get$capitalized_name$Count();\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$$type$ get$capitalized_name$(int index);\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateMembers(io::Printer* printer) const {
  printer->Print(variables_,
    "private java.util.List<$type$> $name$_;\n");
  PrintExtraFieldInfo(variables_, printer);
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public java.util.List<$type$> get$capitalized_name$List() {\n"
    "  return $name$_;\n"   // note:  unmodifiable list
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public int get$capitalized_name$Count() {\n"
    "  return $name$_.size();\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public $type$ get$capitalized_name$(int index) {\n"
    "  return $name$_.get(index);\n"
    "}\n");

  if (descriptor_->options().packed() &&
      HasGeneratedMethods(descriptor_->containing_type())) {
    printer->Print(variables_,
      "private int $name$MemoizedSerializedSize;\n");
  }
}

void RepeatedImmutableEnumFieldGenerator::
GenerateBuilderMembers(io::Printer* printer) const {
  printer->Print(variables_,
    // One field is the list and the other field keeps track of whether the
    // list is immutable. If it's immutable, the invariant is that it must
    // either an instance of Collections.emptyList() or it's an ArrayList
    // wrapped in a Collections.unmodifiableList() wrapper and nobody else has
    // a refererence to the underlying ArrayList. This invariant allows us to
    // share instances of lists between protocol buffers avoiding expensive
    // memory allocations. Note, immutable is a strong guarantee here -- not
    // just that the list cannot be modified via the reference but that the
    // list can never be modified.
    "private java.util.List<$type$> $name$_ =\n"
    "  java.util.Collections.emptyList();\n"

    "private void ensure$capitalized_name$IsMutable() {\n"
    "  if (!$get_mutable_bit_builder$) {\n"
    "    $name$_ = new java.util.ArrayList<$type$>($name$_);\n"
    "    $set_mutable_bit_builder$;\n"
    "  }\n"
    "}\n");

  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    // Note:  We return an unmodifiable list because otherwise the caller
    //   could hold on to the returned list and modify it after the message
    //   has been built, thus mutating the message which is supposed to be
    //   immutable.
    "$deprecation$public java.util.List<$type$> get$capitalized_name$List() {\n"
    "  return java.util.Collections.unmodifiableList($name$_);\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public int get$capitalized_name$Count() {\n"
    "  return $name$_.size();\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public $type$ get$capitalized_name$(int index) {\n"
    "  return $name$_.get(index);\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public Builder set$capitalized_name$(\n"
    "    int index, $type$ value) {\n"
    "  if (value == null) {\n"
    "    throw new NullPointerException();\n"
    "  }\n"
    "  ensure$capitalized_name$IsMutable();\n"
    "  $name$_.set(index, value);\n"
    "  $on_changed$\n"
    "  return this;\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public Builder add$capitalized_name$($type$ value) {\n"
    "  if (value == null) {\n"
    "    throw new NullPointerException();\n"
    "  }\n"
    "  ensure$capitalized_name$IsMutable();\n"
    "  $name$_.add(value);\n"
    "  $on_changed$\n"
    "  return this;\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public Builder addAll$capitalized_name$(\n"
    "    java.lang.Iterable<? extends $type$> values) {\n"
    "  ensure$capitalized_name$IsMutable();\n"
    "  com.google.protobuf.AbstractMessageLite.Builder.addAll(\n"
    "      values, $name$_);\n"
    "  $on_changed$\n"
    "  return this;\n"
    "}\n");
  WriteFieldDocComment(printer, descriptor_);
  printer->Print(variables_,
    "$deprecation$public Builder clear$capitalized_name$() {\n"
    "  $name$_ = java.util.Collections.emptyList();\n"
    "  $clear_mutable_bit_builder$;\n"
    "  $on_changed$\n"
    "  return this;\n"
    "}\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateFieldBuilderInitializationCode(io::Printer* printer)  const {
  // noop for enums
}

void RepeatedImmutableEnumFieldGenerator::
GenerateInitializationCode(io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = java.util.Collections.emptyList();\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateBuilderClearCode(io::Printer* printer) const {
  printer->Print(variables_,
    "$name$_ = java.util.Collections.emptyList();\n"
    "$clear_mutable_bit_builder$;\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateMergingCode(io::Printer* printer) const {
  // The code below does two optimizations:
  //   1. If the other list is empty, there's nothing to do. This ensures we
  //      don't allocate a new array if we already have an immutable one.
  //   2. If the other list is non-empty and our current list is empty, we can
  //      reuse the other list which is guaranteed to be immutable.
  printer->Print(variables_,
    "if (!other.$name$_.isEmpty()) {\n"
    "  if ($name$_.isEmpty()) {\n"
    "    $name$_ = other.$name$_;\n"
    "    $clear_mutable_bit_builder$;\n"
    "  } else {\n"
    "    ensure$capitalized_name$IsMutable();\n"
    "    $name$_.addAll(other.$name$_);\n"
    "  }\n"
    "  $on_changed$\n"
    "}\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateBuildingCode(io::Printer* printer) const {
  // The code below ensures that the result has an immutable list. If our
  // list is immutable, we can just reuse it. If not, we make it immutable.
  printer->Print(variables_,
    "if ($get_mutable_bit_builder$) {\n"
    "  $name$_ = java.util.Collections.unmodifiableList($name$_);\n"
    "  $clear_mutable_bit_builder$;\n"
    "}\n"
    "result.$name$_ = $name$_;\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateParsingCode(io::Printer* printer) const {
  // Read and store the enum
  printer->Print(variables_,
    "int rawValue = input.readEnum();\n"
    "$type$ value = $type$.valueOf(rawValue);\n"
    "if (value == null) {\n");
  if (UseUnknownFieldSet(descriptor_->containing_type())) {
    printer->Print(variables_,
      "  unknownFields.mergeVarintField($number$, rawValue);\n");
  } else {
    printer->Print(variables_,
      "  unknownFieldsCodedOutput.writeRawVarint32(tag);\n"
      "  unknownFieldsCodedOutput.writeRawVarint32(rawValue);\n");
  }
  printer->Print(variables_,
    "  } else {\n"
    "  if (!$get_mutable_bit_parser$) {\n"
    "    $name$_ = new java.util.ArrayList<$type$>();\n"
    "    $set_mutable_bit_parser$;\n"
    "  }\n"
    "  $name$_.add(value);\n"
    "}\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateParsingCodeFromPacked(io::Printer* printer) const {
  // Wrap GenerateParsingCode's contents with a while loop.

  printer->Print(variables_,
    "int length = input.readRawVarint32();\n"
    "int oldLimit = input.pushLimit(length);\n"
    "while(input.getBytesUntilLimit() > 0) {\n");
  printer->Indent();

  GenerateParsingCode(printer);

  printer->Outdent();
  printer->Print(variables_,
    "}\n"
    "input.popLimit(oldLimit);\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateParsingDoneCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if ($get_mutable_bit_parser$) {\n"
    "  $name$_ = java.util.Collections.unmodifiableList($name$_);\n"
    "}\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateSerializationCode(io::Printer* printer) const {
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "if (get$capitalized_name$List().size() > 0) {\n"
      "  output.writeRawVarint32($tag$);\n"
      "  output.writeRawVarint32($name$MemoizedSerializedSize);\n"
      "}\n"
      "for (int i = 0; i < $name$_.size(); i++) {\n"
      "  output.writeEnumNoTag($name$_.get(i).getNumber());\n"
      "}\n");
  } else {
    printer->Print(variables_,
      "for (int i = 0; i < $name$_.size(); i++) {\n"
      "  output.writeEnum($number$, $name$_.get(i).getNumber());\n"
      "}\n");
  }
}

void RepeatedImmutableEnumFieldGenerator::
GenerateSerializedSizeCode(io::Printer* printer) const {
  printer->Print(variables_,
    "{\n"
    "  int dataSize = 0;\n");
  printer->Indent();

  printer->Print(variables_,
    "for (int i = 0; i < $name$_.size(); i++) {\n"
    "  dataSize += com.google.protobuf.CodedOutputStream\n"
    "    .computeEnumSizeNoTag($name$_.get(i).getNumber());\n"
    "}\n");
  printer->Print(
    "size += dataSize;\n");
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "if (!get$capitalized_name$List().isEmpty()) {"
      "  size += $tag_size$;\n"
      "  size += com.google.protobuf.CodedOutputStream\n"
      "    .computeRawVarint32Size(dataSize);\n"
      "}");
  } else {
    printer->Print(variables_,
      "size += $tag_size$ * $name$_.size();\n");
  }

  // cache the data size for packed fields.
  if (descriptor_->options().packed()) {
    printer->Print(variables_,
      "$name$MemoizedSerializedSize = dataSize;\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateEqualsCode(io::Printer* printer) const {
  printer->Print(variables_,
    "result = result && get$capitalized_name$List()\n"
    "    .equals(other.get$capitalized_name$List());\n");
}

void RepeatedImmutableEnumFieldGenerator::
GenerateHashCode(io::Printer* printer) const {
  printer->Print(variables_,
    "if (get$capitalized_name$Count() > 0) {\n"
    "  hash = (37 * hash) + $constant_name$;\n"
    "  hash = (53 * hash) + com.google.protobuf.Internal.hashEnumList(\n"
    "      get$capitalized_name$List());\n"
    "}\n");
}

string RepeatedImmutableEnumFieldGenerator::GetBoxedType() const {
  return name_resolver_->GetImmutableClassName(descriptor_->enum_type());
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
