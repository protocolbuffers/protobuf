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
      {"DeclaredCppType", DeclaredCppTypeMethodName(field->cpp_type())},
      {"Oneof", field->real_containing_oneof() ? "Oneof" : ""},
      {"Utf8", IsStrictUtf8String(field, opts) ? "Utf8" : "Raw"},
      {"StrType", IsStrictUtf8String(field, opts) ? "String" : "Bytes"},
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
                                       const Options& options)
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
      is_inlined_ = IsStringInlined(field, options);
      is_bytes_ = field->type() == FieldDescriptor::TYPE_BYTES;
      has_default_constexpr_constructor_ = is_repeated_or_map;
      break;
    case FieldDescriptor::CPPTYPE_MESSAGE:
      is_message_ = true;
      is_group_ = field->type() == FieldDescriptor::TYPE_GROUP;
      is_foreign_ = IsCrossFileMessage(field);
      is_weak_ = IsImplicitWeakField(field, options);
      is_lazy_ = IsLazy(field, options);
      has_trivial_value_ = !(is_repeated_or_map || is_lazy_);
      has_default_constexpr_constructor_ = is_repeated_or_map || is_lazy_;
      break;
  }

  has_trivial_zero_default_ = CanInitializeByZeroing(field, options);
  has_brace_default_assign_ = has_trivial_zero_default_ && !is_lazy_;
}

void FieldGeneratorBase::GenerateMemberConstexprConstructor(
    io::Printer* p) const {
  ABSL_CHECK(!field_->is_extension());
  if (field_->is_repeated() || field_->is_map()) {
    p->Emit({InternalMetadataOffsetSub(p)},
            R"cc(
              $name$_ { visibility, $internal_metadata_offset$ }
            )cc");
  } else {
    p->Emit({{"default", DefaultValue(options_, field_)}},
            "$name$_{$default$}");
  }
}

void FieldGeneratorBase::GenerateMemberConstructor(io::Printer* p) const {
  ABSL_CHECK(!field_->is_extension());
  if (field_->is_repeated() || field_->is_map()) {
    if (ShouldSplit(field_, options_)) {
      ABSL_CHECK(!field_->is_map());
      p->Emit("$name$_{}");  // RawPtr<Repeated>
    } else {
      p->Emit({InternalMetadataOffsetSub(p)},
              R"cc(
                $name$_ { visibility, $internal_metadata_offset$ }
              )cc");
    }
  } else {
    p->Emit({{"default", DefaultValue(options_, field_)}},
            "$name$_{$default$}");
  }
}

