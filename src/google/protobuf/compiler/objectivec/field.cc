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

#include "google/protobuf/compiler/objectivec/field.h"

#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/objectivec/enum_field.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/map_field.h"
#include "google/protobuf/compiler/objectivec/message_field.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/primitive_field.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

void SetCommonFieldVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  std::string camel_case_name = FieldName(descriptor);
  std::string raw_field_name;
  if (descriptor->type() == FieldDescriptor::TYPE_GROUP) {
    raw_field_name = descriptor->message_type()->name();
  } else {
    raw_field_name = descriptor->name();
  }
  // The logic here has to match -[GGPBFieldDescriptor textFormatName].
  const std::string un_camel_case_name(
      UnCamelCaseFieldName(camel_case_name, descriptor));
  const bool needs_custom_name = (raw_field_name != un_camel_case_name);

  const std::string& classname = ClassName(descriptor->containing_type());
  (*variables)["classname"] = classname;
  (*variables)["name"] = camel_case_name;
  const std::string& capitalized_name = FieldNameCapitalized(descriptor);
  (*variables)["capitalized_name"] = capitalized_name;
  (*variables)["raw_field_name"] = raw_field_name;
  (*variables)["field_number_name"] =
      absl::StrCat(classname, "_FieldNumber_", capitalized_name);
  (*variables)["field_number"] = absl::StrCat(descriptor->number());
  (*variables)["field_type"] = GetCapitalizedType(descriptor);
  (*variables)["deprecated_attribute"] =
      GetOptionalDeprecatedAttribute(descriptor);
  std::vector<std::string> field_flags;
  if (descriptor->is_repeated()) field_flags.push_back("GPBFieldRepeated");
  if (descriptor->is_required()) field_flags.push_back("GPBFieldRequired");
  if (descriptor->is_optional()) field_flags.push_back("GPBFieldOptional");
  if (descriptor->is_packed()) field_flags.push_back("GPBFieldPacked");

  // ObjC custom flags.
  if (descriptor->has_default_value())
    field_flags.push_back("GPBFieldHasDefaultValue");
  if (needs_custom_name) field_flags.push_back("GPBFieldTextFormatNameCustom");
  if (descriptor->type() == FieldDescriptor::TYPE_ENUM) {
    field_flags.push_back("GPBFieldHasEnumDescriptor");
    if (descriptor->enum_type()->is_closed()) {
      field_flags.push_back("GPBFieldClosedEnum");
    }
  }
  // It will clear on a zero value if...
  //  - not repeated/map
  //  - doesn't have presence
  bool clear_on_zero =
      (!descriptor->is_repeated() && !descriptor->has_presence());
  if (clear_on_zero) {
    field_flags.push_back("GPBFieldClearHasIvarOnZero");
  }

  (*variables)["fieldflags"] = BuildFlagsString(FLAGTYPE_FIELD, field_flags);

  (*variables)["default"] = DefaultValue(descriptor);
  (*variables)["default_name"] = GPBGenericValueFieldName(descriptor);

  (*variables)["dataTypeSpecific_name"] = "clazz";
  (*variables)["dataTypeSpecific_value"] = "Nil";

  (*variables)["storage_offset_value"] = absl::StrCat(
      "(uint32_t)offsetof(", classname, "__storage_, ", camel_case_name, ")");
  (*variables)["storage_offset_comment"] = "";

  // Clear some common things so they can be set just when needed.
  (*variables)["storage_attribute"] = "";
}

