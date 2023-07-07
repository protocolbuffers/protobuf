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

#include "google/protobuf/compiler/cpp/parse_function_generator.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_tctable_gen.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/wire_format.h"
#include "google/protobuf/wire_format_lite.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {
using internal::TailCallTableInfo;
using internal::cpp::Utf8CheckMode;
using google::protobuf::internal::WireFormat;
using google::protobuf::internal::WireFormatLite;

bool HasWeakFields(const Descriptor* descriptor) {
  for (int i = 0; i < descriptor->field_count(); i++) {
    if (descriptor->field(i)->options().weak()) {
      return true;
    }
  }
  return false;
}
bool UseDirectTcParserTable(const FieldDescriptor* field,
                            const Options& options) {
  if (field->cpp_type() != field->CPPTYPE_MESSAGE) return false;
  auto* m = field->message_type();
  return !m->options().message_set_wire_format() &&
         m->file()->options().optimize_for() != FileOptions::CODE_SIZE &&
         !HasSimpleBaseClass(m, options) && !HasTracker(m, options) &&
         !HasWeakFields(m)
      ;  // NOLINT(whitespace/semicolon)
}

std::vector<const FieldDescriptor*> GetOrderedFields(
    const Descriptor* descriptor, const Options& options) {
  std::vector<const FieldDescriptor*> ordered_fields;
  for (auto field : FieldRange(descriptor)) {
    ordered_fields.push_back(field);
  }
  std::sort(ordered_fields.begin(), ordered_fields.end(),
            [](const FieldDescriptor* a, const FieldDescriptor* b) {
              return a->number() < b->number();
            });
  return ordered_fields;
}

bool HasInternalAccessors(const FieldOptions::CType ctype) {
  return ctype == FieldOptions::STRING || ctype == FieldOptions::CORD;
}

}  // namespace

class ParseFunctionGenerator::GeneratedOptionProvider final
    : public internal::TailCallTableInfo::OptionProvider {
 public:
  explicit GeneratedOptionProvider(ParseFunctionGenerator* gen) : gen_(gen) {}
  TailCallTableInfo::PerFieldOptions GetForField(
      const FieldDescriptor* field) const final {
    const auto verify_flag = [&] {
      if (IsEagerlyVerifiedLazy(field, gen_->options_, gen_->scc_analyzer_))
        return internal::field_layout::kTvEager;
      if (IsLazilyVerifiedLazy(field, gen_->options_))
        return internal::field_layout::kTvLazy;
      return internal::field_layout::TransformValidation{};
    };
    return {
        GetPresenceProbability(field, gen_->options_),
        verify_flag(),
        IsStringInlined(field, gen_->options_),
        IsImplicitWeakField(field, gen_->options_, gen_->scc_analyzer_),
        UseDirectTcParserTable(field, gen_->options_),
        GetOptimizeFor(field->file(), gen_->options_) ==
            FileOptions::LITE_RUNTIME,
        ShouldSplit(field, gen_->options_),
        /* uses_codegen */ true,
    };
  }

 private:
  ParseFunctionGenerator* gen_;
};

ParseFunctionGenerator::ParseFunctionGenerator(
    const Descriptor* descriptor, int max_has_bit_index,
    const std::vector<int>& has_bit_indices,
    const std::vector<int>& inlined_string_indices, const Options& options,
    MessageSCCAnalyzer* scc_analyzer,
    const absl::flat_hash_map<absl::string_view, std::string>& vars)
    : descriptor_(descriptor),
      scc_analyzer_(scc_analyzer),
      options_(options),
      variables_(vars),
      inlined_string_indices_(inlined_string_indices),
      ordered_fields_(GetOrderedFields(descriptor_, options_)),
      num_hasbits_(max_has_bit_index) {
  if (should_generate_tctable()) {
    tc_table_info_.reset(new TailCallTableInfo(
        descriptor_, ordered_fields_, GeneratedOptionProvider(this),
        has_bit_indices, inlined_string_indices));
  }
  SetCommonMessageDataVariables(descriptor_, &variables_);
  SetUnknownFieldsVariable(descriptor_, options_, &variables_);
  variables_["classname"] = ClassName(descriptor, false);
}

void ParseFunctionGenerator::GenerateMethodDecls(io::Printer* printer) {
  if (HasWeakFields(descriptor_)) {
    // We use the reflection based one.
    ABSL_CHECK(HasDescriptorMethods(descriptor_->file(), options_));
    return;
  }
  Formatter format(printer, variables_);
  format(
      "const char* _InternalParse(const char* ptr, "
      "::$proto_ns$::internal::ParseContext* ctx) final;\n");
}

void ParseFunctionGenerator::GenerateMethodImpls(io::Printer* printer) {
  Formatter format(printer, variables_);
  bool need_parse_function = true;
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    need_parse_function = false;
    format(
        "const char* $classname$::_InternalParse(const char* ptr,\n"
        "                  ::_pbi::ParseContext* ctx) {\n"
        "$annotate_deserialize$");
    if (!options_.unverified_lazy_message_sets &&
        ShouldVerify(descriptor_, options_, scc_analyzer_)) {
      format(
          "  ctx->set_lazy_eager_verify_func(&$classname$::InternalVerify);\n");
    }
    format(
        "  return $extensions$.ParseMessageSet(ptr, \n"
        "      internal_default_instance(), &_internal_metadata_, ctx);\n"
        "}\n");
  }
  if (HasWeakFields(descriptor_)) {
    // We use the reflection based one.
    ABSL_CHECK(HasDescriptorMethods(descriptor_->file(), options_));
    need_parse_function = false;
  }
  if (!should_generate_tctable()) {
    if (need_parse_function) {
      GenerateLoopingParseFunction(format);
    }
    return;
  }
  if (need_parse_function) {
    GenerateTailcallParseFunction(format);
  }
}

bool ParseFunctionGenerator::should_generate_tctable() const {
  if (options_.tctable_mode == Options::kTCTableNever) {
    return false;
  }
  if (HasSimpleBaseClass(descriptor_, options_) || HasWeakFields(descriptor_)) {
    return false;
  }
  return true;
}

void ParseFunctionGenerator::GenerateTailcallParseFunction(Formatter& format) {
  ABSL_CHECK(should_generate_tctable());

  // Generate an `_InternalParse` that starts the tail-calling loop.
  format(
      "const char* $classname$::_InternalParse(\n"
      "    const char* ptr, ::_pbi::ParseContext* ctx) {\n"
      "$annotate_deserialize$"
      "  ptr = ::_pbi::TcParser::ParseLoop(this, ptr, ctx, "
      "&_table_.header);\n");
  format(
      "  return ptr;\n"
      "}\n\n");
}