void FieldGeneratorBase::GenerateMemberCopyConstructor(io::Printer* p) const {
  ABSL_CHECK(!field_->is_extension());
  if (field_->is_repeated() || field_->is_map()) {
    p->Emit({InternalMetadataOffsetSub(p)},
            R"cc(
              $name$_ {
                visibility, $internal_metadata_offset$, from.$name$_
              }
            )cc");
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

pb::CppFeatures::StringType FieldGeneratorBase::GetDeclaredStringType() const {
  return CppGenerator::GetResolvedSourceFeatures(*field_)
      .GetExtension(pb::cpp)
      .string_type();
}

Sub FieldGeneratorBase::InternalMetadataOffsetSub(io::Printer* p) {
  return Sub("internal_metadata_offset",
             [p] {
               p->Emit(R"cc(
                 ::_pbi::InternalMetadataOffset::Build<
                     $classtype$,
                     PROTOBUF_FIELD_OFFSET($classtype$, _impl_.$name$_)>()
               )cc");
             })
      .WithSuffix("");
}

namespace {
std::unique_ptr<FieldGeneratorBase> MakeGenerator(const FieldDescriptor* field,
                                                  const Options& options) {

  if (field->is_map()) {
    ABSL_CHECK(
        !(field->options().lazy() || field->options().unverified_lazy()));
    return MakeMapGenerator(field, options);
  }
  if (field->is_repeated()) {
    ABSL_CHECK(!field->options().unverified_lazy());

    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_MESSAGE:
        return MakeRepeatedMessageGenerator(field, options);
      case FieldDescriptor::CPPTYPE_STRING: {
        if (field->cpp_string_type() == FieldDescriptor::CppStringType::kView) {
          return MakeRepeatedStringViewGenerator(field, options);
        } else {
          return MakeRepeatedStringGenerator(field, options);
        }
      }
      case FieldDescriptor::CPPTYPE_ENUM:
        return MakeRepeatedEnumGenerator(field, options);
      default:
        return MakeRepeatedPrimitiveGenerator(field, options);
    }
  }

  if (field->real_containing_oneof() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    return MakeOneofMessageGenerator(field, options);
  }

  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return MakeSinguarMessageGenerator(field, options);
    case FieldDescriptor::CPPTYPE_ENUM:
      return MakeSinguarEnumGenerator(field, options);
    case FieldDescriptor::CPPTYPE_STRING: {
      switch (field->cpp_string_type()) {
        case FieldDescriptor::CppStringType::kView:
          return MakeSingularStringViewGenerator(field, options);
        case FieldDescriptor::CppStringType::kCord:
          if (field->type() == FieldDescriptor::TYPE_BYTES) {
            if (field->real_containing_oneof()) {
              return MakeOneofCordGenerator(field, options);
            } else {
              return MakeSingularCordGenerator(field, options);
            }
          }
          ABSL_FALLTHROUGH_INTENDED;
        default:
          return MakeSinguarStringGenerator(field, options);
      }
    }
    default:
      return MakeSinguarPrimitiveGenerator(field, options);
  }
}

void HasBitVars(const FieldDescriptor* field, const Options& opts,
                absl::optional<uint32_t> idx, std::vector<Sub>& vars) {
  if (!idx.has_value()) {
    vars.emplace_back(Sub("set_hasbit", "").WithSuffix(";"));
    vars.emplace_back(Sub("clear_hasbit", "").WithSuffix(";"));
    vars.emplace_back("exclude_mask", "0xFFFFFFFFU");
    return;
  }

  ABSL_CHECK(HasHasbit(field, opts));

  int32_t index = *idx / 32;
  std::string mask = absl::StrFormat("0x%08xU", 1u << (*idx % 32));

  absl::string_view has_bits = IsMapEntryMessage(field->containing_type())
                                   ? "_has_bits_"
                                   : "_impl_._has_bits_";

  auto has_bits_array = absl::StrFormat("%s[%d]", has_bits, index);
  auto for_repeated = field->is_repeated() ? "ForRepeated" : "";
  auto has = absl::StrFormat("CheckHasBit%s(%s, %s)", for_repeated,
                             has_bits_array, mask);
  auto set = absl::StrFormat("SetHasBit%s(%s, %s);", for_repeated,
                             has_bits_array, mask);
  auto clr = absl::StrFormat("ClearHasBit%s(%s, %s);", for_repeated,
                             has_bits_array, mask);

  vars.emplace_back("has_bits_array", has_bits_array);
  vars.emplace_back("has_mask", mask);
  vars.emplace_back("has_hasbit", has);
  vars.emplace_back(Sub("set_hasbit", set).WithSuffix(";"));
  vars.emplace_back(Sub("clear_hasbit", clr).WithSuffix(";"));
  vars.emplace_back("exclude_mask",
                    absl::StrFormat("0x%08xU", ~(1u << (*idx % 32))));
}
}  // namespace

FieldGenerator::FieldGenerator(const FieldDescriptor* field,
                               const Options& options,
                               absl::optional<uint32_t> hasbit_index)
    : impl_(MakeGenerator(field, options)),
      field_vars_(FieldVars(field, options)),
      tracker_vars_(MakeTrackerCalls(field, options)),
      per_generator_vars_(impl_->MakeVars()) {
  HasBitVars(field, options, hasbit_index, field_vars_);
}

void FieldGeneratorTable::Build(const Options& options,
                                absl::Span<const int32_t> has_bit_indices) {
  // Construct all the FieldGenerators.
  fields_.reserve(static_cast<size_t>(descriptor_->field_count()));
  for (const auto* field : internal::FieldRange(descriptor_)) {
    size_t index = static_cast<size_t>(field->index());
    absl::optional<uint32_t> has_bit_index;
    if (!has_bit_indices.empty() && has_bit_indices[index] >= 0) {
      has_bit_index = static_cast<uint32_t>(has_bit_indices[index]);
    }

    fields_.push_back(FieldGenerator(field, options, has_bit_index));
  }
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
