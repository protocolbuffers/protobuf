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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/compiler/cpp/field.h>

#include <cstdint>
#include <memory>
#include <string>

#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/substitute.h>
#include <google/protobuf/compiler/cpp/helpers.h>
#include <google/protobuf/compiler/cpp/primitive_field.h>
#include <google/protobuf/compiler/cpp/string_field.h>
#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/compiler/cpp/enum_field.h>
#include <google/protobuf/compiler/cpp/map_field.h>
#include <google/protobuf/compiler/cpp/message_field.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::WireFormat;

namespace {

void MaySetAnnotationVariable(const Options& options,
                              StringPiece annotation_name,
                              StringPiece substitute_template_prefix,
                              StringPiece prepared_template,
                              int field_index, StringPiece access_type,
                              std::map<std::string, std::string>* variables) {
  if (options.field_listener_options.forbidden_field_listener_events.count(
          std::string(annotation_name)))
    return;
  (*variables)[StrCat("annotate_", annotation_name)] = strings::Substitute(
      StrCat(substitute_template_prefix, prepared_template, ");\n"),
      field_index, access_type);
}

std::string GenerateTemplateForOneofString(const FieldDescriptor* descriptor,
                                           StringPiece proto_ns,
                                           StringPiece field_member) {
  std::string field_name = google::protobuf::compiler::cpp::FieldName(descriptor);
  std::string field_pointer =
      descriptor->options().ctype() == google::protobuf::FieldOptions::STRING
          ? "$0.UnsafeGetPointer()"
          : "$0";

  if (descriptor->default_value_string().empty()) {
    return strings::Substitute(StrCat("_internal_has_", field_name, "() ? ",
                                         field_pointer, ": nullptr"),
                            field_member);
  }

  if (descriptor->options().ctype() == google::protobuf::FieldOptions::STRING_PIECE) {
    return strings::Substitute(StrCat("_internal_has_", field_name, "() ? ",
                                         field_pointer, ": nullptr"),
                            field_member);
  }

  std::string default_value_pointer =
      descriptor->options().ctype() == google::protobuf::FieldOptions::STRING
          ? "&$1.get()"
          : "&$1";
  return strings::Substitute(
      StrCat("_internal_has_", field_name, "() ? ", field_pointer, " : ",
                   default_value_pointer),
      field_member, MakeDefaultFieldName(descriptor));
}

std::string GenerateTemplateForSingleString(const FieldDescriptor* descriptor,
                                            StringPiece field_member) {
  if (descriptor->default_value_string().empty()) {
    return StrCat("&", field_member);
  }

  if (descriptor->options().ctype() == google::protobuf::FieldOptions::STRING) {
    return strings::Substitute(
        "$0.IsDefault() ? &$1.get() : $0.UnsafeGetPointer()", field_member,
        MakeDefaultFieldName(descriptor));
  }

  return StrCat("&", field_member);
}

}  // namespace

