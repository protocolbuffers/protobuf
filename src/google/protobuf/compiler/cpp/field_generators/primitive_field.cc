// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using ::google::protobuf::internal::WireFormat;
using ::google::protobuf::internal::WireFormatLite;
using Sub = ::google::protobuf::io::Printer::Sub;
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

// For encodings with fixed sizes, returns that size in bytes.
std::optional<size_t> FixedSize(FieldDescriptor::Type type) {
  switch (type) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_ENUM:
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      return std::nullopt;

    case FieldDescriptor::TYPE_FIXED32:
      return WireFormatLite::kFixed32Size;
    case FieldDescriptor::TYPE_FIXED64:
      return WireFormatLite::kFixed64Size;
    case FieldDescriptor::TYPE_SFIXED32:
      return WireFormatLite::kSFixed32Size;
    case FieldDescriptor::TYPE_SFIXED64:
      return WireFormatLite::kSFixed64Size;
    case FieldDescriptor::TYPE_FLOAT:
      return WireFormatLite::kFloatSize;
    case FieldDescriptor::TYPE_DOUBLE:
      return WireFormatLite::kDoubleSize;
    case FieldDescriptor::TYPE_BOOL:
      return WireFormatLite::kBoolSize;

      // No default because we want the compiler to complain if any new
      // types are added.
  }

  ABSL_LOG(FATAL) << "Can't get here.";
  return std::nullopt;
}

std::vector<Sub> Vars(const FieldDescriptor* field, const Options& options) {
  bool cold = ShouldSplit(field, options);
  return {
      {"Type", PrimitiveTypeName(options, field->cpp_type())},
      {"kDefault", DefaultValue(options, field)},
      {"_field_cached_byte_size_", MakeVarintCachedSizeFieldName(field, cold)},
  };
}

class SingularPrimitive final : public FieldGeneratorBase {
 public:
  SingularPrimitive(const FieldDescriptor* field, const Options& opts,
                    MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc), opts_(&opts) {}
  ~SingularPrimitive() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  void GeneratePrivateMembers(io::Printer* p) const override {
    p->Emit(R"cc(
      $Type$ $name$_;
    )cc");
  }

  void GenerateClearingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      $field_$ = $kDefault$;
    )cc");
  }

  void GenerateMergingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _this->$field_$ = from.$field_$;
    )cc");
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    if (is_oneof()) {
      // Don't print any swapping code. Swapping the union will swap this field.
      return;
    }

    p->Emit(R"cc(
      //~ A `using std::swap;` is already present in this function.
      swap($field_$, other->$field_$);
    )cc");
  }

  void GenerateConstructorCode(io::Printer* p) const override {
    if (!is_oneof()) {
      return;
    }

    p->Emit(R"cc(
      $pkg$::_$Msg$_default_instance_.$field_$ = $kDefault$;
    )cc");
  }

  void GenerateCopyConstructorCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _this->$field_$ = from.$field_$;
    )cc");
  }

  void GenerateConstexprAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      /*decltype($field_$)*/ $kDefault$,
    )cc");
  }

  void GenerateAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$){$kDefault$},
    )cc");
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$){},
    )cc");
  }

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateByteSize(io::Printer* p) const override;

 private:
  const Options* opts_;
};

void SingularPrimitive::GenerateAccessorDeclarations(io::Printer* p) const {
  auto v = p->WithVars(
      AnnotatedAccessors(field_, {"", "_internal_", "_internal_set_"}));
  auto vs = p->WithVars(AnnotatedAccessors(field_, {"set_"}, Semantic::kSet));
  p->Emit(R"cc(
    $DEPRECATED$ $Type$ $name$() const;
    $DEPRECATED$ void $set_name$($Type$ value);

    private:
    $Type$ $_internal_name$() const;
    void $_internal_set_name$($Type$ value);

    public:
  )cc");
}

void SingularPrimitive::GenerateInlineAccessorDefinitions(
    io::Printer* p) const {
  p->Emit(R"cc(
    inline $Type$ $Msg$::$name$() const {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return _internal_$name_internal$();
    }
  )cc");

  if (is_oneof()) {
    p->Emit(R"cc(
      inline void $Msg$::set_$name$($Type$ value) {
        $WeakDescriptorSelfPin$;
        $PrepareSplitMessageForWrite$;
        if ($not_has_field$) {
          clear_$oneof_name$();
          set_has_$name_internal$();
        }
        $field_$ = value;
        $annotate_set$;
        // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
      }
      inline $Type$ $Msg$::_internal_$name_internal$() const {
        if ($has_field$) {
          return $field_$;
        }
        return $kDefault$;
      }
    )cc");
  } else {
    p->Emit(R"cc(
      inline void $Msg$::set_$name$($Type$ value) {
        $WeakDescriptorSelfPin$;
        $PrepareSplitMessageForWrite$;
        _internal_set_$name_internal$(value);
        $set_hasbit$;
        $annotate_set$;
        // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
      }
      inline $Type$ $Msg$::_internal_$name_internal$() const {
        $TsanDetectConcurrentRead$;
        return $field_$;
      }
      inline void $Msg$::_internal_set_$name_internal$($Type$ value) {
        $TsanDetectConcurrentMutation$;
        $field_$ = value;
      }
    )cc");
  }
}