struct SkipEntry16 {
  uint16_t skipmap;
  uint16_t field_entry_offset;
};
struct SkipEntryBlock {
  uint32_t first_fnum;
  std::vector<SkipEntry16> entries;
};
struct NumToEntryTable {
  uint32_t skipmap32;  // for fields #1 - #32
  std::vector<SkipEntryBlock> blocks;
  // Compute the number of uint16_t required to represent this table.
  int size16() const {
    int size = 2;  // for the termination field#
    for (const auto& block : blocks) {
      // 2 for the field#, 1 for a count of skip entries, 2 for each entry.
      size += 3 + block.entries.size() * 2;
    }
    return size;
  }
};

static NumToEntryTable MakeNumToEntryTable(
    const std::vector<const FieldDescriptor*>& field_descriptors);

static int FieldNameDataSize(const std::vector<uint8_t>& data) {
  // We add a +1 here to allow for a NUL termination character. It makes the
  // codegen nicer.
  return data.empty() ? 0 : data.size() + 1;
}

void ParseFunctionGenerator::GenerateDataDecls(io::Printer* printer) {
  if (!should_generate_tctable()) {
    return;
  }
  Formatter format(printer, variables_);
  auto field_num_to_entry_table = MakeNumToEntryTable(ordered_fields_);
  format(
      "friend class ::$proto_ns$::internal::TcParser;\n"
      "static const ::$proto_ns$::internal::"
      "TcParseTable<$1$, $2$, $3$, $4$, $5$> _table_;\n",
      tc_table_info_->table_size_log2, ordered_fields_.size(),
      tc_table_info_->aux_entries.size(),
      FieldNameDataSize(tc_table_info_->field_name_data),
      field_num_to_entry_table.size16());
}

void ParseFunctionGenerator::GenerateDataDefinitions(io::Printer* printer) {
  if (!should_generate_tctable()) {
    return;
  }
  Formatter format(printer, variables_);
  GenerateTailCallTable(format);
}

void ParseFunctionGenerator::GenerateLoopingParseFunction(Formatter& format) {
  format(
      "const char* $classname$::_InternalParse(const char* ptr, "
      "::_pbi::ParseContext* ctx) {\n"
      "$annotate_deserialize$"
      "#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure\n");
  format.Indent();
  format.Set("msg", "");
  format.Set("this", "this");
  int hasbits_size = 0;
  if (num_hasbits_ > 0) {
    hasbits_size = (num_hasbits_ + 31) / 32;
  }
  // For now only optimize small hasbits.
  if (hasbits_size != 1) hasbits_size = 0;
  if (hasbits_size) {
    format("_Internal::HasBits has_bits{};\n");
    format.Set("has_bits", "has_bits");
  } else {
    format.Set("has_bits", "_impl_._has_bits_");
  }
  format.Set("next_tag", "continue");
  format("while (!ctx->Done(&ptr)) {\n");
  format.Indent();

  format(
      "::uint32_t tag;\n"
      "ptr = ::_pbi::ReadTag(ptr, &tag);\n");
  GenerateParseIterationBody(format, descriptor_, ordered_fields_);

  format.Outdent();
  format("}  // while\n");

  format.Outdent();
  format("message_done:\n");
  if (hasbits_size) format("  _impl_._has_bits_.Or(has_bits);\n");

  format(
      "  return ptr;\n"
      "failure:\n"
      "  ptr = nullptr;\n"
      "  goto message_done;\n"
      "#undef CHK_\n"
      "}\n");
}

static NumToEntryTable MakeNumToEntryTable(
    const std::vector<const FieldDescriptor*>& field_descriptors) {
  NumToEntryTable num_to_entry_table;
  num_to_entry_table.skipmap32 = static_cast<uint32_t>(-1);

  // skip_entry_block is the current block of SkipEntries that we're
  // appending to.  cur_block_first_fnum is the number of the first
  // field represented by the block.
  uint16_t field_entry_index = 0;
  uint16_t N = field_descriptors.size();
  // First, handle field numbers 1-32, which affect only the initial
  // skipmap32 and don't generate additional skip-entry blocks.
  for (; field_entry_index != N; ++field_entry_index) {
    auto* field_descriptor = field_descriptors[field_entry_index];
    if (field_descriptor->number() > 32) break;
    auto skipmap32_index = field_descriptor->number() - 1;
    num_to_entry_table.skipmap32 -= 1 << skipmap32_index;
  }
  // If all the field numbers were less than or equal to 32, we will have
  // no further entries to process, and we are already done.
  if (field_entry_index == N) return num_to_entry_table;

  SkipEntryBlock* block = nullptr;
  bool start_new_block = true;
  // To determine sparseness, track the field number corresponding to
  // the start of the most recent skip entry.
  uint32_t last_skip_entry_start = 0;
  for (; field_entry_index != N; ++field_entry_index) {
    auto* field_descriptor = field_descriptors[field_entry_index];
    uint32_t fnum = field_descriptor->number();
    ABSL_CHECK_GT(fnum, last_skip_entry_start);
    if (start_new_block == false) {
      // If the next field number is within 15 of the last_skip_entry_start, we
      // continue writing just to that entry.  If it's between 16 and 31 more,
      // then we just extend the current block by one. If it's more than 31
      // more, we have to add empty skip entries in order to continue using the
      // existing block.  Obviously it's just 32 more, it doesn't make sense to
      // start a whole new block, since new blocks mean having to write out
      // their starting field number, which is 32 bits, as well as the size of
      // the additional block, which is 16... while an empty SkipEntry16 only
      // costs 32 bits.  So if it was 48 more, it's a slight space win; we save
      // 16 bits, but probably at the cost of slower run time.  We're choosing
      // 96 for now.
      if (fnum - last_skip_entry_start > 96) start_new_block = true;
    }
    if (start_new_block) {
      num_to_entry_table.blocks.push_back(SkipEntryBlock{fnum});
      block = &num_to_entry_table.blocks.back();
      start_new_block = false;
    }

    auto skip_entry_num = (fnum - block->first_fnum) / 16;
    auto skip_entry_index = (fnum - block->first_fnum) % 16;
    while (skip_entry_num >= block->entries.size())
      block->entries.push_back({0xFFFF, field_entry_index});
    block->entries[skip_entry_num].skipmap -= 1 << (skip_entry_index);

    last_skip_entry_start = fnum - skip_entry_index;
  }
  return num_to_entry_table;
}

