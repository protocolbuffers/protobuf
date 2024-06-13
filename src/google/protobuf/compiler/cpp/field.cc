// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/cpp/field.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/generator.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/compiler/cpp/tracker.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
using ::google::protobuf::internal::WireFormat;
using Sub = ::google::protobuf::io::Printer::Sub;

std::vector<Sub> FieldVars(const FieldDescriptor* field, const Options& opts) {
  bool split = ShouldSplit(field, opts);
  std::vector<Sub> vars = {
      // This will eventually be renamed to "field", once the existing "field"
      // variable is replaced with "field_" everywhere.
      {"name", FieldName(field)},
      // Same as above, but represents internal use.
      {"name_internal", FieldName(field)},

      {"index", field->index()},
      {"number", field->number()},
      {"pkg.Msg.field", field->full_name()},

      {"field_", FieldMemberName(field, split)},
      {"DeclaredType", DeclaredTypeMethodName(field->type())},
      {"kTagBytes", WireFormat::TagSize(field->number(), field->type())},
      Sub("PrepareSplitMessageForWrite",
          split ? "PrepareSplitMessageForWrite();" : "")
          .WithSuffix(";"),
      Sub("DEPRECATED", DeprecatedAttribute(opts, field)).WithSuffix(" "),

      // These variables are placeholders to pick out the beginning and ends of
      // identifiers for annotations (when doing so with existing variables
      // would be ambiguous or impossible). They should never be set to anything
      // but the empty string.
      {"{", ""},
      {"}", ""},

      // For TSan validation.
      {"TsanDetectConcurrentMutation",
       absl::StrCat("::", ProtobufNamespace(opts),
                    "::internal::TSanWrite(&_impl_)")},
      {"TsanDetectConcurrentRead",
       absl::StrCat("::", ProtobufNamespace(opts),
                    "::internal::TSanRead(&_impl_)")},

      // Old-style names.
      {"field", FieldMemberName(field, split)},
      {"declared_type", DeclaredTypeMethodName(field->type())},
      {"classname", ClassName(FieldScope(field), false)},
      {"ns", Namespace(field, opts)},
      {"tag_size", WireFormat::TagSize(field->number(), field->type())},
      {"deprecated_attr", DeprecatedAttribute(opts, field)},
      Sub("WeakDescriptorSelfPin",
          UsingImplicitWeakDescriptor(field->file(), opts)
              ? absl::StrCat(
                    StrongReferenceToType(field->containing_type(), opts), ";")
              : "")
          .WithSuffix(";"),
  };

  if (const auto* oneof = field->containing_oneof()) {
    auto field_name = UnderscoresToCamelCase(field->name(), true);

    vars.push_back({"oneof_name", oneof->name()});
    vars.push_back({"field_name", field_name});
    vars.push_back({"oneof_index", oneof->index()});
    vars.push_back({"has_field", absl::StrFormat("%s_case() == k%s",
                                                 oneof->name(), field_name)});
    vars.push_back(
        {"not_has_field",
         absl::StrFormat("%s_case() != k%s", oneof->name(), field_name)});
  }

  return vars;
}

FieldGeneratorBase::FieldGeneratorBase(const FieldDescriptor* field,
                                       const Options& options,
                                       MessageSCCAnalyzer* scc)
    : field_(field), options_(options) {
  bool is_repeated_or_map = field->is_repeated();
  should_split_ = ShouldSplit(field, options);
  is_oneof_ = field->real_containing_oneof() != nullptr;
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_ENUM:
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_UINT64:
    case FieldDescriptor::CPPTYPE_FLOAT:
    case FieldDescriptor::CPPTYPE_DOUBLE:
    case FieldDescriptor::CPPTYPE_BOOL:
      is_trivial_ = has_trivial_value_ = !is_repeated_or_map;
      has_default_constexpr_constructor_ = is_repeated_or_map;
      break;
    case FieldDescriptor::CPPTYPE_STRING:
      is_string_ = true;
      string_type_ = field->options().ctype();
      is_inlined_ = IsStringInlined(field, options);
      is_bytes_ = field->type() == FieldDescriptor::TYPE_BYTES;
      has_default_constexpr_constructor_ = is_repeated_or_map;
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      is_message_ = true;
      is_group_ = field->type() == FieldDescriptor::TYPE_GROUP;
      is_foreign_ = IsCrossFileMessage(field);
      is_weak_ = IsImplicitWeakField(field, options, scc);
      is_lazy_ = IsLazy(field, options, scc);
      has_trivial_value_ = !(is_repeated_or_map || is_lazy_);
      has_default_constexpr_constructor_ = is_repeated_or_map || is_lazy_;
      break;
  }

  has_trivial_zero_default_ = CanInitializeByZeroing(field, options, scc);
}