void SingularPrimitive::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if ((field_->number() < 16) &&
      (field_->type() == FieldDescriptor::TYPE_INT32 ||
       field_->type() == FieldDescriptor::TYPE_INT64 ||
       field_->type() == FieldDescriptor::TYPE_ENUM)) {
    // Call special non-inlined routine with tag number hardcoded as a
    // template parameter that handles the EnsureSpace and the writing
    // of the tag+value to the array
    p->Emit(R"cc(
      target =
          $pbi$::WireFormatLite::Write$declared_type$ToArrayWithField<$number$>(
              stream, this_._internal_$name$(), target);
    )cc");
  } else {
    p->Emit(R"cc(
      target = stream->EnsureSpace(target);
      target = ::_pbi::WireFormatLite::Write$DeclaredType$ToArray(
          $number$, this_._internal_$name$(), target);
    )cc");
  }
}

void SingularPrimitive::GenerateByteSize(io::Printer* p) const {
  size_t tag_size = WireFormat::TagSize(field_->number(), field_->type());

  auto fixed_size = FixedSize(field_->type());
  if (fixed_size.has_value()) {
    p->Emit({{"kFixedBytes", tag_size + *fixed_size}}, R"cc(
      total_size += $kFixedBytes$;
    )cc");
    return;
  }

  // Adding one is very common and it turns out it can be done for
  // free inside of WireFormatLite, so we can save an instruction here.
  if (tag_size == 1) {
    p->Emit(R"cc(
      total_size += ::_pbi::WireFormatLite::$DeclaredType$SizePlusOne(
          this_._internal_$name$());
    )cc");
    return;
  }

  p->Emit(R"cc(
    total_size += $kTagBytes$ + ::_pbi::WireFormatLite::$DeclaredType$Size(
                                    this_._internal_$name$());
  )cc");
}


class RepeatedPrimitive final : public FieldGeneratorBase {
 public:
  RepeatedPrimitive(const FieldDescriptor* field, const Options& opts,
                    MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc), opts_(&opts) {}
  ~RepeatedPrimitive() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  void GenerateClearingCode(io::Printer* p) const override {
    if (should_split()) {
      p->Emit("$field_$.ClearIfNotDefault();\n");
    } else {
      p->Emit("$field_$.Clear();\n");
    }
  }

  void GenerateMergingCode(io::Printer* p) const override {
    // TODO: experiment with simplifying this to be
    // `if (!from.empty()) { body(); }` for both split and non-split cases.
    auto body = [&] {
      p->Emit(R"cc(
        _this->_internal_mutable_$name$()->MergeFrom(from._internal_$name$());
      )cc");
    };
    if (!should_split()) {
      body();
    } else {
      p->Emit({{"body", body}}, R"cc(
        if (!from.$field_$.IsDefault()) {
          $body$;
        }
      )cc");
    }
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    ABSL_CHECK(!should_split());
    p->Emit(R"cc(
      $field_$.InternalSwap(&other->$field_$);
    )cc");
  }

  void GenerateDestructorCode(io::Printer* p) const override {
    if (should_split()) {
      p->Emit(R"cc(
        this_.$field_$.DeleteIfNotDefault();
      )cc");
    }
  }

  void GenerateConstructorCode(io::Printer* p) const override {}

  void GenerateCopyConstructorCode(io::Printer* p) const override {
    if (should_split()) {
      p->Emit(R"cc(
        if (!from._internal_$name$().empty()) {
          _internal_mutable_$name$()->MergeFrom(from._internal_$name$());
        }
      )cc");
    }
  }

