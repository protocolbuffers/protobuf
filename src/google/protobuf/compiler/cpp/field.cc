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

#include "google/protobuf/compiler/cpp/field.h"

#include <cstdint>
#include <memory>
#include <string>

#include "google/protobuf/descriptor.h"
#include "absl/container/flat_hash_map.h"
#include "google/protobuf/stubs/logging.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/wire_format.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
using internal::WireFormat;

absl::flat_hash_map<absl::string_view, std::string> FieldVars(
    const FieldDescriptor* desc, const Options& opts) {
  bool split = ShouldSplit(desc, opts);
  absl::flat_hash_map<absl::string_view, std::string> vars = {
      {"ns", Namespace(desc, opts)},
      {"name", FieldName(desc)},
      {"index", absl::StrCat(desc->index())},
      {"number", absl::StrCat(desc->number())},
      {"classname", ClassName(FieldScope(desc), false)},
      {"declared_type", DeclaredTypeMethodName(desc->type())},
      {"field", FieldMemberName(desc, split)},
      {"tag_size",
       absl::StrCat(WireFormat::TagSize(desc->number(), desc->type()))},
      {"deprecated_attr", DeprecatedAttribute(opts, desc)},
      {"set_hasbit", ""},
      {"clear_hasbit", ""},
      {"maybe_prepare_split_message",
       split ? "PrepareSplitMessageForWrite();" : ""},

      // These variables are placeholders to pick out the beginning and ends of
      // identifiers for annotations (when doing so with existing variables
      // would be ambiguous or impossible). They should never be set to anything
      // but the empty string.
      {"{", ""},
      {"}", ""},
  };

  return vars;
}

void SetCommonFieldVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    const Options& options) {
  SetCommonMessageDataVariables(descriptor->containing_type(), variables);

  for (auto& pair : FieldVars(descriptor, options)) {
    variables->emplace(pair);
  }
}

absl::flat_hash_map<absl::string_view, std::string> OneofFieldVars(
    const FieldDescriptor* descriptor) {
  if (descriptor->containing_oneof() == nullptr) {
    return {};
  }
  std::string oneof_name = descriptor->containing_oneof()->name();
  std::string field_name = UnderscoresToCamelCase(descriptor->name(), true);

  return {
      {"oneof_name", oneof_name},
      {"field_name", field_name},
      {"oneof_index", absl::StrCat(descriptor->containing_oneof()->index())},
      {"has_field",
       absl::StrFormat("%s_case() == k%s", oneof_name, field_name)},
      {"not_has_field",
       absl::StrFormat("%s_case() != k%s", oneof_name, field_name)},
  };
}

void SetCommonOneofFieldVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  for (auto& pair : OneofFieldVars(descriptor)) {
    variables->emplace(pair);
  }
}

void FieldGenerator::SetHasBitIndex(int32_t has_bit_index) {
  if (!internal::cpp::HasHasbit(descriptor_) || has_bit_index < 0) {
    GOOGLE_ABSL_CHECK_EQ(has_bit_index, -1);
    return;
  }
  int32_t index = has_bit_index / 32;
  std::string mask =
      absl::StrCat(absl::Hex(1u << (has_bit_index % 32), absl::kZeroPad8));
  const std::string& has_bits = variables_["has_bits"];

  variables_["has_hasbit"] =
      absl::StrFormat("%s[%d] & 0x%su", has_bits, index, mask);
  variables_["set_hasbit"] =
      absl::StrFormat("%s[%d] |= 0x%su;", has_bits, index, mask);
  variables_["clear_hasbit"] =
      absl::StrFormat("%s[%d] &= ~0x%su;", has_bits, index, mask);
}