void ParseFunctionGenerator::GenerateTailCallTable(Formatter& format) {
  ABSL_CHECK(should_generate_tctable());
  // All entries without a fast-path parsing function need a fallback.
  std::string fallback = "::_pbi::TcParser::GenericFallback";
  if (GetOptimizeFor(descriptor_->file(), options_) ==
      FileOptions::LITE_RUNTIME) {
    absl::StrAppend(&fallback, "Lite");
  }

  // For simplicity and speed, the table is not covering all proto
  // configurations. This model uses a fallback to cover all situations that
  // the table can't accommodate, together with unknown fields or extensions.
  // These are number of fields over 32, fields with 3 or more tag bytes,
  // maps, weak fields, lazy, more than 1 extension range. In the cases
  // the table is sufficient we can use a generic routine, that just handles
  // unknown fields and potentially an extension range.
  auto field_num_to_entry_table = MakeNumToEntryTable(ordered_fields_);
  format(
      "$1$ ::_pbi::TcParseTable<$2$, $3$, $4$, $5$, $6$> "
      "$classname$::_table_ = "
      "{\n",
      // FileDescriptorProto's table must be constant initialized. For MSVC this
      // means using `constexpr`. However, we can't use `constexpr` for all
      // tables because it breaks when crossing DLL boundaries.
      // FileDescriptorProto is safe from this.
      IsFileDescriptorProto(descriptor_->file(), options_)
          ? "constexpr"
          : "PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1\nconst",
      tc_table_info_->table_size_log2, ordered_fields_.size(),
      tc_table_info_->aux_entries.size(),
      FieldNameDataSize(tc_table_info_->field_name_data),
      field_num_to_entry_table.size16());
  {
    auto table_scope = format.ScopedIndent();
    format("{\n");
    {
      auto header_scope = format.ScopedIndent();
      if (num_hasbits_ > 0 || IsMapEntryMessage(descriptor_)) {
        format("PROTOBUF_FIELD_OFFSET($classname$, _impl_._has_bits_),\n");
      } else {
        format("0,  // no _has_bits_\n");
      }
      if (descriptor_->extension_range_count() != 0) {
        format("PROTOBUF_FIELD_OFFSET($classname$, $extensions$),\n");
      } else {
        format("0, // no _extensions_\n");
      }
      format("$1$, $2$,  // max_field_number, fast_idx_mask\n",
             (ordered_fields_.empty() ? 0 : ordered_fields_.back()->number()),
             (((1 << tc_table_info_->table_size_log2) - 1) << 3));
      format(
          "offsetof(decltype(_table_), field_lookup_table),\n"
          "$1$,  // skipmap\n",
          field_num_to_entry_table.skipmap32);
      if (ordered_fields_.empty()) {
        format(
            "offsetof(decltype(_table_), field_names),  // no field_entries\n");
      } else {
        format("offsetof(decltype(_table_), field_entries),\n");
      }

      format(
          "$1$,  // num_field_entries\n"
          "$2$,  // num_aux_entries\n",
          ordered_fields_.size(), tc_table_info_->aux_entries.size());
      if (tc_table_info_->aux_entries.empty()) {
        format(
            "offsetof(decltype(_table_), field_names),  // no aux_entries\n");
      } else {
        format("offsetof(decltype(_table_), aux_entries),\n");
      }
      format(
          "&$1$._instance,\n"
          "$2$,  // fallback\n"
          "",
          DefaultInstanceName(descriptor_, options_), fallback);
    }
    format("}, {{\n");
    {
      // fast_entries[]
      auto fast_scope = format.ScopedIndent();
      GenerateFastFieldEntries(format);
    }
    format("}}, {{\n");
    {
      // field_lookup_table[]
      auto field_lookup_scope = format.ScopedIndent();
      int line_entries = 0;
      for (int i = 0, N = field_num_to_entry_table.blocks.size(); i < N; ++i) {
        SkipEntryBlock& entry_block = field_num_to_entry_table.blocks[i];
        format("$1$, $2$, $3$,\n", entry_block.first_fnum & 65535,
               entry_block.first_fnum / 65536, entry_block.entries.size());
        for (auto se16 : entry_block.entries) {
          if (line_entries == 0) {
            format("$1$, $2$,", se16.skipmap, se16.field_entry_offset);
            ++line_entries;
          } else if (line_entries < 5) {
            format(" $1$, $2$,", se16.skipmap, se16.field_entry_offset);
            ++line_entries;
          } else {
            format(" $1$, $2$,\n", se16.skipmap, se16.field_entry_offset);
            line_entries = 0;
          }
        }
      }
      if (line_entries) format("\n");
      format("65535, 65535\n");
    }
    if (ordered_fields_.empty()) {
      ABSL_DLOG_IF(FATAL, !tc_table_info_->aux_entries.empty())
          << "Invalid message: " << descriptor_->full_name() << " has "
          << tc_table_info_->aux_entries.size()
          << " auxiliary field entries, but no fields";
      format(
          "}},\n"
          "// no field_entries, or aux_entries\n"
          "{{\n");
    } else {
      format("}}, {{\n");
      {
        // field_entries[]
        auto field_scope = format.ScopedIndent();
        GenerateFieldEntries(format);
      }
      if (tc_table_info_->aux_entries.empty()) {
        format(
            "}},\n"
            "// no aux_entries\n"
            "{{\n");
      } else {
        format("}}, {{\n");
        {
          // aux_entries[]
          auto aux_scope = format.ScopedIndent();
          for (const auto& aux_entry : tc_table_info_->aux_entries) {
            switch (aux_entry.type) {
              case TailCallTableInfo::kNothing:
                format("{},\n");
                break;
              case TailCallTableInfo::kInlinedStringDonatedOffset:
                format(
                    "{_fl::Offset{offsetof($classname$, "
                    "_impl_._inlined_string_donated_)}},\n");
                break;
              case TailCallTableInfo::kSplitOffset:
                format(
                    "{_fl::Offset{offsetof($classname$, _impl_._split_)}},\n");
                break;
              case TailCallTableInfo::kSplitSizeof:
                format("{_fl::Offset{sizeof($classname$::Impl_::Split)}},\n");
                break;
              case TailCallTableInfo::kSubMessage:
                format("{::_pbi::FieldAuxDefaultMessage{}, &$1$},\n",
                       QualifiedDefaultInstanceName(
                           aux_entry.field->message_type(), options_));
                break;
              case TailCallTableInfo::kSubTable:
                format("{::_pbi::TcParser::GetTable<$1$>()},\n",
                       QualifiedClassName(aux_entry.field->message_type(),
                                          options_));
                break;
              case TailCallTableInfo::kSubMessageWeak:
                format("{::_pbi::FieldAuxDefaultMessage{}, &$1$},\n",
                       QualifiedDefaultInstancePtr(
                           aux_entry.field->message_type(), options_));
                break;
              case TailCallTableInfo::kMessageVerifyFunc:
                format("{$1$::InternalVerify},\n",
                       QualifiedClassName(aux_entry.field->message_type(),
                                          options_));
                break;
              case TailCallTableInfo::kEnumRange:
                format("{$1$, $2$},\n", aux_entry.enum_range.start,
                       aux_entry.enum_range.size);
                break;
              case TailCallTableInfo::kEnumValidator:
                format(
                    "{$1$_IsValid},\n",
                    QualifiedClassName(aux_entry.field->enum_type(), options_));
                break;
              case TailCallTableInfo::kNumericOffset:
                format("{_fl::Offset{$1$}},\n", aux_entry.offset);
                break;
              case TailCallTableInfo::kMapAuxInfo: {
                auto utf8_check = internal::cpp::GetUtf8CheckMode(
                    aux_entry.field,
                    GetOptimizeFor(aux_entry.field->file(), options_) ==
                        FileOptions::LITE_RUNTIME);
                auto* map_value = aux_entry.field->message_type()->map_value();
                const bool validated_enum =
                    map_value->type() == FieldDescriptor::TYPE_ENUM &&
                    !internal::cpp::HasPreservingUnknownEnumSemantics(
                        map_value);
                format(
                    "{::_pbi::TcParser::GetMapAuxInfo<decltype($classname$("
                    ").$1$)>($2$, $3$, $4$)},\n",
                    FieldMemberName(aux_entry.field,
                                    ShouldSplit(aux_entry.field, options_)),
                    utf8_check == internal::cpp::Utf8CheckMode::kStrict,
                    utf8_check == internal::cpp::Utf8CheckMode::kVerify,
                    validated_enum);
                break;
              }
              case TailCallTableInfo::kCreateInArena:
                format("{::_pbi::TcParser::CreateInArenaStorageCb<$1$>},\n",
                       QualifiedClassName(aux_entry.desc, options_));
                break;
            }
          }
        }
        format("}}, {{\n");
      }
    }  // ordered_fields_.empty()
      {
        // field_names[]
        auto field_name_scope = format.ScopedIndent();
        GenerateFieldNames(format);
      }
      format("}},\n");
  }
  format("};\n\n");  // _table_
}

