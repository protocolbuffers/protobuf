// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/map_field.h"

#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/objectivec/field.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// MapFieldGenerator uses RepeatedFieldGenerator as the parent because it
// provides a bunch of things (no has* methods, comments for contained type,
// etc.).

MapFieldGenerator::MapFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : RepeatedFieldGenerator(descriptor, generation_options) {
  const FieldDescriptor* key_descriptor = descriptor->message_type()->map_key();
  const FieldDescriptor* value_descriptor =
      descriptor->message_type()->map_value();
  value_field_generator_.reset(
      FieldGenerator::Make(value_descriptor, generation_options));

  // Pull over some variables_ from the value.
  variables_.Set("field_type", value_field_generator_->variable("field_type"));
  variables_.Set("default", value_field_generator_->variable("default"));
  variables_.Set("default_name",
                 value_field_generator_->variable("default_name"));

  // Build custom field flags.
  std::vector<std::string> field_flags;
  field_flags.push_back(
      absl::StrCat("GPBFieldMapKey", GetCapitalizedType(key_descriptor)));
  // Pull over the current text format custom name values that was calculated.
  if (absl::StrContains(variable("fieldflags"),
                        "GPBFieldTextFormatNameCustom")) {
    field_flags.push_back("GPBFieldTextFormatNameCustom");
  }
  // Pull over some info from the value's flags.
  const std::string& value_field_flags =
      value_field_generator_->variable("fieldflags");
  if (absl::StrContains(value_field_flags, "GPBFieldHasDefaultValue")) {
    field_flags.push_back("GPBFieldHasDefaultValue");
  }

  variables_.Set("fieldflags", BuildFlagsString(FLAGTYPE_FIELD, field_flags));

  variables_.Set("dataTypeSpecific_name",
                 value_field_generator_->variable("dataTypeSpecific_name"));
  variables_.Set("dataTypeSpecific_value",
                 value_field_generator_->variable("dataTypeSpecific_value"));
}

void MapFieldGenerator::EmitArrayComment(io::Printer* printer) const {
  // Use the array_comment support in RepeatedFieldGenerator to output what the
  // values in the map are.
  const FieldDescriptor* value_descriptor =
      descriptor_->message_type()->map_value();
  if (GetObjectiveCType(value_descriptor) == OBJECTIVECTYPE_ENUM) {
    printer->Emit(
        {{"name", variable("name")},
         {"enum_name", value_field_generator_->variable("enum_name")}},
        R"objc(
          // |$name$| values are |$enum_name$|
        )objc");
  }
}

void MapFieldGenerator::DetermineForwardDeclarations(
    absl::btree_set<std::string>* fwd_decls,
    bool include_external_types) const {
  RepeatedFieldGenerator::DetermineForwardDeclarations(fwd_decls,
                                                       include_external_types);
  const FieldDescriptor* value_descriptor =
      descriptor_->message_type()->map_value();
  // NOTE: Maps with values of enums don't have to worry about adding the
  // forward declaration because `GPB*EnumDictionary` isn't generic to the
  // specific enum (like say `NSDictionary<String, MyMessage>`) and thus doesn't
  // reference the type in the header.
  if (GetObjectiveCType(value_descriptor) != OBJECTIVECTYPE_MESSAGE) {
    return;
  }

  const Descriptor* value_msg_descriptor = value_descriptor->message_type();

  // Within a file there is no requirement on the order of the messages, so
  // local references need a forward declaration. External files (not WKTs),
  // need one when requested.
  if ((include_external_types &&
       !IsProtobufLibraryBundledProtoFile(value_msg_descriptor->file())) ||
      descriptor_->file() == value_msg_descriptor->file()) {
    const std::string& value_type =
        value_field_generator_->variable("msg_type");
    fwd_decls->insert(absl::StrCat("@class ", value_type, ";"));
  }
}

void MapFieldGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  // Class name is already in value's "msg_type".
  const FieldDescriptor* value_descriptor =
      descriptor_->message_type()->map_value();
  if (GetObjectiveCType(value_descriptor) == OBJECTIVECTYPE_MESSAGE) {
    fwd_decls->insert(
        ObjCClassDeclaration(value_field_generator_->variable("msg_type")));
  }
}

void MapFieldGenerator::DetermineNeededFiles(
    absl::flat_hash_set<const FileDescriptor*>* deps) const {
  const FieldDescriptor* value_descriptor =
      descriptor_->message_type()->map_value();
  const ObjectiveCType value_objc_type = GetObjectiveCType(value_descriptor);
  if (value_objc_type == OBJECTIVECTYPE_MESSAGE) {
    const Descriptor* value_msg_descriptor = value_descriptor->message_type();
    if (descriptor_->file() != value_msg_descriptor->file()) {
      deps->insert(value_msg_descriptor->file());
    }
  } else if (value_objc_type == OBJECTIVECTYPE_ENUM) {
    const EnumDescriptor* value_enum_descriptor = value_descriptor->enum_type();
    if (descriptor_->file() != value_enum_descriptor->file()) {
      deps->insert(value_enum_descriptor->file());
    }
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