void FieldGenerator::SetInlinedStringIndex(int32_t inlined_string_index) {
  if (!IsStringInlined(descriptor_, options_)) {
    GOOGLE_ABSL_CHECK_EQ(inlined_string_index, -1);
    return;
  }
  // The first bit is the tracking bit for on demand registering ArenaDtor.
  GOOGLE_ABSL_CHECK_GT(inlined_string_index, 0)
      << "_inlined_string_donated_'s bit 0 is reserved for arena dtor tracking";
  variables_["inlined_string_donated"] = absl::StrCat(
      "(", variables_["inlined_string_donated_array"], "[",
      inlined_string_index / 32, "] & 0x",
      absl::Hex(1u << (inlined_string_index % 32), absl::kZeroPad8),
      "u) != 0;");
  variables_["donating_states_word"] =
      absl::StrCat(variables_["inlined_string_donated_array"], "[",
                   inlined_string_index / 32, "]");
  variables_["mask_for_undonate"] = absl::StrCat(
      "~0x", absl::Hex(1u << (inlined_string_index % 32), absl::kZeroPad8),
      "u");
}

void FieldGenerator::GenerateAggregateInitializer(io::Printer* p) const {
  Formatter format(p, variables_);
  if (ShouldSplit(descriptor_, options_)) {
    format("decltype(Impl_::Split::$name$_){arena}");
    return;
  }
  format("decltype($field$){arena}");
}

void FieldGenerator::GenerateConstexprAggregateInitializer(
    io::Printer* p) const {
  Formatter format(p, variables_);
  format("/*decltype($field$)*/{}");
}

void FieldGenerator::GenerateCopyAggregateInitializer(io::Printer* p) const {
  Formatter format(p, variables_);
  format("decltype($field$){from.$field$}");
}

void FieldGenerator::GenerateCopyConstructorCode(io::Printer* p) const {
  if (ShouldSplit(descriptor_, options_)) {
    // There is no copy constructor for the `Split` struct, so we need to copy
    // the value here.
    Formatter format(p, variables_);
    format("$field$ = from.$field$;\n");
  }
}

void FieldGenerator::GenerateIfHasField(io::Printer* p) const {
  GOOGLE_ABSL_CHECK(internal::cpp::HasHasbit(descriptor_));
  GOOGLE_ABSL_CHECK(variables_.find("has_hasbit") != variables_.end());

  Formatter format(p, variables_);
  format("if (($has_hasbit$) != 0) {\n");
}

namespace {
std::unique_ptr<FieldGenerator> MakeGenerator(const FieldDescriptor* field,
                                              const Options& options,
                                              MessageSCCAnalyzer* scc) {

  if (field->is_map()) {
    return MakeMapGenerator(field, options, scc);
  }
  if (field->is_repeated()) {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        return MakeRepeatedMessageGenerator(field, options, scc);
      case FieldDescriptor::CPPTYPE_STRING:
        return MakeRepeatedStringGenerator(field, options, scc);
      case FieldDescriptor::CPPTYPE_ENUM:
        return MakeRepeatedEnumGenerator(field, options, scc);
      default:
        return MakeRepeatedPrimitiveGenerator(field, options, scc);
    }
  }

  if (field->real_containing_oneof()) {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        return MakeOneofMessageGenerator(field, options, scc);
      case FieldDescriptor::CPPTYPE_STRING:
        return MakeOneofStringGenerator(field, options, scc);
      case FieldDescriptor::CPPTYPE_ENUM:
        return MakeOneofEnumGenerator(field, options, scc);
      default:
        return MakeOneofPrimitiveGenerator(field, options, scc);
    }
  }

  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return MakeSinguarMessageGenerator(field, options, scc);
    case FieldDescriptor::CPPTYPE_STRING:
      return MakeSinguarStringGenerator(field, options, scc);
    case FieldDescriptor::CPPTYPE_ENUM:
      return MakeSinguarEnumGenerator(field, options, scc);
    default:
      return MakeSinguarPrimitiveGenerator(field, options, scc);
  }
}
}  // namespace

FieldGenWrapper::FieldGenWrapper(const FieldDescriptor* field,
                                 const Options& options,
                                 MessageSCCAnalyzer* scc_analyzer)
    : impl_(MakeGenerator(field, options, scc_analyzer)) {}

FieldGeneratorMap::FieldGeneratorMap(const Descriptor* descriptor,
                                     const Options& options,
                                     MessageSCCAnalyzer* scc)
    : descriptor_(descriptor) {
  // Construct all the FieldGenerators.
  fields_.reserve(descriptor->field_count());
  for (const auto* field : internal::FieldRange(descriptor)) {
    fields_.emplace_back(field, options, scc);
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