void ParseFunctionGenerator::GenerateFastFieldEntries(Formatter& format) {
  for (const auto& info : tc_table_info_->fast_path_fields) {
    if (info.field != nullptr) {
        PrintFieldComment(format, info.field, options_);
    }
    if (info.func_name.empty()) {
      format("{::_pbi::TcParser::MiniParse, {}},\n");
    } else if (info.field == nullptr) {
      // Fast slot that is not associated with a field. Eg end group tags.
      format("{$1$, {$2$, $3$}},\n", info.func_name, info.coded_tag,
             info.nonfield_info);
    } else {
      ABSL_CHECK(!ShouldSplit(info.field, options_));

      std::string func_name = info.func_name;
      if (GetOptimizeFor(info.field->file(), options_) == FileOptions::SPEED) {
        // For 1-byte tags we have a more optimized version of the varint parser
        // that can hardcode the offset and has bit.
        if (absl::EndsWith(func_name, "V8S1") ||
            absl::EndsWith(func_name, "V32S1") ||
            absl::EndsWith(func_name, "V64S1")) {
          std::string field_type = absl::EndsWith(func_name, "V8S1") ? "bool"
                                   : absl::EndsWith(func_name, "V32S1")
                                       ? "::uint32_t"
                                       : "::uint64_t";
          func_name = absl::StrCat(
              "::_pbi::TcParser::SingularVarintNoZag1<", field_type,
              ", offsetof(",                                 //
              ClassName(info.field->containing_type()),      //
              ", ",                                          //
              FieldMemberName(info.field, /*split=*/false),  //
              "), ",                                         //
              info.hasbit_idx,                               //
              ">()");
        }
      }

      format(
          "{$1$,\n"
          " {$2$, $3$, $4$, PROTOBUF_FIELD_OFFSET($classname$, $5$)}},\n",
          func_name, info.coded_tag, info.hasbit_idx, info.aux_idx,
          FieldMemberName(info.field, /*split=*/false));
    }
  }
}

