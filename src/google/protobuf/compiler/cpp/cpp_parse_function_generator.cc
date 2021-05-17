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

#include <google/protobuf/wire_format.h>

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

bool IsTCTableEnabled(const Options& options) {
  return options.tctable_mode == Options::kTCTableAlways;
}
bool IsTCTableGuarded(const Options& options) {
  return options.tctable_mode == Options::kTCTableGuarded;
}
bool IsTCTableDisabled(const Options& options) {
  return options.tctable_mode == Options::kTCTableNever;
}

}  // namespace

ParseFunctionGenerator::ParseFunctionGenerator(const Descriptor* descriptor,
                                               int num_hasbits,
                                               const Options& options,
                                               MessageSCCAnalyzer* scc_analyzer)
    : descriptor_(descriptor),
      scc_analyzer_(scc_analyzer),
      options_(options),
      num_hasbits_(num_hasbits) {
  SetCommonVars(options_, &variables_);
  SetUnknkownFieldsVariable(descriptor_, options_, &variables_);
  variables_["classname"] = ClassName(descriptor, false);
}

void ParseFunctionGenerator::GenerateMethodDecls(io::Printer* printer) {
  Formatter format(printer, variables_);
  if (IsTCTableGuarded(options_)) {
    format.Outdent();
    format("#ifdef PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
    format.Indent();
  }
  if (IsTCTableGuarded(options_) || IsTCTableEnabled(options_)) {
    format.Outdent();
    format("#error Tail Call Table parsing not yet implemented.\n");
    format.Indent();
  }
  if (IsTCTableGuarded(options_)) {
    format.Outdent();
    format("#else\n");
    format.Indent();
  }
  if (IsTCTableGuarded(options_) || IsTCTableDisabled(options_)) {
    format(
        "const char* _InternalParse(const char* ptr, "
        "::$proto_ns$::internal::ParseContext* ctx) final;\n");
  }
  if (IsTCTableGuarded(options_)) {
    format.Outdent();
    format("#endif\n");
    format.Indent();
  }
}

void ParseFunctionGenerator::GenerateMethodImpls(io::Printer* printer) {
  Formatter format(printer, variables_);
  if (descriptor_->options().message_set_wire_format()) {
    // Special-case MessageSet.
    format(
        "const char* $classname$::_InternalParse(const char* ptr,\n"
        "                  ::$proto_ns$::internal::ParseContext* ctx) {\n"
        "  return _extensions_.ParseMessageSet(ptr, \n"
        "      internal_default_instance(), &_internal_metadata_, ctx);\n"
        "}\n");
    return;
  }
  if (IsTCTableGuarded(options_)) {
    format("#ifdef PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
  }
  if (IsTCTableGuarded(options_) || IsTCTableEnabled(options_)) {
    format("#error Tail Call Table parsing not yet implemented.\n");
  }
  if (IsTCTableGuarded(options_)) {
    format("#else  // PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
  }
  if (IsTCTableGuarded(options_) || IsTCTableDisabled(options_)) {
    GenerateLoopingParseFunction(format);
  }
  if (IsTCTableGuarded(options_)) {
    format("#endif  // PROTOBUF_TAIL_CALL_TABLE_PARSER_ENABLED\n");
  }
}

void ParseFunctionGenerator::GenerateLoopingParseFunction(Formatter& format) {
  format(
      "const char* $classname$::_InternalParse(const char* ptr, "
      "::$proto_ns$::internal::ParseContext* ctx) {\n"
      "#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure\n");
  format.Indent();
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
  format.Set("continue", "continue");
  format("while (!ctx->Done(&ptr)) {\n");
  format.Indent();

  GenerateParseIterationBody(format, descriptor_,
                             GetOrderedFields(descriptor_, options_));

  format.Outdent();
  format("}  // while\n");

  format.Outdent();
  format("success:\n");
  if (hasbits_size) format("  _has_bits_.Or(has_bits);\n");

  format(
      "  return ptr;\n"
      "failure:\n"
      "  ptr = nullptr;\n"
      "  goto success;\n"
      "#undef CHK_\n"
      "}\n");
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
      "  ptr = ctx->ReadArenaString(ptr, &$1$_, arena);\n"
      "} else {\n"
      "  ptr = ::$proto_ns$::internal::InlineGreedyStringParser("
      "$1$_.MutableNoArenaNoDefault(&$2$), ptr, ctx);\n"
      "}\n"
      "const std::string* str = &$1$_.Get(); (void)str;\n",
      FieldName(field), default_string);
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
    std::string name;
    switch (ctype) {
      case FieldOptions::STRING:
        name = "GreedyStringParser";
        break;
      case FieldOptions::CORD:
        name = "CordParser";
        break;
      case FieldOptions::STRING_PIECE:
        name = "StringPieceParser";
        break;
    }
    format(
        "auto str = $1$$2$_$3$();\n"
        "ptr = ::$proto_ns$::internal::Inline$4$(str, ptr, ctx);\n",
        HasInternalAccessors(ctype) ? "_internal_" : "",
        field->is_repeated() && !field->is_packable() ? "add" : "mutable",
        FieldName(field), name);
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
    std::string enum_validator;
    if (field->type() == FieldDescriptor::TYPE_ENUM &&
        !HasPreservingUnknownEnumSemantics(field)) {
      enum_validator =
          StrCat(", ", QualifiedClassName(field->enum_type(), options_),
                       "_IsValid, &_internal_metadata_, ", field->number());
      format(
          "ptr = "
          "::$proto_ns$::internal::Packed$1$Parser<$unknown_fields_type$>("
          "_internal_mutable_$2$(), ptr, ctx$3$);\n",
          DeclaredTypeMethodName(field->type()), FieldName(field),
          enum_validator);
    } else {
      format(
          "ptr = ::$proto_ns$::internal::Packed$1$Parser("
          "_internal_mutable_$2$(), ptr, ctx$3$);\n",
          DeclaredTypeMethodName(field->type()), FieldName(field),
          enum_validator);
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
                "::$proto_ns$::internal::InitEnumParseWrapper<$unknown_"
                "fields_type$>("
                "&$1$_, $2$_IsValid, $3$, &_internal_metadata_);\n"
                "ptr = ctx->ParseMessage(&object, ptr);\n",
                FieldName(field), QualifiedClassName(val->enum_type()),
                field->number());
          } else {
            format("ptr = ctx->ParseMessage(&$1$_, ptr);\n", FieldName(field));
          }
        } else if (IsLazy(field, options_)) {
          if (field->real_containing_oneof()) {
            format(
                "if (!_internal_has_$1$()) {\n"
                "  clear_$2$();\n"
                "  $2$_.$1$_ = ::$proto_ns$::Arena::CreateMessage<\n"
                "      ::$proto_ns$::internal::LazyField>("
                "GetArenaForAllocation());\n"
                "  set_has_$1$();\n"
                "}\n"
                "ptr = ctx->ParseMessage($2$_.$1$_, ptr);\n",
                FieldName(field), field->containing_oneof()->name());
          } else if (HasHasbit(field)) {
            format(
                "_Internal::set_has_$1$(&$has_bits$);\n"
                "ptr = ctx->ParseMessage(&$1$_, ptr);\n",
                FieldName(field));
          } else {
            format("ptr = ctx->ParseMessage(&$1$_, ptr);\n", FieldName(field));
          }
        } else if (IsImplicitWeakField(field, options_, scc_analyzer_)) {
          if (!field->is_repeated()) {
            format(
                "ptr = ctx->ParseMessage(_Internal::mutable_$1$(this), "
                "ptr);\n",
                FieldName(field));
          } else {
            format(
                "ptr = ctx->ParseMessage($1$_.AddWeak(reinterpret_cast<const "
                "::$proto_ns$::MessageLite*>($2$::_$3$_default_instance_ptr_)"
                "), ptr);\n",
                FieldName(field), Namespace(field->message_type(), options_),
                ClassName(field->message_type()));
          }
        } else if (IsWeak(field, options_)) {
          format(
              "{\n"
              "  auto* default_ = &reinterpret_cast<const Message&>($1$);\n"
              "  ptr = ctx->ParseMessage(_weak_field_map_.MutableMessage($2$,"
              " default_), ptr);\n"
              "}\n",
              QualifiedDefaultInstanceName(field->message_type(), options_),
              field->number());
        } else {
          format("ptr = ctx->ParseMessage(_internal_$1$_$2$(), ptr);\n",
                 field->is_repeated() ? "add" : "mutable", FieldName(field));
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
  uint32_t tag = WireFormatLite::MakeTag(field->number(), wiretype);
  switch (wiretype) {
    case WireFormatLite::WIRETYPE_VARINT: {
      std::string type = PrimitiveTypeName(options_, field->cpp_type());
      std::string prefix = field->is_repeated() ? "add" : "set";
      if (field->type() == FieldDescriptor::TYPE_ENUM) {
        format(
            "$uint64$ val = ::$proto_ns$::internal::ReadVarint64(&ptr);\n"
            "CHK_(ptr);\n");
        if (!HasPreservingUnknownEnumSemantics(field)) {
          format("if (PROTOBUF_PREDICT_TRUE($1$_IsValid(val))) {\n",
                 QualifiedClassName(field->enum_type(), options_));
          format.Indent();
        }
        format("_internal_$1$_$2$(static_cast<$3$>(val));\n", prefix,
               FieldName(field),
               QualifiedClassName(field->enum_type(), options_));
        if (!HasPreservingUnknownEnumSemantics(field)) {
          format.Outdent();
          format(
              "} else {\n"
              "  ::$proto_ns$::internal::WriteVarint("
              "$1$, val, mutable_unknown_fields());\n"
              "}\n",
              field->number());
        }
      } else {
        std::string size = (field->type() == FieldDescriptor::TYPE_SINT32 ||
                            field->type() == FieldDescriptor::TYPE_UINT32)
                               ? "32"
                               : "64";
        std::string zigzag;
        if ((field->type() == FieldDescriptor::TYPE_SINT32 ||
             field->type() == FieldDescriptor::TYPE_SINT64)) {
          zigzag = "ZigZag";
        }
        if (field->is_repeated() || field->real_containing_oneof()) {
          std::string prefix = field->is_repeated() ? "add" : "set";
          format(
              "_internal_$1$_$2$("
              "::$proto_ns$::internal::ReadVarint$3$$4$(&ptr));\n"
              "CHK_(ptr);\n",
              prefix, FieldName(field), zigzag, size);
        } else {
          if (HasHasbit(field)) {
            format("_Internal::set_has_$1$(&$has_bits$);\n", FieldName(field));
          }
          format(
              "$1$_ = ::$proto_ns$::internal::ReadVarint$2$$3$(&ptr);\n"
              "CHK_(ptr);\n",
              FieldName(field), zigzag, size);
        }
      }
      break;
    }
    case WireFormatLite::WIRETYPE_FIXED32:
    case WireFormatLite::WIRETYPE_FIXED64: {
      std::string type = PrimitiveTypeName(options_, field->cpp_type());
      if (field->is_repeated() || field->real_containing_oneof()) {
        std::string prefix = field->is_repeated() ? "add" : "set";
        format(
            "_internal_$1$_$2$("
            "::$proto_ns$::internal::UnalignedLoad<$3$>(ptr));\n"
            "ptr += sizeof($3$);\n",
            prefix, FieldName(field), type);
      } else {
        if (HasHasbit(field)) {
          format("_Internal::set_has_$1$(&$has_bits$);\n", FieldName(field));
        }
        format(
            "$1$_ = ::$proto_ns$::internal::UnalignedLoad<$2$>(ptr);\n"
            "ptr += sizeof($2$);\n",
            FieldName(field), type);
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
          "ptr = ctx->ParseGroup(_internal_$1$_$2$(), ptr, $3$);\n"
          "CHK_(ptr);\n",
          field->is_repeated() ? "add" : "mutable", FieldName(field), tag);
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

void ParseFunctionGenerator::GenerateParseIterationBody(
    Formatter& format, const Descriptor* descriptor,
    const std::vector<const FieldDescriptor*>& ordered_fields) {
  format(
      "$uint32$ tag;\n"
      "ptr = ::$proto_ns$::internal::ReadTag(ptr, &tag);\n");
  if (!ordered_fields.empty()) format("switch (tag >> 3) {\n");

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
    format.Outdent();
    format(
        "  } else goto handle_unusual;\n"
        "  $continue$;\n");
  }  // for loop over ordered fields

  // Default case
  if (!ordered_fields.empty()) format("default: {\n");
  if (!ordered_fields.empty()) format("handle_unusual:\n");
  format(
      "  if ((tag == 0) || ((tag & 7) == 4)) {\n"
      "    CHK_(ptr);\n"
      "    ctx->SetLastTag(tag);\n"
      "    goto success;\n"
      "  }\n");
  if (IsMapEntryMessage(descriptor)) {
    format("  $continue$;\n");
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
      format(") {\n");
      format(
          "  ptr = _extensions_.ParseField(tag, ptr,\n"
          "      internal_default_instance(), &_internal_metadata_, ctx);\n"
          "  CHK_(ptr != nullptr);\n"
          "  $continue$;\n"
          "}\n");
    }
    format(
        "  ptr = UnknownFieldParse(tag,\n"
        "      _internal_metadata_.mutable_unknown_fields<$unknown_"
        "fields_type$>(),\n"
        "      ptr, ctx);\n"
        "  CHK_(ptr != nullptr);\n"
        "  $continue$;\n");
  }
  if (!ordered_fields.empty()) format("}\n");  // default case
  format.Outdent();
  if (!ordered_fields.empty()) format("}  // switch\n");
}

}  // namespace cpp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
