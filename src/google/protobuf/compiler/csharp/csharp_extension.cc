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

#include <google/protobuf/compiler/csharp/csharp_extension.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_field_base.h>

using google::protobuf::internal::scoped_ptr;

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

ExtensionGenerator::ExtensionGenerator(const FieldDescriptor* descriptor)
    : FieldGeneratorBase(descriptor, 0) {
  if (descriptor_->extension_scope()) {
    variables_["scope"] = GetClassName(descriptor_->extension_scope());
  } else {
    variables_["scope"] = GetFullUmbrellaClassName(descriptor_->file());
  }
  variables_["extends"] = GetClassName(descriptor_->containing_type());
  variables_["capitalized_type_name"] = capitalized_type_name();
  variables_["full_name"] = descriptor_->full_name();
  variables_["access_level"] = class_access_level();
  variables_["index"] = SimpleItoa(descriptor_->index());
  variables_["property_name"] = property_name();
  variables_["type_name"] = type_name();
  if (use_lite_runtime()) {
    variables_["generated_extension"] = descriptor_->is_repeated() ?
      "GeneratedRepeatExtensionLite" : "GeneratedExtensionLite";
  } else {
    variables_["generated_extension"] = descriptor_->is_repeated() ?
      "GeneratedRepeatExtension" : "GeneratedExtension";
  }
}

ExtensionGenerator::~ExtensionGenerator() {
}

void ExtensionGenerator::Generate(io::Printer* printer) {
  printer->Print(
    "public const int $constant_name$ = $number$;\n",
    "constant_name", GetFieldConstantName(descriptor_),
    "number", SimpleItoa(descriptor_->number()));

  if (use_lite_runtime()) {
    // TODO(jtattermusch): include the following check
    //if (Descriptor.MappedType == MappedType.Message && Descriptor.MessageType.Options.MessageSetWireFormat)
    //{
    //    throw new ArgumentException(
    //        "option message_set_wire_format = true; is not supported in Lite runtime extensions.");
    //}

    printer->Print(
      variables_,
      "$access_level$ static pb::$generated_extension$<$extends$, $type_name$> $property_name$;\n");
  } else if (descriptor_->is_repeated()) {
    printer->Print(
      variables_,
      "$access_level$ static pb::GeneratedExtensionBase<scg::IList<$type_name$>> $property_name$;\n");
  } else {
    printer->Print(
      variables_,
      "$access_level$ static pb::GeneratedExtensionBase<$type_name$> $property_name$;\n");
  }
}

void ExtensionGenerator::GenerateStaticVariableInitializers(io::Printer* printer) {
  if (use_lite_runtime()) {
    printer->Print(
      variables_,
      "$scope$.$property_name$ = \n");
    printer->Indent();
    printer->Print(
      variables_,
      "new pb::$generated_extension$<$extends$, $type_name$>(\n");
    printer->Indent();
    printer->Print(
      variables_,
      "\"$full_name$\",\n"
      "$extends$.DefaultInstance,\n");
    if (!descriptor_->is_repeated()) {
      std::string default_val;
      if (descriptor_->has_default_value()) {
        default_val = default_value();
      } else {
        default_val = is_nullable_type() ? "null" : ("default(" + type_name() + ")");
      }
      printer->Print("$default_val$,\n", "default_val", default_val);
    }
    printer->Print(
      "$message_val$,\n",
      "message_val",
      (GetCSharpType(descriptor_->type()) == CSHARPTYPE_MESSAGE) ?
	  type_name() + ".DefaultInstance" : "null");
    printer->Print(
      "$enum_val$,\n",
      "enum_val",
      (GetCSharpType(descriptor_->type()) == CSHARPTYPE_ENUM) ?
	  "new EnumLiteMap<" + type_name() + ">()" : "null");
    printer->Print(
      variables_,
      "$scope$.$property_name$FieldNumber,\n"
      "pbd::FieldType.$capitalized_type_name$");
    if (descriptor_->is_repeated()) {
      printer->Print(
        ",\n"
        "$is_packed$",
        "is_packed", descriptor_->is_packed() ? "true" : "false");
    }
    printer->Outdent();
    printer->Print(");\n");
    printer->Outdent();
  }
  else if (descriptor_->is_repeated())
  {
    printer->Print(
      variables_,
      "$scope$.$property_name$ = pb::GeneratedRepeatExtension<$type_name$>.CreateInstance($scope$.Descriptor.Extensions[$index$]);\n");
  }
  else
  {
    printer->Print(
      variables_,
      "$scope$.$property_name$ = pb::GeneratedSingleExtension<$type_name$>.CreateInstance($scope$.Descriptor.Extensions[$index$]);\n");
  }
}

void ExtensionGenerator::GenerateExtensionRegistrationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "registry.Add($scope$.$property_name$);\n");
}

void ExtensionGenerator::WriteHash(io::Printer* printer) {
}

void ExtensionGenerator::WriteEquals(io::Printer* printer) {
}

void ExtensionGenerator::WriteToString(io::Printer* printer) {
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
