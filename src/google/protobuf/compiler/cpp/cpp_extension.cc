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
  vars["extendee"   ] = ClassName(descriptor_->containing_type(), true);
  vars["type_traits"] = type_traits_;
  vars["name"       ] = descriptor_->name();

  // If this is a class member, it needs to be declared "static".  Otherwise,
  // it needs to be "extern".
  vars["qualifier"] =
    (descriptor_->extension_scope() == NULL) ? "extern" : "static";

  if (!dllexport_decl_.empty()) {
    vars["qualifier"] = dllexport_decl_ + " " + vars["qualifier"];
  }

  printer->Print(vars,
    "$qualifier$ ::google::protobuf::internal::ExtensionIdentifier< $extendee$,\n"
    "  ::google::protobuf::internal::$type_traits$ > $name$;\n");
}

void ExtensionGenerator::GenerateDefinition(io::Printer* printer) {
  map<string, string> vars;
  vars["extendee"   ] = ClassName(descriptor_->containing_type(), true);
  vars["number"     ] = SimpleItoa(descriptor_->number());
  vars["type_traits"] = type_traits_;
  vars["name"       ] = descriptor_->name();

  // If this is a class member, it needs to be declared in its class scope.
  vars["scope"] = (descriptor_->extension_scope() == NULL) ? "" :
    ClassName(descriptor_->extension_scope(), false) + "::";

  printer->Print(vars,
    "::google::protobuf::internal::ExtensionIdentifier< $extendee$,\n"
    "  ::google::protobuf::internal::$type_traits$ > $scope$$name$($number$);\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