bool HasNonZeroDefaultValue(const FieldDescriptor* field) {
  // Repeated fields don't have defaults.
  if (field->is_repeated()) {
    return false;
  }

  // As much as checking field->has_default_value() seems useful, it isn't
  // because of enums. proto2 syntax allows the first item in an enum (the
  // default) to be non zero. So checking field->has_default_value() would
  // result in missing this non zero default.  See MessageWithOneBasedEnum in
  // objectivec/Tests/unittest_objc.proto for a test Message to confirm this.

  // Some proto file set the default to the zero value, so make sure the value
  // isn't the zero case.
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32:
      return field->default_value_int32() != 0;
    case FieldDescriptor::CPPTYPE_UINT32:
      return field->default_value_uint32() != 0U;
    case FieldDescriptor::CPPTYPE_INT64:
      return field->default_value_int64() != 0LL;
    case FieldDescriptor::CPPTYPE_UINT64:
      return field->default_value_uint64() != 0ULL;
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return field->default_value_double() != 0.0;
    case FieldDescriptor::CPPTYPE_FLOAT:
      return field->default_value_float() != 0.0f;
    case FieldDescriptor::CPPTYPE_BOOL:
      return field->default_value_bool();
    case FieldDescriptor::CPPTYPE_STRING: {
      const std::string& default_string = field->default_value_string();
      return default_string.length() != 0;
    }
    case FieldDescriptor::CPPTYPE_ENUM:
      return field->default_value_enum()->number() != 0;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return false;
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return false;
}

}  // namespace

FieldGenerator* FieldGenerator::Make(const FieldDescriptor* field) {
  FieldGenerator* result = nullptr;
  if (field->is_repeated()) {
    switch (GetObjectiveCType(field)) {
      case OBJECTIVECTYPE_MESSAGE: {
        if (field->is_map()) {
          result = new MapFieldGenerator(field);
        } else {
          result = new RepeatedMessageFieldGenerator(field);
        }
        break;
      }
      case OBJECTIVECTYPE_ENUM:
        result = new RepeatedEnumFieldGenerator(field);
        break;
      default:
        result = new RepeatedPrimitiveFieldGenerator(field);
        break;
    }
  } else {
    switch (GetObjectiveCType(field)) {
      case OBJECTIVECTYPE_MESSAGE: {
        result = new MessageFieldGenerator(field);
        break;
      }
      case OBJECTIVECTYPE_ENUM:
        result = new EnumFieldGenerator(field);
        break;
      default:
        if (IsReferenceType(field)) {
          result = new PrimitiveObjFieldGenerator(field);
        } else {
          result = new PrimitiveFieldGenerator(field);
        }
        break;
    }
  }
  result->FinishInitialization();
  return result;
}

FieldGenerator::FieldGenerator(const FieldDescriptor* descriptor)
    : descriptor_(descriptor) {
  SetCommonFieldVariables(descriptor, &variables_);
}

void FieldGenerator::GenerateFieldNumberConstant(io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit("$field_number_name$ = $field_number$,\n");
}

void FieldGenerator::GenerateCFunctionDeclarations(io::Printer* printer) const {
  // Nothing
}

void FieldGenerator::GenerateCFunctionImplementations(
    io::Printer* printer) const {
  // Nothing
}

void FieldGenerator::DetermineForwardDeclarations(
    absl::btree_set<std::string>* fwd_decls,
    bool include_external_types) const {
  // Nothing
}

void FieldGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  // Nothing
}

void FieldGenerator::GenerateFieldDescription(io::Printer* printer,
                                              bool include_default) const {
  // Printed in the same order as the structure decl.
  auto vars = printer->WithVars(variables_);
  printer->Emit(
      {{"prefix", include_default ? ".core" : ""},
       {"maybe_default",
        [&] {
          if (include_default) {
            printer->Emit(".defaultValue.$default_name$ = $default$,\n");
          }
        }}},
      R"objc(
        {
          $maybe_default$,
          $prefix$.name = "$name$",
          $prefix$.dataTypeSpecific.$dataTypeSpecific_name$ = $dataTypeSpecific_value$,
          $prefix$.number = $field_number_name$,
          $prefix$.hasIndex = $has_index$,
          $prefix$.offset = $storage_offset_value$,$storage_offset_comment$
          $prefix$.flags = $fieldflags$,
          $prefix$.dataType = GPBDataType$field_type$,
        },
      )objc");
}

void FieldGenerator::SetRuntimeHasBit(int has_index) {
  variables_["has_index"] = absl::StrCat(has_index);
}

void FieldGenerator::SetNoHasBit() { variables_["has_index"] = "GPBNoHasBit"; }

int FieldGenerator::ExtraRuntimeHasBitsNeeded() const { return 0; }

void FieldGenerator::SetExtraRuntimeHasBitsBase(int index_base) {
  ABSL_LOG(FATAL)
      << "Error: should have overridden SetExtraRuntimeHasBitsBase().";
}