static void FormatFieldKind(Formatter& format,
                            const TailCallTableInfo::FieldEntryInfo& entry) {
  // In here we convert the runtime value of entry.type_card back into a
  // sequence of literal enum labels. We use the mnenonic labels for nicer
  // codegen.
  namespace fl = internal::field_layout;
  const uint16_t type_card = entry.type_card;
  const int rep_index = (type_card & fl::kRepMask) >> fl::kRepShift;
  const int tv_index = (type_card & fl::kTvMask) >> fl::kTvShift;

  // Use `0|` prefix to eagerly convert the enums to int to avoid enum-enum
  // operations. They are deprecated in C++20.
  format("(0 | ");
  static constexpr const char* kFieldCardNames[] = {"Singular", "Optional",
                                                    "Repeated", "Oneof"};
  static_assert((fl::kFcSingular >> fl::kFcShift) == 0, "");
  static_assert((fl::kFcOptional >> fl::kFcShift) == 1, "");
  static_assert((fl::kFcRepeated >> fl::kFcShift) == 2, "");
  static_assert((fl::kFcOneof >> fl::kFcShift) == 3, "");

  format("::_fl::kFc$1$",
         kFieldCardNames[(type_card & fl::kFcMask) >> fl::kFcShift]);

#define PROTOBUF_INTERNAL_TYPE_CARD_CASE(x) \
  case fl::k##x:                            \
    format(" | ::_fl::k" #x);               \
    break

  switch (type_card & fl::kFkMask) {
    case fl::kFkString: {
      switch (type_card & ~fl::kFcMask & ~fl::kRepMask & ~fl::kSplitMask) {
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Bytes);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(RawString);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Utf8String);
        default:
          ABSL_LOG(FATAL) << "Unknown type_card: 0x" << type_card;
      }

      static constexpr const char* kRepNames[] = {"AString", "IString", "Cord",
                                                  "SPiece", "SString"};
      static_assert((fl::kRepAString >> fl::kRepShift) == 0, "");
      static_assert((fl::kRepIString >> fl::kRepShift) == 1, "");
      static_assert((fl::kRepCord >> fl::kRepShift) == 2, "");
      static_assert((fl::kRepSPiece >> fl::kRepShift) == 3, "");
      static_assert((fl::kRepSString >> fl::kRepShift) == 4, "");

      format(" | ::_fl::kRep$1$", kRepNames[rep_index]);
      break;
    }

    case fl::kFkMessage: {
      format(" | ::_fl::kMessage");

      static constexpr const char* kRepNames[] = {nullptr, "Group", "Lazy"};
      static_assert((fl::kRepGroup >> fl::kRepShift) == 1, "");
      static_assert((fl::kRepLazy >> fl::kRepShift) == 2, "");

      if (auto* rep = kRepNames[rep_index]) {
        format(" | ::_fl::kRep$1$", rep);
      }

      static constexpr const char* kXFormNames[2][4] = {
          {nullptr, "Default", "Table", "WeakPtr"}, {nullptr, "Eager", "Lazy"}};

      static_assert((fl::kTvDefault >> fl::kTvShift) == 1, "");
      static_assert((fl::kTvTable >> fl::kTvShift) == 2, "");
      static_assert((fl::kTvWeakPtr >> fl::kTvShift) == 3, "");
      static_assert((fl::kTvEager >> fl::kTvShift) == 1, "");
      static_assert((fl::kTvLazy >> fl::kTvShift) == 2, "");

      if (auto* xform = kXFormNames[rep_index == 2][tv_index]) {
        format(" | ::_fl::kTv$1$", xform);
      }
      break;
    }

    case fl::kFkMap:
      format(" | ::_fl::kMap");
      break;

    case fl::kFkNone:
      break;

    case fl::kFkVarint:
    case fl::kFkPackedVarint:
    case fl::kFkFixed:
    case fl::kFkPackedFixed: {
      switch (type_card & ~fl::kFcMask & ~fl::kSplitMask) {
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Bool);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Fixed32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(UInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(SFixed32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Int32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(SInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Float);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Enum);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(EnumRange);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(OpenEnum);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Fixed64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(UInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(SFixed64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Int64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(SInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(Double);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedBool);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedFixed32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedUInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedSFixed32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedSInt32);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedFloat);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedEnum);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedEnumRange);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedOpenEnum);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedFixed64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedUInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedSFixed64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedSInt64);
        PROTOBUF_INTERNAL_TYPE_CARD_CASE(PackedDouble);
        default:
          ABSL_LOG(FATAL) << "Unknown type_card: 0x" << type_card;
      }
    }
  }

  if (type_card & fl::kSplitMask) {
    format(" | ::_fl::kSplitTrue");
  }

#undef PROTOBUF_INTERNAL_TYPE_CARD_CASE

  format(")");
}

void ParseFunctionGenerator::GenerateFieldEntries(Formatter& format) {
  for (const auto& entry : tc_table_info_->field_entries) {
    const FieldDescriptor* field = entry.field;
    PrintFieldComment(format, field, options_);
    format("{");
    if (IsWeak(field, options_)) {
      // Weak fields are handled by the generated fallback function.
      // (These are handled by legacy Google-internal logic.)
      format("/* weak */ 0, 0, 0, 0");
    } else {
      const OneofDescriptor* oneof = field->real_containing_oneof();
      bool split = ShouldSplit(field, options_);
      if (split) {
        format("PROTOBUF_FIELD_OFFSET($classname$::Impl_::Split, $1$), ",
               absl::StrCat(FieldName(field), "_"));
      } else {
        format("PROTOBUF_FIELD_OFFSET($classname$, $1$), ",
               FieldMemberName(field, /*cold=*/false));
      }
      if (oneof) {
        format("_Internal::kOneofCaseOffset + $1$, ", 4 * oneof->index());
      } else if (num_hasbits_ > 0 || IsMapEntryMessage(descriptor_)) {
        if (entry.hasbit_idx >= 0) {
          format("_Internal::kHasBitsOffset + $1$, ", entry.hasbit_idx);
        } else {
          format("$1$, ", entry.hasbit_idx);
        }
      } else {
        format("0, ");
      }
      format("$1$,\n ", entry.aux_idx);
      FormatFieldKind(format, entry);
    }
    format("},\n");
  }
}

void ParseFunctionGenerator::GenerateFieldNames(Formatter& format) {
  if (tc_table_info_->field_name_data.empty()) {
    // No names to output.
    return;
  }

  // We could just output the bytes directly, but we want it to look better than
  // that in the source code. Also, it is more efficient for compilation time to
  // have a literal string than an initializer list of chars.

  const int total_sizes =
      static_cast<int>(((tc_table_info_->field_entries.size() + 1) + 7) & ~7);
  const uint8_t* p = tc_table_info_->field_name_data.data();
  const uint8_t* sizes = p;
  const uint8_t* sizes_end = sizes + total_sizes;

  // First print all the sizes as octal
  format("\"");
  for (int i = 0; i < total_sizes; ++i) {
    int size = *p++;
    int octal_size = ((size >> 6) & 3) * 100 +  //
                     ((size >> 3) & 7) * 10 +   //
                     ((size >> 0) & 7);
    format("\\$1$", octal_size);
  }
  format("\"\n");

  // Then print each name in a line of its own
  for (; sizes < sizes_end; p += *sizes++) {
    if (*sizes != 0) format("\"$1$\"\n", std::string(p, p + *sizes));
  }
}

void ParseFunctionGenerator::GenerateArenaString(Formatter& format,
                                                 const FieldDescriptor* field) {
  if (internal::cpp::HasHasbit(field)) {
    format("_Internal::set_has_$1$(&$has_bits$);\n", FieldName(field));
  }
  format(
      "if (arena != nullptr) {\n"
      "  ptr = ctx->ReadArenaString(ptr, &$msg$$field$, arena");
  if (IsStringInlined(field, options_)) {
    ABSL_DCHECK(!inlined_string_indices_.empty());
    int inlined_string_index = inlined_string_indices_[field->index()];
    ABSL_DCHECK_GT(inlined_string_index, 0);
    format(", &$msg$$inlined_string_donated_array$[0], $1$, $this$",
           inlined_string_index);
  } else {
    ABSL_DCHECK(field->default_value_string().empty());
  }
  format(
      ");\n"
      "} else {\n"
      "  ptr = ::_pbi::InlineGreedyStringParser("
      "$msg$$field$.MutableNoCopy(nullptr), ptr, ctx);\n"
      "}\n"
      "const std::string* str = &$msg$$field$.Get(); (void)str;\n");
}

