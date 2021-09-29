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

#include <google/protobuf/compiler/cpp/cpp_parse_function_generator.h>

#include <algorithm>
#include <limits>
#include <string>

#include <google/protobuf/compiler/cpp/cpp_helpers.h>
#include <google/protobuf/wire_format.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace cpp {

namespace {
using google::protobuf::internal::TcFieldData;
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

const char* CodedTagType(int tag_size) {
  return tag_size == 1 ? "uint8_t" : "uint16_t";
}

const char* TagType(const FieldDescriptor* field) {
  return CodedTagType(TagSize(field->number()));
}

std::string TcParserBaseName(const Options& options) {
  return StrCat("::", ProtobufNamespace(options),
                      "::internal::TcParserBase::");
}

std::string MessageTcParseFunctionName(const FieldDescriptor* field,
                                       const Options& options) {
  if (field->message_type()->field_count() == 0 ||
      !HasGeneratedMethods(field->message_type()->file(), options)) {
    // For files with `option optimize_for = CODE_SIZE`, or which derive from
    // `ZeroFieldsBase`, we need to call the `_InternalParse` function, because
    // there is no generated tailcall function. For tailcall parsing, this is
    // done by helpers in TcParserBase.
    return StrCat(TcParserBaseName(options),
                        (field->is_repeated() ? "Repeated" : "Singular"),
                        "ParseMessage<",
                        QualifiedClassName(field->message_type()),  //
                        ", ", TagType(field), ">");
  }
  // This matches macros in generated_message_tctable_impl.h:
  return StrCat("PROTOBUF_TC_PARSE_",
                      (field->is_repeated() ? "REPEATED" : "SINGULAR"),
                      TagSize(field->number()), "(",
                      QualifiedClassName(field->message_type()), ")");
}

std::string FieldParseFunctionName(const FieldDescriptor* field,
                                   const Options& options,
                                   uint32_t table_size_log2);

}  // namespace