void AddAccessorAnnotations(const FieldDescriptor* descriptor,
                            const Options& options,
                            std::map<std::string, std::string>* variables) {
  // Can be expanded to include more specific calls, for example, for arena or
  // clear calls.
  static constexpr const char* kAccessorsAnnotations[] = {
      "annotate_add",     "annotate_get",         "annotate_has",
      "annotate_list",    "annotate_mutable",     "annotate_mutable_list",
      "annotate_release", "annotate_set",         "annotate_size",
      "annotate_clear",   "annotate_add_mutable",
  };
  for (size_t i = 0; i < GOOGLE_ARRAYSIZE(kAccessorsAnnotations); ++i) {
    (*variables)[kAccessorsAnnotations[i]] = "";
  }
  if (options.annotate_accessor) {
    for (size_t i = 0; i < GOOGLE_ARRAYSIZE(kAccessorsAnnotations); ++i) {
      (*variables)[kAccessorsAnnotations[i]] = StrCat(
          "  ", FieldName(descriptor), "_AccessedNoStrip = true;\n");
    }
  }
  if (!options.field_listener_options.inject_field_listener_events) {
    return;
  }
  if (descriptor->file()->options().optimize_for() ==
      google::protobuf::FileOptions::LITE_RUNTIME) {
    return;
  }
  std::string field_member = (*variables)["field"];
  const google::protobuf::OneofDescriptor* oneof_member =
      descriptor->real_containing_oneof();
  const std::string proto_ns = (*variables)["proto_ns"];
  const std::string substitute_template_prefix =
      StrCat("  ", (*variables)["tracker"], ".$1<$0>(this, ");
  std::string prepared_template;

  // Flat template is needed if the prepared one is introspecting the values
  // inside the returned values, for example, for repeated fields and maps.
  std::string prepared_flat_template;
  std::string prepared_add_template;
  // TODO(b/190614678): Support fields with type Message or Map.
  if (descriptor->is_repeated() && !descriptor->is_map()) {
    if (descriptor->type() != FieldDescriptor::TYPE_MESSAGE &&
        descriptor->type() != FieldDescriptor::TYPE_GROUP) {
      prepared_template = strings::Substitute("&$0.Get(index)", field_member);
      prepared_add_template =
          strings::Substitute("&$0.Get($0.size() - 1)", field_member);
    } else {
      prepared_template = "nullptr";
      prepared_add_template = "nullptr";
    }
  } else if (descriptor->is_map()) {
    prepared_template = "nullptr";
  } else if (descriptor->type() == FieldDescriptor::TYPE_MESSAGE &&
             !IsExplicitLazy(descriptor)) {
    prepared_template = "nullptr";
  } else if (descriptor->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
    if (oneof_member) {
      prepared_template = GenerateTemplateForOneofString(
          descriptor, (*variables)["proto_ns"], field_member);
    } else {
      prepared_template =
          GenerateTemplateForSingleString(descriptor, field_member);
    }
  } else {
    prepared_template = StrCat("&", field_member);
  }
  if (descriptor->is_repeated() && !descriptor->is_map() &&
      descriptor->type() != FieldDescriptor::TYPE_MESSAGE &&
      descriptor->type() != FieldDescriptor::TYPE_GROUP) {
    prepared_flat_template = StrCat("&", field_member);
  } else {
    prepared_flat_template = prepared_template;
  }

  MaySetAnnotationVariable(options, "get", substitute_template_prefix,
                           prepared_template, descriptor->index(), "OnGet",
                           variables);
  MaySetAnnotationVariable(options, "set", substitute_template_prefix,
                           prepared_template, descriptor->index(), "OnSet",
                           variables);
  MaySetAnnotationVariable(options, "has", substitute_template_prefix,
                           prepared_template, descriptor->index(), "OnHas",
                           variables);
  MaySetAnnotationVariable(options, "mutable", substitute_template_prefix,
                           prepared_template, descriptor->index(), "OnMutable",
                           variables);
  MaySetAnnotationVariable(options, "release", substitute_template_prefix,
                           prepared_template, descriptor->index(), "OnRelease",
                           variables);
  MaySetAnnotationVariable(options, "clear", substitute_template_prefix,
                           prepared_flat_template, descriptor->index(),
                           "OnClear", variables);
  MaySetAnnotationVariable(options, "size", substitute_template_prefix,
                           prepared_flat_template, descriptor->index(),
                           "OnSize", variables);
  MaySetAnnotationVariable(options, "list", substitute_template_prefix,
                           prepared_flat_template, descriptor->index(),
                           "OnList", variables);
  MaySetAnnotationVariable(options, "mutable_list", substitute_template_prefix,
                           prepared_flat_template, descriptor->index(),
                           "OnMutableList", variables);
  MaySetAnnotationVariable(options, "add", substitute_template_prefix,
                           prepared_add_template, descriptor->index(), "OnAdd",
                           variables);
  MaySetAnnotationVariable(options, "add_mutable", substitute_template_prefix,
                           prepared_add_template, descriptor->index(),
                           "OnAddMutable", variables);
}

void SetCommonFieldVariables(const FieldDescriptor* descriptor,
                             std::map<std::string, std::string>* variables,
                             const Options& options) {
  SetCommonVars(options, variables);
  SetCommonMessageDataVariables(descriptor->containing_type(), variables);

  (*variables)["ns"] = Namespace(descriptor, options);
  (*variables)["name"] = FieldName(descriptor);
  (*variables)["index"] = StrCat(descriptor->index());
  (*variables)["number"] = StrCat(descriptor->number());
  (*variables)["classname"] = ClassName(FieldScope(descriptor), false);
  (*variables)["declared_type"] = DeclaredTypeMethodName(descriptor->type());
  bool split = ShouldSplit(descriptor, options);
  (*variables)["field"] = FieldMemberName(descriptor, split);

  (*variables)["tag_size"] = StrCat(
      WireFormat::TagSize(descriptor->number(), descriptor->type()));
  (*variables)["deprecated_attr"] = DeprecatedAttribute(options, descriptor);

  (*variables)["set_hasbit"] = "";
  (*variables)["clear_hasbit"] = "";
  (*variables)["maybe_prepare_split_message"] =
      split ? "  PrepareSplitMessageForWrite();\n" : "";

  AddAccessorAnnotations(descriptor, options, variables);

  // These variables are placeholders to pick out the beginning and ends of
  // identifiers for annotations (when doing so with existing variables would
  // be ambiguous or impossible). They should never be set to anything but the
  // empty string.
  (*variables)["{"] = "";
  (*variables)["}"] = "";
}

void FieldGenerator::SetHasBitIndex(int32_t has_bit_index) {
  if (!HasHasbit(descriptor_)) {
    GOOGLE_CHECK_EQ(has_bit_index, -1);
    return;
  }
  variables_["set_hasbit"] = StrCat(
      variables_["has_bits"], "[", has_bit_index / 32, "] |= 0x",
      strings::Hex(1u << (has_bit_index % 32), strings::ZERO_PAD_8), "u;");
  variables_["clear_hasbit"] = StrCat(
      variables_["has_bits"], "[", has_bit_index / 32, "] &= ~0x",
      strings::Hex(1u << (has_bit_index % 32), strings::ZERO_PAD_8), "u;");
}

