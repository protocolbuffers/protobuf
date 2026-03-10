// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/enum_field.h"

#include <string>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/objectivec/field.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

void SetEnumVariables(const FieldDescriptor* descriptor,
                      const GenerationOptions& generation_options,
                      SubstitutionMap& variables) {
  const std::string type = EnumName(descriptor->enum_type());
  const std::string enum_desc_func = absl::StrCat(type, "_EnumDescriptor");
  variables.Set("enum_name", type);
  // When using fwd decls, for non repeated fields, if it was defined in a
  // different file, the property decls need to use "enum NAME" rather than just
  // "NAME" to support the forward declaration of the enums.
  if (generation_options.headers_use_forward_declarations &&
      !descriptor->is_repeated() &&
      !IsProtobufLibraryBundledProtoFile(descriptor->enum_type()->file()) &&
      (descriptor->file() != descriptor->enum_type()->file())) {
    variables.Set("property_type", absl::StrCat("enum ", type, " "));
  }
  variables.Set("enum_verifier", absl::StrCat(type, "_IsValidValue"));
  variables.Set("enum_desc_func", enum_desc_func);

  variables.Set("dataTypeSpecific_name", "enumDescFunc");
  variables.Set("dataTypeSpecific_value", enum_desc_func);

  const Descriptor* msg_descriptor = descriptor->containing_type();
  variables.Set("owning_message_class", ClassName(msg_descriptor));
}
}  // namespace

EnumFieldGenerator::EnumFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : SingleFieldGenerator(descriptor, generation_options) {
  SetEnumVariables(descriptor, generation_options, variables_);
}

void EnumFieldGenerator::GenerateCFunctionDeclarations(
    io::Printer* printer) const {
  if (descriptor_->enum_type()->is_closed()) {
    return;
  }

  auto vars = variables_.Install(printer);
  printer->Emit(R"objc(
    /**
     * Fetches the raw value of a @c $owning_message_class$'s @c $name$ property, even
     * if the value was not defined by the enum at the time the code was generated.
     **/
    int32_t $owning_message_class$_$capitalized_name$_RawValue($owning_message_class$ *message);
    /**
     * Sets the raw value of an @c $owning_message_class$'s @c $name$ property, allowing
     * it to be set to a value that was not defined by the enum at the time the code
     * was generated.
     **/
    void Set$owning_message_class$_$capitalized_name$_RawValue($owning_message_class$ *message, int32_t value);
  )objc");
  printer->Emit("\n");
}

void EnumFieldGenerator::GenerateCFunctionImplementations(
    io::Printer* printer) const {
  if (descriptor_->enum_type()->is_closed()) {
    return;
  }

  auto vars = variables_.Install(printer);
  printer->Emit(R"objc(
    int32_t $owning_message_class$_$capitalized_name$_RawValue($owning_message_class$ *message) {
      GPBDescriptor *descriptor = [$owning_message_class$ descriptor];
      GPBFieldDescriptor *field = [descriptor fieldWithNumber:$field_number_name$];
      return GPBGetMessageRawEnumField(message, field);
    }

    void Set$owning_message_class$_$capitalized_name$_RawValue($owning_message_class$ *message, int32_t value) {
      GPBDescriptor *descriptor = [$owning_message_class$ descriptor];
      GPBFieldDescriptor *field = [descriptor fieldWithNumber:$field_number_name$];
      GPBSetMessageRawEnumField(message, field, value);
    }
  )objc");
  printer->Emit("\n");
}

void EnumFieldGenerator::DetermineForwardDeclarations(
    absl::btree_set<std::string>* fwd_decls,
    bool include_external_types) const {
  SingleFieldGenerator::DetermineForwardDeclarations(fwd_decls,
                                                     include_external_types);
  // If it is an enum defined in a different file (and not a WKT), then we'll
  // need a forward declaration for it.  When it is in our file, all the enums
  // are output before the message, so it will be declared before it is needed.
  if (include_external_types &&
      descriptor_->file() != descriptor_->enum_type()->file() &&
      !IsProtobufLibraryBundledProtoFile(descriptor_->enum_type()->file())) {
    const std::string& name = variable("enum_name");
    fwd_decls->insert(absl::StrCat("GPB_ENUM_FWD_DECLARE(", name, ");"));
  }
}

void EnumFieldGenerator::DetermineNeededFiles(
    absl::flat_hash_set<const FileDescriptor*>* deps) const {
  if (descriptor_->file() != descriptor_->enum_type()->file()) {
    deps->insert(descriptor_->enum_type()->file());
  }
}

RepeatedEnumFieldGenerator::RepeatedEnumFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : RepeatedFieldGenerator(descriptor, generation_options) {
  SetEnumVariables(descriptor, generation_options, variables_);
}

void RepeatedEnumFieldGenerator::EmitArrayComment(io::Printer* printer) const {
  auto vars = variables_.Install(printer);
  printer->Emit(R"objc(
    // |$name$| contains |$enum_name$|
  )objc");
}

// NOTE: RepeatedEnumFieldGenerator::DetermineForwardDeclarations isn't needed
// because `GPBEnumArray` isn't generic (like `NSArray` would be for messages)
// and thus doesn't reference the type in the header.

void RepeatedEnumFieldGenerator::DetermineNeededFiles(
    absl::flat_hash_set<const FileDescriptor*>* deps) const {
  if (descriptor_->file() != descriptor_->enum_type()->file()) {
    deps->insert(descriptor_->enum_type()->file());
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