TailCallTableInfo::TailCallTableInfo(const Descriptor* descriptor,
                                     const Options& options,
                                     const std::vector<int>& has_bit_indices,
                                     MessageSCCAnalyzer* scc_analyzer) {
  std::vector<const FieldDescriptor*> ordered_fields =
      GetOrderedFields(descriptor, options);

  // The table size is rounded up to the nearest power of 2, clamping at 2^5.
  // Note that this is a naive approach: a better approach should only consider
  // table-eligible fields. We may also want to push rarely-encountered fields
  // into the fallback, to make the table smaller.
  table_size_log2 = ordered_fields.size() >= 16  ? 5
                    : ordered_fields.size() >= 8 ? 4
                    : ordered_fields.size() >= 4 ? 3
                    : ordered_fields.size() >= 2 ? 2
                                                 : 1;
  const unsigned table_size = 1 << table_size_log2;

  // Construct info for each possible entry. Fields that do not use table-driven
  // parsing will still have an entry that nominates the fallback function.
  fast_path_fields.resize(table_size);

  for (const auto* field : ordered_fields) {
    // Eagerly assume slow path. If we can handle this field on the fast path,
    // we will pop its entry from `fallback_fields`.
    fallback_fields.push_back(field);

    // Anything difficult slow path:
    if (field->is_map()) continue;
    if (field->real_containing_oneof()) continue;
    if (field->options().weak()) continue;
    if (IsImplicitWeakField(field, options, scc_analyzer)) continue;
    if (IsLazy(field, options, scc_analyzer)) continue;

    // The largest tag that can be read by the tailcall parser is two bytes
    // when varint-coded. This allows 14 bits for the numeric tag value:
    //   byte 0   byte 1
    //   1nnnnttt 0nnnnnnn
    //    ^^^^^^^  ^^^^^^^
    uint32_t tag = WireFormat::MakeTag(field);
    if (tag >= 1 << 14) {
      continue;
    } else if (tag >= 1 << 7) {
      tag = ((tag << 1) & 0x7F00) | 0x80 | (tag & 0x7F);
    }
    // The field index is determined by the low bits of the field number, where
    // the table size determines the width of the mask. The largest table
    // supported is 32 entries. The parse loop uses these bits directly, so that
    // the dispatch does not require arithmetic:
    //   byte 0   byte 1
    //   1nnnnttt 0nnnnnnn
    //   ^^^^^
    // This means that any field number that does not fit in the lower 4 bits
    // will always have the top bit of its table index asserted:
    uint32_t idx = (tag >> 3) & (table_size - 1);
    // If this entry in the table is already used, then this field will be
    // handled by the generated fallback function.
    if (!fast_path_fields[idx].func_name.empty()) continue;

    // Determine the hasbit mask for this field, if needed. (Note that fields
    // without hasbits use different parse functions.)
    int hasbit_idx;
    if (HasHasbit(field)) {
      hasbit_idx = has_bit_indices[field->index()];
      GOOGLE_CHECK_NE(-1, hasbit_idx) << field->DebugString();
      // The tailcall parser can only update the first 32 hasbits. If this
      // field's has-bit is beyond that, then it will need to be handled by the
      // fallback parse function.
      if (hasbit_idx >= 32) continue;
    } else {
      // The tailcall parser only ever syncs 32 has-bits, so if there is no
      // presence, set a bit that will not be used.
      hasbit_idx = 63;
    }

    // Determine the name of the fastpath parse function to use for this field.
    std::string name;

    switch (field->type()) {
      case FieldDescriptor::TYPE_MESSAGE:
        name = MessageTcParseFunctionName(field, options);
        break;

      case FieldDescriptor::TYPE_FIXED64:
      case FieldDescriptor::TYPE_FIXED32:
      case FieldDescriptor::TYPE_SFIXED64:
      case FieldDescriptor::TYPE_SFIXED32:
      case FieldDescriptor::TYPE_DOUBLE:
      case FieldDescriptor::TYPE_FLOAT:
      case FieldDescriptor::TYPE_INT64:
      case FieldDescriptor::TYPE_INT32:
      case FieldDescriptor::TYPE_UINT64:
      case FieldDescriptor::TYPE_UINT32:
      case FieldDescriptor::TYPE_SINT64:
      case FieldDescriptor::TYPE_SINT32:
      case FieldDescriptor::TYPE_BOOL:
        name = FieldParseFunctionName(field, options, table_size_log2);
        break;

      case FieldDescriptor::TYPE_BYTES:
        if (field->options().ctype() == FieldOptions::STRING &&
            field->default_value_string().empty() &&
            !IsStringInlined(field, options)) {
          name = FieldParseFunctionName(field, options, table_size_log2);
        }
        break;

      default:
        break;
    }

    if (name.empty()) {
      continue;
    }
    // This field made it into the fast path, so remove it from the fallback
    // fields and fill in the table entry.
    fallback_fields.pop_back();
    fast_path_fields[idx].func_name = name;
    fast_path_fields[idx].bits = TcFieldData(tag, hasbit_idx, 0);
    fast_path_fields[idx].field = field;
  }

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
      num_hasbits_(max_has_bit_index) {
  if (should_generate_tctable()) {
    tc_table_info_.reset(new TailCallTableInfo(descriptor_, options_,
                                               has_bit_indices, scc_analyzer));
  }
  SetCommonVars(options_, &variables_);
  SetUnknownFieldsVariable(descriptor_, options_, &variables_);
  variables_["classname"] = ClassName(descriptor, false);
}

void ParseFunctionGenerator::GenerateMethodDecls(io::Printer* printer) {
  Formatter format(printer, variables_);
  if (should_generate_tctable()) {
    auto declare_function = [&format](const char* name,
                                      const std::string& guard) {
      if (!guard.empty()) {
        format.Outdent();
        format("#if $1$\n", guard);
        format.Indent();
      }
      format("static const char* $1$(PROTOBUF_TC_PARAM_DECL);\n", name);
      if (!guard.empty()) {
        format.Outdent();
        format("#endif  // $1$\n", guard);
        format.Indent();
      }
    };
    if (should_generate_guarded_tctable()) {
      format.Outdent();
      format("#ifdef PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
      format.Indent();
    }
    format("// The Tct_* functions are internal to the protobuf runtime:\n");
    // These guards are defined in port_def.inc:
    declare_function("Tct_ParseS1", "PROTOBUF_TC_STATIC_PARSE_SINGULAR1");
    declare_function("Tct_ParseS2", "PROTOBUF_TC_STATIC_PARSE_SINGULAR2");
    declare_function("Tct_ParseR1", "PROTOBUF_TC_STATIC_PARSE_REPEATED1");
    declare_function("Tct_ParseR2", "PROTOBUF_TC_STATIC_PARSE_REPEATED2");
    if (tc_table_info_->use_generated_fallback) {
      format.Outdent();
      format(
          " private:\n"
          "  ");
      declare_function("Tct_ParseFallback", "");
      format(" public:\n");
      format.Indent();
    }
    if (should_generate_guarded_tctable()) {
      format.Outdent();
      format("#endif\n");
      format.Indent();
    }
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
        "                  ::$proto_ns$::internal::ParseContext* ctx) {\n"
        "$annotate_deserialize$"
        "  return _extensions_.ParseMessageSet(ptr, \n"
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
  GenerateTailcallFieldParseFunctions(format);
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
      "    const char* ptr, ::$proto_ns$::internal::ParseContext* ctx) {\n"
      "$annotate_deserialize$"
      "  ptr = ::$proto_ns$::internal::TcParser<$1$>::ParseLoop(\n"
      "      this, ptr, ctx, &_table_.header);\n",
      tc_table_info_->table_size_log2);
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
    format("typed_msg->_has_bits_[0] = hasbits;\n");
  }

  format.Set("msg", "typed_msg->");
  format.Set("this", "typed_msg");
  format.Set("has_bits", "typed_msg->_has_bits_");
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

void ParseFunctionGenerator::GenerateTailcallFieldParseFunctions(
    Formatter& format) {
  GOOGLE_CHECK(should_generate_tctable());
  // There are four cases where a tailcall target are needed for messages:
  //   {singular, repeated} x {1, 2}-byte tag
  struct {
    const char* type;
    int size;
  } const kTagLayouts[] = {
      {"uint8_t", 1},
      {"uint16_t", 2},
  };
  // Singular:
  for (const auto& layout : kTagLayouts) {
    // Guard macros are defined in port_def.inc.
    format(
        "#if PROTOBUF_TC_STATIC_PARSE_SINGULAR$1$\n"
        "const char* $classname$::Tct_ParseS$1$(PROTOBUF_TC_PARAM_DECL) {\n"
        "  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<$2$>() != 0))\n"
        "    PROTOBUF_MUSTTAIL "
        "return table->fallback(PROTOBUF_TC_PARAM_PASS);\n"
        "  ptr += $1$;\n"
        "  hasbits |= (uint64_t{1} << data.hasbit_idx());\n"
        "  ::$proto_ns$::internal::TcParserBase::SyncHasbits"
        "(msg, hasbits, table);\n"
        "  auto& field = ::$proto_ns$::internal::TcParserBase::"
        "RefAt<$classtype$*>(msg, data.offset());\n"
        "  if (field == nullptr)\n"
        "    field = CreateMaybeMessage<$classtype$>(ctx->data().arena);\n"
        "  return ctx->ParseMessage(field, ptr);\n"
        "}\n"
        "#endif  // PROTOBUF_TC_STATIC_PARSE_SINGULAR$1$\n",
        layout.size, layout.type);
  }
  // Repeated:
  for (const auto& layout : kTagLayouts) {
    // Guard macros are defined in port_def.inc.
    format(
        "#if PROTOBUF_TC_STATIC_PARSE_REPEATED$1$\n"
        "const char* $classname$::Tct_ParseR$1$(PROTOBUF_TC_PARAM_DECL) {\n"
        "  if (PROTOBUF_PREDICT_FALSE(data.coded_tag<$2$>() != 0)) {\n"
        "    PROTOBUF_MUSTTAIL "
        "return table->fallback(PROTOBUF_TC_PARAM_PASS);\n"
        "  }\n"
        "  ptr += $1$;\n"
        "  auto& field = ::$proto_ns$::internal::TcParserBase::RefAt<"
        "::$proto_ns$::RepeatedPtrField<$classname$>>(msg, data.offset());\n"
        "  ::$proto_ns$::internal::TcParserBase::SyncHasbits"
        "(msg, hasbits, table);\n"
        "  ptr = ctx->ParseMessage(field.Add(), ptr);\n"
        "  return ptr;\n"
        "}\n"
        "#endif  // PROTOBUF_TC_STATIC_PARSE_REPEATED$1$\n",
        layout.size, layout.type);
  }
}

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
  format(
      "static const ::$proto_ns$::internal::TailCallParseTable<$1$>\n"
      "    _table_;\n",
      tc_table_info_->table_size_log2);
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
      "::$proto_ns$::internal::ParseContext* ctx) {\n"
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
    format.Set("has_bits", "_has_bits_");
  }
  format.Set("next_tag", "continue");
  format("while (!ctx->Done(&ptr)) {\n");
  format.Indent();

  GenerateParseIterationBody(format, descriptor_,
                             GetOrderedFields(descriptor_, options_));

  format.Outdent();
  format("}  // while\n");

  format.Outdent();
  format("message_done:\n");
  if (hasbits_size) format("  _has_bits_.Or(has_bits);\n");

  format(
      "  return ptr;\n"
      "failure:\n"
      "  ptr = nullptr;\n"
      "  goto message_done;\n"
      "#undef CHK_\n"
      "}\n");
}