void FieldGenerator::SetOneofIndexBase(int index_base) {
  const OneofDescriptor* oneof = descriptor_->real_containing_oneof();
  if (oneof != nullptr) {
    int index = oneof->index() + index_base;
    // Flip the sign to mark it as a oneof.
    variables_["has_index"] = absl::StrCat(-index);
  }
}

bool FieldGenerator::WantsHasProperty() const {
  return descriptor_->has_presence() && !descriptor_->real_containing_oneof();
}

void FieldGenerator::FinishInitialization() {
  // If "property_type" wasn't set, make it "storage_type".
  if ((variables_.find("property_type") == variables_.end()) &&
      (variables_.find("storage_type") != variables_.end())) {
    variables_["property_type"] = variable("storage_type");
  }
}

SingleFieldGenerator::SingleFieldGenerator(const FieldDescriptor* descriptor)
    : FieldGenerator(descriptor) {
  // Nothing
}

void SingleFieldGenerator::GenerateFieldStorageDeclaration(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit("$storage_type$ $name$;\n");
}

void SingleFieldGenerator::GeneratePropertyDeclaration(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit(
      {{"comments", [&] { EmitCommentsString(printer, descriptor_); }}},
      R"objc(
        $comments$
        @property(nonatomic, readwrite) $property_type$ $name$$deprecated_attribute$;
      )objc");
  if (WantsHasProperty()) {
    printer->Emit(R"objc(
      @property(nonatomic, readwrite) BOOL has$capitalized_name$$deprecated_attribute$;
    )objc");
  }
  printer->Emit("\n");
}

void SingleFieldGenerator::GeneratePropertyImplementation(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  if (WantsHasProperty()) {
    printer->Emit("@dynamic has$capitalized_name$, $name$;\n");
  } else {
    printer->Emit("@dynamic $name$;\n");
  }
}

bool SingleFieldGenerator::RuntimeUsesHasBit() const {
  if (descriptor_->real_containing_oneof()) {
    // The oneof tracks what is set instead.
    return false;
  }
  return true;
}

ObjCObjFieldGenerator::ObjCObjFieldGenerator(const FieldDescriptor* descriptor)
    : SingleFieldGenerator(descriptor) {
  variables_["property_storage_attribute"] = "strong";
  if (IsRetainedName(variables_["name"])) {
    variables_["storage_attribute"] = " NS_RETURNS_NOT_RETAINED";
  }
}

void ObjCObjFieldGenerator::GenerateFieldStorageDeclaration(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit("$storage_type$ *$name$;\n");
}

void ObjCObjFieldGenerator::GeneratePropertyDeclaration(
    io::Printer* printer) const {
  // Differs from SingleFieldGenerator::GeneratePropertyDeclaration() in that
  // it uses pointers and deals with Objective-C's rules around storage name
  // conventions (init*, new*, etc.)

  auto vars = printer->WithVars(variables_);
  printer->Emit(
      {{"comments", [&] { EmitCommentsString(printer, descriptor_); }}},
      R"objc(
        $comments$
        @property(nonatomic, readwrite, $property_storage_attribute$, null_resettable) $property_type$ *$name$$storage_attribute$$deprecated_attribute$;
      )objc");
  if (WantsHasProperty()) {
    printer->Emit(R"objc(
        /** Test to see if @c $name$ has been set. */
        @property(nonatomic, readwrite) BOOL has$capitalized_name$$deprecated_attribute$;
    )objc");
  }
  if (IsInitName(variables_.find("name")->second)) {
    // If property name starts with init we need to annotate it to get past ARC.
    // http://stackoverflow.com/questions/18723226/how-do-i-annotate-an-objective-c-property-with-an-objc-method-family/18723227#18723227
    printer->Emit(R"objc(
      - ($property_type$ *)$name$ GPB_METHOD_FAMILY_NONE$deprecated_attribute$;
    )objc");
  }
  printer->Emit("\n");
}

RepeatedFieldGenerator::RepeatedFieldGenerator(
    const FieldDescriptor* descriptor)
    : ObjCObjFieldGenerator(descriptor) {}

