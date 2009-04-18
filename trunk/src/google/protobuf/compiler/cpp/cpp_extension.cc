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

#include <google/protobuf/compiler/cpp/cpp_extension.h>
#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/io/printer.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

ExtensionGenerator::ExtensionGenerator(const FieldDescriptor* descriptor,
                                       const string& dllexport_decl)
  : descriptor_(descriptor),
    dllexport_decl_(dllexport_decl) {
  // Construct type_traits_.
  if (descriptor_->is_repeated()) {
    type_traits_ = "Repeated";
  }

  switch (descriptor_->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
      type_traits_.append("EnumTypeTraits< ");
      type_traits_.append(ClassName(descriptor_->enum_type(), true));
      type_traits_.append(" >");
      break;
    case FieldDescriptor::CPPTYPE_STRING:
      type_traits_.append("StringTypeTraits");
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      type_traits_.append("MessageTypeTraits< ");
      type_traits_.append(ClassName(descriptor_->message_type(), true));
      type_traits_.append(" >");
      break;
    default:
      type_traits_.append("PrimitiveTypeTraits< ");
      type_traits_.append(PrimitiveTypeName(descriptor_->cpp_type()));
      type_traits_.append(" >");
      break;
  }
}

ExtensionGenerator::~ExtensionGenerator() {}

void ExtensionGenerator::GenerateDeclaration(io::Printer* printer) {
  map<string, string> vars;
  vars["extendee"     ] = ClassName(descriptor_->containing_type(), true);
  vars["number"       ] = SimpleItoa(descriptor_->number());
  vars["type_traits"  ] = type_traits_;
  vars["name"         ] = descriptor_->name();
  vars["constant_name"] = FieldConstantName(descriptor_);

  // If this is a class member, it needs to be declared "static".  Otherwise,
  // it needs to be "extern".
  vars["qualifier"] =
    (descriptor_->extension_scope() == NULL) ? "extern" : "static";

  if (!dllexport_decl_.empty()) {
    vars["qualifier"] = dllexport_decl_ + " " + vars["qualifier"];
  }

  printer->Print(vars,
    "static const int $constant_name$ = $number$;\n"
    "$qualifier$ ::google::protobuf::internal::ExtensionIdentifier< $extendee$,\n"
    "  ::google::protobuf::internal::$type_traits$ > $name$;\n");
}

void ExtensionGenerator::GenerateDefinition(io::Printer* printer) {
  map<string, string> vars;
  vars["extendee"     ] = ClassName(descriptor_->containing_type(), true);
  vars["type_traits"  ] = type_traits_;
  vars["name"         ] = descriptor_->name();
  vars["constant_name"] = FieldConstantName(descriptor_);

  // If this is a class member, it needs to be declared in its class scope.
  vars["scope"] = (descriptor_->extension_scope() == NULL) ? "" :
    ClassName(descriptor_->extension_scope(), false) + "::";

  // Likewise, class members need to declare the field constant variable.
  if (descriptor_->extension_scope() != NULL) {
    printer->Print(vars,
      "#ifndef _MSC_VER\n"
      "const int $scope$$constant_name$;\n"
      "#endif\n");
  }

  printer->Print(vars,
    "::google::protobuf::internal::ExtensionIdentifier< $extendee$,\n"
    "  ::google::protobuf::internal::$type_traits$ > $scope$$name$("
      "$constant_name$);\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