  void GenerateConstexprAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      /*decltype($field_$)*/ {},
    )cc");
    GenerateCacheSizeInitializer(p);
  }

  void GenerateAggregateInitializer(io::Printer* p) const override {
    ABSL_CHECK(!should_split());
    p->Emit(R"cc(
      decltype($field_$){arena},
    )cc");
    GenerateCacheSizeInitializer(p);
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    ABSL_CHECK(!should_split());
    p->Emit(R"cc(
      decltype($field_$){from.$field_$},
    )cc");
    GenerateCacheSizeInitializer(p);
  }

  void GenerateMemberConstexprConstructor(io::Printer* p) const override {
    p->Emit("$name$_{}");
    if (HasCachedSize()) {
      p->Emit(",\n_$name$_cached_byte_size_{0}");
    }
  }

  void GenerateMemberConstructor(io::Printer* p) const override {
    p->Emit("$name$_{visibility, arena}");
    if (HasCachedSize()) {
      p->Emit(",\n_$name$_cached_byte_size_{0}");
    }
  }

  void GenerateMemberCopyConstructor(io::Printer* p) const override {
    p->Emit("$name$_{visibility, arena, from.$name$_}");
    if (HasCachedSize()) {
      p->Emit(",\n_$name$_cached_byte_size_{0}");
    }
  }

  void GenerateOneofCopyConstruct(io::Printer* p) const override {
    ABSL_LOG(FATAL) << "Not supported";
  }

  void GeneratePrivateMembers(io::Printer* p) const override;
  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateByteSize(io::Printer* p) const override;

 private:
  bool HasCachedSize() const {
    bool is_packed_varint =
        field_->is_packed() && !FixedSize(field_->type()).has_value();
    return is_packed_varint && HasGeneratedMethods(field_->file(), *opts_) &&
           !should_split();
  }

  void GenerateCacheSizeInitializer(io::Printer* p) const {
    if (!HasCachedSize()) return;
    // std::atomic has no move constructor, which prevents explicit aggregate
    // initialization pre-C++17.
    p->Emit(R"cc(
      /* $_field_cached_byte_size_$ = */ {0},
    )cc");
  }

  const Options* opts_;
};

void RepeatedPrimitive::GeneratePrivateMembers(io::Printer* p) const {
  if (should_split()) {
    p->Emit(R"cc(
      $pbi$::RawPtr<$pb$::RepeatedField<$Type$>> $name$_;
    )cc");
  } else {
    p->Emit(R"cc(
      $pb$::RepeatedField<$Type$> $name$_;
    )cc");
  }

  if (HasCachedSize()) {
    p->Emit({{"_cached_size_", MakeVarintCachedSizeName(field_)}},
            R"cc(
              $pbi$::CachedSize $_cached_size_$;
            )cc");
  }
}

void RepeatedPrimitive::GenerateAccessorDeclarations(io::Printer* p) const {
  auto v = p->WithVars(
      AnnotatedAccessors(field_, {"", "_internal_", "_internal_mutable_"}));
  auto vs =
      p->WithVars(AnnotatedAccessors(field_, {"set_", "add_"}, Semantic::kSet));
  auto va =
      p->WithVars(AnnotatedAccessors(field_, {"mutable_"}, Semantic::kAlias));
  p->Emit(R"cc(
    $DEPRECATED$ $Type$ $name$(int index) const;
    $DEPRECATED$ void $set_name$(int index, $Type$ value);
    $DEPRECATED$ void $add_name$($Type$ value);
    $DEPRECATED$ const $pb$::RepeatedField<$Type$>& $name$() const;
    $DEPRECATED$ $pb$::RepeatedField<$Type$>* $nonnull$ $mutable_name$();

    private:
    const $pb$::RepeatedField<$Type$>& $_internal_name$() const;
    $pb$::RepeatedField<$Type$>* $nonnull$ $_internal_mutable_name$();

    public:
  )cc");
}

