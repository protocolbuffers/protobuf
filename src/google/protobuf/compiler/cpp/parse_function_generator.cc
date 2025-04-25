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
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/compiler/cpp/helpers.h"
#include "google/protobuf/compiler/cpp/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_tctable_gen.h"
#include "google/protobuf/generated_message_tctable_impl.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

using internal::TailCallTableInfo;
using internal::cpp::Utf8CheckMode;

std::vector<const FieldDescriptor*> GetOrderedFields(
    const Descriptor* descriptor) {
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

ParseFunctionGenerator::ParseFunctionGenerator(
    const Descriptor* descriptor, int max_has_bit_index,
    absl::Span<const int> has_bit_indices,
    absl::Span<const int> inlined_string_indices, const Options& options,
    MessageSCCAnalyzer* scc_analyzer,
    const absl::flat_hash_map<absl::string_view, std::string>& vars,
    int index_in_file_messages)
    : descriptor_(descriptor),
      scc_analyzer_(scc_analyzer),
      options_(options),
      variables_(vars),
      // Copy the absl::Span into a vector owned by the class.
      inlined_string_indices_(inlined_string_indices.begin(),
                              inlined_string_indices.end()),
      ordered_fields_(GetOrderedFields(descriptor_)),
      num_hasbits_(max_has_bit_index),
      index_in_file_messages_(index_in_file_messages) {
  auto fields =
      BuildFieldOptions(descriptor_, ordered_fields_, options_, scc_analyzer_,
                        has_bit_indices, inlined_string_indices_);
  tc_table_info_ = std::make_unique<TailCallTableInfo>(
      BuildTcTableInfoFromDescriptor(descriptor_, options_, fields));
  SetCommonMessageDataVariables(descriptor_, &variables_);
  SetUnknownFieldsVariable(descriptor_, options_, &variables_);
  variables_["classname"] = ClassName(descriptor, false);
}

std::vector<internal::TailCallTableInfo::FieldOptions>
ParseFunctionGenerator::BuildFieldOptions(
    const Descriptor* descriptor,
    absl::Span<const FieldDescriptor* const> ordered_fields,
    const Options& options, MessageSCCAnalyzer* scc_analyzer,
    absl::Span<const int> has_bit_indices,
    absl::Span<const int> inlined_string_indices) {
  std::vector<TailCallTableInfo::FieldOptions> fields;
  fields.reserve(ordered_fields.size());
  for (size_t i = 0; i < ordered_fields.size(); ++i) {
    auto* field = ordered_fields[i];
    ABSL_CHECK_GE(field->index(), 0);
    size_t index = static_cast<size_t>(field->index());
    fields.push_back({
        field,
        index < has_bit_indices.size() ? has_bit_indices[index] : -1,
        GetPresenceProbability(field, options)
            .value_or(kUnknownPresenceProbability),
        GetLazyStyle(field, options, scc_analyzer),
        IsStringInlined(field, options),
        IsImplicitWeakField(field, options, scc_analyzer),
        /* use_direct_tcparser_table */ true,
        ShouldSplit(field, options),
        index < inlined_string_indices.size() ? inlined_string_indices[index]
                                              : -1,
        IsMicroString(field, options),
    });
  }
  return fields;
}

TailCallTableInfo ParseFunctionGenerator::BuildTcTableInfoFromDescriptor(
    const Descriptor* descriptor, const Options& options,
    absl::Span<const TailCallTableInfo::FieldOptions> field_options) {
  TailCallTableInfo tc_table_info(
      descriptor,
      TailCallTableInfo::MessageOptions{
          /* is_lite */ GetOptimizeFor(descriptor->file(), options) ==
              FileOptions::LITE_RUNTIME,
          /* uses_codegen */ true},
      field_options);
  return tc_table_info;
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
    absl::Span<const FieldDescriptor* const> field_descriptors);

static int FieldNameDataSize(absl::Span<const uint8_t> data) {
  // We add a +1 here to allow for a NUL termination character. It makes the
  // codegen nicer.
  return data.empty() ? 0 : static_cast<int>(data.size()) + 1;
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
        friend class $pbi$::TcParser;
        $SECTION$
        static const $pbi$::TcParseTable<$table_size_log2$, $num_field_entries$,
                                         $num_field_aux$, $name_table_size$,
                                         $field_lookup_size$>
            _table_;
      )cc");
}

void ParseFunctionGenerator::GenerateDataDefinitions(io::Printer* printer) {
  GenerateTailCallTable(printer);
}

