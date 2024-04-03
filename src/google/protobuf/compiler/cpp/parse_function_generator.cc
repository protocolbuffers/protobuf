// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/cpp/parse_function_generator.h"

#include <algorithm>
#include <cstddef>
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

}  // namespace

ParseFunctionGenerator::ParseFunctionGenerator(
    const Descriptor* descriptor, int max_has_bit_index,
    const std::vector<int>& has_bit_indices,
    const std::vector<int>& inlined_string_indices, const Options& options,
    MessageSCCAnalyzer* scc_analyzer,
    const absl::flat_hash_map<absl::string_view, std::string>& vars,
    int index_in_file_messages)
    : descriptor_(descriptor),
      scc_analyzer_(scc_analyzer),
      options_(options),
      variables_(vars),
      inlined_string_indices_(inlined_string_indices),
      ordered_fields_(GetOrderedFields(descriptor_, options_)),
      num_hasbits_(max_has_bit_index),
      index_in_file_messages_(index_in_file_messages) {
  std::vector<TailCallTableInfo::FieldOptions> fields;
  fields.reserve(ordered_fields_.size());
  for (size_t i = 0; i < ordered_fields_.size(); ++i) {
    auto* field = ordered_fields_[i];
    fields.push_back({
        field,
        field->index() < has_bit_indices.size()
            ? has_bit_indices[field->index()]
            : -1,
        GetPresenceProbability(field, options_),
        GetLazyStyle(field, options_, scc_analyzer_),
        IsStringInlined(field, options_),
        IsImplicitWeakField(field, options_, scc_analyzer_),
        /* use_direct_tcparser_table */ true,
        ShouldSplit(field, options_),
        field->index() < inlined_string_indices.size()
            ? inlined_string_indices[field->index()]
            : -1,
    });
  }
  tc_table_info_.reset(new TailCallTableInfo(
      descriptor_,
      {/* is_lite */ GetOptimizeFor(descriptor->file(), options_) ==
           FileOptions::LITE_RUNTIME,
       /* uses_codegen */ true, options_.profile_driven_cluster_aux_subtable},
      fields));
  SetCommonMessageDataVariables(descriptor_, &variables_);
  SetUnknownFieldsVariable(descriptor_, options_, &variables_);
  variables_["classname"] = ClassName(descriptor, false);
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

void ParseFunctionGenerator::GenerateDataDecls(io::Printer* p) {
  auto v = p->WithVars(variables_);
  auto field_num_to_entry_table = MakeNumToEntryTable(ordered_fields_);
  p->Emit(
      {
          {"SECTION",
           [&] {
             if (!IsProfileDriven(options_)) return;
             std::string section_name;
             // Since most (>80%) messages are never present, messages that are
             // present are considered hot enough to be clustered together.
             // When using weak descriptors we use unique sections for each
             // table to allow for GC to work. pth/ptl names must be in sync
             // with the linker script.
             if (UsingImplicitWeakDescriptor(descriptor_->file(), options_)) {
               section_name = WeakDescriptorDataSection(
                   IsPresentMessage(descriptor_, options_) ? "pth" : "ptl",
                   descriptor_, index_in_file_messages_, options_);
             } else if (IsPresentMessage(descriptor_, options_)) {
               section_name = "proto_parse_table_hot";
             } else {
               section_name = "proto_parse_table_lukewarm";
             }
             p->Emit({{"section_name", section_name}},
                     "ABSL_ATTRIBUTE_SECTION_VARIABLE($section_name$)");
           }},
          {"table_size_log2", tc_table_info_->table_size_log2},
          {"num_field_entries", ordered_fields_.size()},
          {"num_field_aux", tc_table_info_->aux_entries.size()},
          {"name_table_size",
           FieldNameDataSize(tc_table_info_->field_name_data)},
          {"field_lookup_size", field_num_to_entry_table.size16()},
      },
      R"cc(
        friend class ::$proto_ns$::internal::TcParser;
        $SECTION$
        static const ::$proto_ns$::internal::TcParseTable<
            $table_size_log2$, $num_field_entries$, $num_field_aux$,
            $name_table_size$, $field_lookup_size$>
            _table_;
      )cc");
}

void ParseFunctionGenerator::GenerateDataDefinitions(io::Printer* printer) {
  GenerateTailCallTable(printer);
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

static std::string TcParseFunctionName(internal::TcParseFunction func) {
#define PROTOBUF_TC_PARSE_FUNCTION_X(value) #value,
  static constexpr absl::string_view kNames[] = {
      {}, PROTOBUF_TC_PARSE_FUNCTION_LIST};
#undef PROTOBUF_TC_PARSE_FUNCTION_X
  const int func_index = static_cast<int>(func);
  ABSL_CHECK_GE(func_index, 0);
  ABSL_CHECK_LT(func_index, std::end(kNames) - std::begin(kNames));
  static constexpr absl::string_view ns = "::_pbi::TcParser::";
  return absl::StrCat(ns, kNames[func_index]);
}

void ParseFunctionGenerator::GenerateTailCallTable(io::Printer* printer) {
  Formatter format(printer, variables_);
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
      format("&$1$._instance,\n", DefaultInstanceName(descriptor_, options_));
      if (NeedsPostLoopHandler(descriptor_, options_)) {
        printer->Emit(R"cc(
          &$classname$::PostLoopHandler,
        )cc");
      } else {
        printer->Emit(R"cc(
          nullptr,  // post_loop_handler
        )cc");
      }
      format("$1$,  // fallback\n",
             TcParseFunctionName(tc_table_info_->fallback_function));
      std::vector<const FieldDescriptor*> subtable_fields;
      for (const auto& aux : tc_table_info_->aux_entries) {
        if (aux.type == internal::TailCallTableInfo::kSubTable) {
          subtable_fields.push_back(aux.field);
        }
      }
      const auto* hottest = FindHottestField(subtable_fields, options_);
      // We'll prefetch `to_prefetch->to_prefetch` unconditionally to avoid
      // branches. Set the pointer to itself to avoid nullptr.
      printer->Emit(
          {{"hottest_type_name",
            QualifiedClassName(
                hottest == nullptr ? descriptor_ : hottest->message_type(),
                options_)}},
          // clang-format off
          R"cc(
#ifdef PROTOBUF_PREFETCH_PARSE_TABLE
::_pbi::TcParser::GetTable<$hottest_type_name$>(),  // to_prefetch
#endif  // PROTOBUF_PREFETCH_PARSE_TABLE
          )cc");
      // clang-format on
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
    if (ordered_fields_.empty() &&
        !descriptor_->options().message_set_wire_format()) {
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
              case TailCallTableInfo::kSelfVerifyFunc:
                if (ShouldVerify(descriptor_, options_, scc_analyzer_)) {
                  format("{&InternalVerify},\n");
                } else {
                  format("{},\n");
                }
                break;
              case TailCallTableInfo::kEnumRange:
                format("{$1$, $2$},\n", aux_entry.enum_range.start,
                       aux_entry.enum_range.size);
                break;
              case TailCallTableInfo::kEnumValidator:
                format(
                    "{::_pbi::FieldAuxEnumData{}, $1$_internal_data_},\n",
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
                auto* map_key = aux_entry.field->message_type()->map_key();
                auto* map_value = aux_entry.field->message_type()->map_value();
                const bool validated_enum =
                    map_value->type() == FieldDescriptor::TYPE_ENUM &&
                    !internal::cpp::HasPreservingUnknownEnumSemantics(
                        map_value);
                printer->Emit(
                    {
                        {"field", FieldMemberName(
                                      aux_entry.field,
                                      ShouldSplit(aux_entry.field, options_))},
                        {"strict",
                         utf8_check == internal::cpp::Utf8CheckMode::kStrict},
                        {"verify",
                         utf8_check == internal::cpp::Utf8CheckMode::kVerify},
                        {"validate", validated_enum},
                        {"key_wire", map_key->type()},
                        {"value_wire", map_value->type()},
                    },
                    R"cc(
                      {::_pbi::TcParser::GetMapAuxInfo<
                          decltype($classname$().$field$)>(
                          $strict$, $verify$, $validate$, $key_wire$,
                          $value_wire$)},
                    )cc");
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
    if (auto* nonfield = info.AsNonField()) {
      // Fast slot that is not associated with a field. Eg end group tags.
      format("{$1$, {$2$, $3$}},\n", TcParseFunctionName(nonfield->func),
             nonfield->coded_tag, nonfield->nonfield_info);
    } else if (auto* as_field = info.AsField()) {
      PrintFieldComment(format, as_field->field, options_);
      ABSL_CHECK(!ShouldSplit(as_field->field, options_));

      std::string func_name = TcParseFunctionName(as_field->func);
      if (GetOptimizeFor(as_field->field->file(), options_) ==
          FileOptions::SPEED) {
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
              ", offsetof(",                                      //
              ClassName(as_field->field->containing_type()),      //
              ", ",                                               //
              FieldMemberName(as_field->field, /*split=*/false),  //
              "), ",                                              //
              as_field->hasbit_idx,                               //
              ">()");
        }
      }

      format(
          "{$1$,\n"
          " {$2$, $3$, $4$, PROTOBUF_FIELD_OFFSET($classname$, $5$)}},\n",
          func_name, as_field->coded_tag, as_field->hasbit_idx,
          as_field->aux_idx, FieldMemberName(as_field->field, /*split=*/false));
    } else {
      ABSL_DCHECK(info.is_empty());
      format("{::_pbi::TcParser::MiniParse, {}},\n");
    }
  }
}

void ParseFunctionGenerator::GenerateFieldEntries(Formatter& format) {
  for (const auto& entry : tc_table_info_->field_entries) {
    const FieldDescriptor* field = entry.field;
    PrintFieldComment(format, field, options_);
    format("{");
    if (IsWeak(field, options_)) {
      // Weak fields are handled by the reflection fallback function.
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
      // Use `0|` prefix to eagerly convert the enums to int to avoid enum-enum
      // operations. They are deprecated in C++20.
      format("(0 | $1$)", internal::TypeCardToString(entry.type_card));
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

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