void ParseFunctionGenerator::GenerateTailCallTable(Formatter& format) {
  GOOGLE_CHECK(should_generate_tctable());
  // All entries without a fast-path parsing function need a fallback.
  std::string fallback;
  if (tc_table_info_->use_generated_fallback) {
    fallback = ClassName(descriptor_) + "::Tct_ParseFallback";
  } else {
    fallback = TcParserBaseName(options_) + "GenericFallback";
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
  format(
      "const ::$proto_ns$::internal::TailCallParseTable<$1$>\n"
      "    $classname$::_table_ = {\n",
      tc_table_info_->table_size_log2);
  format.Indent();
  format("{\n");
  format.Indent();
  if (num_hasbits_ > 0 || IsMapEntryMessage(descriptor_)) {
    format("PROTOBUF_FIELD_OFFSET($classname$, _has_bits_),\n");
  } else {
    format("0,  // no _has_bits_\n");
  }
  if (descriptor_->extension_range_count() == 1) {
    format(
        "PROTOBUF_FIELD_OFFSET($classname$, _extensions_),\n"
        "$1$, $2$,  // extension_range_{low,high}\n",
        descriptor_->extension_range(0)->start,
        descriptor_->extension_range(0)->end);
  } else {
    format("0, 0, 0,  // no _extensions_\n");
  }
  format(
      "&$1$._instance,\n"
      "$2$  // fallback\n",
      DefaultInstanceName(descriptor_, options_), fallback);
  format.Outdent();
  format("}, {\n");
  format.Indent();
  for (const auto& info : tc_table_info_->fast_path_fields) {
    if (info.field != nullptr) {
      PrintFieldComment(format, info.field);
    }
    format("{$1$, ", info.func_name.empty() ? fallback : info.func_name);
    if (info.bits.data) {
      GOOGLE_DCHECK_NE(nullptr, info.field);
      format(
          "{$1$, $2$, "
          "static_cast<uint16_t>(PROTOBUF_FIELD_OFFSET($classname$, $3$_))}",
          info.bits.coded_tag(), info.bits.hasbit_idx(), FieldName(info.field));
    } else {
      format("{}");
    }
    format("},\n");
  }
  format.Outdent();
  format("},\n");  // entries[]
  format.Outdent();
  format("};\n\n");  // _table_
}

void ParseFunctionGenerator::GenerateArenaString(Formatter& format,
                                                 const FieldDescriptor* field) {
  if (HasHasbit(field)) {
    format("_Internal::set_has_$1$(&$has_bits$);\n", FieldName(field));
  }
  std::string default_string =
      field->default_value_string().empty()
          ? "::" + ProtobufNamespace(options_) +
                "::internal::GetEmptyStringAlreadyInited()"
          : QualifiedClassName(field->containing_type(), options_) +
                "::" + MakeDefaultName(field) + ".get()";
  format(
      "if (arena != nullptr) {\n"
      "  ptr = ctx->ReadArenaString(ptr, &$msg$$name$_, arena");
  if (IsStringInlined(field, options_)) {
    GOOGLE_DCHECK(!inlined_string_indices_.empty());
    int inlined_string_index = inlined_string_indices_[field->index()];
    GOOGLE_DCHECK_GE(inlined_string_index, 0);
    format(
        ", $msg$_internal_$name$_donated()"
        ", &$msg$_inlined_string_donated_[$1$]"
        ", ~0x$2$u",
        inlined_string_index / 32,
        strings::Hex(1u << (inlined_string_index % 32), strings::ZERO_PAD_8));
  }
  format(
      ");\n"
      "} else {\n"
      "  ptr = ::$proto_ns$::internal::InlineGreedyStringParser("
      "$msg$$name$_.MutableNoArenaNoDefault(&$1$), ptr, ctx);\n"
      "}\n"
      "const std::string* str = &$msg$$name$_.Get(); (void)str;\n",
      default_string);
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
        "ptr = ::$proto_ns$::internal::Inline$3$(str, ptr, ctx);\n",
        HasInternalAccessors(ctype) ? "_internal_" : "",
        field->is_repeated() && !field->is_packable() ? "add" : "mutable",
        parser_name);
  }
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
  format("::$proto_ns$::internal::VerifyUTF8(str, $1$)", field_name);
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
          const FieldDescriptor* val =
              field->message_type()->FindFieldByName("value");
          GOOGLE_CHECK(val);
          if (val->type() == FieldDescriptor::TYPE_ENUM &&
              !HasPreservingUnknownEnumSemantics(field)) {
            format(
                "auto object = "
                "::$proto_ns$::internal::InitEnumParseWrapper<"
                "$unknown_fields_type$>(&$msg$$name$_, $1$_IsValid, "
                "$2$, &$msg$_internal_metadata_);\n"
                "ptr = ctx->ParseMessage(&object, ptr);\n",
                QualifiedClassName(val->enum_type(), options_),
                field->number());
          } else {
            format("ptr = ctx->ParseMessage(&$msg$$name$_, ptr);\n");
          }
        } else if (IsLazy(field, options_, scc_analyzer_)) {
          if (field->real_containing_oneof()) {
            format(
                "if (!$msg$_internal_has_$name$()) {\n"
                "  $msg$clear_$1$();\n"
                "  $msg$$1$_.$name$_ = ::$proto_ns$::Arena::CreateMessage<\n"
                "      ::$proto_ns$::internal::LazyField>("
                "$msg$GetArenaForAllocation());\n"
                "  $msg$set_has_$name$();\n"
                "}\n"
                "ptr = ctx->ParseMessage($msg$$1$_.$name$_, ptr);\n",
                field->containing_oneof()->name());
          } else if (HasHasbit(field)) {
            format(
                "_Internal::set_has_$name$(&$has_bits$);\n"
                "ptr = ctx->ParseMessage(&$msg$$name$_, ptr);\n");
          } else {
            format("ptr = ctx->ParseMessage(&$msg$$name$_, ptr);\n");
          }
        } else if (IsImplicitWeakField(field, options_, scc_analyzer_)) {
          if (!field->is_repeated()) {
            format(
                "ptr = ctx->ParseMessage(_Internal::mutable_$name$($this$), "
                "ptr);\n");
          } else {
            format(
                "ptr = ctx->ParseMessage($msg$$name$_.AddWeak("
                "reinterpret_cast<const ::$proto_ns$::MessageLite*>($1$ptr_)"
                "), ptr);\n",
                QualifiedDefaultInstanceName(field->message_type(), options_));
          }
        } else if (IsWeak(field, options_)) {
          format(
              "{\n"
              "  auto* default_ = &reinterpret_cast<const Message&>($1$);\n"
              "  ptr = ctx->ParseMessage($msg$_weak_field_map_.MutableMessage("
              "$2$, default_), ptr);\n"
              "}\n",
              QualifiedDefaultInstanceName(field->message_type(), options_),
              field->number());
        } else {
          format(
              "ptr = ctx->ParseMessage($msg$_internal_$mutable_field$(), "
              "ptr);\n");
        }
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
              "$msg$$name$_ = ::$proto_ns$::internal::ReadVarint$1$$2$(&ptr);\n"
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
            "$msg$$name$_ = "
            "::$proto_ns$::internal::UnalignedLoad<$primitive_type$>(ptr);\n"
            "ptr += sizeof($primitive_type$);\n");
      }
      break;
    }
    case WireFormatLite::WIRETYPE_LENGTH_DELIMITED: {
      GenerateLengthDelim(format, field);
      format("CHK_(ptr);\n");
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
    const std::vector<const FieldDescriptor*>& ordered_fields) {
  format(
      "$uint32$ tag;\n"
      "ptr = ::$proto_ns$::internal::ReadTag(ptr, &tag);\n");

  if (!ordered_fields.empty()) {
    GenerateFieldSwitch(format, ordered_fields);
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
          "  ptr = $msg$_extensions_.ParseField(tag, ptr, "
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
    Formatter& format,
    const std::vector<const FieldDescriptor*>& ordered_fields) {
  format("switch (tag >> 3) {\n");
  format.Indent();

  for (const auto* field : ordered_fields) {
    PrintFieldComment(format, field);
    format("case $1$:\n", field->number());
    format.Indent();
    uint32_t fallback_tag = 0;
    uint32_t expected_tag = ExpectedTag(field, &fallback_tag);
    format("if (PROTOBUF_PREDICT_TRUE(static_cast<$uint8$>(tag) == $1$)) {\n",
           expected_tag & 0xFF);
    format.Indent();
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

std::string FieldParseFunctionName(const FieldDescriptor* field,
                                   const Options& options,
                                   uint32_t table_size_log2) {
  ParseCardinality card =  //
      field->is_packed()               ? ParseCardinality::kPacked
      : field->is_repeated()           ? ParseCardinality::kRepeated
      : field->real_containing_oneof() ? ParseCardinality::kOneof
                                       : ParseCardinality::kSingular;

  TypeFormat type_format;
  switch (field->type()) {
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_DOUBLE:
      type_format = TypeFormat::kFixed64;
      break;

    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_FLOAT:
      type_format = TypeFormat::kFixed32;
      break;

    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
      type_format = TypeFormat::kVar64;
      break;

    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
      type_format = TypeFormat::kVar32;
      break;

    case FieldDescriptor::TYPE_SINT64:
      type_format = TypeFormat::kSInt64;
      break;

    case FieldDescriptor::TYPE_SINT32:
      type_format = TypeFormat::kSInt32;
      break;

    case FieldDescriptor::TYPE_BOOL:
      type_format = TypeFormat::kBool;
      break;

    case FieldDescriptor::TYPE_BYTES:
      type_format = TypeFormat::kBytes;
      break;

    case FieldDescriptor::TYPE_STRING:
      switch (GetUtf8CheckMode(field, options)) {
        case Utf8CheckMode::kNone:
          type_format = TypeFormat::kBytes;
          break;
        case Utf8CheckMode::kStrict:
          type_format = TypeFormat::kString;
          break;
        case Utf8CheckMode::kVerify:
          type_format = TypeFormat::kStringValidateOnly;
          break;
        default:
          GOOGLE_LOG(DFATAL) << "Mode not handled: "
                      << static_cast<int>(GetUtf8CheckMode(field, options));
          return "";
      }
      break;

    default:
      GOOGLE_LOG(DFATAL) << "Type not handled: " << field->DebugString();
      return "";
  }

  return "::" + ProtobufNamespace(options) + "::internal::" +
         GetTailCallFieldHandlerName(card, type_format, table_size_log2,
                                     TagSize(field->number()), options);
}

}  // namespace

std::string GetTailCallFieldHandlerName(ParseCardinality card,
                                        TypeFormat type_format,
                                        int table_size_log2,
                                        int tag_length_bytes,
                                        const Options& options) {
  std::string name;

  switch (card) {
    case ParseCardinality::kPacked:
    case ParseCardinality::kRepeated:
      name = "TcParserBase::";
      break;

    case ParseCardinality::kSingular:
    case ParseCardinality::kOneof:
      switch (type_format) {
        case TypeFormat::kBytes:
        case TypeFormat::kString:
        case TypeFormat::kStringValidateOnly:
          name = "TcParserBase::";
          break;

        default:
          name = StrCat("TcParser<", table_size_log2, ">::");
          break;
      }
  }

  // The field implementation functions are prefixed by cardinality:
  //   `Singular` for optional or implicit fields.
  //   `Repeated` for non-packed repeated.
  //   `Packed` for packed repeated.
  switch (card) {
    case ParseCardinality::kSingular:
      name.append("Singular");
      break;
    case ParseCardinality::kOneof:
      name.append("Oneof");
      break;
    case ParseCardinality::kRepeated:
      name.append("Repeated");
      break;
    case ParseCardinality::kPacked:
      name.append("Packed");
      break;
  }

  // Next in the function name is the TypeFormat-specific name.
  switch (type_format) {
    case TypeFormat::kFixed64:
    case TypeFormat::kFixed32:
      name.append("Fixed");
      break;

    case TypeFormat::kVar64:
    case TypeFormat::kVar32:
    case TypeFormat::kSInt64:
    case TypeFormat::kSInt32:
    case TypeFormat::kBool:
      name.append("Varint");
      break;

    case TypeFormat::kBytes:
    case TypeFormat::kString:
    case TypeFormat::kStringValidateOnly:
      name.append("String");
      break;

    default:
      break;
  }

  name.append("<");

  // Determine the numeric layout type for the parser to use, independent of
  // the specific parsing logic used.
  switch (type_format) {
    case TypeFormat::kVar64:
    case TypeFormat::kFixed64:
      name.append("uint64_t, ");
      break;

    case TypeFormat::kSInt64:
      name.append("int64_t, ");
      break;

    case TypeFormat::kVar32:
    case TypeFormat::kFixed32:
      name.append("uint32_t, ");
      break;

    case TypeFormat::kSInt32:
      name.append("int32_t, ");
      break;

    case TypeFormat::kBool:
      name.append("bool, ");
      break;

    default:
      break;
  }

  name.append(CodedTagType(tag_length_bytes));

  switch (type_format) {
    case TypeFormat::kVar64:
    case TypeFormat::kVar32:
    case TypeFormat::kBool:
      name.append(
          StrCat(", ", TcParserBaseName(options), "kNoConversion"));
      break;

    case TypeFormat::kSInt64:
    case TypeFormat::kSInt32:
      name.append(StrCat(", ", TcParserBaseName(options), "kZigZag"));
      break;

    case TypeFormat::kBytes:
      name.append(StrCat(", ", TcParserBaseName(options), "kNoUtf8"));
      break;

    case TypeFormat::kString:
      name.append(StrCat(", ", TcParserBaseName(options), "kUtf8"));
      break;

    case TypeFormat::kStringValidateOnly:
      name.append(
          StrCat(", ", TcParserBaseName(options), "kUtf8ValidateOnly"));
      break;

    default:
      break;
  }

  name.append(">");
  return name;
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
