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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "google/protobuf/compiler/cpp/field_generators/generators.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {
namespace {
using Sub = ::google::protobuf::io::Printer::Sub;

class MapField : public FieldGeneratorBase {
 public:
  MapField(const FieldDescriptor* field, const Options& opts,
           MessageSCCAnalyzer* scc)
      : FieldGeneratorBase(field, opts),
        field_(field),
        opts_(&opts),
        has_required_(scc->HasRequiredFields(field->message_type())) {}
  ~MapField() override = default;

  std::vector<Sub> MakeVars() const override;

  void GeneratePrivateMembers(io::Printer* p) const override {
    p->Emit(
        R"cc(
          $pbi$::MapField$Lite$<$Entry$, $Key$, $Value$,
                                $pbi$::WireFormatLite::TYPE_$KEY$,
                                $pbi$::WireFormatLite::TYPE_$VALUE$>
              $name$_;
        )cc");
  }

  void GenerateAccessorDeclarations(io::Printer* p) const override {
    auto v = p->WithVars(AnnotatedAccessors(
        field_, {"", "mutable_", "_internal_", "_internal_mutable_"}));

    p->Emit(R"cc(
      public:
      $DEPRECATED$ const $pb$::Map<$Key$, $Value$>& $name$() const;
      $DEPRECATED$ $pb$::Map<$Key$, $Value$>* $mutable_name$();

      private:
      const $pb$::Map<$Key$, $Value$>& $_internal_name$() const;
      $pb$::Map<$Key$, $Value$>* $_internal_mutable_name$();

      public:
    )cc");
  }

  void GenerateClearingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      $field_$.Clear();
    )cc");
  }

  void GenerateMergingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      _this->$field_$.MergeFrom(from.$field_$);
    )cc");
  }

  void GenerateSwappingCode(io::Printer* p) const override {
    p->Emit(R"cc(
      $field_$.InternalSwap(&other->$field_$);
    )cc");
  }

  void GenerateConstructorCode(io::Printer* p) const override {}

  void GenerateCopyConstructorCode(io::Printer* p) const override {
    p->Emit(R"cc(
      $field_$.MergeFrom(from.$field_$);
    )cc");
  }

  void GenerateByteSize(io::Printer* p) const override {
    p->Emit(R"cc(
      total_size +=
          $kTagBytes$ * $pbi$::FromIntSize(this->_internal_$name$_size());
      for (const auto& e : this->_internal_$name$()) {
        total_size += $Entry$::Funcs::ByteSizeLong(e.first, e.second);
      }
    )cc");
  }

  void GenerateIsInitialized(io::Printer* p) const override {
    if (!has_required_) return;
    p->Emit(R"cc(
      if (!$pbi$::AllAreInitialized($field_$)) return false;
    )cc");
  }

  void GenerateConstexprAggregateInitializer(io::Printer* p) const override {
    if (HasDescriptorMethods(field_->file(), *opts_)) {
      p->Emit(R"cc(
        /*decltype($field_$)*/ { ::_pbi::ConstantInitialized() }
      )cc");
      return;
    }

    p->Emit(R"cc(
      /*decltype($field_$)*/ {}
    )cc");
  }

  void GenerateCopyAggregateInitializer(io::Printer* p) const override {
    // MapField has no move constructor, which prevents explicit aggregate
    // initialization pre-C++17.
    p->Emit(R"cc(
      /*decltype($field_$)*/ {}
    )cc");
  }

  void GenerateAggregateInitializer(io::Printer* p) const override {
    if (ShouldSplit(field_, *opts_)) {
      p->Emit(R"cc(
        /*decltype($Msg$::Split::$name$_)*/ {
          ::_pbi::ArenaInitialized(), arena
        }
      )cc");
      return;
    }

    // MapField has no move constructor.
    p->Emit(R"cc(
      /*decltype($field_$)*/ { ::_pbi::ArenaInitialized(), arena }
    )cc");
  }

  void GenerateDestructorCode(io::Printer* p) const override {
    if (ShouldSplit(field_, *opts_)) {
      p->Emit(R"cc(
        $cached_split_ptr$->$name$_.Destruct();
        $cached_split_ptr$->$name$_.~MapField$Lite$();
      )cc");
      return;
    }

    p->Emit(R"cc(
      $field_$.Destruct();
      $field_$.~MapField$Lite$();
    )cc");
  }

  void GenerateArenaDestructorCode(io::Printer* p) const override {
    if (NeedsArenaDestructor() == ArenaDtorNeeds::kNone) {
      return;
    }

    p->Emit(R"cc(
      _this->$field_$.Destruct();
    )cc");
  }

  ArenaDtorNeeds NeedsArenaDestructor() const override {
    return HasDescriptorMethods(field_->file(), *opts_)
               ? ArenaDtorNeeds::kRequired
               : ArenaDtorNeeds::kNone;
  }

  void GenerateInlineAccessorDefinitions(io::Printer* p) const override;
  void GenerateSerializeWithCachedSizesToArray(io::Printer* p) const override;

 private:
  const FieldDescriptor* field_;
  const Options* opts_;
  bool has_required_;
};