static NumToEntryTable MakeNumToEntryTable(
    absl::Span<const FieldDescriptor* const> field_descriptors) {
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
    uint32_t fnum = static_cast<uint32_t>(field_descriptor->number());
    ABSL_CHECK_GT(fnum, last_skip_entry_start);
    if (start_new_block == false) {
      // If the next field number is within 15 of the last_skip_entry_start, we
      // continue writing just to that entry.  If it's between 16 and 31 more,
      // then we just extend the current block by one. If it's greater than 31
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

    ABSL_DCHECK(block != nullptr);
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

void ParseFunctionGenerator::GenerateTailCallTable(io::Printer* p) {
  auto v = p->WithVars(variables_);
  // For simplicity and speed, the table is not covering all proto
  // configurations. This model uses a fallback to cover all situations that
  // the table can't accommodate, together with unknown fields or extensions.
  // These are number of fields over 32, fields with 3 or more tag bytes,
  // maps, weak fields, lazy, more than 1 extension range. In the cases
  // the table is sufficient we can use a generic routine, that just handles
  // unknown fields and potentially an extension range.
  NumToEntryTable field_num_to_entry_table =
      MakeNumToEntryTable(ordered_fields_);

  auto GenerateTableBase = [&] {
    p->Emit(
        {{"has_bits_offset",
          [&] {
            if (num_hasbits_ > 0 || IsMapEntryMessage(descriptor_)) {
              p->Emit(
                  "PROTOBUF_FIELD_OFFSET($classname$, _impl_._has_bits_),\n");
            } else {
              p->Emit("0,  // no _has_bits_\n");
            }
          }},
         {"extension_offset",
          [&] {
            if (descriptor_->extension_range_count() != 0) {
              p->Emit("PROTOBUF_FIELD_OFFSET($classname$, $extensions$),\n");
            } else {
              p->Emit("0, // no _extensions_\n");
            }
          }},
         {"max_field_number",
          ordered_fields_.empty() ? 0 : ordered_fields_.back()->number()},
         {"fast_idx_mask", (((1 << tc_table_info_->table_size_log2) - 1) << 3)},
         {"skipmap32", field_num_to_entry_table.skipmap32},
         {"field_entries_offset",
          [&] {
            if (ordered_fields_.empty()) {
              p->Emit(R"cc(
                offsetof(decltype(_table_), field_names),  // no field_entries
              )cc");
            } else {
              p->Emit(R"cc(
                offsetof(decltype(_table_), field_entries),
              )cc");
            }
          }},
         {"num_field_entries", ordered_fields_.size()},
         {"num_aux_entries", tc_table_info_->aux_entries.size()},
         {"aux_offset",
          [&] {
            if (tc_table_info_->aux_entries.empty()) {
              p->Emit(R"cc(
                offsetof(decltype(_table_), field_names),  // no aux_entries
              )cc");
            } else {
              p->Emit(R"cc(
                offsetof(decltype(_table_), aux_entries),
              )cc");
            }
          }},
         {"class_data", [&] { p->Emit("$classname$_class_data_.base(),\n"); }},
         {"post_loop_handler",
          [&] {
            if (NeedsPostLoopHandler(descriptor_, options_)) {
              p->Emit("&$classname$::PostLoopHandler,\n");
            } else {
              p->Emit("nullptr,  // post_loop_handler\n");
            }
          }},
         {"fallback", TcParseFunctionName(tc_table_info_->fallback_function)},
         {"to_prefetch",
          [&] {
            std::vector<const FieldDescriptor*> subtable_fields;
            for (const auto& aux : tc_table_info_->aux_entries) {
              if (aux.type == internal::TailCallTableInfo::kSubTable) {
                subtable_fields.push_back(aux.field);
              }
            }
            const auto* hottest = FindHottestField(subtable_fields, options_);
            p->Emit(
                {{"hot_type", QualifiedClassName(hottest == nullptr
                                                     ? descriptor_
                                                     : hottest->message_type(),
                                                 options_)}},
                R"cc(
#ifdef PROTOBUF_PREFETCH_PARSE_TABLE
                  ::_pbi::TcParser::GetTable<$hot_type$>(),  // to_prefetch
#endif  // PROTOBUF_PREFETCH_PARSE_TABLE)cc");
          }}},
        // clang-format off
        R"cc(
        $has_bits_offset$,
        $extension_offset$,
        $max_field_number$, $fast_idx_mask$,  // max_field_number, fast_idx_mask
        offsetof(decltype(_table_), field_lookup_table),
        $skipmap32$,  // skipmap
        $field_entries_offset$,
        $num_field_entries$,  // num_field_entries
        $num_aux_entries$,  // num_aux_entries
        $aux_offset$,
        $class_data$,
        $post_loop_handler$,
        $fallback$,  // fallback
        $to_prefetch$)cc"
        // clang-format on
    );
  };

  auto GenerateAuxEntries = [&] {
    for (const auto& aux_entry : tc_table_info_->aux_entries) {
      switch (aux_entry.type) {
        case TailCallTableInfo::kNothing:
          p->Emit("{},\n");
          break;
        case TailCallTableInfo::kInlinedStringDonatedOffset:
          p->Emit(
              "{_fl::Offset{offsetof($classname$, "
              "_impl_._inlined_string_donated_)}},\n");
          break;
        case TailCallTableInfo::kSplitOffset:
          p->Emit("{_fl::Offset{offsetof($classname$, _impl_._split_)}},\n");
          break;
        case TailCallTableInfo::kSplitSizeof:
          p->Emit("{_fl::Offset{sizeof($classname$::Impl_::Split)}},\n");
          break;
        case TailCallTableInfo::kSubMessage:
          p->Emit({{"name", QualifiedDefaultInstanceName(
                                aux_entry.field->message_type(), options_)}},
                  "{::_pbi::FieldAuxDefaultMessage{}, &$name$},\n");
          break;
        case TailCallTableInfo::kSubTable:
          p->Emit({{"name", QualifiedClassName(aux_entry.field->message_type(),
                                               options_)}},
                  "{::_pbi::TcParser::GetTable<$name$>()},\n");
          break;
        case TailCallTableInfo::kSubMessageWeak:
          p->Emit({{"ptr", QualifiedDefaultInstancePtr(
                               aux_entry.field->message_type(), options_)}},
                  "{::_pbi::FieldAuxDefaultMessage{}, &$ptr$},\n");
          break;
        case TailCallTableInfo::kMessageVerifyFunc:
          p->Emit({{"name", QualifiedClassName(aux_entry.field->message_type(),
                                               options_)}},
                  "{$name$::InternalVerify},\n");
          break;
        case TailCallTableInfo::kSelfVerifyFunc:
          if (ShouldVerify(descriptor_, options_, scc_analyzer_)) {
            p->Emit("{&InternalVerify},\n");
          } else {
            p->Emit("{},\n");
          }
          break;
        case TailCallTableInfo::kEnumRange:
          p->Emit(
              {
                  {"first", aux_entry.enum_range.first},
                  {"last", aux_entry.enum_range.last},
              },
              "{$first$, $last$},\n");
          break;
        case TailCallTableInfo::kEnumValidator:
          p->Emit({{"name", QualifiedClassName(aux_entry.field->enum_type(),
                                               options_)}},
                  "{::_pbi::FieldAuxEnumData{}, $name$_internal_data_},\n");
          break;
        case TailCallTableInfo::kNumericOffset:
          p->Emit(
              {
                  {"offset", aux_entry.offset},
              },
              "{_fl::Offset{$offset$}},\n");
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
              !internal::cpp::HasPreservingUnknownEnumSemantics(map_value);
          p->Emit(
              {
                  {"strict", utf8_check == Utf8CheckMode::kStrict},
                  {"verify", utf8_check == Utf8CheckMode::kVerify},
                  {"validate", validated_enum},
                  {"key_wire", map_key->type()},
                  {"value_wire", map_value->type()},
                  {"is_lite",
                   !HasDescriptorMethods(aux_entry.field->file(), options_)},
              },
              R"cc(
                {::_pbi::TcParser::GetMapAuxInfo($strict$, $verify$, $validate$,
                                                 $key_wire$, $value_wire$,
                                                 $is_lite$)},
              )cc");
          break;
        }
      }
    }
  };

  p->Emit(
      {{"const",
        // FileDescriptorProto's table must be constant initialized. For MSVC
        // this means using `constexpr`. However, we can't use `constexpr`
        // for all tables because it breaks when crossing DLL boundaries.
        // FileDescriptorProto is safe from this.
        IsFileDescriptorProto(descriptor_->file(), options_)
            ? "constexpr"
            : "PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1\nconst"},
       {"table_size_log2", tc_table_info_->table_size_log2},
       {"ordered_size", ordered_fields_.size()},
       {"aux_size", tc_table_info_->aux_entries.size()},
       {"data_size", FieldNameDataSize(tc_table_info_->field_name_data)},
       {"field_num_to_entry_table_size", field_num_to_entry_table.size16()},
       {"table_base", GenerateTableBase},
       {"fast_entries",
        [&] {
          // TODO: refactor this to use Emit.
          Formatter format(p, variables_);
          GenerateFastFieldEntries(format);
        }},
       {"field_lookup_table",
        [&] {
          for (SkipEntryBlock& entry_block : field_num_to_entry_table.blocks) {
            p->Emit(
                {
                    {"lower", entry_block.first_fnum & 65535},
                    {"upper", entry_block.first_fnum / 65536},
                    {"size", entry_block.entries.size()},
                },
                "$lower$, $upper$, $size$,\n");

            for (SkipEntry16 se16 : entry_block.entries) {
              p->Emit({{"skipmap", se16.skipmap},
                       {"offset", se16.field_entry_offset}},
                      R"cc(
                        $skipmap$, $offset$,
                      )cc");
            }
          }

          // The last entry of the skipmap are all 1's.
          p->Emit("65535, 65535\n");
        }},
       {"field_and_aux_entries",
        [&] {
          if (ordered_fields_.empty() &&
              !descriptor_->options().message_set_wire_format()) {
            ABSL_DLOG_IF(FATAL, !tc_table_info_->aux_entries.empty())
                << "Invalid message: " << descriptor_->full_name() << " has "
                << tc_table_info_->aux_entries.size()
                << " auxiliary field entries, but no fields";
            p->Emit("// no field_entries, or aux_entries\n");
            return;
          }

          p->Emit(
              {
                  {"field_entries", [&] { GenerateFieldEntries(p); }},
                  {"aux_entries",
                   [&] {
                     if (tc_table_info_->aux_entries.empty()) {
                       p->Emit("// no aux_entries\n");
                     } else {
                       p->Emit(
                           {
                               {"aux_entries_list", GenerateAuxEntries},
                           },
                           // clang-format off
                              R"cc(
                                {{
                                    $aux_entries_list$
                                }},
                              )cc"
                           // clang-format on
                       );
                     }
                   }},
              },
              // clang-format off
                 R"cc(
                 {{
                   $field_entries$,
                 }},
                 $aux_entries$)cc"
              // clang-format on
          );
        }},
       {"field_names",
        [&] {
          Formatter format(p, variables_);
          GenerateFieldNames(format);
        }}},
      // Disable clang-format, as we want to generate a denser table
      // representation than what clang-format typically wants. clang would
      // insert a newline at every brace, whereas we prefer {{ ... }} here.
      // clang-format off
R"cc(
$const$ ::_pbi::TcParseTable<$table_size_log2$, $ordered_size$, $aux_size$, $data_size$, $field_num_to_entry_table_size$>
$classname$::_table_ = {
  {
    $table_base$
  }, {{
    $fast_entries$
  }}, {{
    $field_lookup_table$
  }}, $field_and_aux_entries$,
  {{
    $field_names$,
  }},
};
)cc"
      // clang-format on
  );
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

