// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <memory>
#include <string>
#include <vector>

#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/memory/memory.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;
using Sub = ::google::protobuf::io::Printer::Sub;

std::vector<Sub> Vars(const FieldDescriptor* field, const Options& opts) {
  const EnumValueDescriptor* default_value = field->default_value_enum();
  bool split = ShouldSplit(field, opts);
  bool is_open = internal::cpp::HasPreservingUnknownEnumSemantics(field);
  auto enum_name = QualifiedClassName(field->enum_type(), opts);
  return {
      {"Enum", enum_name},
      {"kDefault", Int32ToString(default_value->number())},
      Sub("assert_valid", is_open ? ""
                                  : absl::Substitute(
                                        R"cc(
                                          assert(::$0::internal::ValidateEnum(
                                              value, $1_internal_data_));
                                        )cc",
                                        ProtobufNamespace(opts), enum_name))
          .WithSuffix(";"),

      {"cached_size_name", MakeVarintCachedSizeName(field)},
      {"cached_size_", MakeVarintCachedSizeFieldName(field, split)},
  };
}

class SingularEnum : public FieldGeneratorBase {
 public:
  SingularEnum(const FieldDescriptor* field, const Options& opts,
               MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc), opts_(&opts) {}
  ~SingularEnum() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  void GeneratePrivateMembers(io::Printer* p) const override {
    p->Emit(R"cc(
      int $name$_;
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
    if (is_oneof()) return;

    p->Emit(R"cc(
      swap($field_$, other->$field_$);
    )cc");
  }

  void GenerateConstructorCode(io::Printer* p) const override {
    if (!is_oneof()) return;
    p->Emit(R"cc(
      $ns$::_$Msg$_default_instance_.$field_$ = $kDefault$;
    )cc");
  }

  void GenerateCopyConstructorCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _this->$field_$ = from.$field_$;
    )cc");
  }

  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override {
    p->Emit(R"cc(
      target = stream->EnsureSpace(target);
      target = ::_pbi::WireFormatLite::WriteEnumToArray(
          $number$, this_._internal_$name$(), target);
    )cc");
  }

  void GenerateByteSize(io::Printer* p) const override {
    p->Emit(R"cc(
      total_size += $kTagBytes$ +
                    ::_pbi::WireFormatLite::EnumSize(this_._internal_$name$());
    )cc");
  }


  void GenerateConstexprAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      /*decltype($field_$)*/ $kDefault$,
    )cc");
  }

  void GenerateAggregateInitializer(io::Printer* p) const override {
    if (should_split()) {
      p->Emit(R"cc(
        decltype(Impl_::Split::$name$_){$kDefault$},
      )cc");
    } else {
      p->Emit(R"cc(
        decltype($field_$){$kDefault$},
      )cc");
    }
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$){},
    )cc");
  }

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;

 private:
  const Options* opts_;
};

void SingularEnum::GenerateAccessorDeclarations(io::Printer* p) const {
  auto v = p->WithVars(
      AnnotatedAccessors(field_, {"", "_internal_", "_internal_set_"}));
  auto vs = p->WithVars(AnnotatedAccessors(field_, {"set_"}, Semantic::kSet));
  p->Emit(R"cc(
    $DEPRECATED$ $Enum$ $name$() const;
    $DEPRECATED$ void $set_name$($Enum$ value);

    private:
    $Enum$ $_internal_name$() const;
    void $_internal_set_name$($Enum$ value);

    public:
  )cc");
}

void SingularEnum::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit(R"cc(
    inline $Enum$ $Msg$::$name$() const {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return _internal_$name_internal$();
    }
  )cc");

  if (is_oneof()) {
    p->Emit(R"cc(
      inline void $Msg$::set_$name$($Enum$ value) {
        $WeakDescriptorSelfPin$;
        $PrepareSplitMessageForWrite$;
        $assert_valid$;
        if ($not_has_field$) {
          clear_$oneof_name$();
          set_has_$name_internal$();
        }
        $field_$ = value;
        $annotate_set$;
        // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
      }
      inline $Enum$ $Msg$::_internal_$name_internal$() const {
        if ($has_field$) {
          return static_cast<$Enum$>($field_$);
        }
        return static_cast<$Enum$>($kDefault$);
      }
    )cc");
  } else {
    p->Emit(R"cc(
      inline void $Msg$::set_$name$($Enum$ value) {
        $WeakDescriptorSelfPin$;
        $PrepareSplitMessageForWrite$;
        _internal_set_$name_internal$(value);
        $set_hasbit$;
        $annotate_set$;
        // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
      }
      inline $Enum$ $Msg$::_internal_$name_internal$() const {
        $TsanDetectConcurrentRead$;
        return static_cast<$Enum$>($field_$);
      }
      inline void $Msg$::_internal_set_$name_internal$($Enum$ value) {
        $TsanDetectConcurrentMutation$;
        $assert_valid$;
        $field_$ = value;
      }
    )cc");
  }
}

