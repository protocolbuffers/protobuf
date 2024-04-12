// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/extension.h"

#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

ExtensionGenerator::ExtensionGenerator(
    absl::string_view root_or_message_class_name,
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : method_name_(ExtensionMethodName(descriptor)),
      full_method_name_(
          absl::StrCat(root_or_message_class_name, "_", method_name_)),
      descriptor_(descriptor),
      generation_options_(generation_options) {
  ABSL_CHECK(!descriptor->is_map())
      << "error: Extension is a map<>!"
      << " That used to be blocked by the compiler.";
}

void ExtensionGenerator::GenerateMembersHeader(io::Printer* printer) const {
  printer->Emit(
      {{"method_name", method_name_},
       {"comments",
        [&] { EmitCommentsString(printer, generation_options_, descriptor_); }},
       {"storage_attribute",
        IsRetainedName(method_name_) ? "NS_RETURNS_NOT_RETAINED" : ""},
       {"deprecated_attribute",
        // Unlike normal message fields, check if the file for the extension was
        // deprecated.
        GetOptionalDeprecatedAttribute(descriptor_, descriptor_->file())}},
      R"objc(
        $comments$
        + (GPBExtensionDescriptor *)$method_name$$ storage_attribute$$ deprecated_attribute$;
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

void ExtensionGenerator::DetermineNeededFiles(
    absl::flat_hash_set<const FileDescriptor*>* deps) const {
  const Descriptor* extended_type = descriptor_->containing_type();
  if (descriptor_->file() != extended_type->file()) {
    deps->insert(extended_type->file());
  }

  const ObjectiveCType objc_type = GetObjectiveCType(descriptor_);
  if (objc_type == OBJECTIVECTYPE_MESSAGE) {
    const Descriptor* value_msg_descriptor = descriptor_->message_type();
    if (descriptor_->file() != value_msg_descriptor->file()) {
      deps->insert(value_msg_descriptor->file());
    }
  } else if (objc_type == OBJECTIVECTYPE_ENUM) {
    const EnumDescriptor* value_enum_descriptor = descriptor_->enum_type();
    if (descriptor_->file() != value_enum_descriptor->file()) {
      deps->insert(value_enum_descriptor->file());
    }
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