void ParseFunctionGenerator::GenerateStrings(Formatter& format,
                                             const FieldDescriptor* field,
                                             bool check_utf8) {
  FieldOptions::CType ctype = FieldOptions::STRING;

  if (!field->is_repeated() && field->type() == FieldDescriptor::TYPE_BYTES &&
      field->options().ctype() == FieldOptions::CORD) {
    ctype = FieldOptions::CORD;
  } else {
    if (!options_.opensource_runtime) {
      // Open source doesn't support other ctypes;
      ctype = field->options().ctype();
    }
  }

  if (!field->is_repeated() && !options_.opensource_runtime &&
      GetOptimizeFor(field->file(), options_) != FileOptions::LITE_RUNTIME &&
      // For now only use arena string for strings with empty defaults.
      field->default_value_string().empty() &&
      !field->real_containing_oneof() && ctype == FieldOptions::STRING) {
    GenerateArenaString(format, field);
  } else {
    std::string parser_name;
    switch (ctype) {
      case FieldOptions::STRING:
        parser_name = "GreedyStringParser";
        break;
      case FieldOptions::CORD:
        parser_name = "CordParser";
        break;
      case FieldOptions::STRING_PIECE:
        parser_name = "StringPieceParser";
        break;
    }
    // Repeated cord doesn't have _internal_mutable_$name$(), unlike other
    // repeated fields.
    if (field->options().ctype() == FieldOptions::CORD &&
        field->is_repeated() && !field->is_packable()) {
      format("auto str = $msg$$field$.Add();\n");
    } else {
      format("auto str = $msg$$1$mutable_$name$()$2$;\n",
             HasInternalAccessors(ctype) ? "_internal_" : "",
             field->is_repeated() && !field->is_packable() ? "->Add()" : "");
    }
    format("ptr = ::_pbi::Inline$1$(str, ptr, ctx);\n", parser_name);
  }
  // It is intentionally placed before VerifyUTF8 because it doesn't make sense
  // to verify UTF8 when we already know parsing failed.
  format("CHK_(ptr);\n");
  if (!check_utf8) return;  // return if this is a bytes field
  auto level = internal::cpp::GetUtf8CheckMode(
      field,
      GetOptimizeFor(field->file(), options_) == FileOptions::LITE_RUNTIME);
  switch (level) {
    case Utf8CheckMode::kNone:
      return;
    case Utf8CheckMode::kVerify:
      format("#ifndef NDEBUG\n");
      break;
    case Utf8CheckMode::kStrict:
      format("CHK_(");
      break;
  }
  std::string field_name;
  field_name = "nullptr";
  if (HasDescriptorMethods(field->file(), options_)) {
    field_name = absl::StrCat("\"", field->full_name(), "\"");
  }
  format("::_pbi::VerifyUTF8(str, $1$)", field_name);
  switch (level) {
    case Utf8CheckMode::kNone:
      return;
    case Utf8CheckMode::kVerify:
      format(
          ";\n"
          "#endif  // !NDEBUG\n");
      break;
    case Utf8CheckMode::kStrict:
      format(");\n");
      break;
  }
}

void ParseFunctionGenerator::GenerateLengthDelim(Formatter& format,
                                                 const FieldDescriptor* field) {
  if (field->is_packable()) {
    if (field->type() == FieldDescriptor::TYPE_ENUM &&
        !internal::cpp::HasPreservingUnknownEnumSemantics(field)) {
      std::string enum_type = QualifiedClassName(field->enum_type(), options_);
      format(
          "ptr = "
          "::$proto_ns$::internal::Packed$1$Parser<$unknown_fields_type$>("
          "$msg$_internal_mutable_$name$(), ptr, ctx, $2$_IsValid, "
          "&$msg$_internal_metadata_, $3$);\n",
          DeclaredTypeMethodName(field->type()), enum_type, field->number());
    } else {
      format(
          "ptr = ::$proto_ns$::internal::Packed$1$Parser("
          "$msg$_internal_mutable_$name$(), ptr, ctx);\n",
          DeclaredTypeMethodName(field->type()));
    }
    format("CHK_(ptr);\n");
  } else {
    auto field_type = field->type();
    switch (field_type) {
      case FieldDescriptor::TYPE_STRING:
        GenerateStrings(format, field, true /* utf8 */);
        break;
      case FieldDescriptor::TYPE_BYTES:
        GenerateStrings(format, field, false /* utf8 */);
        break;
      case FieldDescriptor::TYPE_MESSAGE: {
        if (field->is_map()) {
          const FieldDescriptor* val = field->message_type()->map_value();
          ABSL_CHECK(val);
          if (val->type() == FieldDescriptor::TYPE_ENUM &&
              !internal::cpp::HasPreservingUnknownEnumSemantics(val)) {
            format(
                "auto object = "
                "::$proto_ns$::internal::InitEnumParseWrapper<"
                "$unknown_fields_type$>(&$msg$$field$, $1$_IsValid, "
                "$2$, &$msg$_internal_metadata_);\n"
                "ptr = ctx->ParseMessage(&object, ptr);\n",
                QualifiedClassName(val->enum_type(), options_),
                field->number());
          } else {
            format("ptr = ctx->ParseMessage(&$msg$$field$, ptr);\n");
          }
        } else if (IsLazy(field, options_, scc_analyzer_)) {
          bool eager_verify =
              IsEagerlyVerifiedLazy(field, options_, scc_analyzer_);
          if (ShouldVerify(descriptor_, options_, scc_analyzer_)) {
            format(
                "ctx->set_lazy_eager_verify_func($1$);\n",
                eager_verify
                    ? absl::StrCat("&", ClassName(field->message_type(), true),
                                   "::InternalVerify")
                    : "nullptr");
          }
          if (field->real_containing_oneof()) {
            format(
                "if ($msg$$1$_case() != k$2$) {\n"
                "  $msg$clear_$1$();\n"
                "  $msg$$field$ = ::$proto_ns$::Arena::CreateMessage<\n"
                "      ::$proto_ns$::internal::LazyField>("
                "$msg$GetArenaForAllocation());\n"
                "  $msg$set_has_$name$();\n"
                "}\n"
                "auto* lazy_field = $msg$$field$;\n",
                field->containing_oneof()->name(),
                UnderscoresToCamelCase(field->name(), true));
          } else if (internal::cpp::HasHasbit(field)) {
            format(
                "_Internal::set_has_$name$(&$has_bits$);\n"
                "auto* lazy_field = &$msg$$field$;\n");
          } else {
            format("auto* lazy_field = &$msg$$field$;\n");
          }
          format(
              "::$proto_ns$::internal::LazyFieldParseHelper<\n"
              "  ::$proto_ns$::internal::LazyField> parse_helper(\n"
              "    $1$::default_instance(),\n"
              "    $msg$GetArenaForAllocation(),\n"
              "    ::google::protobuf::internal::LazyVerifyOption::$2$,\n"
              "    lazy_field);\n"
              "ptr = ctx->ParseMessage(&parse_helper, ptr);\n",
              FieldMessageTypeName(field, options_),
              eager_verify ? "kEager" : "kLazy");
          if (ShouldVerify(descriptor_, options_, scc_analyzer_) &&
              eager_verify) {
            format("ctx->set_lazy_eager_verify_func(nullptr);\n");
          }
        } else if (IsImplicitWeakField(field, options_, scc_analyzer_)) {
          if (!field->is_repeated()) {
            format(
                "ptr = ctx->ParseMessage(_Internal::mutable_$name$($this$), "
                "ptr);\n");
          } else {
            format(
                "ptr = ctx->ParseMessage($msg$$field$.AddWeak("
                "reinterpret_cast<const ::$proto_ns$::MessageLite*>($1$ptr_)"
                "), ptr);\n",
                QualifiedDefaultInstanceName(field->message_type(), options_));
          }
        } else if (IsWeak(field, options_)) {
          format(
              "{\n"
              "  auto* default_ = &reinterpret_cast<const Message&>($1$);\n"
              "  ptr = ctx->ParseMessage($msg$$weak_field_map$.MutableMessage("
              "$2$, default_), ptr);\n"
              "}\n",
              QualifiedDefaultInstanceName(field->message_type(), options_),
              field->number());
        } else {
          format(
              "ptr = ctx->ParseMessage($msg$_internal_$mutable_field$(), "
              "ptr);\n");
        }
        format("CHK_(ptr);\n");
        break;
      }
      default:
        ABSL_LOG(FATAL) << "Illegal combination for length delimited wiretype "
                        << " filed type is " << field->type();
    }
  }
}