class RepeatedEnum : public FieldGeneratorBase {
 public:
  RepeatedEnum(const FieldDescriptor* field, const Options& opts,
               MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts, scc),
        opts_(&opts),
        has_cached_size_(field_->is_packed() &&
                         HasGeneratedMethods(field_->file(), opts) &&
                         !should_split()) {}
  ~RepeatedEnum() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  void GeneratePrivateMembers(io::Printer* p) const override {
    if (should_split()) {
      p->Emit(R"cc(
        $pbi$::RawPtr<$pb$::RepeatedField<int>> $name$_;
      )cc");
    } else {
      p->Emit(R"cc(
        $pb$::RepeatedField<int> $name$_;
      )cc");
    }

    if (has_cached_size_) {
      p->Emit(R"cc(
        $pbi$::CachedSize $cached_size_name$;
      )cc");
    }
  }

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

  void GenerateConstexprAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      /*decltype($field_$)*/ {},
    )cc");
    if (has_cached_size_) {
      p->Emit(R"cc(
        /*decltype($cached_size_$)*/ {0},
      )cc");
    }
  }

  void GenerateAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$){arena},
    )cc");
    if (has_cached_size_) {
      // std::atomic has no copy constructor, which prevents explicit aggregate
      // initialization pre-C++17.
      p->Emit(R"cc(
        /*decltype($cached_size_$)*/ {0},
      )cc");
    }
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$){from._internal_$name$()},
    )cc");
    if (has_cached_size_) {
      // std::atomic has no copy constructor.
      p->Emit(R"cc(
        /*decltype($cached_size_$)*/ {0},
      )cc");
    }
  }

  void GenerateMemberConstexprConstructor(io::Printer* p) const override {
    p->Emit("$name$_{}");
    if (has_cached_size_) {
      p->Emit(",\n_$name$_cached_byte_size_{0}");
    }
  }

  void GenerateMemberConstructor(io::Printer* p) const override {
    p->Emit("$name$_{visibility, arena}");
    if (has_cached_size_) {
      p->Emit(",\n_$name$_cached_byte_size_{0}");
    }
  }

  void GenerateMemberCopyConstructor(io::Printer* p) const override {
    p->Emit("$name$_{visibility, arena, from.$name$_}");
    if (has_cached_size_) {
      p->Emit(",\n_$name$_cached_byte_size_{0}");
    }
  }

  void GenerateOneofCopyConstruct(io::Printer* p) const override {
    ABSL_LOG(FATAL) << "Not supported";
  }

  void GenerateCopyConstructorCode(io::Printer* p) const override {
    if (should_split()) {
      p->Emit(R"cc(
        if (!from._internal_$name$().empty()) {
          _internal_mutable_$name$()->MergeFrom(from._internal_$name$());
        }
      )cc");
    }
  }

  void GenerateConstructorCode(io::Printer* p) const override {}

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateByteSize(io::Printer* p) const override;

 private:
  const Options* opts_;
  bool has_cached_size_;
};

void RepeatedEnum::GenerateAccessorDeclarations(io::Printer* p) const {
  auto v = p->WithVars(
      AnnotatedAccessors(field_, {"", "_internal_", "_internal_mutable_"}));
  auto vs =
      p->WithVars(AnnotatedAccessors(field_, {"set_", "add_"}, Semantic::kSet));
  auto vm =
      p->WithVars(AnnotatedAccessors(field_, {"mutable_"}, Semantic::kAlias));

  p->Emit(R"cc(
    public:
    $DEPRECATED$ $Enum$ $name$(int index) const;
    $DEPRECATED$ void $set_name$(int index, $Enum$ value);
    $DEPRECATED$ void $add_name$($Enum$ value);
    $DEPRECATED$ const $pb$::RepeatedField<int>& $name$() const;
    $DEPRECATED$ $pb$::RepeatedField<int>* $nonnull$ $mutable_name$();

    private:
    const $pb$::RepeatedField<int>& $_internal_name$() const;
    $pb$::RepeatedField<int>* $nonnull$ $_internal_mutable_name$();

    public:
  )cc");
}

