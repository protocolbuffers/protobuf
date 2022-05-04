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

#include <google/protobuf/compiler/cpp/parse_function_generator.h>

#include <algorithm>
#include <limits>
#include <string>
#include <utility>

#include <google/protobuf/wire_format.h>
#include <google/protobuf/compiler/cpp/helpers.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {
using google::protobuf::internal::WireFormat;
using google::protobuf::internal::WireFormatLite;

std::vector<const FieldDescriptor*> GetOrderedFields(
    const Descriptor* descriptor, const Options& options) {
  std::vector<const FieldDescriptor*> ordered_fields;
  for (auto field : FieldRange(descriptor)) {
    if (!IsFieldStripped(field, options)) {
      ordered_fields.push_back(field);
    }
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

int TagSize(uint32_t field_number) {
  if (field_number < 16) return 1;
  GOOGLE_CHECK_LT(field_number, (1 << 14))
      << "coded tag for " << field_number << " too big for uint16_t";
  return 2;
}

std::string FieldParseFunctionName(
    const TailCallTableInfo::FieldEntryInfo& entry, const Options& options);

bool IsFieldEligibleForFastParsing(
    const TailCallTableInfo::FieldEntryInfo& entry, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  const auto* field = entry.field;
  // Map, oneof, weak, and lazy fields are not handled on the fast path.
  if (field->is_map() || field->real_containing_oneof() ||
      field->options().weak() ||
      IsImplicitWeakField(field, options, scc_analyzer) ||
      IsLazy(field, options, scc_analyzer)) {
    return false;
  }

  // We will check for a valid auxiliary index range later. However, we might
  // want to change the value we check for inlined string fields.
  int aux_idx = entry.aux_idx;

  switch (field->type()) {
    case FieldDescriptor::TYPE_ENUM:
      // If enum values are not validated at parse time, then this field can be
      // handled on the fast path like an int32.
      if (HasPreservingUnknownEnumSemantics(field)) {
        break;
      }
      if (field->is_repeated() && field->is_packed()) {
        return false;
      }
      break;

      // Some bytes fields can be handled on fast path.
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      if (field->options().ctype() != FieldOptions::STRING) {
        return false;
      }
      if (IsStringInlined(field, options)) {
        GOOGLE_CHECK(!field->is_repeated());
        // For inlined strings, the donation state index is stored in the
        // `aux_idx` field of the fast parsing info. We need to check the range
        // of that value instead of the auxiliary index.
        aux_idx = entry.inlined_string_idx;
      }
      break;

    default:
      break;
  }

  if (HasHasbit(field)) {
    // The tailcall parser can only update the first 32 hasbits. Fields with
    // has-bits beyond the first 32 are handled by mini parsing/fallback.
    GOOGLE_CHECK_GE(entry.hasbit_idx, 0) << field->DebugString();
    if (entry.hasbit_idx >= 32) return false;
  }

  // If the field needs auxiliary data, then the aux index is needed. This
  // must fit in a uint8_t.
  if (aux_idx > std::numeric_limits<uint8_t>::max()) {
    return false;
  }

  // The largest tag that can be read by the tailcall parser is two bytes
  // when varint-coded. This allows 14 bits for the numeric tag value:
  //   byte 0   byte 1
  //   1nnnnttt 0nnnnnnn
  //    ^^^^^^^  ^^^^^^^
  if (field->number() >= 1 << 11) return false;

  return true;
}

std::vector<TailCallTableInfo::FastFieldInfo> SplitFastFieldsForSize(
    const std::vector<TailCallTableInfo::FieldEntryInfo>& field_entries,
    int table_size_log2, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  std::vector<TailCallTableInfo::FastFieldInfo> result(1 << table_size_log2);
  const uint32_t idx_mask = result.size() - 1;

  for (const auto& entry : field_entries) {
    if (!IsFieldEligibleForFastParsing(entry, options, scc_analyzer)) {
      continue;
    }

    const auto* field = entry.field;
    uint32_t tag = WireFormat::MakeTag(field);

    // Construct the varint-coded tag. If it is more than 7 bits, we need to
    // shift the high bits and add a continue bit.
    if (uint32_t hibits = tag & 0xFFFFFF80) {
      tag = tag + hibits + 128;  // tag = lobits + 2*hibits + 128
    }

    // The field index is determined by the low bits of the field number, where
    // the table size determines the width of the mask. The largest table
    // supported is 32 entries. The parse loop uses these bits directly, so that
    // the dispatch does not require arithmetic:
    //        byte 0   byte 1
    //   tag: 1nnnnttt 0nnnnnnn
    //        ^^^^^
    //         idx (table_size_log2=5)
    // This means that any field number that does not fit in the lower 4 bits
    // will always have the top bit of its table index asserted.
    const uint32_t fast_idx = (tag >> 3) & idx_mask;

    TailCallTableInfo::FastFieldInfo& info = result[fast_idx];
    if (info.field != nullptr) {
      // This field entry is already filled.
      continue;
    }

    // Fill in this field's entry:
    GOOGLE_CHECK(info.func_name.empty()) << info.func_name;
    info.func_name = FieldParseFunctionName(entry, options);
    info.field = field;
    info.coded_tag = tag;
    // If this field does not have presence, then it can set an out-of-bounds
    // bit (tailcall parsing uses a uint64_t for hasbits, but only stores 32).
    info.hasbit_idx = HasHasbit(field) ? entry.hasbit_idx : 63;
    if (IsStringInlined(field, options)) {
      GOOGLE_CHECK(!field->is_repeated());
      info.aux_idx = static_cast<uint8_t>(entry.inlined_string_idx);
    } else {
      info.aux_idx = static_cast<uint8_t>(entry.aux_idx);
    }
  }
  return result;
}

// Filter out fields that will be handled by mini parsing.
std::vector<const FieldDescriptor*> FilterMiniParsedFields(
    const std::vector<const FieldDescriptor*>& fields, const Options& options,
    MessageSCCAnalyzer* scc_analyzer) {
  std::vector<const FieldDescriptor*> generated_fallback_fields;

  for (const auto* field : fields) {
    bool handled = false;
    switch (field->type()) {
      case FieldDescriptor::TYPE_DOUBLE:
      case FieldDescriptor::TYPE_FLOAT:
      case FieldDescriptor::TYPE_FIXED32:
      case FieldDescriptor::TYPE_SFIXED32:
      case FieldDescriptor::TYPE_FIXED64:
      case FieldDescriptor::TYPE_SFIXED64:
      case FieldDescriptor::TYPE_BOOL:
      case FieldDescriptor::TYPE_UINT32:
      case FieldDescriptor::TYPE_SINT32:
      case FieldDescriptor::TYPE_INT32:
      case FieldDescriptor::TYPE_UINT64:
      case FieldDescriptor::TYPE_SINT64:
      case FieldDescriptor::TYPE_INT64:
        // These are handled by MiniParse, so we don't need any generated
        // fallback code.
        handled = true;
        break;

      case FieldDescriptor::TYPE_ENUM:
        if (field->is_repeated() && !HasPreservingUnknownEnumSemantics(field)) {
          // TODO(b/206890171): handle packed repeated closed enums
          // Non-packed repeated can be handled using tables, but we still
          // need to generate fallback code for all repeated enums in order to
          // handle packed encoding. This is because of the lite/full split
          // when handling invalid enum values in a packed field.
          handled = false;
        } else {
          handled = true;
        }
        break;

      case FieldDescriptor::TYPE_BYTES:
      case FieldDescriptor::TYPE_STRING:
        if (IsStringInlined(field, options)) {
          // TODO(b/198211897): support InilnedStringField.
          handled = false;
        } else {
          handled = true;
        }
        break;

      case FieldDescriptor::TYPE_MESSAGE:
      case FieldDescriptor::TYPE_GROUP:
        // TODO(b/210762816): support remaining field types.
        if (field->is_map() || IsWeak(field, options) ||
            IsImplicitWeakField(field, options, scc_analyzer) ||
            IsLazy(field, options, scc_analyzer)) {
          handled = false;
        } else {
          handled = true;
        }
        break;

      default:
        handled = false;
        break;
    }
    if (!handled) generated_fallback_fields.push_back(field);
  }

  return generated_fallback_fields;
}

}  // namespace

TailCallTableInfo::TailCallTableInfo(
    const Descriptor* descriptor, const Options& options,
    const std::vector<const FieldDescriptor*>& ordered_fields,
    const std::vector<int>& has_bit_indices,
    const std::vector<int>& inlined_string_indices,
    MessageSCCAnalyzer* scc_analyzer) {
  int oneof_count = descriptor->real_oneof_decl_count();
  // If this message has any oneof fields, store the case offset in the first
  // auxiliary entry.
  if (oneof_count > 0) {
    GOOGLE_LOG_IF(DFATAL, ordered_fields.empty())
        << "Invalid message: " << descriptor->full_name() << " has "
        << oneof_count << " oneof declarations, but no fields";
    aux_entries.push_back(StrCat("_fl::Offset{offsetof(",
                                       ClassName(descriptor),
                                       ", _impl_._oneof_case_)}"));
  }

  // If this message has any inlined string fields, store the donation state
  // offset in the second auxiliary entry.
  if (!inlined_string_indices.empty()) {
    aux_entries.resize(2);  // pad if necessary
    aux_entries[1] =
        StrCat("_fl::Offset{offsetof(", ClassName(descriptor),
                     ", _impl_._inlined_string_donated_)}");
  }

  // Fill in mini table entries.
  for (const FieldDescriptor* field : ordered_fields) {
    field_entries.push_back(
        {field, (HasHasbit(field) ? has_bit_indices[field->index()] : -1)});
    auto& entry = field_entries.back();

    if (field->type() == FieldDescriptor::TYPE_MESSAGE ||
        field->type() == FieldDescriptor::TYPE_GROUP) {
      // Message-typed fields have a FieldAux with the default instance pointer.
      if (field->is_map()) {
        // TODO(b/205904770): generate aux entries for maps
      } else if (IsWeak(field, options)) {
        // Don't generate anything for weak fields. They are handled by the
        // generated fallback.
      } else if (IsImplicitWeakField(field, options, scc_analyzer)) {
        // Implicit weak fields don't need to store a default instance pointer.
      } else if (IsLazy(field, options, scc_analyzer)) {
        // Lazy fields are handled by the generated fallback function.
      } else {
        field_entries.back().aux_idx = aux_entries.size();
        const Descriptor* field_type = field->message_type();
        aux_entries.push_back(StrCat(
            "reinterpret_cast<const ", QualifiedClassName(field_type, options),
            "*>(&", QualifiedDefaultInstanceName(field_type, options), ")"));
      }
    } else if (field->type() == FieldDescriptor::TYPE_ENUM &&
               !HasPreservingUnknownEnumSemantics(field)) {
      // Enum fields which preserve unknown values (proto3 behavior) are
      // effectively int32 fields with respect to parsing -- i.e., the value
      // does not need to be validated at parse time.
      //
      // Enum fields which do not preserve unknown values (proto2 behavior) use
      // a FieldAux to store validation information. If the enum values are
      // sequential (and within a range we can represent), then the FieldAux
      // entry represents the range using the minimum value (which must fit in
      // an int16_t) and count (a uint16_t). Otherwise, the entry holds a
      // pointer to the generated Name_IsValid function.

      entry.aux_idx = aux_entries.size();
      const EnumDescriptor* enum_type = field->enum_type();
      GOOGLE_CHECK_GT(enum_type->value_count(), 0) << enum_type->DebugString();

      // Check if the enum values are a single, contiguous range.
      std::vector<int> enum_values;
      for (int i = 0, N = enum_type->value_count(); i < N; ++i) {
        enum_values.push_back(enum_type->value(i)->number());
      }
      auto values_begin = enum_values.begin();
      auto values_end = enum_values.end();
      std::sort(values_begin, values_end);
      enum_values.erase(std::unique(values_begin, values_end), values_end);

      if (enum_values.back() - enum_values[0] == enum_values.size() - 1 &&
          enum_values[0] >= std::numeric_limits<int16_t>::min() &&
          enum_values[0] <= std::numeric_limits<int16_t>::max() &&
          enum_values.size() <= std::numeric_limits<uint16_t>::max()) {
        entry.is_enum_range = true;
        aux_entries.push_back(
            StrCat(enum_values[0], ", ", enum_values.size()));
      } else {
        entry.is_enum_range = false;
        aux_entries.push_back(
            StrCat(QualifiedClassName(enum_type, options), "_IsValid"));
      }
    } else if ((field->type() == FieldDescriptor::TYPE_STRING ||
                field->type() == FieldDescriptor::TYPE_BYTES) &&
               IsStringInlined(field, options)) {
      GOOGLE_CHECK(!field->is_repeated());
      // Inlined strings have an extra marker to represent their donation state.
      int idx = inlined_string_indices[field->index()];
      // For mini parsing, the donation state index is stored as an `offset`
      // auxiliary entry.
      entry.aux_idx = aux_entries.size();
      aux_entries.push_back(StrCat("_fl::Offset{", idx, "}"));
      // For fast table parsing, the donation state index is stored instead of
      // the aux_idx (this will limit the range to 8 bits).
      entry.inlined_string_idx = idx;
    }
  }

  // Choose the smallest fast table that covers the maximum number of fields.
  table_size_log2 = 0;  // fallback value
  int num_fast_fields = -1;
  for (int try_size_log2 : {0, 1, 2, 3, 4, 5}) {
    size_t try_size = 1 << try_size_log2;
    auto split_fields = SplitFastFieldsForSize(field_entries, try_size_log2,
                                               options, scc_analyzer);
    GOOGLE_CHECK_EQ(split_fields.size(), try_size);
    int try_num_fast_fields = 0;
    for (const auto& info : split_fields) {
      if (info.field != nullptr) ++try_num_fast_fields;
    }
    // Use this size if (and only if) it covers more fields.
    if (try_num_fast_fields > num_fast_fields) {
      fast_path_fields = std::move(split_fields);
      table_size_log2 = try_size_log2;
      num_fast_fields = try_num_fast_fields;
    }
    // The largest table we allow has the same number of entries as the message
    // has fields, rounded up to the next power of 2 (e.g., a message with 5
    // fields can have a fast table of size 8). A larger table *might* cover
    // more fields in certain cases, but a larger table in that case would have
    // mostly empty entries; so, we cap the size to avoid pathologically sparse
    // tables.
    if (try_size > ordered_fields.size()) {
      break;
    }
  }

  // Filter out fields that are handled by MiniParse. We don't need to generate
  // a fallback for these, which saves code size.
  fallback_fields = FilterMiniParsedFields(ordered_fields, options,
                                           scc_analyzer);

  // If there are no fallback fields, and at most one extension range, the
  // parser can use a generic fallback function. Otherwise, a message-specific
  // fallback routine is needed.
  use_generated_fallback =
      !fallback_fields.empty() || descriptor->extension_range_count() > 1;
}

ParseFunctionGenerator::ParseFunctionGenerator(
    const Descriptor* descriptor, int max_has_bit_index,
    const std::vector<int>& has_bit_indices,
    const std::vector<int>& inlined_string_indices, const Options& options,
    MessageSCCAnalyzer* scc_analyzer,
    const std::map<std::string, std::string>& vars)
    : descriptor_(descriptor),
      scc_analyzer_(scc_analyzer),
      options_(options),
      variables_(vars),
      inlined_string_indices_(inlined_string_indices),
      ordered_fields_(GetOrderedFields(descriptor_, options_)),
      num_hasbits_(max_has_bit_index) {
  if (should_generate_tctable()) {
    tc_table_info_.reset(new TailCallTableInfo(
        descriptor_, options_, ordered_fields_, has_bit_indices,
        inlined_string_indices, scc_analyzer));
  }
  SetCommonVars(options_, &variables_);
  SetCommonMessageDataVariables(descriptor_, &variables_);
  SetUnknownFieldsVariable(descriptor_, options_, &variables_);
  variables_["classname"] = ClassName(descriptor, false);
}

void ParseFunctionGenerator::GenerateMethodDecls(io::Printer* printer) {
  Formatter format(printer, variables_);
  if (should_generate_tctable()) {
    format.Outdent();
    if (should_generate_guarded_tctable()) {
      format("#ifdef PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
    }
    format(
        " private:\n"
        "  static const char* Tct_ParseFallback(PROTOBUF_TC_PARAM_DECL);\n"
        " public:\n");
    if (should_generate_guarded_tctable()) {
      format("#endif\n");
    }
    format.Indent();
  }
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
  if (!should_generate_tctable()) {
    if (need_parse_function) {
      GenerateLoopingParseFunction(format);
    }
    return;
  }
  if (should_generate_guarded_tctable()) {
    format("#ifdef PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n\n");
  }
  if (need_parse_function) {
    GenerateTailcallParseFunction(format);
  }
  if (tc_table_info_->use_generated_fallback) {
    GenerateTailcallFallbackFunction(format);
  }
  if (should_generate_guarded_tctable()) {
    if (need_parse_function) {
      format("\n#else  // PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n\n");
      GenerateLoopingParseFunction(format);
    }
    format("\n#endif  // PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
  }
}

bool ParseFunctionGenerator::should_generate_tctable() const {
  if (options_.tctable_mode == Options::kTCTableNever) {
    return false;
  }
  return true;
}

void ParseFunctionGenerator::GenerateTailcallParseFunction(Formatter& format) {
  GOOGLE_CHECK(should_generate_tctable());

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

void ParseFunctionGenerator::GenerateTailcallFallbackFunction(
    Formatter& format) {
  GOOGLE_CHECK(should_generate_tctable());
  format(
      "const char* $classname$::Tct_ParseFallback(PROTOBUF_TC_PARAM_DECL) {\n"
      "#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) return nullptr\n");
  format.Indent();
  format("auto* typed_msg = static_cast<$classname$*>(msg);\n");

  if (num_hasbits_ > 0) {
    // Sync hasbits
    format("typed_msg->_impl_._has_bits_[0] = hasbits;\n");
  }
  format("uint32_t tag = data.tag();\n");

  format.Set("msg", "typed_msg->");
  format.Set("this", "typed_msg");
  format.Set("has_bits", "typed_msg->_impl_._has_bits_");
  format.Set("next_tag", "goto next_tag");
  GenerateParseIterationBody(format, descriptor_,
                             tc_table_info_->fallback_fields);

  format.Outdent();
  format(
      "next_tag:\n"
      "message_done:\n"
      "  return ptr;\n"
      "#undef CHK_\n"
      "}\n");
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

void ParseFunctionGenerator::GenerateDataDecls(io::Printer* printer) {
  if (!should_generate_tctable()) {
    return;
  }
  Formatter format(printer, variables_);
  if (should_generate_guarded_tctable()) {
    format.Outdent();
    format("#ifdef PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
    format.Indent();
  }
  auto field_num_to_entry_table = MakeNumToEntryTable(ordered_fields_);
  format(
      "static const ::$proto_ns$::internal::"
      "TcParseTable<$1$, $2$, $3$, $4$, $5$> _table_;\n",
      tc_table_info_->table_size_log2, ordered_fields_.size(),
      tc_table_info_->aux_entries.size(), CalculateFieldNamesSize(),
      field_num_to_entry_table.size16());
  if (should_generate_guarded_tctable()) {
    format.Outdent();
    format("#endif  // PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
    format.Indent();
  }
}

void ParseFunctionGenerator::GenerateDataDefinitions(io::Printer* printer) {
  if (!should_generate_tctable()) {
    return;
  }
  Formatter format(printer, variables_);
  if (should_generate_guarded_tctable()) {
    format("#ifdef PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
  }
  GenerateTailCallTable(format);
  if (should_generate_guarded_tctable()) {
    format("#endif  // PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
  }
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
      "uint32_t tag;\n"
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
    GOOGLE_CHECK_GT(fnum, last_skip_entry_start);
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
  GOOGLE_CHECK(should_generate_tctable());
  // All entries without a fast-path parsing function need a fallback.
  std::string fallback;
  if (tc_table_info_->use_generated_fallback) {
    fallback = ClassName(descriptor_) + "::Tct_ParseFallback";
  } else {
    fallback = "::_pbi::TcParser::GenericFallback";
    if (GetOptimizeFor(descriptor_->file(), options_) ==
        FileOptions::LITE_RUNTIME) {
      fallback += "Lite";
    }
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
      "PROTOBUF_ATTRIBUTE_INIT_PRIORITY1\n"
      "const ::_pbi::TcParseTable<$1$, $2$, $3$, $4$, $5$> "
      "$classname$::_table_ = "
      "{\n",
      tc_table_info_->table_size_log2, ordered_fields_.size(),
      tc_table_info_->aux_entries.size(), CalculateFieldNamesSize(),
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
      if (descriptor_->extension_range_count() == 1) {
        format(
            "PROTOBUF_FIELD_OFFSET($classname$, $extensions$),\n"
            "$1$, $2$,  // extension_range_{low,high}\n",
            descriptor_->extension_range(0)->start,
            descriptor_->extension_range(0)->end);
      } else {
        format("0, 0, 0,  // no _extensions_\n");
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
      GOOGLE_LOG_IF(DFATAL, !tc_table_info_->aux_entries.empty())
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
          for (const std::string& aux_entry : tc_table_info_->aux_entries) {
            format("{$1$},\n", aux_entry);
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
      PrintFieldComment(format, info.field);
    }
    if (info.func_name.empty()) {
      format("{::_pbi::TcParser::MiniParse, {}},\n");
    } else {
      bool cold = ShouldSplit(info.field, options_);
      format(
          "{$1$,\n"
          " {$2$, $3$, $4$, PROTOBUF_FIELD_OFFSET($classname$$5$, $6$)}},\n",
          info.func_name, info.coded_tag, info.hasbit_idx, info.aux_idx,
          cold ? "::Impl_::Split" : "",
          cold ? FieldName(info.field) + "_"
               : FieldMemberName(info.field, /*cold=*/false));
    }
  }
}

static void FormatFieldKind(Formatter& format,
                            const TailCallTableInfo::FieldEntryInfo& entry,
                            const Options& options,
                            MessageSCCAnalyzer* scc_analyzer) {
  const FieldDescriptor* field = entry.field;
  // Spell the field kind in proto language declaration order, starting with
  // cardinality:
  format("(::_fl::kFc");
  if (HasHasbit(field)) {
    format("Optional");
  } else if (field->is_repeated()) {
    format("Repeated");
  } else if (field->real_containing_oneof()) {
    format("Oneof");
  } else {
    format("Singular");
  }

  // The rest of the type uses convenience aliases:
  format(" | ::_fl::k");
  if (field->is_repeated() && field->is_packed()) {
    format("Packed");
  }
  switch (field->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
      format("Double");
      break;
    case FieldDescriptor::TYPE_FLOAT:
      format("Float");
      break;
    case FieldDescriptor::TYPE_FIXED32:
      format("Fixed32");
      break;
    case FieldDescriptor::TYPE_SFIXED32:
      format("SFixed32");
      break;
    case FieldDescriptor::TYPE_FIXED64:
      format("Fixed64");
      break;
    case FieldDescriptor::TYPE_SFIXED64:
      format("SFixed64");
      break;
    case FieldDescriptor::TYPE_BOOL:
      format("Bool");
      break;
    case FieldDescriptor::TYPE_ENUM:
      if (HasPreservingUnknownEnumSemantics(field)) {
        // No validation is required.
        format("OpenEnum");
      } else if (entry.is_enum_range) {
        // Validation is done by range check (start/length in FieldAux).
        format("EnumRange");
      } else {
        // Validation uses the generated _IsValid function.
        format("Enum");
      }
      break;
    case FieldDescriptor::TYPE_UINT32:
      format("UInt32");
      break;
    case FieldDescriptor::TYPE_SINT32:
      format("SInt32");
      break;
    case FieldDescriptor::TYPE_INT32:
      format("Int32");
      break;
    case FieldDescriptor::TYPE_UINT64:
      format("UInt64");
      break;
    case FieldDescriptor::TYPE_SINT64:
      format("SInt64");
      break;
    case FieldDescriptor::TYPE_INT64:
      format("Int64");
      break;

    case FieldDescriptor::TYPE_BYTES:
      format("Bytes");
      break;
    case FieldDescriptor::TYPE_STRING: {
      auto mode = GetUtf8CheckMode(field, options);
      switch (mode) {
        case Utf8CheckMode::kStrict:
          format("Utf8String");
          break;
        case Utf8CheckMode::kVerify:
          format("RawString");
          break;
        case Utf8CheckMode::kNone:
          // Treat LITE_RUNTIME strings as bytes.
          format("Bytes");
          break;
        default:
          GOOGLE_LOG(FATAL) << "Invalid Utf8CheckMode (" << static_cast<int>(mode)
                     << ") for " << field->DebugString();
      }
      break;
    }

    case FieldDescriptor::TYPE_GROUP:
      format("Message | ::_fl::kRepGroup");
      break;
    case FieldDescriptor::TYPE_MESSAGE:
      if (field->is_map()) {
        format("Map");
      } else {
        format("Message");
        if (IsLazy(field, options, scc_analyzer)) {
          format(" | ::_fl::kRepLazy");
        } else if (IsImplicitWeakField(field, options, scc_analyzer)) {
          format(" | ::_fl::kRepIWeak");
        }
      }
      break;
  }

  // Fill in extra information about string and bytes field representations.
  if (field->type() == FieldDescriptor::TYPE_BYTES ||
      field->type() == FieldDescriptor::TYPE_STRING) {
    if (field->is_repeated()) {
      format(" | ::_fl::kRepSString");
    } else {
      format(" | ::_fl::kRepAString");
    }
  }

  format(")");
}

void ParseFunctionGenerator::GenerateFieldEntries(Formatter& format) {
  for (const auto& entry : tc_table_info_->field_entries) {
    const FieldDescriptor* field = entry.field;
    PrintFieldComment(format, field);
    format("{");
    if (IsWeak(field, options_)) {
      // Weak fields are handled by the generated fallback function.
      // (These are handled by legacy Google-internal logic.)
      format("/* weak */ 0, 0, 0, 0");
    } else {
      const OneofDescriptor* oneof = field->real_containing_oneof();
      bool cold = ShouldSplit(field, options_);
      format("PROTOBUF_FIELD_OFFSET($classname$$1$, $2$), $3$, $4$,\n ",
             cold ? "::Impl_::Split" : "",
             cold ? FieldName(field) + "_"
                  : FieldMemberName(field, /*cold=*/false),
             (oneof ? oneof->index() : entry.hasbit_idx), entry.aux_idx);
      FormatFieldKind(format, entry, options_, scc_analyzer_);
    }
    format("},\n");
  }
}

static constexpr int kMaxNameLength = 255;

int ParseFunctionGenerator::CalculateFieldNamesSize() const {
  // The full name of the message appears first.
  int size = std::min(static_cast<int>(descriptor_->full_name().size()),
                      kMaxNameLength);
  int lengths_size = 1;
  for (const auto& entry : tc_table_info_->field_entries) {
    const FieldDescriptor* field = entry.field;
    GOOGLE_CHECK_LE(field->name().size(), kMaxNameLength);
    size += field->name().size();
    lengths_size += 1;
  }
  // align to an 8-byte boundary
  lengths_size = (lengths_size + 7) & -8;
  return size + lengths_size + 1;
}

static void FormatOctal(Formatter& format, int size) {
  int octal_size = ((size >> 6) & 3) * 100 +  //
                   ((size >> 3) & 7) * 10 +   //
                   ((size >> 0) & 7);
  format("\\$1$", octal_size);
}

void ParseFunctionGenerator::GenerateFieldNames(Formatter& format) {
  // First, we output the size of each string, as an unsigned byte. The first
  // string is the message name.
  int count = 1;
  format("\"");
  FormatOctal(format,
              std::min(static_cast<int>(descriptor_->full_name().size()), 255));
  for (const auto& entry : tc_table_info_->field_entries) {
    FormatOctal(format, entry.field->name().size());
    ++count;
  }
  while (count & 7) {  // align to an 8-byte boundary
    format("\\0");
    ++count;
  }
  format("\"\n");
  // The message name is stored at the beginning of the string
  std::string message_name = descriptor_->full_name();
  if (message_name.size() > kMaxNameLength) {
    static constexpr int kNameHalfLength = (kMaxNameLength - 3) / 2;
    message_name = StrCat(
        message_name.substr(0, kNameHalfLength), "...",
        message_name.substr(message_name.size() - kNameHalfLength));
  }
  format("\"$1$\"\n", message_name);
  // Then we output the actual field names
  for (const auto& entry : tc_table_info_->field_entries) {
    const FieldDescriptor* field = entry.field;
    format("\"$1$\"\n", field->name());
  }
}

void ParseFunctionGenerator::GenerateArenaString(Formatter& format,
                                                 const FieldDescriptor* field) {
  if (HasHasbit(field)) {
    format("_Internal::set_has_$1$(&$has_bits$);\n", FieldName(field));
  }
  format(
      "if (arena != nullptr) {\n"
      "  ptr = ctx->ReadArenaString(ptr, &$msg$$field$, arena");
  if (IsStringInlined(field, options_)) {
    GOOGLE_DCHECK(!inlined_string_indices_.empty());
    int inlined_string_index = inlined_string_indices_[field->index()];
    GOOGLE_DCHECK_GT(inlined_string_index, 0);
    format(", &$msg$$inlined_string_donated_array$[0], $1$, $this$",
           inlined_string_index);
  } else {
    GOOGLE_DCHECK(field->default_value_string().empty());
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
  if (!options_.opensource_runtime) {
    // Open source doesn't support other ctypes;
    ctype = field->options().ctype();
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
    format(
        "auto str = $msg$$1$$2$_$name$();\n"
        "ptr = ::_pbi::Inline$3$(str, ptr, ctx);\n",
        HasInternalAccessors(ctype) ? "_internal_" : "",
        field->is_repeated() && !field->is_packable() ? "add" : "mutable",
        parser_name);
  }
  // It is intentionally placed before VerifyUTF8 because it doesn't make sense
  // to verify UTF8 when we already know parsing failed.
  format("CHK_(ptr);\n");
  if (!check_utf8) return;  // return if this is a bytes field
  auto level = GetUtf8CheckMode(field, options_);
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
    field_name = StrCat("\"", field->full_name(), "\"");
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
        !HasPreservingUnknownEnumSemantics(field)) {
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
          GOOGLE_CHECK(val);
          if (val->type() == FieldDescriptor::TYPE_ENUM &&
              !HasPreservingUnknownEnumSemantics(field)) {
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
                    ? StrCat("&", ClassName(field->message_type(), true),
                                   "::InternalVerify")
                    : "nullptr");
          }
          if (field->real_containing_oneof()) {
            format(
                "if (!$msg$_internal_has_$name$()) {\n"
                "  $msg$clear_$1$();\n"
                "  $msg$$field$ = ::$proto_ns$::Arena::CreateMessage<\n"
                "      ::$proto_ns$::internal::LazyField>("
                "$msg$GetArenaForAllocation());\n"
                "  $msg$set_has_$name$();\n"
                "}\n"
                "auto* lazy_field = $msg$$field$;\n",
                field->containing_oneof()->name());
          } else if (HasHasbit(field)) {
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
        GOOGLE_LOG(FATAL) << "Illegal combination for length delimited wiretype "
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
    format.AddMap({{"put_field", StrCat("add_", FieldName(field))},
                   {"mutable_field", StrCat("add_", FieldName(field))}});
  } else {
    format.AddMap(
        {{"put_field", StrCat("set_", FieldName(field))},
         {"mutable_field", StrCat("mutable_", FieldName(field))}});
  }
  uint32_t tag = WireFormatLite::MakeTag(field->number(), wiretype);
  switch (wiretype) {
    case WireFormatLite::WIRETYPE_VARINT: {
      std::string type = PrimitiveTypeName(options_, field->cpp_type());
      if (field->type() == FieldDescriptor::TYPE_ENUM) {
        format.Set("enum_type",
                   QualifiedClassName(field->enum_type(), options_));
        format(
            "$uint64$ val = ::$proto_ns$::internal::ReadVarint64(&ptr);\n"
            "CHK_(ptr);\n");
        if (!HasPreservingUnknownEnumSemantics(field)) {
          format("if (PROTOBUF_PREDICT_TRUE($enum_type$_IsValid(val))) {\n");
          format.Indent();
        }
        format("$msg$_internal_$put_field$(static_cast<$enum_type$>(val));\n");
        if (!HasPreservingUnknownEnumSemantics(field)) {
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
          if (HasHasbit(field)) {
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
        if (HasHasbit(field)) {
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
      GOOGLE_LOG(FATAL) << "Can't have end group field\n";
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
    GOOGLE_CHECK(expected_wiretype != WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
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
            range->start, static_cast<WireFormatLite::WireType>(0));
        uint32_t end_tag = WireFormatLite::MakeTag(
            range->end, static_cast<WireFormatLite::WireType>(0));

        if (range->end > FieldDescriptor::kMaxNumber) {
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
    PrintFieldComment(format, field);
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
        "} else\n"
        "  goto handle_unusual;\n"
        "$next_tag$;\n");
    format.Outdent();
  }  // for loop over ordered fields

  format(
      "default:\n"
      "  goto handle_unusual;\n");
  format.Outdent();
  format("}  // switch\n");
}

namespace {

std::string FieldParseFunctionName(
    const TailCallTableInfo::FieldEntryInfo& entry, const Options& options) {
  const FieldDescriptor* field = entry.field;
  std::string name = "::_pbi::TcParser::Fast";

  switch (field->type()) {
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FLOAT:
      name.append("F32");
      break;

    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_DOUBLE:
      name.append("F64");
      break;

    case FieldDescriptor::TYPE_BOOL:
      name.append("V8");
      break;
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
      name.append("V32");
      break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
      name.append("V64");
      break;

    case FieldDescriptor::TYPE_ENUM:
      if (HasPreservingUnknownEnumSemantics(field)) {
        name.append("V32");
        break;
      }
      if (field->is_repeated() && field->is_packed()) {
        GOOGLE_LOG(DFATAL) << "Enum validation not handled: " << field->DebugString();
        return "";
      }
      name.append(entry.is_enum_range ? "Er" : "Ev");
      break;

    case FieldDescriptor::TYPE_SINT32:
      name.append("Z32");
      break;
    case FieldDescriptor::TYPE_SINT64:
      name.append("Z64");
      break;

    case FieldDescriptor::TYPE_BYTES:
      name.append("B");
      if (IsStringInlined(field, options)) {
        name.append("i");
      }
      break;
    case FieldDescriptor::TYPE_STRING:
      switch (GetUtf8CheckMode(field, options)) {
        case Utf8CheckMode::kNone:
          name.append("B");
          break;
        case Utf8CheckMode::kVerify:
          name.append("S");
          break;
        case Utf8CheckMode::kStrict:
          name.append("U");
          break;
        default:
          GOOGLE_LOG(DFATAL) << "Mode not handled: "
                      << static_cast<int>(GetUtf8CheckMode(field, options));
          return "";
      }
      if (IsStringInlined(field, options)) {
        name.append("i");
      }
      break;

    case FieldDescriptor::TYPE_MESSAGE:
      name.append("M");
      break;
    case FieldDescriptor::TYPE_GROUP:
      name.append("G");
      break;

    default:
      GOOGLE_LOG(DFATAL) << "Type not handled: " << field->DebugString();
      return "";
  }

  // The field implementation functions are prefixed by cardinality:
  //   `S` for optional or implicit fields.
  //   `R` for non-packed repeated.
  //   `P` for packed repeated.
  name.append(field->is_packed()               ? "P"
              : field->is_repeated()           ? "R"
              : field->real_containing_oneof() ? "O"
                                               : "S");

  // Append the tag length. Fast parsing only handles 1- or 2-byte tags.
  name.append(TagSize(field->number()) == 1 ? "1" : "2");

  return name;
}

}  // namespace

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