void RepeatedFieldGenerator::FinishInitialization() {
  FieldGenerator::FinishInitialization();
  if (variables_.find("array_property_type") == variables_.end()) {
    variables_["array_property_type"] = variable("array_storage_type");
  }
}

void RepeatedFieldGenerator::GenerateFieldStorageDeclaration(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit("$array_storage_type$ *$name$;\n");
}

void RepeatedFieldGenerator::GeneratePropertyImplementation(
    io::Printer* printer) const {
  auto vars = printer->WithVars(variables_);
  printer->Emit("@dynamic $name$, $name$_Count;\n");
}

void RepeatedFieldGenerator::GeneratePropertyDeclaration(
    io::Printer* printer) const {
  // Repeated fields don't need the has* properties, but they do expose a
  // *Count (to check without autocreation).  So for the field property we need
  // the same logic as ObjCObjFieldGenerator::GeneratePropertyDeclaration() for
  // dealing with needing Objective-C's rules around storage name conventions
  // (init*, new*, etc.)

  auto vars = printer->WithVars(variables_);
  printer->Emit(
      {{"comments", [&] { EmitCommentsString(printer, descriptor_); }},
       {"array_comment", [&] { EmitArrayComment(printer); }}},
      R"objc(
        $comments$
        $array_comment$
        @property(nonatomic, readwrite, strong, null_resettable) $array_property_type$ *$name$$storage_attribute$$deprecated_attribute$;
        /** The number of items in @c $name$ without causing the container to be created. */
        @property(nonatomic, readonly) NSUInteger $name$_Count$deprecated_attribute$;
      )objc");
  if (IsInitName(variables_.find("name")->second)) {
    // If property name starts with init we need to annotate it to get past ARC.
    // http://stackoverflow.com/questions/18723226/how-do-i-annotate-an-objective-c-property-with-an-objc-method-family/18723227#18723227
    printer->Emit(R"objc(
      - ($array_property_type$ *)$name$ GPB_METHOD_FAMILY_NONE$deprecated_attribute$;
    )objc");
  }
  printer->Emit("\n");
}

bool RepeatedFieldGenerator::RuntimeUsesHasBit() const {
  return false;  // The array (or map/dict) having anything is what is used.
}

void RepeatedFieldGenerator::EmitArrayComment(io::Printer* printer) const {
  // Nothing for the default
}

FieldGeneratorMap::FieldGeneratorMap(const Descriptor* descriptor)
    : descriptor_(descriptor),
      field_generators_(descriptor->field_count()),
      extension_generators_(descriptor->extension_count()) {
  // Construct all the FieldGenerators.
  for (int i = 0; i < descriptor->field_count(); i++) {
    field_generators_[i].reset(FieldGenerator::Make(descriptor->field(i)));
  }
  for (int i = 0; i < descriptor->extension_count(); i++) {
    extension_generators_[i].reset(
        FieldGenerator::Make(descriptor->extension(i)));
  }
}

const FieldGenerator& FieldGeneratorMap::get(
    const FieldDescriptor* field) const {
  ABSL_CHECK_EQ(field->containing_type(), descriptor_);
  return *field_generators_[field->index()];
}

const FieldGenerator& FieldGeneratorMap::get_extension(int index) const {
  return *extension_generators_[index];
}

int FieldGeneratorMap::CalculateHasBits() {
  int total_bits = 0;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (field_generators_[i]->RuntimeUsesHasBit()) {
      field_generators_[i]->SetRuntimeHasBit(total_bits);
      ++total_bits;
    } else {
      field_generators_[i]->SetNoHasBit();
    }
    int extra_bits = field_generators_[i]->ExtraRuntimeHasBitsNeeded();
    if (extra_bits) {
      field_generators_[i]->SetExtraRuntimeHasBitsBase(total_bits);
      total_bits += extra_bits;
    }
  }
  return total_bits;
}

void FieldGeneratorMap::SetOneofIndexBase(int index_base) {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_[i]->SetOneofIndexBase(index_base);
  }
}

bool FieldGeneratorMap::DoesAnyFieldHaveNonZeroDefault() const {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (HasNonZeroDefaultValue(descriptor_->field(i))) {
      return true;
    }
  }

  return false;
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