void RepeatedEnum::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit(R"cc(
    inline $Enum$ $Msg$::$name$(int index) const {
      $WeakDescriptorSelfPin$;
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return static_cast<$Enum$>(_internal_$name_internal$().Get(index));
    }
  )cc");
  p->Emit(R"cc(
    inline void $Msg$::set_$name$(int index, $Enum$ value) {
      $WeakDescriptorSelfPin$;
      $assert_valid$;
      _internal_mutable_$name_internal$()->Set(index, value);
      $annotate_set$
      // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
    }
  )cc");
  p->Emit(R"cc(
    inline void $Msg$::add_$name$($Enum$ value) {
      $WeakDescriptorSelfPin$;
      $assert_valid$;
      $TsanDetectConcurrentMutation$;
      _internal_mutable_$name_internal$()->Add(value);
      $set_hasbit$;
      $annotate_add$
      // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
    }
  )cc");
  p->Emit(R"cc(
    inline const $pb$::RepeatedField<int>& $Msg$::$name$() const
        ABSL_ATTRIBUTE_LIFETIME_BOUND {
      $WeakDescriptorSelfPin$;
      $annotate_list$;
      // @@protoc_insertion_point(field_list:$pkg.Msg.field$)
      return _internal_$name_internal$();
    }
  )cc");
  p->Emit(R"cc(
    inline $pb$::RepeatedField<int>* $nonnull$ $Msg$::mutable_$name$()
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
      inline const $pb$::RepeatedField<int>& $Msg$::_internal_$name_internal$()
          const {
        $TsanDetectConcurrentRead$;
        return *$field_$;
      }
      inline $pb$::RepeatedField<int>* $nonnull$
      $Msg$::_internal_mutable_$name_internal$() {
        $TsanDetectConcurrentRead$;
        $PrepareSplitMessageForWrite$;
        if ($field_$.IsDefault()) {
          $field_$.Set($pb$::Arena::Create<$pb$::RepeatedField<int>>(GetArena()));
        }
        return $field_$.Get();
      }
    )cc");
  } else {
    p->Emit(R"cc(
      inline const $pb$::RepeatedField<int>& $Msg$::_internal_$name_internal$()
          const {
        $TsanDetectConcurrentRead$;
        return $field_$;
      }
      inline $pb$::RepeatedField<int>* $nonnull$
      $Msg$::_internal_mutable_$name_internal$() {
        $TsanDetectConcurrentRead$;
        return &$field_$;
      }
    )cc");
  }
}

void RepeatedEnum::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (field_->is_packed()) {
    p->Emit(
        {
            {"byte_size",
             [&] {
               if (has_cached_size_) {
                 p->Emit(
                     R"cc(::size_t byte_size = this_.$cached_size_$.Get();)cc");
               } else {
                 p->Emit(R"cc(
                   ::size_t byte_size = 0;
                   auto count = static_cast<::size_t>(this_._internal_$name$_size());

                   for (::size_t i = 0; i < count; ++i) {
                     byte_size += ::_pbi::WireFormatLite::EnumSize(
                         this_._internal_$name$().Get(static_cast<int>(i)));
                   }
                 )cc");
               }
             }},
        },
        R"cc(
          {
            $byte_size$;
            if (byte_size > 0) {
              target = stream->WriteEnumPacked(
                  $number$, this_._internal_$name$(), byte_size, target);
            }
          }
        )cc");
    return;
  }
  p->Emit(R"cc(
    for (int i = 0, n = this_._internal_$name$_size(); i < n; ++i) {
      target = stream->EnsureSpace(target);
      target = ::_pbi::WireFormatLite::WriteEnumToArray(
          $number$, static_cast<$Enum$>(this_._internal_$name$().Get(i)),
          target);
    }
  )cc");
}

void RepeatedEnum::GenerateByteSize(io::Printer* p) const {
  if (has_cached_size_) {
    ABSL_CHECK(field_->is_packed());
    p->Emit(R"cc(
      total_size += ::_pbi::WireFormatLite::EnumSizeWithPackedTagSize(
          this_._internal_$name$(), $kTagBytes$, this_.$cached_size_$);
    )cc");
    return;
  }
  p->Emit(
      {
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
        ::size_t data_size =
            ::_pbi::WireFormatLite::EnumSize(this_._internal_$name$());
        ::size_t tag_size = $tag_size$;
        total_size += data_size + tag_size;
      )cc");
}
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSinguarEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<SingularEnum>(desc, options, scc);
}

std::unique_ptr<FieldGeneratorBase> MakeRepeatedEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<RepeatedEnum>(desc, options, scc);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