std::vector<Sub> MapField::MakeVars() const {
  auto* key = field_->message_type()->map_key();
  auto* val = field_->message_type()->map_value();

  std::string val_type;
  switch (val->cpp_type()) {
    case FieldDescriptor::CPPTYPE_MESSAGE:
      val_type = FieldMessageTypeName(val, *opts_);
      break;
    case FieldDescriptor::CPPTYPE_ENUM:
      val_type = ClassName(val->enum_type(), true);
      break;
    default:
      val_type = PrimitiveTypeName(*opts_, val->cpp_type());
      break;
  }

  return {
      {"Key", PrimitiveTypeName(*opts_, key->cpp_type())},
      {"Value", std::move(val_type)},
      {"Entry", ClassName(field_->message_type(), false)},

      {"KEY", absl::AsciiStrToUpper(DeclaredTypeMethodName(key->type()))},
      {"VALUE", absl::AsciiStrToUpper(DeclaredTypeMethodName(val->type()))},

      {"Lite", !HasDescriptorMethods(field_->file(), *opts_) ? "Lite" : ""},
  };
}

void MapField::GenerateInlineAccessorDefinitions(io::Printer* p) const {
  p->Emit(R"cc(
    inline const $pb$::Map<$Key$, $Value$>& $Msg$::_internal_$name$() const {
      return $field_$.GetMap();
    }
    inline const $pb$::Map<$Key$, $Value$>& $Msg$::$name$() const {
      $annotate_get$;
      // @@protoc_insertion_point(field_map:pkg.Msg.field)
      return _internal_$name$();
    }
    inline $pb$::Map<$Key$, $Value$>* $Msg$::_internal_mutable_$name$() {
      $PrepareSplitMessageForWrite$;
      return $field_$.MutableMap();
    }
    inline $pb$::Map<$Key$, $Value$>* $Msg$::mutable_$name$() {
      $annotate_mutable$;
      // @@protoc_insertion_point(field_mutable_map:pkg.Msg.field)
      return _internal_mutable_$name$();
    }
  )cc");
}

void MapField::GenerateSerializeWithCachedSizesToArray(io::Printer* p) const {
  const FieldDescriptor* key_field = field_->message_type()->map_key();
  const FieldDescriptor* value_field = field_->message_type()->map_value();
  bool string_key = key_field->type() == FieldDescriptor::TYPE_STRING;
  bool string_value = value_field->type() == FieldDescriptor::TYPE_STRING;

  p->Emit(
      {{"key_check",
        [&] {
          if (!string_key) return;
          GenerateUtf8CheckCodeForString(
              p, key_field, *opts_, false,
              "entry.first.data(), static_cast<int>(entry.first.length()),");
        }},
       {"value_check",
        [&] {
          if (!string_value) return;
          GenerateUtf8CheckCodeForString(
              p, value_field, *opts_, false,
              "entry.second.data(), "
              "static_cast<int>(entry.second.length()),");
        }},
       {"Kind", string_key ? "Ptr" : "Flat"}},
      R"cc(
        if (!_internal_$name$().empty()) {
          using MapType = ::_pb::Map<$Key$, $Value$>;
          using WireHelper = $Entry$::Funcs;
          auto check_utf8 = [](const MapType::value_type& entry) {
            //~ `entry` may be unused when GetUtf8CheckMode evaluates to
            //~ kNone.
            (void)entry;
            $key_check$;
            $value_check$;
          };

          const auto& map_field = this->_internal_$name$();
          if (stream->IsSerializationDeterministic() && map_field.size() > 1) {
            for (const auto& entry : ::_pbi::MapSorter$Kind$<MapType>(map_field)) {
              target = WireHelper::InternalSerialize(
                  $number$, entry.first, entry.second, target, stream);
              check_utf8(entry);
            }
          } else {
            for (const auto& entry : map_field) {
              target = WireHelper::InternalSerialize(
                  $number$, entry.first, entry.second, target, stream);
              check_utf8(entry);
            }
          }
        }
      )cc");
}
}  // namespace

std::unique_ptr<FieldGeneratorBase> MakeMapGenerator(
    const FieldDescriptor* desc, const Options& options,
    MessageSCCAnalyzer* scc) {
  return std::make_unique<MapField>(desc, options, scc);
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