void FieldGeneratorBase::GenerateMemberConstexprConstructor(
    io::Printer* p) const {
  ABSL_CHECK(!field_->is_extension());
  if (field_->is_repeated()) {
    p->Emit("$name$_{}");
  } else {
    p->Emit({{"default", DefaultValue(options_, field_)}},
            "$name$_{$default$}");
  }
}

void FieldGeneratorBase::GenerateMemberConstructor(io::Printer* p) const {
  ABSL_CHECK(!field_->is_extension());
  if (field_->is_map()) {
    p->Emit("$name$_{visibility, arena}");
  } else if (field_->is_repeated()) {
    if (ShouldSplit(field_, options_)) {
      p->Emit("$name$_{}");  // RawPtr<Repeated>
    } else {
      p->Emit("$name$_{visibility, arena}");
    }
  } else {
    p->Emit({{"default", DefaultValue(options_, field_)}},
            "$name$_{$default$}");
  }
}

void FieldGeneratorBase::GenerateMemberCopyConstructor(io::Printer* p) const {
  ABSL_CHECK(!field_->is_extension());
  if (field_->is_repeated()) {
    p->Emit("$name$_{visibility, arena, from.$name$_}");
  } else {
    p->Emit("$name$_{from.$name$_}");
  }
}

void FieldGeneratorBase::GenerateOneofCopyConstruct(io::Printer* p) const {
  ABSL_CHECK(!field_->is_extension()) << "Not supported";
  ABSL_CHECK(!field_->is_repeated()) << "Not supported";
  ABSL_CHECK(!field_->is_map()) << "Not supported";
  p->Emit("$field$ = from.$field$;\n");
}

void FieldGeneratorBase::GenerateAggregateInitializer(io::Printer* p) const {
  if (ShouldSplit(field_, options_)) {
    p->Emit(R"cc(
      decltype(Impl_::Split::$name$_){arena},
    )cc");
  } else {
    p->Emit(R"cc(
      decltype($field$){arena},
    )cc");
  }
}

void FieldGeneratorBase::GenerateConstexprAggregateInitializer(
    io::Printer* p) const {
  p->Emit(R"cc(
    /*decltype($field$)*/ {},
  )cc");
}

void FieldGeneratorBase::GenerateCopyAggregateInitializer(
    io::Printer* p) const {
  p->Emit(R"cc(
    decltype($field$){from.$field$},
  )cc");
}

void FieldGeneratorBase::GenerateCopyConstructorCode(io::Printer* p) const {
  if (should_split()) {
    // There is no copy constructor for the `Split` struct, so we need to copy
    // the value here.
    Formatter format(p, variables_);
    format("$field$ = from.$field$;\n");
  }
}

namespace {
// Use internal types instead of ctype or string_type.
enum class StringType {
  kView,
  kString,
  kCord,
  kStringPiece,
};

StringType GetStringType(const FieldDescriptor& field) {
  ABSL_CHECK_EQ(field.cpp_type(), FieldDescriptor::CPPTYPE_STRING);

  if (field.options().has_ctype()) {
    switch (field.options().ctype()) {
      case FieldOptions::CORD:
        return StringType::kCord;
      case FieldOptions::STRING_PIECE:
        return StringType::kStringPiece;
      default:
        return StringType::kString;
    }
  }

  const pb::CppFeatures& cpp_features =
      CppGenerator::GetResolvedSourceFeatures(field).GetExtension(::pb::cpp);
  switch (cpp_features.string_type()) {
    case pb::CppFeatures::CORD:
      return StringType::kCord;
    case pb::CppFeatures::VIEW:
      return StringType::kView;
    default:
      return StringType::kString;
  }
}

std::unique_ptr<FieldGeneratorBase> MakeGenerator(const FieldDescriptor* field,
                                                  const Options& options,
                                                  MessageSCCAnalyzer* scc) {

  if (field->is_map()) {
    ABSL_CHECK(
        !(field->options().lazy() || field->options().unverified_lazy()));
    return MakeMapGenerator(field, options, scc);
  }
  if (field->is_repeated()) {
    ABSL_CHECK(!field->options().unverified_lazy());

    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        return MakeRepeatedMessageGenerator(field, options, scc);
      case FieldDescriptor::CPPTYPE_STRING: {
        if (GetStringType(*field) == StringType::kView) {
          return MakeRepeatedStringViewGenerator(field, options, scc);
        } else {
          return MakeRepeatedStringGenerator(field, options, scc);
        }
      }
      case FieldDescriptor::CPPTYPE_ENUM:
        return MakeRepeatedEnumGenerator(field, options, scc);
      default:
        return MakeRepeatedPrimitiveGenerator(field, options, scc);
    }
  }