void RepeatedPrimitive::GenerateInlineAccessorDefinitions(
    io::Printer* p) const {
  p->Emit(R"cc(
    inline $Type$ $Msg$::$name$(int index) const {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return _internal_$name_internal$().Get(index);
    }
  )cc");
  p->Emit(R"cc(
    inline void $Msg$::set_$name$(int index, $Type$ value) {
      $WeakDescriptorSelfPin$;
      $annotate_set$;
      _internal_mutable_$name_internal$()->Set(index, value);
      // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
    }
  )cc");
  p->Emit(R"cc(
    inline void $Msg$::add_$name$($Type$ value) {
      $WeakDescriptorSelfPin$;
      $TsanDetectConcurrentMutation$;
      _internal_mutable_$name_internal$()->Add(value);
      $set_hasbit$;
      $annotate_add$;
      // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
    }
  )cc");
  p->Emit(R"cc(
    inline const $pb$::RepeatedField<$Type$>& $Msg$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $annotate_list$;
      // @@protoc_insertion_point(field_list:$pkg.Msg.field$)
      return _internal_$name_internal$();
    }
  )cc");
  p->Emit(R"cc(
    inline $pb$::RepeatedField<$Type$>* $nonnull$ $Msg$::mutable_$name$()
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $set_hasbit$;
      $annotate_mutable_list$;
      // @@protoc_insertion_point(field_mutable_list:$pkg.Msg.field$)
      $TsanDetectConcurrentMutation$;
      return _internal_mutable_$name_internal$();
    }
  )cc");

  if (should_split()) {
    p->Emit(R"cc(
      inline const $pb$::RepeatedField<$Type$>&
      $Msg$::_internal_$name_internal$() const {
        $TsanDetectConcurrentRead$;
        return *$field_$;
      }
      inline $pb$::RepeatedField<$Type$>* $nonnull$
      $Msg$::_internal_mutable_$name_internal$() {
        $TsanDetectConcurrentRead$;
        $PrepareSplitMessageForWrite$;
        if ($field_$.IsDefault()) {
          $field_$.Set($pb$::Arena::Create<$pb$::RepeatedField<$Type$>>(GetArena()));
        }
        return $field_$.Get();
      }
    )cc");
  } else {
    p->Emit(R"cc(
      inline const $pb$::RepeatedField<$Type$>&
      $Msg$::_internal_$name_internal$() const {
        $TsanDetectConcurrentRead$;
        return $field_$;
      }
      inline $pb$::RepeatedField<$Type$>* $nonnull$
      $Msg$::_internal_mutable_$name_internal$() {
        $TsanDetectConcurrentRead$;
        return &$field_$;
      }
    )cc");
  }
}

void RepeatedPrimitive::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (!field_->is_packed()) {
    p->Emit(R"cc(
      for (int i = 0, n = this_._internal_$name$_size(); i < n; ++i) {
        target = stream->EnsureSpace(target);
        target = ::_pbi::WireFormatLite::Write$DeclaredType$ToArray(
            $number$, this_._internal_$name$().Get(i), target);
      }
    )cc");
    return;
  }

  if (FixedSize(field_->type()).has_value()) {
    p->Emit(R"cc(
      if (this_._internal_$name$_size() > 0) {
        target = stream->WriteFixedPacked($number$, this_._internal_$name$(), target);
      }
    )cc");
    return;
  }

  p->Emit(
      {
          {"byte_size",
           [&] {
             if (HasCachedSize()) {
               p->Emit(R"cc(this_.$_field_cached_byte_size_$.Get();)cc");
             } else {
               p->Emit(R"cc(
                 ::_pbi::WireFormatLite::$DeclaredType$Size(
                     this_._internal_$name$());
               )cc");
             }
           }},
      },
      R"cc(
        {
          int byte_size = $byte_size$;
          if (byte_size > 0) {
            target = stream->Write$DeclaredType$Packed(
                $number$, this_._internal_$name$(), byte_size, target);
          }
        }
      )cc");
}

void RepeatedPrimitive::GenerateByteSize(io::Printer* p) const {
  if (HasCachedSize()) {
    ABSL_CHECK(field_->is_packed());
    p->Emit(
        R"cc(
          total_size +=
              ::_pbi::WireFormatLite::$DeclaredType$SizeWithPackedTagSize(
                  this_._internal_$name$(), $kTagBytes$,
                  this_.$_field_cached_byte_size_$);
        )cc");
    return;
  }
  p->Emit(
      {
          {"data_size",
           [&] {
             auto fixed_size = FixedSize(field_->type());
             if (fixed_size.has_value()) {
               p->Emit({{"kFixed", *fixed_size}}, R"cc(
                 ::size_t{$kFixed$} *
                     ::_pbi::FromIntSize(this_._internal_$name$_size());
               )cc");
             } else {
               p->Emit(R"cc(
                 ::_pbi::WireFormatLite::$DeclaredType$Size(
                     this_._internal_$name$());
               )cc");
             }
           }},
          {"tag_size",
           [&] {
             if (field_->is_packed()) {
               p->Emit(R"cc(
                 data_size == 0
                     ? 0
                     : $kTagBytes$ + ::_pbi::WireFormatLite::Int32Size(
                                         static_cast<::int32_t>(data_size));
               )cc");
             } else {
               p->Emit(R"cc(
                 ::size_t{$kTagBytes$} *
                     ::_pbi::FromIntSize(this_._internal_$name$_size());
               )cc");
             }
           }},
      },
      R"cc(
        ::size_t data_size = $data_size$;
        ::size_t tag_size = $tag_size$;
        total_size += tag_size + data_size;
      )cc");
}
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSinguarPrimitiveGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<SingularPrimitive>(desc, options, scc);
}

std::unique_ptr<FieldGeneratorBase> MakeRepeatedPrimitiveGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<RepeatedPrimitive>(desc, options, scc);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
