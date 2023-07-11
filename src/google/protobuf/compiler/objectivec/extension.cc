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

#include "google/protobuf/compiler/objectivec/extension.h"

#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

ExtensionGenerator::ExtensionGenerator(
    absl::string_view root_or_message_class_name,
    const FieldDescriptor* descriptor)
    : method_name_(ExtensionMethodName(descriptor)),
      full_method_name_(
          absl::StrCat(root_or_message_class_name, "_", method_name_)),
      descriptor_(descriptor) {
  ABSL_CHECK(!descriptor->is_map())
      << "error: Extension is a map<>!"
      << " That used to be blocked by the compiler.";
}

void ExtensionGenerator::GenerateMembersHeader(io::Printer* printer) const {
  printer->Emit(
      {{"method_name", method_name_},
       {"comments", [&] { EmitCommentsString(printer, descriptor_); }},
       {"storage_attribute",
        IsRetainedName(method_name_) ? "NS_RETURNS_NOT_RETAINED" : ""},
       {"deprecated_attribute",
        // Unlike normal message fields, check if the file for the extension was
        // deprecated.
        GetOptionalDeprecatedAttribute(descriptor_, descriptor_->file())}},
      R"objc(
        $comments$
        + (GPBExtensionDescriptor *)$method_name$$ storage_attribute$$deprecated_attribute$;
      )objc");
}

void ExtensionGenerator::GenerateStaticVariablesInitialization(
    io::Printer* printer) const {
  const std::string containing_type = ClassName(descriptor_->containing_type());
  ObjectiveCType objc_type = GetObjectiveCType(descriptor_);

  std::vector<std::string> options;
  if (descriptor_->is_repeated()) options.push_back("GPBExtensionRepeated");
  if (descriptor_->is_packed()) options.push_back("GPBExtensionPacked");
  if (descriptor_->containing_type()->options().message_set_wire_format()) {
    options.push_back("GPBExtensionSetWireFormat");
  }

  printer->Emit(
      {{"default",
        descriptor_->is_repeated() ? "nil" : DefaultValue(descriptor_)},
       {"default_name", GPBGenericValueFieldName(descriptor_)},
       {"enum_desc_func_name",
        objc_type == OBJECTIVECTYPE_ENUM
            ? absl::StrCat(EnumName(descriptor_->enum_type()),
                           "_EnumDescriptor")
            : "NULL"},
       {"extended_type", ObjCClass(containing_type)},
       {"extension_type",
        absl::StrCat("GPBDataType", GetCapitalizedType(descriptor_))},
       {"number", descriptor_->number()},
       {"options", BuildFlagsString(FLAGTYPE_EXTENSION, options)},
       {"full_method_name", full_method_name_},
       {"type", objc_type == OBJECTIVECTYPE_MESSAGE
                    ? ObjCClass(ClassName(descriptor_->message_type()))
                    : "Nil"}},
      R"objc(
        {
          .defaultValue.$default_name$ = $default$,
          .singletonName = GPBStringifySymbol($full_method_name$),
          .extendedClass.clazz = $extended_type$,
          .messageOrGroupClass.clazz = $type$,
          .enumDescriptorFunc = $enum_desc_func_name$,
          .fieldNumber = $number$,
          .dataType = $extension_type$,
          .options = $options$,
        },
      )objc");
}

void ExtensionGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  std::string extended_type = ClassName(descriptor_->containing_type());
  fwd_decls->insert(ObjCClassDeclaration(extended_type));
  ObjectiveCType objc_type = GetObjectiveCType(descriptor_);
  if (objc_type == OBJECTIVECTYPE_MESSAGE) {
    std::string message_type = ClassName(descriptor_->message_type());
    fwd_decls->insert(ObjCClassDeclaration(message_type));
  }
}

void ExtensionGenerator::GenerateRegistrationSource(
    io::Printer* printer) const {
  printer->Emit({{"full_method_name", full_method_name_}},
                R"objc(
                  [registry addExtension:$full_method_name$];
                )objc");
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