static bool ShouldRepeat(const FieldDescriptor* descriptor,
                         WireFormatLite::WireType wiretype) {
  constexpr int kMaxTwoByteFieldNumber = 16 * 128;
  return descriptor->number() < kMaxTwoByteFieldNumber &&
         descriptor->is_repeated() &&
         (!descriptor->is_packable() ||
          wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
}

void ParseFunctionGenerator::GenerateFieldBody(
    Formatter& format, WireFormatLite::WireType wiretype,
    const FieldDescriptor* field) {
  Formatter::SaveState formatter_state(&format);
  format.AddMap(
      {{"name", FieldName(field)},
       {"primitive_type", PrimitiveTypeName(options_, field->cpp_type())}});
  if (field->is_repeated()) {
    const std::string add_field =
        absl::StrCat("mutable_", FieldName(field), "()->Add");
    format.AddMap({{"put_field", add_field}, {"mutable_field", add_field}});
  } else {
    format.AddMap(
        {{"put_field", absl::StrCat("set_", FieldName(field))},
         {"mutable_field", absl::StrCat("mutable_", FieldName(field))}});
  }
  uint32_t tag = WireFormatLite::MakeTag(field->number(), wiretype);
  switch (wiretype) {
    case WireFormatLite::WIRETYPE_VARINT: {
      if (field->type() == FieldDescriptor::TYPE_ENUM) {
        format.Set("enum_type",
                   QualifiedClassName(field->enum_type(), options_));
        format(
            "$int32$ val = ::$proto_ns$::internal::ReadVarint32(&ptr);\n"
            "CHK_(ptr);\n");
        if (!internal::cpp::HasPreservingUnknownEnumSemantics(field)) {
          format(
              "if "
              "(PROTOBUF_PREDICT_TRUE($enum_type$_IsValid(static_cast<int>(val)"
              "))) {\n");
          format.Indent();
        }
        format("$msg$_internal_$put_field$(static_cast<$enum_type$>(val));\n");
        if (!internal::cpp::HasPreservingUnknownEnumSemantics(field)) {
          format.Outdent();
          format(
              "} else {\n"
              "  ::$proto_ns$::internal::WriteVarint("
              "$1$, val, $msg$mutable_unknown_fields());\n"
              "}\n",
              field->number());
        }
      } else {
        std::string size = (field->type() == FieldDescriptor::TYPE_INT32 ||
                            field->type() == FieldDescriptor::TYPE_SINT32 ||
                            field->type() == FieldDescriptor::TYPE_UINT32)
                               ? "32"
                               : "64";
        std::string zigzag;
        if ((field->type() == FieldDescriptor::TYPE_SINT32 ||
             field->type() == FieldDescriptor::TYPE_SINT64)) {
          zigzag = "ZigZag";
        }
        if (field->is_repeated() || field->real_containing_oneof()) {
          format(
              "$msg$_internal_$put_field$("
              "::$proto_ns$::internal::ReadVarint$1$$2$(&ptr));\n"
              "CHK_(ptr);\n",
              zigzag, size);
        } else {
          if (internal::cpp::HasHasbit(field)) {
            format("_Internal::set_has_$name$(&$has_bits$);\n");
          }
          format(
              "$msg$$field$ = ::$proto_ns$::internal::ReadVarint$1$$2$(&ptr);\n"
              "CHK_(ptr);\n",
              zigzag, size);
        }
      }
      break;
    }
    case WireFormatLite::WIRETYPE_FIXED32:
    case WireFormatLite::WIRETYPE_FIXED64: {
      if (field->is_repeated() || field->real_containing_oneof()) {
        format(
            "$msg$_internal_$put_field$("
            "::$proto_ns$::internal::UnalignedLoad<$primitive_type$>(ptr));\n"
            "ptr += sizeof($primitive_type$);\n");
      } else {
        if (internal::cpp::HasHasbit(field)) {
          format("_Internal::set_has_$name$(&$has_bits$);\n");
        }
        format(
            "$msg$$field$ = "
            "::$proto_ns$::internal::UnalignedLoad<$primitive_type$>(ptr);\n"
            "ptr += sizeof($primitive_type$);\n");
      }
      break;
    }
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED: {
      GenerateLengthDelim(format, field);
      break;
    }
    case WireFormatLite::WIRETYPE_START_GROUP: {
      format(
          "ptr = ctx->ParseGroup($msg$_internal_$mutable_field$(), ptr, $1$);\n"
          "CHK_(ptr);\n",
          tag);
      break;
    }
    case WireFormatLite::WIRETYPE_END_GROUP: {
      ABSL_LOG(FATAL) << "Can't have end group field\n";
      break;
    }
  }  // switch (wire_type)
}

// Returns the tag for this field and in case of repeated packable fields,
// sets a fallback tag in fallback_tag_ptr.
static uint32_t ExpectedTag(const FieldDescriptor* field,
                            uint32_t* fallback_tag_ptr) {
  uint32_t expected_tag;
  if (field->is_packable()) {
    auto expected_wiretype = WireFormat::WireTypeForFieldType(field->type());
    expected_tag = WireFormatLite::MakeTag(field->number(), expected_wiretype);
    ABSL_CHECK(expected_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
    auto fallback_wiretype = WireFormatLite::WIRETYPE_LENGTH_DELIMITED;
    uint32_t fallback_tag =
        WireFormatLite::MakeTag(field->number(), fallback_wiretype);

    if (field->is_packed()) std::swap(expected_tag, fallback_tag);
    *fallback_tag_ptr = fallback_tag;
  } else {
    auto expected_wiretype = WireFormat::WireTypeForField(field);
    expected_tag = WireFormatLite::MakeTag(field->number(), expected_wiretype);
  }
  return expected_tag;
}

// These variables are used by the generated parse iteration, and must already
// be defined in the generated code:
// - `const char* ptr`: the input buffer.
// - `ParseContext* ctx`: the associated context for `ptr`.
// - implicit `this`: i.e., we must be in a non-static member function.
//
// The macro `CHK_(x)` must be defined. It should return an error condition if
// the macro parameter is false.
//
// Whenever an END_GROUP tag was read, or tag 0 was read, the generated code
// branches to the label `message_done`.
//
// These formatter variables are used:
// - `next_tag`: a single statement to begin parsing the next tag.
//
// At the end of the generated code, the enclosing function should proceed to
// parse the next tag in the stream.
void ParseFunctionGenerator::GenerateParseIterationBody(
    Formatter& format, const Descriptor* descriptor,
    const std::vector<const FieldDescriptor*>& fields) {
  if (!fields.empty()) {
    GenerateFieldSwitch(format, fields);
    // Each field `case` only considers field number. Field numbers that are
    // not defined in the message, or tags with an incompatible wire type, are
    // considered "unusual" cases. They will be handled by the logic below.
    format.Outdent();
    format("handle_unusual:\n");
    format.Indent();
  }

  // Unusual/extension/unknown case:
  format(
      "if ((tag == 0) || ((tag & 7) == 4)) {\n"
      "  CHK_(ptr);\n"
      "  ctx->SetLastTag(tag);\n"
      "  goto message_done;\n"
      "}\n");
  if (IsMapEntryMessage(descriptor)) {
    format("$next_tag$;\n");
  } else {
    if (descriptor->extension_range_count() > 0) {
      format("if (");
      for (int i = 0; i < descriptor->extension_range_count(); i++) {
        const Descriptor::ExtensionRange* range =
            descriptor->extension_range(i);
        if (i > 0) format(" ||\n    ");

        uint32_t start_tag = WireFormatLite::MakeTag(
            range->start_number(), static_cast<WireFormatLite::WireType>(0));
        uint32_t end_tag = WireFormatLite::MakeTag(
            range->end_number(), static_cast<WireFormatLite::WireType>(0));

        if (range->end_number() > FieldDescriptor::kMaxNumber) {
          format("($1$u <= tag)", start_tag);
        } else {
          format("($1$u <= tag && tag < $2$u)", start_tag, end_tag);
        }
      }
      format(
          ") {\n"
          "  ptr = $msg$$extensions$.ParseField(tag, ptr, "
          "internal_default_instance(), &$msg$_internal_metadata_, ctx);\n"
          "  CHK_(ptr != nullptr);\n"
          "  $next_tag$;\n"
          "}\n");
    }
    format(
        "ptr = UnknownFieldParse(\n"
        "    tag,\n"
        "    $msg$_internal_metadata_.mutable_unknown_fields<"
        "$unknown_fields_type$>(),\n"
        "    ptr, ctx);\n"
        "CHK_(ptr != nullptr);\n");
  }
}

void ParseFunctionGenerator::GenerateFieldSwitch(
    Formatter& format, const std::vector<const FieldDescriptor*>& fields) {
  format("switch (tag >> 3) {\n");
  format.Indent();

  for (const auto* field : fields) {
    bool cold = ShouldSplit(field, options_);
    format.Set("field", FieldMemberName(field, cold));
    PrintFieldComment(format, field, options_);
    format("case $1$:\n", field->number());
    format.Indent();
    uint32_t fallback_tag = 0;
    uint32_t expected_tag = ExpectedTag(field, &fallback_tag);
    format("if (PROTOBUF_PREDICT_TRUE(static_cast<$uint8$>(tag) == $1$)) {\n",
           expected_tag & 0xFF);
    format.Indent();
    if (cold) {
      format("$msg$PrepareSplitMessageForWrite();\n");
    }
    auto wiretype = WireFormatLite::GetTagWireType(expected_tag);
    uint32_t tag = WireFormatLite::MakeTag(field->number(), wiretype);
    int tag_size = io::CodedOutputStream::VarintSize32(tag);
    bool is_repeat = ShouldRepeat(field, wiretype);
    if (is_repeat) {
      format(
          "ptr -= $1$;\n"
          "do {\n"
          "  ptr += $1$;\n",
          tag_size);
      format.Indent();
    }
    GenerateFieldBody(format, wiretype, field);
    if (is_repeat) {
      format.Outdent();
      format(
          "  if (!ctx->DataAvailable(ptr)) break;\n"
          "} while (::$proto_ns$::internal::ExpectTag<$1$>(ptr));\n",
          tag);
    }
    format.Outdent();
    if (fallback_tag) {
      format("} else if (static_cast<$uint8$>(tag) == $1$) {\n",
             fallback_tag & 0xFF);
      format.Indent();
      GenerateFieldBody(format, WireFormatLite::GetTagWireType(fallback_tag),
                        field);
      format.Outdent();
    }
    format(
        "} else {\n"
        "  goto handle_unusual;\n"
        "}\n"
        "$next_tag$;\n");
    format.Outdent();
  }  // for loop over ordered fields

  format(
      "default:\n"
      "  goto handle_unusual;\n");
  format.Outdent();
  format("}  // switch\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
