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

#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/cpp/field.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/descriptor.pb.h"

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
      Sub("assert_valid",
          is_open ? ""
                  : absl::Substitute("assert($0_IsValid(value));", enum_name))
          .WithSuffix(";"),

      {"cached_size_name", MakeVarintCachedSizeName(field)},
      {"cached_size_", MakeVarintCachedSizeFieldName(field, split)},
  };
}

class SingularEnum : public FieldGeneratorBase {
 public:
  SingularEnum(const FieldDescriptor* field, const Options& opts)
      : FieldGeneratorBase(field, opts),
        field_(field),
        opts_(&opts),
        is_oneof_(field->real_containing_oneof() != nullptr) {}
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
      _this->_internal_set_$name$(from._internal_$name$());
    )cc");
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    if (is_oneof_) return;

    p->Emit(R"cc(
      swap($field_$, other->$field_$);
    )cc");
  }

  void GenerateConstructorCode(io::Printer* p) const override {
    if (!is_oneof_) return;
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
          $number$, this->_internal_$name$(), target);
    )cc");
  }

  void GenerateByteSize(io::Printer* p) const override {
    p->Emit(R"cc(
      total_size += $kTagBytes$ +
                    ::_pbi::WireFormatLite::EnumSize(this->_internal_$name$());
    )cc");
  }

  void GenerateConstexprAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      /*decltype($field_$)*/ $kDefault$
    )cc");
  }

  void GenerateAggregateInitializer(io::Printer* p) const override {
    if (ShouldSplit(descriptor_, options_)) {
      p->Emit(R"cc(
        decltype(Impl_::Split::$name$_) { $kDefault$ }
      )cc");
      return;
    }

    p->Emit(R"cc(
      decltype($field_$) { $kDefault$ }
    )cc");
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$) {}
    )cc");
  }

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;

 private:
  const FieldDescriptor* field_;
  const Options* opts_;
  bool is_oneof_;
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
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return _internal_$name$();
    }
    inline void $Msg$::set_$name$($Enum$ value) {
      $PrepareSplitMessageForWrite$ _internal_set_$name$(value);
      $annotate_set$;
      // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
    }
  )cc");

  if (is_oneof_) {
    p->Emit(R"cc(
      inline $Enum$ $Msg$::_internal_$name$() const {
        if ($has_field$) {
          return static_cast<$Enum$>($field_$);
        }
        return static_cast<$Enum$>($kDefault$);
      }
      inline void $Msg$::_internal_set_$name$($Enum$ value) {
        $assert_valid$;
        if ($not_has_field$) {
          clear_$oneof_name$();
          set_has_$name$();
        }
        $field_$ = value;
      }
    )cc");
  } else {
    p->Emit(R"cc(
      inline $Enum$ $Msg$::_internal_$name$() const {
        return static_cast<$Enum$>($field_$);
      }
      inline void $Msg$::_internal_set_$name$($Enum$ value) {
        $assert_valid$;
        $set_hasbit$;
        $field_$ = value;
      }
    )cc");
  }
}

class RepeatedEnum : public FieldGeneratorBase {
 public:
  RepeatedEnum(const FieldDescriptor* field, const Options& opts)
      : FieldGeneratorBase(field, opts),
        field_(field),
        opts_(&opts),
        has_cached_size_(field_->is_packed() &&
                         HasGeneratedMethods(field_->file(), opts)) {}
  ~RepeatedEnum() override = default;

  std::vector<Sub> MakeVars() const override { return Vars(field_, *opts_); }

  void GeneratePrivateMembers(io::Printer* p) const override {
    p->Emit(R"cc(
      $pb$::RepeatedField<int> $name$_;
    )cc");

    if (has_cached_size_) {
      p->Emit(R"cc(
        mutable $pbi$::CachedSize $cached_size_name$;
      )cc");
    }
  }

  void GenerateClearingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _internal_mutable_$name$()->Clear();
    )cc");
  }

  void GenerateMergingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _this->_internal_mutable_$name$()->MergeFrom(from._internal_$name$());
    )cc");
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _internal_mutable_$name$()->InternalSwap(
          other->_internal_mutable_$name$());
    )cc");
  }

  void GenerateDestructorCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _internal_mutable_$name$()->~RepeatedField();
    )cc");
  }

  void GenerateConstexprAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      /*decltype($field_$)*/ {}
    )cc");
    if (has_cached_size_) {
      p->Emit(R"cc(
        , /*decltype($cached_size_$)*/ { 0 }
      )cc");
    }
  }

  void GenerateAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$) { arena }
    )cc");
    if (has_cached_size_) {
      // std::atomic has no copy constructor, which prevents explicit aggregate
      // initialization pre-C++17.
      p->Emit(R"cc(
        , /*decltype($cached_size_$)*/ { 0 }
      )cc");
    }
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    p->Emit(R"cc(
      decltype($field_$) { from._internal_$name$() })cc");
    if (has_cached_size_) {
      // std::atomic has no copy constructor.
      p->Emit(R"cc(
        , /*decltype($cached_size_$)*/ { 0 }
      )cc");
    }
  }

  void GenerateCopyConstructorCode(io::Printer* p) const override {
    ABSL_CHECK(!ShouldSplit(field_, *opts_));
  }

  void GenerateConstructorCode(io::Printer* p) const override {}

  void GenerateAccessorDeclarations(io::Printer* p) const override;
  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;
  void GenerateByteSize(io::Printer* p) const override;

 private:
  const FieldDescriptor* field_;
  const Options* opts_;
  bool has_cached_size_;
};