void ParseFunctionGenerator::GenerateFieldEntries(io::Printer* p) {
  for (const auto& entry : tc_table_info_->field_entries) {
    const FieldDescriptor* field = entry.field;
    // TODO: refactor this to use Emit.
    Formatter format(p, variables_);
    PrintFieldComment(format, field, options_);

    bool weak = IsWeak(field, options_);
    bool split = ShouldSplit(field, options_);
    const OneofDescriptor* oneof = field->real_containing_oneof();

    auto v = p->WithVars(
        {{"field_name", FieldName(field)},
         {"field_member_name", FieldMemberName(field, /*split=*/false)}});

    p->Emit(
        {{"offset",
          [&] {
            if (weak) {
              p->Emit("/* weak */ 0,");
            } else if (split) {
              p->Emit(
                  "PROTOBUF_FIELD_OFFSET($classname$::Impl_::Split, "
                  "$field_name$_),");
            } else {
              p->Emit(
                  "PROTOBUF_FIELD_OFFSET($classname$, $field_member_name$),");
            }
          }},
         {"has_idx",
          [&] {
            if (oneof) {
              p->Emit(absl::StrCat("_Internal::kOneofCaseOffset + ",
                                   4 * oneof->index(), ","));
            } else if (num_hasbits_ > 0 || IsMapEntryMessage(descriptor_)) {
              std::string hb_content =
                  entry.hasbit_idx >= 0
                      ? absl::StrCat("_Internal::kHasBitsOffset + ",
                                     entry.hasbit_idx, ",")
                      : absl::StrCat(entry.hasbit_idx, ",");
              p->Emit(hb_content);
            } else {
              p->Emit("0,");
            }
          }},
         {"aux_idx", entry.aux_idx},
         {"type_card", internal::TypeCardToString(entry.type_card)}},
        // Use `0|` prefix to eagerly convert the enums to int to avoid
        // enum-enum operations. They are deprecated in C++20.
        R"cc(
          {$offset$, $has_idx$, $aux_idx$, (0 | $type_card$)},
        )cc");
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
      static_cast<int>(((tc_table_info_->field_entries.size() + 1) + 7) & ~7u);
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
