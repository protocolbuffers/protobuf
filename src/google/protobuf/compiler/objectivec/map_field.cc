// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

#include "google/protobuf/compiler/objectivec/map_field.h"

#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/log/absl_log.h"
#include "absl/strings/match.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

// MapFieldGenerator uses RepeatedFieldGenerator as the parent because it
// provides a bunch of things (no has* methods, comments for contained type,
// etc.).

namespace {

const char* MapEntryTypeName(const FieldDescriptor* descriptor, bool isKey) {
  ObjectiveCType type = GetObjectiveCType(descriptor);
  switch (type) {
    case OBJECTIVECTYPE_INT32:
      return "Int32";
    case OBJECTIVECTYPE_UINT32:
      return "UInt32";
    case OBJECTIVECTYPE_INT64:
      return "Int64";
    case OBJECTIVECTYPE_UINT64:
      return "UInt64";
    case OBJECTIVECTYPE_FLOAT:
      return "Float";
    case OBJECTIVECTYPE_DOUBLE:
      return "Double";
    case OBJECTIVECTYPE_BOOLEAN:
      return "Bool";
    case OBJECTIVECTYPE_STRING:
      return (isKey ? "String" : "Object");
    case OBJECTIVECTYPE_DATA:
      return "Object";
    case OBJECTIVECTYPE_ENUM:
      return "Enum";
    case OBJECTIVECTYPE_MESSAGE:
      return "Object";
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return nullptr;
}

}  // namespace

MapFieldGenerator::MapFieldGenerator(const FieldDescriptor* descriptor)
    : RepeatedFieldGenerator(descriptor) {
  const FieldDescriptor* key_descriptor = descriptor->message_type()->map_key();
  const FieldDescriptor* value_descriptor =
      descriptor->message_type()->map_value();
  value_field_generator_.reset(FieldGenerator::Make(value_descriptor));

  // Pull over some variables_ from the value.
  variables_["field_type"] = value_field_generator_->variable("field_type");
  variables_["default"] = value_field_generator_->variable("default");
  variables_["default_name"] = value_field_generator_->variable("default_name");

  // Build custom field flags.
  std::vector<std::string> field_flags;
  field_flags.push_back(
      absl::StrCat("GPBFieldMapKey", GetCapitalizedType(key_descriptor)));
  // Pull over the current text format custom name values that was calculated.
  if (absl::StrContains(variables_["fieldflags"],
                        "GPBFieldTextFormatNameCustom")) {
    field_flags.push_back("GPBFieldTextFormatNameCustom");
  }
  // Pull over some info from the value's flags.
  const std::string& value_field_flags =
      value_field_generator_->variable("fieldflags");
  if (absl::StrContains(value_field_flags, "GPBFieldHasDefaultValue")) {
    field_flags.push_back("GPBFieldHasDefaultValue");
  }
  if (absl::StrContains(value_field_flags, "GPBFieldHasEnumDescriptor")) {
    field_flags.push_back("GPBFieldHasEnumDescriptor");
    if (absl::StrContains(value_field_flags, "GPBFieldClosedEnum")) {
      field_flags.push_back("GPBFieldClosedEnum");
    }
  }

  variables_["fieldflags"] = BuildFlagsString(FLAGTYPE_FIELD, field_flags);

  ObjectiveCType value_objc_type = GetObjectiveCType(value_descriptor);
  const bool value_is_object_type =
      ((value_objc_type == OBJECTIVECTYPE_STRING) ||
       (value_objc_type == OBJECTIVECTYPE_DATA) ||
       (value_objc_type == OBJECTIVECTYPE_MESSAGE));
  if ((GetObjectiveCType(key_descriptor) == OBJECTIVECTYPE_STRING) &&
      value_is_object_type) {
    variables_["array_storage_type"] = "NSMutableDictionary";
    variables_["array_property_type"] =
        absl::StrCat("NSMutableDictionary<NSString*, ",
                     value_field_generator_->variable("storage_type"), "*>");
  } else {
    std::string class_name =
        absl::StrCat("GPB", MapEntryTypeName(key_descriptor, true),
                     MapEntryTypeName(value_descriptor, false), "Dictionary");
    variables_["array_storage_type"] = class_name;
    if (value_is_object_type) {
      variables_["array_property_type"] =
          absl::StrCat(class_name, "<",
                       value_field_generator_->variable("storage_type"), "*>");
    }
  }

  variables_["dataTypeSpecific_name"] =
      value_field_generator_->variable("dataTypeSpecific_name");
  variables_["dataTypeSpecific_value"] =
      value_field_generator_->variable("dataTypeSpecific_value");
}

void MapFieldGenerator::FinishInitialization() {
  RepeatedFieldGenerator::FinishInitialization();
  // Use the array_comment support in RepeatedFieldGenerator to output what the
  // values in the map are.
  const FieldDescriptor* value_descriptor =
      descriptor_->message_type()->map_value();
  if (GetObjectiveCType(value_descriptor) == OBJECTIVECTYPE_ENUM) {
    std::string name = variables_["name"];
    variables_["array_comment"] =
        absl::StrCat("// |", name, "| values are |",
                     value_field_generator_->variable("storage_type"), "|\n");
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
    const std::string& value_storage_type =
        value_field_generator_->variable("storage_type");
    fwd_decls->insert(absl::StrCat("@class ", value_storage_type, ";"));
  }
}

void MapFieldGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  // Class name is already in "storage_type".
  const FieldDescriptor* value_descriptor =
      descriptor_->message_type()->map_value();
  if (GetObjectiveCType(value_descriptor) == OBJECTIVECTYPE_MESSAGE) {
    fwd_decls->insert(
        ObjCClassDeclaration(value_field_generator_->variable("storage_type")));
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