void RepeatedEnum::GenerateAccessorDeclarations(io::Printer* p) const {
  auto v = p->WithVars(AnnotatedAccessors(
      field_, {"", "_internal_", "_internal_add_", "_internal_mutable_"}));
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
    $DEPRECATED$ $pb$::RepeatedField<int>* $mutable_name$();

    private:
    $Enum$ $_internal_name$(int index) const;
    void $_internal_add_name$($Enum$ value);
    const $pb$::RepeatedField<int>& $_internal_name$() const;
    $pb$::RepeatedField<int>* $_internal_mutable_name$();

    public:
  )cc");
}

void RepeatedEnum::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit(R"cc(
    inline $Enum$ $Msg$::$name$(int index) const {
      $annotate_get$;
      // @@protoc_insertion_point(field_get:$pkg.Msg.field$)
      return _internal_$name$(index);
    }
    inline void $Msg$::set_$name$(int index, $Enum$ value) {
      $assert_valid$;
      _internal_mutable_$name$()->Set(index, value);
      $annotate_set$
      // @@protoc_insertion_point(field_set:$pkg.Msg.field$)
    }
    inline void $Msg$::add_$name$($Enum$ value) {
      _internal_add_$name$(value);
      $annotate_add$
      // @@protoc_insertion_point(field_add:$pkg.Msg.field$)
    }
    inline const $pb$::RepeatedField<int>& $Msg$::$name$() const {
      $annotate_list$;
      // @@protoc_insertion_point(field_list:$pkg.Msg.field$)
      return _internal_$name$();
    }
    inline $pb$::RepeatedField<int>* $Msg$::mutable_$name$() {
      $annotate_mutable_list$;
      // @@protoc_insertion_point(field_mutable_list:$pkg.Msg.field$)
      return _internal_mutable_$name$();
    }
    inline $Enum$ $Msg$::_internal_$name$(int index) const {
      return static_cast<$Enum$>(_internal_$name$().Get(index));
    }
    inline void $Msg$::_internal_add_$name$($Enum$ value) {
      $assert_valid$;
      _internal_mutable_$name$()->Add(value);
    }
    inline const $pb$::RepeatedField<int>& $Msg$::_internal_$name$() const {
      return $field_$;
    }
    inline $pb$::RepeatedField<int>* $Msg$::_internal_mutable_$name$() {
      return &$field_$;
    }
  )cc");
}

void RepeatedEnum::GenerateSerializeWithCachedSizesToArray(
    io::Printer* p) const {
  if (field_->is_packed()) {
    p->Emit(R"cc(
      {
        int byte_size = $cached_size_$.Get();
        if (byte_size > 0) {
          target = stream->WriteEnumPacked($number$, _internal_$name$(),
                                           byte_size, target);
        }
      }
    )cc");
    return;
  }
  p->Emit(R"cc(
    for (int i = 0, n = this->_internal_$name$_size(); i < n; ++i) {
      target = stream->EnsureSpace(target);
      target = ::_pbi::WireFormatLite::WriteEnumToArray(
          $number$, this->_internal_$name$(i), target);
    }
  )cc");
}

void RepeatedEnum::GenerateByteSize(io::Printer* p) const {
  p->Emit(
      {
          {"add_to_size",
           [&] {
             if (!field_->is_packed()) {
               p->Emit(R"cc(
                 total_size += std::size_t{$kTagBytes$} * count;
               )cc");
               return;
             }

             p->Emit(R"cc(
               if (data_size > 0) {
                 total_size += $kTagBytes$;
                 total_size += ::_pbi::WireFormatLite::Int32Size(
                     static_cast<int32_t>(data_size));
               }
               $cached_size_$.Set(::_pbi::ToCachedSize(data_size));
             )cc");
           }},
      },
      R"cc(
        {
          std::size_t data_size = 0;
          auto count = static_cast<std::size_t>(this->_internal_$name$_size());

          for (std::size_t i = 0; i < count; ++i) {
            data_size += ::_pbi::WireFormatLite::EnumSize(
                this->_internal_$name$(static_cast<int>(i)));
          }
          total_size += data_size;
          $add_to_size$;
        }
      )cc");
}
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeSinguarEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<SingularEnum>(desc, options);
}

std::unique_ptr<FieldGeneratorBase> MakeRepeatedEnumGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return absl::make_unique<RepeatedEnum>(desc, options);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