void FieldGenerator::SetInlinedStringIndex(int32_t inlined_string_index) {
  if (!IsStringInlined(descriptor_, options_)) {
    GOOGLE_CHECK_EQ(inlined_string_index, -1);
    return;
  }
  // The first bit is the tracking bit for on demand registering ArenaDtor.
  GOOGLE_CHECK_GT(inlined_string_index, 0)
      << "_inlined_string_donated_'s bit 0 is reserved for arena dtor tracking";
  variables_["inlined_string_donated"] = StrCat(
      "(", variables_["inlined_string_donated_array"], "[",
      inlined_string_index / 32, "] & 0x",
      strings::Hex(1u << (inlined_string_index % 32), strings::ZERO_PAD_8),
      "u) != 0;");
  variables_["donating_states_word"] =
      StrCat(variables_["inlined_string_donated_array"], "[",
                   inlined_string_index / 32, "]");
  variables_["mask_for_undonate"] = StrCat(
      "~0x", strings::Hex(1u << (inlined_string_index % 32), strings::ZERO_PAD_8),
      "u");
}

void FieldGenerator::GenerateAggregateInitializer(io::Printer* printer) const {
  Formatter format(printer, variables_);
  if (ShouldSplit(descriptor_, options_)) {
    format("decltype(Impl_::Split::$name$_){arena}");
    return;
  }
  format("decltype($field$){arena}");
}

void FieldGenerator::GenerateConstexprAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("/*decltype($field$)*/{}");
}

void FieldGenerator::GenerateCopyAggregateInitializer(
    io::Printer* printer) const {
  Formatter format(printer, variables_);
  format("decltype($field$){from.$field$}");
}

void FieldGenerator::GenerateCopyConstructorCode(io::Printer* printer) const {
  if (ShouldSplit(descriptor_, options_)) {
    // There is no copy constructor for the `Split` struct, so we need to copy
    // the value here.
    Formatter format(printer, variables_);
    format("$field$ = from.$field$;\n");
  }
}

void SetCommonOneofFieldVariables(
    const FieldDescriptor* descriptor,
    std::map<std::string, std::string>* variables) {
  const std::string prefix = descriptor->containing_oneof()->name() + "_.";
  (*variables)["oneof_name"] = descriptor->containing_oneof()->name();
}

FieldGenerator::~FieldGenerator() {}

FieldGeneratorMap::FieldGeneratorMap(const Descriptor* descriptor,
                                     const Options& options,
                                     MessageSCCAnalyzer* scc_analyzer)
    : descriptor_(descriptor), field_generators_(descriptor->field_count()) {
  // Construct all the FieldGenerators.
  for (int i = 0; i < descriptor->field_count(); i++) {
    field_generators_[i].reset(
        MakeGenerator(descriptor->field(i), options, scc_analyzer));
  }
}

FieldGenerator* FieldGeneratorMap::MakeGoogleInternalGenerator(
    const FieldDescriptor* field, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {

  return nullptr;
}

FieldGenerator* FieldGeneratorMap::MakeGenerator(
    const FieldDescriptor* field, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  FieldGenerator* generator =
      MakeGoogleInternalGenerator(field, options, scc_analyzer);
  if (generator) {
    return generator;
  }

  if (field->is_repeated()) {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        if (field->is_map()) {
          return new MapFieldGenerator(field, options, scc_analyzer);
        } else {
          return new RepeatedMessageFieldGenerator(field, options,
                                                   scc_analyzer);
        }
      case FieldDescriptor::CPPTYPE_STRING:
        return new RepeatedStringFieldGenerator(field, options);
      case FieldDescriptor::CPPTYPE_ENUM:
        return new RepeatedEnumFieldGenerator(field, options);
      default:
        return new RepeatedPrimitiveFieldGenerator(field, options);
    }
  } else if (field->real_containing_oneof()) {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        return new MessageOneofFieldGenerator(field, options, scc_analyzer);
      case FieldDescriptor::CPPTYPE_STRING:
        return new StringOneofFieldGenerator(field, options);
      case FieldDescriptor::CPPTYPE_ENUM:
        return new EnumOneofFieldGenerator(field, options);
      default:
        return new PrimitiveOneofFieldGenerator(field, options);
    }
  } else {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        return new MessageFieldGenerator(field, options, scc_analyzer);
      case FieldDescriptor::CPPTYPE_STRING:
        return new StringFieldGenerator(field, options);
      case FieldDescriptor::CPPTYPE_ENUM:
        return new EnumFieldGenerator(field, options);
      default:
        return new PrimitiveFieldGenerator(field, options);
    }
  }
}

FieldGeneratorMap::~FieldGeneratorMap() {}

const FieldGenerator& FieldGeneratorMap::get(
    const FieldDescriptor* field) const {
  GOOGLE_CHECK_EQ(field->containing_type(), descriptor_);
  return *field_generators_[field->index()];
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