  if (field->real_containing_oneof() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    return MakeOneofMessageGenerator(field, options, scc);
  }

  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return MakeSinguarMessageGenerator(field, options, scc);
    case FieldDescriptor::CPPTYPE_ENUM:
      return MakeSinguarEnumGenerator(field, options, scc);
    case FieldDescriptor::CPPTYPE_STRING: {
      switch (GetStringType(*field)) {
        case StringType::kView:
          return MakeSingularStringViewGenerator(field, options, scc);
        case StringType::kCord:
          if (field->type() == FieldDescriptor::TYPE_BYTES) {
            if (field->real_containing_oneof()) {
              return MakeOneofCordGenerator(field, options, scc);
            } else {
              return MakeSingularCordGenerator(field, options, scc);
            }
          }
          ABSL_FALLTHROUGH_INTENDED;
        default:
          return MakeSinguarStringGenerator(field, options, scc);
      }
    }
    default:
      return MakeSinguarPrimitiveGenerator(field, options, scc);
  }
}

void HasBitVars(const FieldDescriptor* field, const Options& opts,
                absl::optional<uint32_t> idx, std::vector<Sub>& vars) {
  if (!idx.has_value()) {
    vars.emplace_back(Sub("set_hasbit", "").WithSuffix(";"));
    vars.emplace_back(Sub("clear_hasbit", "").WithSuffix(";"));
    return;
  }

  ABSL_CHECK(internal::cpp::HasHasbit(field));

  int32_t index = *idx / 32;
  std::string mask = absl::StrFormat("0x%08xu", 1u << (*idx % 32));

  absl::string_view has_bits = IsMapEntryMessage(field->containing_type())
                                   ? "_has_bits_"
                                   : "_impl_._has_bits_";

  auto has = absl::StrFormat("%s[%d] & %s", has_bits, index, mask);
  auto set = absl::StrFormat("%s[%d] |= %s;", has_bits, index, mask);
  auto clr = absl::StrFormat("%s[%d] &= ~%s;", has_bits, index, mask);

  vars.emplace_back("has_hasbit", has);
  vars.emplace_back(Sub("set_hasbit", set).WithSuffix(";"));
  vars.emplace_back(Sub("clear_hasbit", clr).WithSuffix(";"));
}

void InlinedStringVars(const FieldDescriptor* field, const Options& opts,
                       absl::optional<uint32_t> idx, std::vector<Sub>& vars) {
  if (!IsStringInlined(field, opts)) {
    ABSL_CHECK(!idx.has_value());
    return;
  }

  // The first bit is the tracking bit for on demand registering ArenaDtor.
  ABSL_CHECK_GT(*idx, 0u)
      << "_inlined_string_donated_'s bit 0 is reserved for arena dtor tracking";

  int32_t index = *idx / 32;
  std::string mask = absl::StrFormat("0x%08xu", 1u << (*idx % 32));
  vars.emplace_back("inlined_string_index", index);
  vars.emplace_back("inlined_string_mask", mask);

  absl::string_view array = IsMapEntryMessage(field->containing_type())
                                ? "_inlined_string_donated_"
                                : "_impl_._inlined_string_donated_";

  vars.emplace_back("inlined_string_donated",
                    absl::StrFormat("(%s[%d] & %s) != 0;", array, index, mask));
  vars.emplace_back("donating_states_word",
                    absl::StrFormat("%s[%d]", array, index));
  vars.emplace_back("mask_for_undonate", absl::StrFormat("~%s", mask));
}
}  // namespace

FieldGenerator::FieldGenerator(const FieldDescriptor* field,
                               const Options& options,
                               MessageSCCAnalyzer* scc_analyzer,
                               absl::optional<uint32_t> hasbit_index,
                               absl::optional<uint32_t> inlined_string_index)
    : impl_(MakeGenerator(field, options, scc_analyzer)),
      field_vars_(FieldVars(field, options)),
      tracker_vars_(MakeTrackerCalls(field, options)),
      per_generator_vars_(impl_->MakeVars()) {
  HasBitVars(field, options, hasbit_index, field_vars_);
  InlinedStringVars(field, options, inlined_string_index, field_vars_);
}

void FieldGeneratorTable::Build(
    const Options& options, MessageSCCAnalyzer* scc,
    absl::Span<const int32_t> has_bit_indices,
    absl::Span<const int32_t> inlined_string_indices) {
  // Construct all the FieldGenerators.
  fields_.reserve(static_cast<size_t>(descriptor_->field_count()));
  for (const auto* field : internal::FieldRange(descriptor_)) {
    size_t index = static_cast<size_t>(field->index());
    absl::optional<uint32_t> has_bit_index;
    if (!has_bit_indices.empty() && has_bit_indices[index] >= 0) {
      has_bit_index = static_cast<uint32_t>(has_bit_indices[index]);
    }

    absl::optional<uint32_t> inlined_string_index;
    if (!inlined_string_indices.empty() && inlined_string_indices[index] >= 0) {
      inlined_string_index =
          static_cast<uint32_t>(inlined_string_indices[index]);
    }

    fields_.push_back(FieldGenerator(field, options, scc, has_bit_index,
                                     inlined_string_index));
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
