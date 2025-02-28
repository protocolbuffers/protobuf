// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/field.h"

#include <cstddef>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/objectivec/enum_field.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/map_field.h"
#include "google/protobuf/compiler/objectivec/message_field.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/compiler/objectivec/primitive_field.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {
using Sub = ::google::protobuf::io::Printer::Sub;

void SetCommonFieldVariables(const FieldDescriptor* descriptor,
                             SubstitutionMap& variables) {
  std::string camel_case_name = FieldName(descriptor);
  std::string raw_field_name(internal::cpp::IsGroupLike(*descriptor)
                                 ? descriptor->message_type()->name()
                                 : descriptor->name());
  // The logic here has to match -[GGPBFieldDescriptor textFormatName].
  const std::string un_camel_case_name(
      UnCamelCaseFieldName(camel_case_name, descriptor));
  const bool needs_custom_name = (raw_field_name != un_camel_case_name);

  const std::string classname = ClassName(descriptor->containing_type());
  variables.Set(Sub("classname", classname).AnnotatedAs(descriptor));
  variables.Set(Sub("name", camel_case_name).AnnotatedAs(descriptor));

  const std::string capitalized_name = FieldNameCapitalized(descriptor);
  variables.Set(
      Sub("hazzer_name", "has" + capitalized_name).AnnotatedAs(descriptor));
  variables.Set(
      Sub("capitalized_name", capitalized_name).AnnotatedAs(descriptor));
  variables.Set("raw_field_name", raw_field_name);
  variables.Set(Sub("field_number_name",
                    absl::StrCat(classname, "_FieldNumber_", capitalized_name))
                    .AnnotatedAs(descriptor));
  variables.Set("field_number", absl::StrCat(descriptor->number()));
  variables.Set(
      "property_type",
      FieldObjCType(descriptor,
                    static_cast<FieldObjCTypeOptions>(
                        kFieldObjCTypeOptions_IncludeSpaceAfterBasicTypes |
                        kFieldObjCTypeOptions_IncludeSpaceBeforeStar)));
  variables.Set(
      "storage_type",
      FieldObjCType(descriptor,
                    static_cast<FieldObjCTypeOptions>(
                        kFieldObjCTypeOptions_IncludeSpaceAfterBasicTypes |
                        kFieldObjCTypeOptions_OmitLightweightGenerics |
                        kFieldObjCTypeOptions_IncludeSpaceBeforeStar)));
  variables.Set("field_type", GetCapitalizedType(descriptor));
  variables.Set("deprecated_attribute",
                GetOptionalDeprecatedAttribute(descriptor));
  std::vector<std::string> field_flags;
  if (descriptor->is_repeated()) field_flags.push_back("GPBFieldRepeated");
  if (descriptor->is_required()) field_flags.push_back("GPBFieldRequired");
  if (descriptor->is_optional()) field_flags.push_back("GPBFieldOptional");
  if (descriptor->is_packed()) field_flags.push_back("GPBFieldPacked");

  // ObjC custom flags.
  if (descriptor->has_default_value())
    field_flags.push_back("GPBFieldHasDefaultValue");
  if (needs_custom_name) field_flags.push_back("GPBFieldTextFormatNameCustom");
  // It will clear on a zero value if...
  //  - not repeated/map
  //  - doesn't have presence
  bool clear_on_zero =
      (!descriptor->is_repeated() && !descriptor->has_presence());
  if (clear_on_zero) {
    field_flags.push_back("GPBFieldClearHasIvarOnZero");
  }

  variables.Set("fieldflags", BuildFlagsString(FLAGTYPE_FIELD, field_flags));

  variables.Set("default", DefaultValue(descriptor));
  variables.Set("default_name", GPBGenericValueFieldName(descriptor));

  variables.Set("dataTypeSpecific_name", "clazz");
  variables.Set("dataTypeSpecific_value", "Nil");

  variables.Set("storage_offset_value",
                absl::StrCat("(uint32_t)offsetof(", classname, "__storage_, ",
                             camel_case_name, ")"));
  variables.Set("storage_offset_comment", "");

  // Clear some common things so they can be set just when needed.
  variables.Set("storage_attribute", "");
}

bool HasNonZeroDefaultValue(const FieldDescriptor* field) {
  // Repeated fields don't have defaults.
  if (field->is_repeated()) {
    return false;
  }

  // Some proto files set the default to the zero value, so make sure the value
  // isn't the zero case instead of relying on has_default_value() to tell when
  // non zero.
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
      return !field->default_value_string().empty();
    }
    case FieldDescriptor::CPPTYPE_ENUM:
      // The default value for an enum field is the first enum value, so there
      // even more reason we can't use has_default_value() for checking for
      // zero.
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

FieldGenerator* FieldGenerator::Make(
    const FieldDescriptor* field, const GenerationOptions& generation_options) {
  if (field->is_repeated()) {
    switch (GetObjectiveCType(field)) {
      case OBJECTIVECTYPE_MESSAGE: {
        if (field->is_map()) {
          return new MapFieldGenerator(field, generation_options);
        } else {
          return new RepeatedMessageFieldGenerator(field, generation_options);
        }
      }
      case OBJECTIVECTYPE_ENUM:
        return new RepeatedEnumFieldGenerator(field, generation_options);
      default:
        return new RepeatedPrimitiveFieldGenerator(field, generation_options);
    }
  }

  switch (GetObjectiveCType(field)) {
    case OBJECTIVECTYPE_MESSAGE: {
      return new MessageFieldGenerator(field, generation_options);
    }
    case OBJECTIVECTYPE_ENUM:
      return new EnumFieldGenerator(field, generation_options);
    default:
      if (IsReferenceType(field)) {
        return new PrimitiveObjFieldGenerator(field, generation_options);
      } else {
        return new PrimitiveFieldGenerator(field, generation_options);
      }
  }
}

FieldGenerator::FieldGenerator(const FieldDescriptor* descriptor,
                               const GenerationOptions& generation_options)
    : descriptor_(descriptor), generation_options_(generation_options) {
  SetCommonFieldVariables(descriptor, variables_);
}

void FieldGenerator::GenerateFieldNumberConstant(io::Printer* printer) const {
  auto vars = variables_.Install(printer);
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

void FieldGenerator::DetermineNeededFiles(
    absl::flat_hash_set<const FileDescriptor*>* deps) const {
  // Nothing
}

void FieldGenerator::GenerateFieldDescription(io::Printer* printer,
                                              bool include_default) const {
  // Printed in the same order as the structure decl.
  auto vars = variables_.Install(printer);
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
  variables_.Set("has_index", has_index);
}

void FieldGenerator::SetNoHasBit() {
  variables_.Set("has_index", "GPBNoHasBit");
}

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
    variables_.Set("has_index", -index);
  }
}

bool FieldGenerator::WantsHasProperty() const {
  return descriptor_->has_presence() && !descriptor_->real_containing_oneof();
}

SingleFieldGenerator::SingleFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : FieldGenerator(descriptor, generation_options) {
  // Nothing
}

void SingleFieldGenerator::GenerateFieldStorageDeclaration(
    io::Printer* printer) const {
  auto vars = variables_.Install(printer);
  printer->Emit("$storage_type$$name$;\n");
}

void SingleFieldGenerator::GeneratePropertyDeclaration(
    io::Printer* printer) const {
  auto vars = variables_.Install(printer);
  printer->Emit({{"comments",
                  [&] {
                    EmitCommentsString(printer, generation_options_,
                                       descriptor_);
                  }}},
                R"objc(
                  $comments$
                  @property(nonatomic, readwrite) $property_type$$name$$ deprecated_attribute$;
                )objc");
  if (WantsHasProperty()) {
    printer->Emit(R"objc(
      @property(nonatomic, readwrite) BOOL $hazzer_name$$ deprecated_attribute$;
    )objc");
  }
  printer->Emit("\n");
}

void SingleFieldGenerator::GeneratePropertyImplementation(
    io::Printer* printer) const {
  auto vars = variables_.Install(printer);
  if (WantsHasProperty()) {
    printer->Emit("@dynamic $hazzer_name$, $name$;\n");
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

ObjCObjFieldGenerator::ObjCObjFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : SingleFieldGenerator(descriptor, generation_options) {
  variables_.Set("property_storage_attribute", "strong");
  if (IsRetainedName(variable("name"))) {
    variables_.Set("storage_attribute", " NS_RETURNS_NOT_RETAINED");
  }
}

void ObjCObjFieldGenerator::GenerateFieldStorageDeclaration(
    io::Printer* printer) const {
  auto vars = variables_.Install(printer);
  printer->Emit("$storage_type$$name$;\n");
}

void ObjCObjFieldGenerator::GeneratePropertyDeclaration(
    io::Printer* printer) const {
  // Differs from SingleFieldGenerator::GeneratePropertyDeclaration() in that
  // it uses pointers and deals with Objective-C's rules around storage name
  // conventions (init*, new*, etc.)

  auto vars = variables_.Install(printer);
  printer->Emit({{"comments",
                  [&] {
                    EmitCommentsString(printer, generation_options_,
                                       descriptor_);
                  }}},
                R"objc(
                  $comments$
                  @property(nonatomic, readwrite, $property_storage_attribute$, null_resettable) $property_type$$name$$storage_attribute$$ deprecated_attribute$;
                )objc");
  if (WantsHasProperty()) {
    printer->Emit(R"objc(
        /** Test to see if @c $name$ has been set. */
        @property(nonatomic, readwrite) BOOL $hazzer_name$$ deprecated_attribute$;
    )objc");
  }
  if (IsInitName(variable("name"))) {
    // If property name starts with init we need to annotate it to get past ARC.
    // http://stackoverflow.com/questions/18723226/how-do-i-annotate-an-objective-c-property-with-an-objc-method-family/18723227#18723227
    printer->Emit(R"objc(
      - ($property_type$)$name$ GPB_METHOD_FAMILY_NONE$ deprecated_attribute$;
    )objc");
  }
  printer->Emit("\n");
}

RepeatedFieldGenerator::RepeatedFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : ObjCObjFieldGenerator(descriptor, generation_options) {}

void RepeatedFieldGenerator::GenerateFieldStorageDeclaration(
    io::Printer* printer) const {
  auto vars = variables_.Install(printer);
  printer->Emit("$storage_type$$name$;\n");
}

void RepeatedFieldGenerator::GeneratePropertyImplementation(
    io::Printer* printer) const {
  auto vars = variables_.Install(printer);
  printer->Emit("@dynamic $name$, $name$_Count;\n");
}

void RepeatedFieldGenerator::GeneratePropertyDeclaration(
    io::Printer* printer) const {
  // Repeated fields don't need the has* properties, but they do expose a
  // *Count (to check without autocreation).  So for the field property we need
  // the same logic as ObjCObjFieldGenerator::GeneratePropertyDeclaration() for
  // dealing with needing Objective-C's rules around storage name conventions
  // (init*, new*, etc.)

  auto vars = variables_.Install(printer);
  printer->Emit(
      {{"comments",
        [&] { EmitCommentsString(printer, generation_options_, descriptor_); }},
       {"array_comment", [&] { EmitArrayComment(printer); }}},
      R"objc(
        $comments$
        $array_comment$
        @property(nonatomic, readwrite, strong, null_resettable) $property_type$$name$$storage_attribute$$ deprecated_attribute$;
        /** The number of items in @c $name$ without causing the container to be created. */
        @property(nonatomic, readonly) NSUInteger $name$_Count$ deprecated_attribute$;
      )objc");
  if (IsInitName(variable("name"))) {
    // If property name starts with init we need to annotate it to get past ARC.
    // http://stackoverflow.com/questions/18723226/how-do-i-annotate-an-objective-c-property-with-an-objc-method-family/18723227#18723227
    printer->Emit(R"objc(
      - ($property_type$)$name$ GPB_METHOD_FAMILY_NONE$ deprecated_attribute$;
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

FieldGeneratorMap::FieldGeneratorMap(
    const Descriptor* descriptor, const GenerationOptions& generation_options)
    : descriptor_(descriptor),
      field_generators_(static_cast<size_t>(descriptor->field_count())) {
  for (int i = 0; i < descriptor->field_count(); i++) {
    field_generators_[(size_t)i].reset(
        FieldGenerator::Make(descriptor->field(i), generation_options));
  }
}

const FieldGenerator& FieldGeneratorMap::get(
    const FieldDescriptor* field) const {
  ABSL_CHECK_EQ(field->containing_type(), descriptor_);
  return *field_generators_[(size_t)field->index()];
}

int FieldGeneratorMap::CalculateHasBits() {
  int total_bits = 0;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (field_generators_[(size_t)i]->RuntimeUsesHasBit()) {
      field_generators_[(size_t)i]->SetRuntimeHasBit(total_bits);
      ++total_bits;
    } else {
      field_generators_[(size_t)i]->SetNoHasBit();
    }
    int extra_bits = field_generators_[(size_t)i]->ExtraRuntimeHasBitsNeeded();
    if (extra_bits) {
      field_generators_[(size_t)i]->SetExtraRuntimeHasBitsBase(total_bits);
      total_bits += extra_bits;
    }
  }
  return total_bits;
}

void FieldGeneratorMap::SetOneofIndexBase(int index_base) {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_[(size_t)i]->SetOneofIndexBase(index_base);
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
