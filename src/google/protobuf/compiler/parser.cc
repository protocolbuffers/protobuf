// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.
//
// Recursive descent FTW.

#include "google/protobuf/compiler/parser.h"


#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/cleanup/cleanup.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/log/absl_log.h"
#include "absl/strings/ascii.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/strtod.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/message_lite.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace {

using TypeNameMap =
    absl::flat_hash_map<absl::string_view, FieldDescriptorProto::Type>;

const TypeNameMap& GetTypeNameTable() {
  static auto* table = new auto([]() {
    TypeNameMap result;

    result["double"] = FieldDescriptorProto::TYPE_DOUBLE;
    result["float"] = FieldDescriptorProto::TYPE_FLOAT;
    result["uint64"] = FieldDescriptorProto::TYPE_UINT64;
    result["fixed64"] = FieldDescriptorProto::TYPE_FIXED64;
    result["fixed32"] = FieldDescriptorProto::TYPE_FIXED32;
    result["bool"] = FieldDescriptorProto::TYPE_BOOL;
    result["string"] = FieldDescriptorProto::TYPE_STRING;
    result["group"] = FieldDescriptorProto::TYPE_GROUP;

    result["bytes"] = FieldDescriptorProto::TYPE_BYTES;
    result["uint32"] = FieldDescriptorProto::TYPE_UINT32;
    result["sfixed32"] = FieldDescriptorProto::TYPE_SFIXED32;
    result["sfixed64"] = FieldDescriptorProto::TYPE_SFIXED64;
    result["int32"] = FieldDescriptorProto::TYPE_INT32;
    result["int64"] = FieldDescriptorProto::TYPE_INT64;
    result["sint32"] = FieldDescriptorProto::TYPE_SINT32;
    result["sint64"] = FieldDescriptorProto::TYPE_SINT64;

    return result;
  }());
  return *table;
}

// Camel-case the field name and append "Entry" for generated map entry name.
// e.g. map<KeyType, ValueType> foo_map => FooMapEntry
std::string MapEntryName(absl::string_view field_name) {
  std::string result;
  static const char kSuffix[] = "Entry";
  result.reserve(field_name.size() + sizeof(kSuffix));
  bool cap_next = true;
  for (const char field_name_char : field_name) {
    if (field_name_char == '_') {
      cap_next = true;
    } else if (cap_next) {
      // Note: Do not use ctype.h due to locales.
      if ('a' <= field_name_char && field_name_char <= 'z') {
        result.push_back(field_name_char - 'a' + 'A');
      } else {
        result.push_back(field_name_char);
      }
      cap_next = false;
    } else {
      result.push_back(field_name_char);
    }
  }
  result.append(kSuffix);
  return result;
}

}  // anonymous namespace

// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) \
  if (STATEMENT) {    \
  } else              \
    return false

// ===================================================================

Parser::Parser()
    : input_(nullptr),
      error_collector_(nullptr),
      source_location_table_(nullptr),
      had_errors_(false),
      require_syntax_identifier_(false),
      stop_after_syntax_identifier_(false) {
}

Parser::~Parser() = default;
// ===================================================================

inline bool Parser::LookingAt(absl::string_view text) {
  return input_->current().text == text;
}

inline bool Parser::LookingAtType(io::Tokenizer::TokenType token_type) {
  return input_->current().type == token_type;
}

inline bool Parser::AtEnd() { return LookingAtType(io::Tokenizer::TYPE_END); }

bool Parser::TryConsume(absl::string_view text) {
  if (LookingAt(text)) {
    input_->Next();
    return true;
  } else {
    return false;
  }
}

bool Parser::Consume(absl::string_view text, ErrorMaker error) {
  if (TryConsume(text)) {
    return true;
  } else {
    RecordError(error);
    return false;
  }
}

bool Parser::Consume(absl::string_view text) {
  return Consume(text,
                 [&] { return absl::StrCat("Expected \"", text, "\"."); });
}

bool Parser::ConsumeIdentifier(std::string* output, ErrorMaker error) {
  if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
    *output = input_->current().text;
    input_->Next();
    return true;
  } else {
    RecordError(error);
    return false;
  }
}

bool Parser::ConsumeInteger(int* output, ErrorMaker error) {
  if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
    uint64_t value = 0;
    if (!io::Tokenizer::ParseInteger(input_->current().text,
                                     std::numeric_limits<int32_t>::max(),
                                     &value)) {
      RecordError("Integer out of range.");
      // We still return true because we did, in fact, parse an integer.
    }
    *output = value;
    input_->Next();
    return true;
  } else {
    RecordError(error);
    return false;
  }
}

bool Parser::ConsumeSignedInteger(int* output, ErrorMaker error) {
  bool is_negative = false;
  uint64_t max_value = std::numeric_limits<int32_t>::max();
  if (TryConsume("-")) {
    is_negative = true;
    max_value += 1;
  }
  uint64_t value = 0;
  DO(ConsumeInteger64(max_value, &value, error));
  if (is_negative) value *= -1;
  *output = value;
  return true;
}

bool Parser::ConsumeInteger64(uint64_t max_value, uint64_t* output,
                              ErrorMaker error) {
  if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
    if (!io::Tokenizer::ParseInteger(input_->current().text, max_value,
                                     output)) {
      RecordError("Integer out of range.");
      // We still return true because we did, in fact, parse an integer.
      *output = 0;
    }
    input_->Next();
    return true;
  } else {
    RecordError(error);
    return false;
  }
}

bool Parser::TryConsumeInteger64(uint64_t max_value, uint64_t* output) {
  if (LookingAtType(io::Tokenizer::TYPE_INTEGER) &&
      io::Tokenizer::ParseInteger(input_->current().text, max_value, output)) {
    input_->Next();
    return true;
  }
  return false;
}

bool Parser::ConsumeNumber(double* output, ErrorMaker error) {
  if (LookingAtType(io::Tokenizer::TYPE_FLOAT)) {
    *output = io::Tokenizer::ParseFloat(input_->current().text);
    input_->Next();
    return true;
  } else if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
    // Also accept integers.
    uint64_t value = 0;
    if (io::Tokenizer::ParseInteger(input_->current().text,
                                    std::numeric_limits<uint64_t>::max(),
                                    &value)) {
      *output = value;
    } else if (input_->current().text[0] == '0') {
      // octal or hexadecimal; don't bother parsing as float
      RecordError("Integer out of range.");
      // We still return true because we did, in fact, parse a number.
    } else if (!io::Tokenizer::TryParseFloat(input_->current().text, output)) {
      // out of int range, and not valid float? 🤷
      RecordError("Integer out of range.");
      // We still return true because we did, in fact, parse a number.
    }
    input_->Next();
    return true;
  } else if (LookingAt("inf")) {
    *output = std::numeric_limits<double>::infinity();
    input_->Next();
    return true;
  } else if (LookingAt("nan")) {
    *output = std::numeric_limits<double>::quiet_NaN();
    input_->Next();
    return true;
  } else {
    RecordError(error);
    return false;
  }
}

bool Parser::ConsumeString(std::string* output, ErrorMaker error) {
  if (LookingAtType(io::Tokenizer::TYPE_STRING)) {
    io::Tokenizer::ParseString(input_->current().text, output);
    input_->Next();
    // Allow C++ like concatenation of adjacent string tokens.
    while (LookingAtType(io::Tokenizer::TYPE_STRING)) {
      io::Tokenizer::ParseStringAppend(input_->current().text, output);
      input_->Next();
    }
    return true;
  } else {
    RecordError(error);
    return false;
  }
}

bool Parser::TryConsumeEndOfDeclaration(absl::string_view text,
                                        const LocationRecorder* location) {
  if (LookingAt(text)) {
    std::string leading, trailing;
    std::vector<std::string> detached;
    input_->NextWithComments(&trailing, &detached, &leading);

    // Save the leading comments for next time, and recall the leading comments
    // from last time.
    leading.swap(upcoming_doc_comments_);

    if (location != nullptr) {
      upcoming_detached_comments_.swap(detached);
      location->AttachComments(&leading, &trailing, &detached);
    } else if (text == "}") {
      // If the current location is null and we are finishing the current scope,
      // drop pending upcoming detached comments.
      upcoming_detached_comments_.swap(detached);
    } else {
      // Otherwise, append the new detached comments to the existing upcoming
      // detached comments.
      upcoming_detached_comments_.insert(upcoming_detached_comments_.end(),
                                         detached.begin(), detached.end());
    }

    return true;
  } else {
    return false;
  }
}

bool Parser::ConsumeEndOfDeclaration(absl::string_view text,
                                     const LocationRecorder* location) {
  if (TryConsumeEndOfDeclaration(text, location)) {
    return true;
  } else {
    RecordError([&] { return absl::StrCat("Expected \"", text, "\"."); });
    return false;
  }
}

// -------------------------------------------------------------------

void Parser::RecordError(int line, int column, ErrorMaker error) {
  if (error_collector_ != nullptr) {
    error_collector_->RecordError(line, column, error.get());
  }
  had_errors_ = true;
}

void Parser::RecordError(ErrorMaker error) {
  RecordError(input_->current().line, input_->current().column, error);
}

void Parser::RecordWarning(int line, int column, ErrorMaker error) {
  if (error_collector_ != nullptr) {
    error_collector_->RecordWarning(line, column, error.get());
  }
}

// Invokes error_collector_->RecordWarning() with the line and column number
// of the current token.
void Parser::RecordWarning(ErrorMaker error) {
  RecordWarning(input_->current().line, input_->current().column, error);
}

// -------------------------------------------------------------------

Parser::LocationRecorder::LocationRecorder(Parser* parser)
    : parser_(parser),
      source_code_info_(parser->source_code_info_),
      location_(parser_->source_code_info_->add_location()) {
  location_->add_span(parser_->input_->current().line);
  location_->add_span(parser_->input_->current().column);
}

Parser::LocationRecorder::LocationRecorder(const LocationRecorder& parent) {
  Init(parent, parent.source_code_info_);
}

Parser::LocationRecorder::LocationRecorder(const LocationRecorder& parent,
                                           int path1,
                                           SourceCodeInfo* source_code_info) {
  Init(parent, source_code_info);
  AddPath(path1);
}

Parser::LocationRecorder::LocationRecorder(const LocationRecorder& parent,
                                           int path1) {
  Init(parent, parent.source_code_info_);
  AddPath(path1);
}

Parser::LocationRecorder::LocationRecorder(const LocationRecorder& parent,
                                           int path1, int path2) {
  Init(parent, parent.source_code_info_);
  AddPath(path1);
  AddPath(path2);
}

void Parser::LocationRecorder::Init(const LocationRecorder& parent,
                                    SourceCodeInfo* source_code_info) {
  parser_ = parent.parser_;
  source_code_info_ = source_code_info;

  location_ = source_code_info_->add_location();
  location_->mutable_path()->CopyFrom(parent.location_->path());

  location_->add_span(parser_->input_->current().line);
  location_->add_span(parser_->input_->current().column);
}

Parser::LocationRecorder::~LocationRecorder() {
  if (location_->span_size() <= 2) {
    EndAt(parser_->input_->previous());
  }
}

void Parser::LocationRecorder::AddPath(int path_component) {
  location_->add_path(path_component);
}

void Parser::LocationRecorder::StartAt(const io::Tokenizer::Token& token) {
  location_->set_span(0, token.line);
  location_->set_span(1, token.column);
}

void Parser::LocationRecorder::StartAt(const LocationRecorder& other) {
  location_->set_span(0, other.location_->span(0));
  location_->set_span(1, other.location_->span(1));
}

void Parser::LocationRecorder::EndAt(const io::Tokenizer::Token& token) {
  if (token.line != location_->span(0)) {
    location_->add_span(token.line);
  }
  location_->add_span(token.end_column);
}

void Parser::LocationRecorder::RecordLegacyLocation(
    const Message* descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location) {
  if (parser_->source_location_table_ != nullptr) {
    parser_->source_location_table_->Add(
        descriptor, location, location_->span(0), location_->span(1));
  }
}

void Parser::LocationRecorder::RecordLegacyImportLocation(
    const Message* descriptor, const std::string& name) {
  if (parser_->source_location_table_ != nullptr) {
    parser_->source_location_table_->AddImport(
        descriptor, name, location_->span(0), location_->span(1));
  }
}

int Parser::LocationRecorder::CurrentPathSize() const {
  return location_->path_size();
}

void Parser::LocationRecorder::AttachComments(
    std::string* leading, std::string* trailing,
    std::vector<std::string>* detached_comments) const {
  ABSL_CHECK(!location_->has_leading_comments());
  ABSL_CHECK(!location_->has_trailing_comments());

  if (!leading->empty()) {
    location_->mutable_leading_comments()->swap(*leading);
  }
  if (!trailing->empty()) {
    location_->mutable_trailing_comments()->swap(*trailing);
  }
  for (int i = 0; i < detached_comments->size(); ++i) {
    location_->add_leading_detached_comments()->swap((*detached_comments)[i]);
  }
  detached_comments->clear();
}

// -------------------------------------------------------------------

void Parser::SkipStatement() {
  while (true) {
    if (AtEnd()) {
      return;
    } else if (LookingAtType(io::Tokenizer::TYPE_SYMBOL)) {
      if (TryConsumeEndOfDeclaration(";", nullptr)) {
        return;
      } else if (TryConsume("{")) {
        SkipRestOfBlock();
        return;
      } else if (LookingAt("}")) {
        return;
      }
    }
    input_->Next();
  }
}

void Parser::SkipRestOfBlock() {
  size_t block_count = 1;
  while (true) {
    if (AtEnd()) {
      return;
    } else if (LookingAtType(io::Tokenizer::TYPE_SYMBOL)) {
      if (TryConsumeEndOfDeclaration("}", nullptr)) {
        if (--block_count == 0) break;
      } else if (TryConsume("{")) {
        ++block_count;
      }
    }
    input_->Next();
  }
}

// ===================================================================

bool Parser::ValidateMessage(const DescriptorProto* proto) {
  for (int i = 0; i < proto->options().uninterpreted_option_size(); i++) {
    const UninterpretedOption& option =
        proto->options().uninterpreted_option(i);
    if (option.name_size() > 0 && !option.name(0).is_extension() &&
        option.name(0).name_part() == "map_entry") {
      int line = -1, col = 0;  // indicates line and column not known
      if (source_location_table_ != nullptr) {
        source_location_table_->Find(
            &option, DescriptorPool::ErrorCollector::OPTION_NAME, &line, &col);
      }
      RecordError(line, col,
                  "map_entry should not be set explicitly. "
                  "Use map<KeyType, ValueType> instead.");
      return false;
    }
  }
  return true;
}

bool Parser::ValidateEnum(const EnumDescriptorProto* proto) {
  bool has_allow_alias = false;
  bool allow_alias = false;

  for (int i = 0; i < proto->options().uninterpreted_option_size(); i++) {
    const UninterpretedOption option = proto->options().uninterpreted_option(i);
    if (option.name_size() > 1) {
      continue;
    }
    if (!option.name(0).is_extension() &&
        option.name(0).name_part() == "allow_alias") {
      has_allow_alias = true;
      if (option.identifier_value() == "true") {
        allow_alias = true;
      }
      break;
    }
  }

  if (has_allow_alias && !allow_alias) {
    // This needlessly clutters declarations with nops.
    RecordError([=] {
      return absl::StrCat(
          "\"", proto->name(),
          "\" declares 'option allow_alias = false;' which has no effect. "
          "Please remove the declaration.");
    });
    return false;
  }

  absl::flat_hash_set<int> used_values;
  bool has_duplicates = false;
  for (int i = 0; i < proto->value_size(); ++i) {
    const EnumValueDescriptorProto& enum_value = proto->value(i);
    if (used_values.find(enum_value.number()) != used_values.end()) {
      has_duplicates = true;
      break;
    } else {
      used_values.insert(enum_value.number());
    }
  }
  if (allow_alias && !has_duplicates) {
    // Generate an error if an enum declares support for duplicate enum values
    // and does not use it protect future authors.
    RecordError([=] {
      return absl::StrCat(
          "\"", proto->name(),
          "\" declares support for enum aliases but no enum values share field "
          "numbers. Please remove the unnecessary 'option allow_alias = true;' "
          "declaration.");
    });
    return false;
  }

  return true;
}

bool Parser::Parse(io::Tokenizer* input, FileDescriptorProto* file) {
  input_ = input;
  had_errors_ = false;
  syntax_identifier_.clear();

  // Note that |file| could be NULL at this point if
  // stop_after_syntax_identifier_ is true.  So, we conservatively allocate
  // SourceCodeInfo on the stack, then swap it into the FileDescriptorProto
  // later on.
  SourceCodeInfo source_code_info;
  source_code_info_ = &source_code_info;

  if (LookingAtType(io::Tokenizer::TYPE_START)) {
    // Advance to first token.
    input_->NextWithComments(nullptr, &upcoming_detached_comments_,
                             &upcoming_doc_comments_);
  }

  {
    LocationRecorder root_location(this);
    root_location.RecordLegacyLocation(file,
                                       DescriptorPool::ErrorCollector::OTHER);

    if (require_syntax_identifier_ || LookingAt("syntax") ||
        LookingAt("edition")) {
      if (!ParseSyntaxIdentifier(file, root_location)) {
        // Don't attempt to parse the file if we didn't recognize the syntax
        // identifier.
        return false;
      }
      // Store the syntax into the file.
      if (file != nullptr) {
        file->set_syntax(syntax_identifier_);
        if (syntax_identifier_ == "editions") {
          file->set_edition(edition_);
        }
      }
    } else if (!stop_after_syntax_identifier_) {
      ABSL_LOG(WARNING) << "No syntax specified for the proto file: "
                        << file->name()
                        << ". Please use 'syntax = \"proto2\";' "
                        << "or 'syntax = \"proto3\";' to specify a syntax "
                        << "version. (Defaulted to proto2 syntax.)";
      syntax_identifier_ = "proto2";
    }

    if (stop_after_syntax_identifier_) return !had_errors_;

    // Repeatedly parse statements until we reach the end of the file.
    while (!AtEnd()) {
      if (!ParseTopLevelStatement(file, root_location)) {
        // This statement failed to parse.  Skip it, but keep looping to parse
        // other statements.
        SkipStatement();

        if (LookingAt("}")) {
          RecordError("Unmatched \"}\".");
          input_->NextWithComments(nullptr, &upcoming_detached_comments_,
                                   &upcoming_doc_comments_);
        }
      }
    }
  }

  input_ = nullptr;
  source_code_info_ = nullptr;
  assert(file != nullptr);
  source_code_info.Swap(file->mutable_source_code_info());
  return !had_errors_;
}

bool Parser::ParseSyntaxIdentifier(const FileDescriptorProto* file,
                                   const LocationRecorder& parent) {
  LocationRecorder syntax_location(parent,
                                   FileDescriptorProto::kSyntaxFieldNumber);
  syntax_location.RecordLegacyLocation(
      file, DescriptorPool::ErrorCollector::EDITIONS);
  bool has_edition = false;
  if (TryConsume("edition")) {
    has_edition = true;
  } else {
    DO(Consume("syntax",
               "File must begin with a syntax statement, e.g. 'syntax = "
               "\"proto2\";'."));
  }

  DO(Consume("="));
  io::Tokenizer::Token syntax_token = input_->current();
  std::string syntax;
  DO(ConsumeString(&syntax, "Expected syntax identifier."));
  DO(ConsumeEndOfDeclaration(";", &syntax_location));

  if (has_edition) {
    if (!Edition_Parse(absl::StrCat("EDITION_", syntax), &edition_) ||
        edition_ == Edition::EDITION_PROTO2 ||
        edition_ == Edition::EDITION_PROTO3 ||
        edition_ == Edition::EDITION_UNKNOWN) {
      RecordError(syntax_token.line, syntax_token.column, [&] {
        return absl::StrCat("Unknown edition \"", syntax, "\".");
      });
      return false;
    }
    syntax_identifier_ = "editions";
    return true;
  }

  syntax_identifier_ = syntax;
  if (syntax != "proto2" && syntax != "proto3" &&
      !stop_after_syntax_identifier_) {
    RecordError(syntax_token.line, syntax_token.column, [&] {
      return absl::StrCat("Unrecognized syntax identifier \"", syntax,
                          "\".  This parser "
                          "only recognizes \"proto2\" and \"proto3\".");
    });
    return false;
  }

  return true;
}

bool Parser::ParseTopLevelStatement(FileDescriptorProto* file,
                                    const LocationRecorder& root_location) {
  if (TryConsumeEndOfDeclaration(";", nullptr)) {
    // empty statement; ignore
    return true;
  } else if (LookingAt("message")) {
    LocationRecorder location(root_location,
                              FileDescriptorProto::kMessageTypeFieldNumber,
                              file->message_type_size());
    // Maximum depth allowed by the DescriptorPool.
    recursion_depth_ = internal::cpp::MaxMessageDeclarationNestingDepth();
    return ParseMessageDefinition(file->add_message_type(), location, file);
  } else if (LookingAt("enum")) {
    LocationRecorder location(root_location,
                              FileDescriptorProto::kEnumTypeFieldNumber,
                              file->enum_type_size());
    return ParseEnumDefinition(file->add_enum_type(), location, file);
  } else if (LookingAt("service")) {
    LocationRecorder location(root_location,
                              FileDescriptorProto::kServiceFieldNumber,
                              file->service_size());
    return ParseServiceDefinition(file->add_service(), location, file);
  } else if (LookingAt("extend")) {
    LocationRecorder location(root_location,
                              FileDescriptorProto::kExtensionFieldNumber);
    return ParseExtend(
        file->mutable_extension(), file->mutable_message_type(), root_location,
        FileDescriptorProto::kMessageTypeFieldNumber, location, file);
  } else if (LookingAt("import")) {
    return ParseImport(file->mutable_dependency(),
                       file->mutable_public_dependency(),
                       file->mutable_weak_dependency(), root_location, file);
  } else if (LookingAt("package")) {
    return ParsePackage(file, root_location, file);
  } else if (LookingAt("option")) {
    LocationRecorder location(root_location,
                              FileDescriptorProto::kOptionsFieldNumber);
    return ParseOption(file->mutable_options(), location, file,
                       OPTION_STATEMENT);
  } else {
    RecordError("Expected top-level statement (e.g. \"message\").");
    return false;
  }
}

// -------------------------------------------------------------------
// Messages

PROTOBUF_NOINLINE static void GenerateSyntheticOneofs(
    DescriptorProto* message) {
  // Add synthetic one-field oneofs for optional fields, except messages which
  // already have presence in proto3.
  //
  // We have to make sure the oneof names don't conflict with any other
  // field or oneof.
  absl::flat_hash_set<std::string> names;
  for (const auto& field : message->field()) {
    names.insert(field.name());
  }
  for (const auto& oneof : message->oneof_decl()) {
    names.insert(oneof.name());
  }

  for (auto& field : *message->mutable_field()) {
    if (field.proto3_optional()) {
      std::string oneof_name = field.name();

      // Prepend 'XXXXX_' until we are no longer conflicting.
      // Avoid prepending a double-underscore because such names are
      // reserved in C++.
      if (oneof_name.empty() || oneof_name[0] != '_') {
        oneof_name.insert(0, "_");
      }
      while (names.count(oneof_name) > 0) {
        oneof_name.insert(0, "X");
      }

      names.insert(oneof_name);
      field.set_oneof_index(message->oneof_decl_size());
      OneofDescriptorProto* oneof = message->add_oneof_decl();
      oneof->set_name(std::move(oneof_name));
    }
  }
}

bool Parser::ParseMessageDefinition(
    DescriptorProto* message, const LocationRecorder& message_location,
    const FileDescriptorProto* containing_file) {
  const auto undo_depth = absl::MakeCleanup([&] { ++recursion_depth_; });
  if (--recursion_depth_ <= 0) {
    RecordError("Reached maximum recursion limit for nested messages.");
    return false;
  }

  DO(Consume("message"));
  {
    LocationRecorder location(message_location,
                              DescriptorProto::kNameFieldNumber);
    location.RecordLegacyLocation(message,
                                  DescriptorPool::ErrorCollector::NAME);
    DO(ConsumeIdentifier(message->mutable_name(), "Expected message name."));
  }
  DO(ParseMessageBlock(message, message_location, containing_file));

  if (syntax_identifier_ == "proto3") {
    GenerateSyntheticOneofs(message);
  }

  return true;
}

namespace {

const int kMaxRangeSentinel = -1;

bool IsMessageSetWireFormatMessage(const DescriptorProto& message) {
  const MessageOptions& options = message.options();
  for (int i = 0; i < options.uninterpreted_option_size(); ++i) {
    const UninterpretedOption& uninterpreted = options.uninterpreted_option(i);
    if (uninterpreted.name_size() == 1 &&
        !uninterpreted.name(0).is_extension() &&
        uninterpreted.name(0).name_part() == "message_set_wire_format" &&
        uninterpreted.identifier_value() == "true") {
      return true;
    }
  }
  return false;
}

// Modifies any extension ranges that specified 'max' as the end of the
// extension range, and sets them to the type-specific maximum. The actual max
// tag number can only be determined after all options have been parsed.
void AdjustExtensionRangesWithMaxEndNumber(DescriptorProto* message) {
  const bool is_message_set = IsMessageSetWireFormatMessage(*message);
  const int max_extension_number = is_message_set
                                       ? std::numeric_limits<int32_t>::max()
                                       : FieldDescriptor::kMaxNumber + 1;
  for (int i = 0; i < message->extension_range_size(); ++i) {
    if (message->extension_range(i).end() == kMaxRangeSentinel) {
      message->mutable_extension_range(i)->set_end(max_extension_number);
    }
  }
}

// Modifies any reserved ranges that specified 'max' as the end of the
// reserved range, and sets them to the type-specific maximum. The actual max
// tag number can only be determined after all options have been parsed.
void AdjustReservedRangesWithMaxEndNumber(DescriptorProto* message) {
  const bool is_message_set = IsMessageSetWireFormatMessage(*message);
  const int max_field_number = is_message_set
                                   ? std::numeric_limits<int32_t>::max()
                                   : FieldDescriptor::kMaxNumber + 1;
  for (int i = 0; i < message->reserved_range_size(); ++i) {
    if (message->reserved_range(i).end() == kMaxRangeSentinel) {
      message->mutable_reserved_range(i)->set_end(max_field_number);
    }
  }
}

}  // namespace

bool Parser::ParseMessageBlock(DescriptorProto* message,
                               const LocationRecorder& message_location,
                               const FileDescriptorProto* containing_file) {
  DO(ConsumeEndOfDeclaration("{", &message_location));

  while (!TryConsumeEndOfDeclaration("}", nullptr)) {
    if (AtEnd()) {
      RecordError("Reached end of input in message definition (missing '}').");
      return false;
    }

    if (!ParseMessageStatement(message, message_location, containing_file)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  }

  if (message->extension_range_size() > 0) {
    AdjustExtensionRangesWithMaxEndNumber(message);
  }
  if (message->reserved_range_size() > 0) {
    AdjustReservedRangesWithMaxEndNumber(message);
  }

  DO(ValidateMessage(message));

  return true;
}

bool Parser::ParseMessageStatement(DescriptorProto* message,
                                   const LocationRecorder& message_location,
                                   const FileDescriptorProto* containing_file) {
  if (TryConsumeEndOfDeclaration(";", nullptr)) {
    // empty statement; ignore
    return true;
  } else if (LookingAt("message")) {
    LocationRecorder location(message_location,
                              DescriptorProto::kNestedTypeFieldNumber,
                              message->nested_type_size());
    return ParseMessageDefinition(message->add_nested_type(), location,
                                  containing_file);
  } else if (LookingAt("enum")) {
    LocationRecorder location(message_location,
                              DescriptorProto::kEnumTypeFieldNumber,
                              message->enum_type_size());
    return ParseEnumDefinition(message->add_enum_type(), location,
                               containing_file);
  } else if (LookingAt("extensions")) {
    LocationRecorder location(message_location,
                              DescriptorProto::kExtensionRangeFieldNumber);
    return ParseExtensions(message, location, containing_file);
  } else if (LookingAt("reserved")) {
    return ParseReserved(message, message_location);
  } else if (LookingAt("extend")) {
    LocationRecorder location(message_location,
                              DescriptorProto::kExtensionFieldNumber);
    return ParseExtend(message->mutable_extension(),
                       message->mutable_nested_type(), message_location,
                       DescriptorProto::kNestedTypeFieldNumber, location,
                       containing_file);
  } else if (LookingAt("option")) {
    LocationRecorder location(message_location,
                              DescriptorProto::kOptionsFieldNumber);
    return ParseOption(message->mutable_options(), location, containing_file,
                       OPTION_STATEMENT);
  } else if (LookingAt("oneof")) {
    int oneof_index = message->oneof_decl_size();
    LocationRecorder oneof_location(
        message_location, DescriptorProto::kOneofDeclFieldNumber, oneof_index);

    return ParseOneof(message->add_oneof_decl(), message, oneof_index,
                      oneof_location, message_location, containing_file);
  } else {
    LocationRecorder location(message_location,
                              DescriptorProto::kFieldFieldNumber,
                              message->field_size());
    return ParseMessageField(
        message->add_field(), message->mutable_nested_type(), message_location,
        DescriptorProto::kNestedTypeFieldNumber, location, containing_file);
  }
}

bool Parser::ParseMessageField(FieldDescriptorProto* field,
                               RepeatedPtrField<DescriptorProto>* messages,
                               const LocationRecorder& parent_location,
                               int location_field_number_for_nested_type,
                               const LocationRecorder& field_location,
                               const FileDescriptorProto* containing_file) {
  {
    FieldDescriptorProto::Label label;
    if (ParseLabel(&label, field_location)) {
      field->set_label(label);
      if (label == FieldDescriptorProto::LABEL_OPTIONAL &&
          syntax_identifier_ == "proto3") {
        field->set_proto3_optional(true);
      }
    }
  }

  return ParseMessageFieldNoLabel(field, messages, parent_location,
                                  location_field_number_for_nested_type,
                                  field_location, containing_file);
}

bool Parser::ParseMessageFieldNoLabel(
    FieldDescriptorProto* field, RepeatedPtrField<DescriptorProto>* messages,
    const LocationRecorder& parent_location,
    int location_field_number_for_nested_type,
    const LocationRecorder& field_location,
    const FileDescriptorProto* containing_file) {
  MapField map_field;
  // Parse type.
  {
    LocationRecorder location(field_location);  // add path later
    location.RecordLegacyLocation(field, DescriptorPool::ErrorCollector::TYPE);

    bool type_parsed = false;
    FieldDescriptorProto::Type type = FieldDescriptorProto::TYPE_INT32;
    std::string type_name;

    // Special case map field. We only treat the field as a map field if the
    // field type name starts with the word "map" with a following "<".
    if (TryConsume("map")) {
      if (LookingAt("<")) {
        map_field.is_map_field = true;
        DO(ParseMapType(&map_field, field, location));
      } else {
        // False positive
        type_parsed = true;
        type_name = "map";
      }
    }
    if (!map_field.is_map_field) {
      // Handle the case where no explicit label is given for a non-map field.
      if (!field->has_label() && DefaultToOptionalFields()) {
        field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
      }
      if (!field->has_label()) {
        RecordError("Expected \"required\", \"optional\", or \"repeated\".");
        // We can actually reasonably recover here by just assuming the user
        // forgot the label altogether.
        field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
      }

      // Handle the case where the actual type is a message or enum named
      // "map", which we already consumed in the code above.
      if (!type_parsed) {
        DO(ParseType(&type, &type_name));
      }
      if (type_name.empty()) {
        location.AddPath(FieldDescriptorProto::kTypeFieldNumber);
        field->set_type(type);
      } else {
        location.AddPath(FieldDescriptorProto::kTypeNameFieldNumber);
        field->set_type_name(type_name);
      }
    }
  }

  // Parse name and '='.
  io::Tokenizer::Token name_token = input_->current();
  {
    LocationRecorder location(field_location,
                              FieldDescriptorProto::kNameFieldNumber);
    location.RecordLegacyLocation(field, DescriptorPool::ErrorCollector::NAME);
    DO(ConsumeIdentifier(field->mutable_name(), "Expected field name."));
  }
  DO(Consume("=", "Missing field number."));

  // Parse field number.
  {
    LocationRecorder location(field_location,
                              FieldDescriptorProto::kNumberFieldNumber);
    location.RecordLegacyLocation(field,
                                  DescriptorPool::ErrorCollector::NUMBER);
    int number;
    DO(ConsumeInteger(&number, "Expected field number."));
    field->set_number(number);
  }

  // Parse options.
  DO(ParseFieldOptions(field, field_location, containing_file));

  // Deal with groups.
  if (field->has_type() && field->type() == FieldDescriptorProto::TYPE_GROUP) {
    // Awkward:  Since a group declares both a message type and a field, we
    //   have to create overlapping locations.
    LocationRecorder group_location(parent_location);
    group_location.StartAt(field_location);
    group_location.AddPath(location_field_number_for_nested_type);
    group_location.AddPath(messages->size());

    DescriptorProto* group = messages->Add();
    group->set_name(field->name());

    // Record name location to match the field name's location.
    {
      LocationRecorder location(group_location,
                                DescriptorProto::kNameFieldNumber);
      location.StartAt(name_token);
      location.EndAt(name_token);
      location.RecordLegacyLocation(group,
                                    DescriptorPool::ErrorCollector::NAME);
    }

    // The field's type_name also comes from the name.  Confusing!
    {
      LocationRecorder location(field_location,
                                FieldDescriptorProto::kTypeNameFieldNumber);
      location.StartAt(name_token);
      location.EndAt(name_token);
    }

    // As a hack for backwards-compatibility, we force the group name to start
    // with a capital letter and lower-case the field name.  New code should
    // not use groups; it should use nested messages.
    if (group->name()[0] < 'A' || 'Z' < group->name()[0]) {
      RecordError(name_token.line, name_token.column,
                  "Group names must start with a capital letter.");
    }
    absl::AsciiStrToLower(field->mutable_name());

    field->set_type_name(group->name());
    if (LookingAt("{")) {
      DO(ParseMessageBlock(group, group_location, containing_file));
    } else {
      RecordError("Missing group body.");
      return false;
    }
  } else {
    DO(ConsumeEndOfDeclaration(";", &field_location));
  }

  // Create a map entry type if this is a map field.
  if (map_field.is_map_field) {
    GenerateMapEntry(map_field, field, messages);
  }

  return true;
}

bool Parser::ParseMapType(MapField* map_field, FieldDescriptorProto* field,
                          LocationRecorder& type_name_location) {
  if (field->has_oneof_index()) {
    RecordError("Map fields are not allowed in oneofs.");
    return false;
  }
  if (field->has_label()) {
    RecordError(
        "Field labels (required/optional/repeated) are not allowed on "
        "map fields.");
    return false;
  }
  if (field->has_extendee()) {
    RecordError("Map fields are not allowed to be extensions.");
    return false;
  }
  field->set_label(FieldDescriptorProto::LABEL_REPEATED);
  DO(Consume("<"));
  DO(ParseType(&map_field->key_type, &map_field->key_type_name));
  DO(Consume(","));
  DO(ParseType(&map_field->value_type, &map_field->value_type_name));
  DO(Consume(">"));
  // Defer setting of the type name of the map field until the
  // field name is parsed. Add the source location though.
  type_name_location.AddPath(FieldDescriptorProto::kTypeNameFieldNumber);
  return true;
}

void Parser::GenerateMapEntry(const MapField& map_field,
                              FieldDescriptorProto* field,
                              RepeatedPtrField<DescriptorProto>* messages) {
  DescriptorProto* entry = messages->Add();
  std::string entry_name = MapEntryName(field->name());
  field->set_type_name(entry_name);
  entry->set_name(entry_name);
  entry->mutable_options()->set_map_entry(true);
  FieldDescriptorProto* key_field = entry->add_field();
  key_field->set_name("key");
  key_field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  key_field->set_number(1);
  if (map_field.key_type_name.empty()) {
    key_field->set_type(map_field.key_type);
  } else {
    key_field->set_type_name(map_field.key_type_name);
  }
  FieldDescriptorProto* value_field = entry->add_field();
  value_field->set_name("value");
  value_field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
  value_field->set_number(2);
  if (map_field.value_type_name.empty()) {
    value_field->set_type(map_field.value_type);
  } else {
    value_field->set_type_name(map_field.value_type_name);
  }
  // Propagate all features to the generated key and value fields. This helps
  // simplify the implementation of code generators and also reflection-based
  // parsing code. Instead of having to implement complex inheritance rules
  // special-casing maps, we can just copy them at generation time.
  //
  // The following definition:
  //   message Foo {
  //     map<string, string> value = 1 [features.some_feature = VALUE];
  //   }
  // will be interpreted as:
  //   message Foo {
  //     message ValueEntry {
  //       option map_entry = true;
  //       string key = 1 [features.some_feature = VALUE];
  //       string value = 2 [features.some_feature = VALUE];
  //     }
  //     repeated ValueEntry value = 1 [features.some_feature = VALUE];
  //  }
  for (int i = 0; i < field->options().uninterpreted_option_size(); ++i) {
    const UninterpretedOption& option =
        field->options().uninterpreted_option(i);
    // Legacy handling for the `enforce_utf8` option, which bears a striking
    // similarity to features in many respects.
    // TODO Delete this once proto2/proto3 have been turned down.
    if (option.name_size() == 1 &&
        option.name(0).name_part() == "enforce_utf8" &&
        !option.name(0).is_extension()) {
      if (key_field->type() == FieldDescriptorProto::TYPE_STRING) {
        *key_field->mutable_options()->add_uninterpreted_option() = option;
      }
      if (value_field->type() == FieldDescriptorProto::TYPE_STRING) {
        *value_field->mutable_options()->add_uninterpreted_option() = option;
      }
    }
    if (option.name(0).name_part() == "features" &&
        !option.name(0).is_extension()) {
      *key_field->mutable_options()->add_uninterpreted_option() = option;
      *value_field->mutable_options()->add_uninterpreted_option() = option;
    }
  }
}

bool Parser::ParseFieldOptions(FieldDescriptorProto* field,
                               const LocationRecorder& field_location,
                               const FileDescriptorProto* containing_file) {
  if (!LookingAt("[")) return true;

  LocationRecorder location(field_location,
                            FieldDescriptorProto::kOptionsFieldNumber);

  DO(Consume("["));

  // Parse field options.
  do {
    if (LookingAt("default")) {
      // We intentionally pass field_location rather than location here, since
      // the default value is not actually an option.
      DO(ParseDefaultAssignment(field, field_location, containing_file));
    } else if (LookingAt("json_name")) {
      // Like default value, this "json_name" is not an actual option.
      DO(ParseJsonName(field, field_location, containing_file));
    } else {
      DO(ParseOption(field->mutable_options(), location, containing_file,
                     OPTION_ASSIGNMENT));
    }
  } while (TryConsume(","));

  DO(Consume("]"));
  return true;
}

bool Parser::ParseDefaultAssignment(
    FieldDescriptorProto* field, const LocationRecorder& field_location,
    const FileDescriptorProto* containing_file) {
  if (field->has_default_value()) {
    RecordError("Already set option \"default\".");
    field->clear_default_value();
  }

  DO(Consume("default"));
  DO(Consume("="));

  LocationRecorder location(field_location,
                            FieldDescriptorProto::kDefaultValueFieldNumber);
  location.RecordLegacyLocation(field,
                                DescriptorPool::ErrorCollector::DEFAULT_VALUE);
  std::string* default_value = field->mutable_default_value();

  if (!field->has_type()) {
    // The field has a type name, but we don't know if it is a message or an
    // enum yet. (If it were a primitive type, |field| would have a type set
    // already.) In this case, simply take the current string as the default
    // value; we will catch the error later if it is not a valid enum value.
    // (N.B. that we do not check whether the current token is an identifier:
    // doing so throws strange errors when the user mistypes a primitive
    // typename and we assume it's an enum. E.g.: "optional int foo = 1 [default
    // = 42]". In such a case the fundamental error is really that "int" is not
    // a type, not that "42" is not an identifier. See b/12533582.)
    *default_value = input_->current().text;
    input_->Next();
    return true;
  }

  switch (field->type()) {
    case FieldDescriptorProto::TYPE_INT32:
    case FieldDescriptorProto::TYPE_INT64:
    case FieldDescriptorProto::TYPE_SINT32:
    case FieldDescriptorProto::TYPE_SINT64:
    case FieldDescriptorProto::TYPE_SFIXED32:
    case FieldDescriptorProto::TYPE_SFIXED64: {
      uint64_t max_value = std::numeric_limits<int64_t>::max();
      if (field->type() == FieldDescriptorProto::TYPE_INT32 ||
          field->type() == FieldDescriptorProto::TYPE_SINT32 ||
          field->type() == FieldDescriptorProto::TYPE_SFIXED32) {
        max_value = std::numeric_limits<int32_t>::max();
      }

      // These types can be negative.
      if (TryConsume("-")) {
        default_value->append("-");
        // Two's complement always has one more negative value than positive.
        ++max_value;
      }
      // Parse the integer to verify that it is not out-of-range.
      uint64_t value;
      DO(ConsumeInteger64(max_value, &value,
                          "Expected integer for field default value."));
      // And stringify it again.
      default_value->append(absl::StrCat(value));
      break;
    }

    case FieldDescriptorProto::TYPE_UINT32:
    case FieldDescriptorProto::TYPE_UINT64:
    case FieldDescriptorProto::TYPE_FIXED32:
    case FieldDescriptorProto::TYPE_FIXED64: {
      uint64_t max_value = std::numeric_limits<uint64_t>::max();
      if (field->type() == FieldDescriptorProto::TYPE_UINT32 ||
          field->type() == FieldDescriptorProto::TYPE_FIXED32) {
        max_value = std::numeric_limits<uint32_t>::max();
      }

      // Numeric, not negative.
      if (TryConsume("-")) {
        RecordError("Unsigned field can't have negative default value.");
      }
      // Parse the integer to verify that it is not out-of-range.
      uint64_t value;
      DO(ConsumeInteger64(max_value, &value,
                          "Expected integer for field default value."));
      // And stringify it again.
      default_value->append(absl::StrCat(value));
      break;
    }

    case FieldDescriptorProto::TYPE_FLOAT:
    case FieldDescriptorProto::TYPE_DOUBLE: {
      // These types can be negative.
      if (TryConsume("-")) {
        default_value->append("-");
      }
      // Parse the integer because we have to convert hex integers to decimal
      // floats.
      double value = 0.0;
      DO(ConsumeNumber(&value, "Expected number."));
      // And stringify it again.
      default_value->append(io::SimpleDtoa(value));
      break;
    }
    case FieldDescriptorProto::TYPE_BOOL:
      if (TryConsume("true")) {
        default_value->assign("true");
      } else if (TryConsume("false")) {
        default_value->assign("false");
      } else {
        RecordError("Expected \"true\" or \"false\".");
        return false;
      }
      break;

    case FieldDescriptorProto::TYPE_STRING:
      // Note: When file option java_string_check_utf8 is true, if a
      // non-string representation (eg byte[]) is later supported, it must
      // be checked for UTF-8-ness.
      DO(ConsumeString(default_value,
                       "Expected string for field default "
                       "value."));
      break;

    case FieldDescriptorProto::TYPE_BYTES:
      DO(ConsumeString(default_value, "Expected string."));
      *default_value = absl::CEscape(*default_value);
      break;

    case FieldDescriptorProto::TYPE_ENUM:
      DO(ConsumeIdentifier(default_value,
                           "Expected enum identifier for field "
                           "default value."));
      break;

    case FieldDescriptorProto::TYPE_MESSAGE:
    case FieldDescriptorProto::TYPE_GROUP:
      RecordError("Messages can't have default values.");
      return false;
  }

  return true;
}

bool Parser::ParseJsonName(FieldDescriptorProto* field,
                           const LocationRecorder& field_location,
                           const FileDescriptorProto* containing_file) {
  if (field->has_json_name()) {
    RecordError("Already set option \"json_name\".");
    field->clear_json_name();
  }

  LocationRecorder location(field_location,
                            FieldDescriptorProto::kJsonNameFieldNumber);
  location.RecordLegacyLocation(field,
                                DescriptorPool::ErrorCollector::OPTION_NAME);

  DO(Consume("json_name"));
  DO(Consume("="));

  LocationRecorder value_location(location);
  value_location.RecordLegacyLocation(
      field, DescriptorPool::ErrorCollector::OPTION_VALUE);

  DO(ConsumeString(field->mutable_json_name(),
                   "Expected string for JSON name."));
  return true;
}

bool Parser::ParseOptionNamePart(UninterpretedOption* uninterpreted_option,
                                 const LocationRecorder& part_location,
                                 const FileDescriptorProto* containing_file) {
  UninterpretedOption::NamePart* name = uninterpreted_option->add_name();
  std::string identifier;  // We parse identifiers into this string.
  if (LookingAt("(")) {    // This is an extension.
    DO(Consume("("));

    {
      LocationRecorder location(
          part_location, UninterpretedOption::NamePart::kNamePartFieldNumber);
      // An extension name consists of dot-separated identifiers, and may begin
      // with a dot.
      if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
        DO(ConsumeIdentifier(&identifier, "Expected identifier."));
        name->mutable_name_part()->append(identifier);
      }
      while (LookingAt(".")) {
        DO(Consume("."));
        name->mutable_name_part()->append(".");
        DO(ConsumeIdentifier(&identifier, "Expected identifier."));
        name->mutable_name_part()->append(identifier);
      }
    }

    DO(Consume(")"));
    name->set_is_extension(true);
  } else {  // This is a regular field.
    LocationRecorder location(
        part_location, UninterpretedOption::NamePart::kNamePartFieldNumber);
    DO(ConsumeIdentifier(&identifier, "Expected identifier."));
    name->mutable_name_part()->append(identifier);
    name->set_is_extension(false);
  }
  return true;
}

bool Parser::ParseUninterpretedBlock(std::string* value) {
  // Note that enclosing braces are not added to *value.
  // We do NOT use ConsumeEndOfStatement for this brace because it's delimiting
  // an expression, not a block of statements.
  DO(Consume("{"));
  int brace_depth = 1;
  while (!AtEnd()) {
    if (LookingAt("{")) {
      brace_depth++;
    } else if (LookingAt("}")) {
      brace_depth--;
      if (brace_depth == 0) {
        input_->Next();
        return true;
      }
    }
    // TODO: Interpret line/column numbers to preserve formatting
    if (!value->empty()) value->push_back(' ');
    value->append(input_->current().text);
    input_->Next();
  }
  RecordError("Unexpected end of stream while parsing aggregate value.");
  return false;
}

// We don't interpret the option here. Instead we store it in an
// UninterpretedOption, to be interpreted later.
bool Parser::ParseOption(Message* options,
                         const LocationRecorder& options_location,
                         const FileDescriptorProto* containing_file,
                         OptionStyle style) {
  // Create an entry in the uninterpreted_option field.
  const FieldDescriptor* uninterpreted_option_field =
      options->GetDescriptor()->FindFieldByName("uninterpreted_option");
  ABSL_CHECK(uninterpreted_option_field != nullptr)
      << "No field named \"uninterpreted_option\" in the Options proto.";

  const Reflection* reflection = options->GetReflection();

  LocationRecorder location(
      options_location, uninterpreted_option_field->number(),
      reflection->FieldSize(*options, uninterpreted_option_field));

  if (style == OPTION_STATEMENT) {
    DO(Consume("option"));
  }

  UninterpretedOption* uninterpreted_option =
      DownCastMessage<UninterpretedOption>(options->GetReflection()->AddMessage(
          options, uninterpreted_option_field));

  // Parse dot-separated name.
  {
    LocationRecorder name_location(location,
                                   UninterpretedOption::kNameFieldNumber);
    name_location.RecordLegacyLocation(
        uninterpreted_option, DescriptorPool::ErrorCollector::OPTION_NAME);

    {
      LocationRecorder part_location(name_location,
                                     uninterpreted_option->name_size());
      DO(ParseOptionNamePart(uninterpreted_option, part_location,
                             containing_file));
    }

    while (LookingAt(".")) {
      DO(Consume("."));
      LocationRecorder part_location(name_location,
                                     uninterpreted_option->name_size());
      DO(ParseOptionNamePart(uninterpreted_option, part_location,
                             containing_file));
    }
  }

  DO(Consume("="));

  {
    LocationRecorder value_location(location);
    value_location.RecordLegacyLocation(
        uninterpreted_option, DescriptorPool::ErrorCollector::OPTION_VALUE);

    // All values are a single token, except for negative numbers, which consist
    // of a single '-' symbol, followed by a positive number.
    bool is_negative = TryConsume("-");

    switch (input_->current().type) {
      case io::Tokenizer::TYPE_START:
        ABSL_LOG(FATAL)
            << "Trying to read value before any tokens have been read.";
        return false;

      case io::Tokenizer::TYPE_END:
        RecordError("Unexpected end of stream while parsing option value.");
        return false;

      case io::Tokenizer::TYPE_WHITESPACE:
      case io::Tokenizer::TYPE_NEWLINE:
        ABSL_CHECK(!input_->report_whitespace() && !input_->report_newlines())
            << "Whitespace tokens were not requested.";
        ABSL_LOG(FATAL) << "Tokenizer reported whitespace.";
        return false;

      case io::Tokenizer::TYPE_IDENTIFIER: {
        value_location.AddPath(
            UninterpretedOption::kIdentifierValueFieldNumber);
        std::string value;
        DO(ConsumeIdentifier(&value, "Expected identifier."));
        if (is_negative) {
          if (value == "inf") {
            uninterpreted_option->set_double_value(
                -std::numeric_limits<double>::infinity());
          } else if (value == "nan") {
            uninterpreted_option->set_double_value(
                std::numeric_limits<double>::quiet_NaN());
          } else {
            RecordError("Identifier after '-' symbol must be inf or nan.");
            return false;
          }
          break;
        }
        uninterpreted_option->set_identifier_value(value);
        break;
      }

      case io::Tokenizer::TYPE_INTEGER: {
        uint64_t value;
        uint64_t max_value =
            is_negative
                ? static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1
                : std::numeric_limits<uint64_t>::max();
        if (TryConsumeInteger64(max_value, &value)) {
          if (is_negative) {
            value_location.AddPath(
                UninterpretedOption::kNegativeIntValueFieldNumber);
            uninterpreted_option->set_negative_int_value(
                static_cast<int64_t>(0 - value));
          } else {
            value_location.AddPath(
                UninterpretedOption::kPositiveIntValueFieldNumber);
            uninterpreted_option->set_positive_int_value(value);
          }
          break;
        }
        // value too large for an integer; fall through below to treat as
        // floating point
        ABSL_FALLTHROUGH_INTENDED;
      }

      case io::Tokenizer::TYPE_FLOAT: {
        value_location.AddPath(UninterpretedOption::kDoubleValueFieldNumber);
        double value = 0.0;
        DO(ConsumeNumber(&value, "Expected number."));
        uninterpreted_option->set_double_value(is_negative ? -value : value);
        break;
      }

      case io::Tokenizer::TYPE_STRING: {
        value_location.AddPath(UninterpretedOption::kStringValueFieldNumber);
        if (is_negative) {
          RecordError("Invalid '-' symbol before string.");
          return false;
        }
        std::string value;
        DO(ConsumeString(&value, "Expected string."));
        uninterpreted_option->set_string_value(value);
        break;
      }

      case io::Tokenizer::TYPE_SYMBOL:
        if (LookingAt("{")) {
          value_location.AddPath(
              UninterpretedOption::kAggregateValueFieldNumber);
          DO(ParseUninterpretedBlock(
              uninterpreted_option->mutable_aggregate_value()));
        } else {
          RecordError("Expected option value.");
          return false;
        }
        break;
    }
  }

  if (style == OPTION_STATEMENT) {
    DO(ConsumeEndOfDeclaration(";", &location));
  }

  return true;
}

bool Parser::ParseExtensions(DescriptorProto* message,
                             const LocationRecorder& extensions_location,
                             const FileDescriptorProto* containing_file) {
  // Parse the declaration.
  DO(Consume("extensions"));

  int old_range_size = message->extension_range_size();

  do {
    // Note that kExtensionRangeFieldNumber was already pushed by the parent.
    LocationRecorder location(extensions_location,
                              message->extension_range_size());

    DescriptorProto::ExtensionRange* range = message->add_extension_range();
    location.RecordLegacyLocation(range,
                                  DescriptorPool::ErrorCollector::NUMBER);

    int start, end;
    io::Tokenizer::Token start_token;

    {
      LocationRecorder start_location(
          location, DescriptorProto::ExtensionRange::kStartFieldNumber);
      start_token = input_->current();
      DO(ConsumeInteger(&start, "Expected field number range."));

      if (start == std::numeric_limits<int>::max()) {
        RecordError("Field number out of bounds.");
        return false;
      }
    }

    if (TryConsume("to")) {
      LocationRecorder end_location(
          location, DescriptorProto::ExtensionRange::kEndFieldNumber);
      if (TryConsume("max")) {
        // Set to the sentinel value - 1 since we increment the value below.
        // The actual value of the end of the range should be set with
        // AdjustExtensionRangesWithMaxEndNumber.
        end = kMaxRangeSentinel - 1;
      } else {
        DO(ConsumeInteger(&end, "Expected integer."));

        if (end == std::numeric_limits<int>::max()) {
          RecordError("Field number out of bounds.");
          return false;
        }
      }
    } else {
      LocationRecorder end_location(
          location, DescriptorProto::ExtensionRange::kEndFieldNumber);
      end_location.StartAt(start_token);
      end_location.EndAt(start_token);
      end = start;
    }

    // Users like to specify inclusive ranges, but in code we like the end
    // number to be exclusive.
    ++end;

    range->set_start(start);
    range->set_end(end);
  } while (TryConsume(","));

  if (LookingAt("[")) {
    int range_number_index = extensions_location.CurrentPathSize();
    SourceCodeInfo info;

    // Parse extension range options in the first range.
    ExtensionRangeOptions* options =
        message->mutable_extension_range(old_range_size)->mutable_options();

    {
      LocationRecorder index_location(
          extensions_location, 0 /* we fill this in w/ actual index below */,
          &info);
      LocationRecorder location(
          index_location, DescriptorProto::ExtensionRange::kOptionsFieldNumber);
      DO(Consume("["));

      do {
        DO(ParseOption(options, location, containing_file, OPTION_ASSIGNMENT));
      } while (TryConsume(","));

      DO(Consume("]"));
    }

    // Then copy the extension range options to all of the other ranges we've
    // parsed.
    for (int i = old_range_size + 1; i < message->extension_range_size(); i++) {
      *message->mutable_extension_range(i)->mutable_options() = *options;
    }
    // and copy source locations to the other ranges, too
    for (int i = old_range_size; i < message->extension_range_size(); i++) {
      for (int j = 0; j < info.location_size(); j++) {
        if (info.location(j).path_size() == range_number_index + 1) {
          // this location's path is up to the extension range index, but
          // doesn't include options; so it's redundant with location above
          continue;
        }
        SourceCodeInfo_Location* dest = source_code_info_->add_location();
        *dest = info.location(j);
        dest->set_path(range_number_index, i);
      }
    }
  }

  DO(ConsumeEndOfDeclaration(";", &extensions_location));
  return true;
}

// This is similar to extension range parsing, except that it accepts field
// name literals.
bool Parser::ParseReserved(DescriptorProto* message,
                           const LocationRecorder& message_location) {
  io::Tokenizer::Token start_token = input_->current();
  // Parse the declaration.
  DO(Consume("reserved"));
  if (LookingAtType(io::Tokenizer::TYPE_STRING)) {
    if (syntax_identifier_ == "editions") {
      RecordError(
          "Reserved names must be identifiers in editions, not string "
          "literals.");
      return false;
    }
    LocationRecorder location(message_location,
                              DescriptorProto::kReservedNameFieldNumber);
    location.StartAt(start_token);
    return ParseReservedNames(message, location);
  } else if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
    if (syntax_identifier_ != "editions") {
      RecordError(
          "Reserved names must be string literals. (Only editions supports "
          "identifiers.)");
      return false;
    }
    LocationRecorder location(message_location,
                              DescriptorProto::kReservedNameFieldNumber);
    location.StartAt(start_token);
    return ParseReservedIdentifiers(message, location);
  } else {
    LocationRecorder location(message_location,
                              DescriptorProto::kReservedRangeFieldNumber);
    location.StartAt(start_token);
    return ParseReservedNumbers(message, location);
  }
}

bool Parser::ParseReservedName(std::string* name, ErrorMaker error_message) {
  // Capture the position of the token, in case we have to report an
  // error after it is consumed.
  int line = input_->current().line;
  int col = input_->current().column;
  DO(ConsumeString(name, error_message));
  if (!io::Tokenizer::IsIdentifier(*name)) {
    // Before Edition 2023, it was possible to reserve any string literal. This
    // doesn't really make sense if the string literal wasn't a valid
    // identifier, so warn about it here.
    // Note that this warning is also load-bearing for tests that intend to
    // verify warnings work as expected today.
    RecordWarning(line, col, [=] {
      return absl::StrFormat("Reserved name \"%s\" is not a valid identifier.",
                             *name);
    });
  }
  return true;
}

bool Parser::ParseReservedNames(DescriptorProto* message,
                                const LocationRecorder& parent_location) {
  do {
    LocationRecorder location(parent_location, message->reserved_name_size());
    DO(ParseReservedName(message->add_reserved_name(),
                         "Expected field name string literal."));
  } while (TryConsume(","));
  DO(ConsumeEndOfDeclaration(";", &parent_location));
  return true;
}

bool Parser::ParseReservedIdentifier(std::string* name,
                                     ErrorMaker error_message) {
  DO(ConsumeIdentifier(name, error_message));
  return true;
}

bool Parser::ParseReservedIdentifiers(DescriptorProto* message,
                                      const LocationRecorder& parent_location) {
  do {
    LocationRecorder location(parent_location, message->reserved_name_size());
    DO(ParseReservedIdentifier(message->add_reserved_name(),
                               "Expected field name identifier."));
  } while (TryConsume(","));
  DO(ConsumeEndOfDeclaration(";", &parent_location));
  return true;
}

bool Parser::ParseReservedNumbers(DescriptorProto* message,
                                  const LocationRecorder& parent_location) {
  bool first = true;
  do {
    LocationRecorder location(parent_location, message->reserved_range_size());

    DescriptorProto::ReservedRange* range = message->add_reserved_range();
    location.RecordLegacyLocation(range,
                                  DescriptorPool::ErrorCollector::NUMBER);
    int start, end;
    io::Tokenizer::Token start_token;
    {
      LocationRecorder start_location(
          location, DescriptorProto::ReservedRange::kStartFieldNumber);
      start_token = input_->current();
      DO(ConsumeInteger(&start, (first ? "Expected field name or number range."
                                       : "Expected field number range.")));
    }

    if (TryConsume("to")) {
      LocationRecorder end_location(
          location, DescriptorProto::ReservedRange::kEndFieldNumber);
      if (TryConsume("max")) {
        // Set to the sentinel value - 1 since we increment the value below.
        // The actual value of the end of the range should be set with
        // AdjustExtensionRangesWithMaxEndNumber.
        end = kMaxRangeSentinel - 1;
      } else {
        DO(ConsumeInteger(&end, "Expected integer."));
      }
    } else {
      LocationRecorder end_location(
          location, DescriptorProto::ReservedRange::kEndFieldNumber);
      end_location.StartAt(start_token);
      end_location.EndAt(start_token);
      end = start;
    }

    // Users like to specify inclusive ranges, but in code we like the end
    // number to be exclusive.
    ++end;

    range->set_start(start);
    range->set_end(end);
    first = false;
  } while (TryConsume(","));

  DO(ConsumeEndOfDeclaration(";", &parent_location));
  return true;
}

bool Parser::ParseReserved(EnumDescriptorProto* proto,
                           const LocationRecorder& enum_location) {
  io::Tokenizer::Token start_token = input_->current();
  // Parse the declaration.
  DO(Consume("reserved"));
  if (LookingAtType(io::Tokenizer::TYPE_STRING)) {
    if (syntax_identifier_ == "editions") {
      RecordError(
          "Reserved names must be identifiers in editions, not string "
          "literals.");
      return false;
    }
    LocationRecorder location(enum_location,
                              EnumDescriptorProto::kReservedNameFieldNumber);
    location.StartAt(start_token);
    return ParseReservedNames(proto, location);
  } else if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
    if (syntax_identifier_ != "editions") {
      RecordError(
          "Reserved names must be string literals. (Only editions supports "
          "identifiers.)");
      return false;
    }
    LocationRecorder location(enum_location,
                              EnumDescriptorProto::kReservedNameFieldNumber);
    location.StartAt(start_token);
    return ParseReservedIdentifiers(proto, location);
  } else {
    LocationRecorder location(enum_location,
                              EnumDescriptorProto::kReservedRangeFieldNumber);
    location.StartAt(start_token);
    return ParseReservedNumbers(proto, location);
  }
}

bool Parser::ParseReservedNames(EnumDescriptorProto* proto,
                                const LocationRecorder& parent_location) {
  do {
    LocationRecorder location(parent_location, proto->reserved_name_size());
    DO(ParseReservedName(proto->add_reserved_name(),
                         "Expected enum value string literal."));
  } while (TryConsume(","));
  DO(ConsumeEndOfDeclaration(";", &parent_location));
  return true;
}

bool Parser::ParseReservedIdentifiers(EnumDescriptorProto* proto,
                                      const LocationRecorder& parent_location) {
  do {
    LocationRecorder location(parent_location, proto->reserved_name_size());
    DO(ParseReservedIdentifier(proto->add_reserved_name(),
                               "Expected enum value identifier."));
  } while (TryConsume(","));
  DO(ConsumeEndOfDeclaration(";", &parent_location));
  return true;
}

bool Parser::ParseReservedNumbers(EnumDescriptorProto* proto,
                                  const LocationRecorder& parent_location) {
  bool first = true;
  do {
    LocationRecorder location(parent_location, proto->reserved_range_size());

    EnumDescriptorProto::EnumReservedRange* range = proto->add_reserved_range();
    location.RecordLegacyLocation(range,
                                  DescriptorPool::ErrorCollector::NUMBER);
    int start, end;
    io::Tokenizer::Token start_token;
    {
      LocationRecorder start_location(
          location, EnumDescriptorProto::EnumReservedRange::kStartFieldNumber);
      start_token = input_->current();
      DO(ConsumeSignedInteger(&start,
                              (first ? "Expected enum value or number range."
                                     : "Expected enum number range.")));
    }

    if (TryConsume("to")) {
      LocationRecorder end_location(
          location, EnumDescriptorProto::EnumReservedRange::kEndFieldNumber);
      if (TryConsume("max")) {
        // This is in the enum descriptor path, which doesn't have the message
        // set duality to fix up, so it doesn't integrate with the sentinel.
        end = INT_MAX;
      } else {
        DO(ConsumeSignedInteger(&end, "Expected integer."));
      }
    } else {
      LocationRecorder end_location(
          location, EnumDescriptorProto::EnumReservedRange::kEndFieldNumber);
      end_location.StartAt(start_token);
      end_location.EndAt(start_token);
      end = start;
    }

    range->set_start(start);
    range->set_end(end);
    first = false;
  } while (TryConsume(","));

  DO(ConsumeEndOfDeclaration(";", &parent_location));
  return true;
}

bool Parser::ParseExtend(RepeatedPtrField<FieldDescriptorProto>* extensions,
                         RepeatedPtrField<DescriptorProto>* messages,
                         const LocationRecorder& parent_location,
                         int location_field_number_for_nested_type,
                         const LocationRecorder& extend_location,
                         const FileDescriptorProto* containing_file) {
  DO(Consume("extend"));

  // Parse the extendee type.
  io::Tokenizer::Token extendee_start = input_->current();
  std::string extendee;
  DO(ParseUserDefinedType(&extendee));
  io::Tokenizer::Token extendee_end = input_->previous();

  // Parse the block.
  DO(ConsumeEndOfDeclaration("{", &extend_location));

  bool is_first = true;

  do {
    if (AtEnd()) {
      RecordError("Reached end of input in extend definition (missing '}').");
      return false;
    }

    // Note that kExtensionFieldNumber was already pushed by the parent.
    LocationRecorder location(extend_location, extensions->size());

    FieldDescriptorProto* field = extensions->Add();

    {
      LocationRecorder extendee_location(
          location, FieldDescriptorProto::kExtendeeFieldNumber);
      extendee_location.StartAt(extendee_start);
      extendee_location.EndAt(extendee_end);

      if (is_first) {
        extendee_location.RecordLegacyLocation(
            field, DescriptorPool::ErrorCollector::EXTENDEE);
        is_first = false;
      }
    }

    field->set_extendee(extendee);

    if (!ParseMessageField(field, messages, parent_location,
                           location_field_number_for_nested_type, location,
                           containing_file)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  } while (!TryConsumeEndOfDeclaration("}", nullptr));

  return true;
}

bool Parser::ParseOneof(OneofDescriptorProto* oneof_decl,
                        DescriptorProto* containing_type, int oneof_index,
                        const LocationRecorder& oneof_location,
                        const LocationRecorder& containing_type_location,
                        const FileDescriptorProto* containing_file) {
  DO(Consume("oneof"));

  {
    LocationRecorder name_location(oneof_location,
                                   OneofDescriptorProto::kNameFieldNumber);
    DO(ConsumeIdentifier(oneof_decl->mutable_name(), "Expected oneof name."));
  }

  DO(ConsumeEndOfDeclaration("{", &oneof_location));

  do {
    if (AtEnd()) {
      RecordError("Reached end of input in oneof definition (missing '}').");
      return false;
    }

    if (LookingAt("option")) {
      LocationRecorder option_location(
          oneof_location, OneofDescriptorProto::kOptionsFieldNumber);
      if (!ParseOption(oneof_decl->mutable_options(), option_location,
                       containing_file, OPTION_STATEMENT)) {
        return false;
      }
      continue;
    }

    // Print a nice error if the user accidentally tries to place a label
    // on an individual member of a oneof.
    if (LookingAt("required") || LookingAt("optional") ||
        LookingAt("repeated")) {
      RecordError(
          "Fields in oneofs must not have labels (required / optional "
          "/ repeated).");
      // We can continue parsing here because we understand what the user
      // meant.  The error report will still make parsing fail overall.
      input_->Next();
    }

    LocationRecorder field_location(containing_type_location,
                                    DescriptorProto::kFieldFieldNumber,
                                    containing_type->field_size());

    FieldDescriptorProto* field = containing_type->add_field();
    field->set_label(FieldDescriptorProto::LABEL_OPTIONAL);
    field->set_oneof_index(oneof_index);

    if (!ParseMessageFieldNoLabel(field, containing_type->mutable_nested_type(),
                                  containing_type_location,
                                  DescriptorProto::kNestedTypeFieldNumber,
                                  field_location, containing_file)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  } while (!TryConsumeEndOfDeclaration("}", nullptr));

  return true;
}

// -------------------------------------------------------------------
// Enums

bool Parser::ParseEnumDefinition(EnumDescriptorProto* enum_type,
                                 const LocationRecorder& enum_location,
                                 const FileDescriptorProto* containing_file) {
  DO(Consume("enum"));

  {
    LocationRecorder location(enum_location,
                              EnumDescriptorProto::kNameFieldNumber);
    location.RecordLegacyLocation(enum_type,
                                  DescriptorPool::ErrorCollector::NAME);
    DO(ConsumeIdentifier(enum_type->mutable_name(), "Expected enum name."));
  }

  DO(ParseEnumBlock(enum_type, enum_location, containing_file));

  DO(ValidateEnum(enum_type));

  return true;
}

bool Parser::ParseEnumBlock(EnumDescriptorProto* enum_type,
                            const LocationRecorder& enum_location,
                            const FileDescriptorProto* containing_file) {
  DO(ConsumeEndOfDeclaration("{", &enum_location));

  while (!TryConsumeEndOfDeclaration("}", nullptr)) {
    if (AtEnd()) {
      RecordError("Reached end of input in enum definition (missing '}').");
      return false;
    }

    if (!ParseEnumStatement(enum_type, enum_location, containing_file)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  }

  return true;
}

bool Parser::ParseEnumStatement(EnumDescriptorProto* enum_type,
                                const LocationRecorder& enum_location,
                                const FileDescriptorProto* containing_file) {
  if (TryConsumeEndOfDeclaration(";", nullptr)) {
    // empty statement; ignore
    return true;
  } else if (LookingAt("option")) {
    LocationRecorder location(enum_location,
                              EnumDescriptorProto::kOptionsFieldNumber);
    return ParseOption(enum_type->mutable_options(), location, containing_file,
                       OPTION_STATEMENT);
  } else if (LookingAt("reserved")) {
    return ParseReserved(enum_type, enum_location);
  } else {
    LocationRecorder location(enum_location,
                              EnumDescriptorProto::kValueFieldNumber,
                              enum_type->value_size());
    return ParseEnumConstant(enum_type->add_value(), location, containing_file);
  }
}

bool Parser::ParseEnumConstant(EnumValueDescriptorProto* enum_value,
                               const LocationRecorder& enum_value_location,
                               const FileDescriptorProto* containing_file) {
  // Parse name.
  {
    LocationRecorder location(enum_value_location,
                              EnumValueDescriptorProto::kNameFieldNumber);
    location.RecordLegacyLocation(enum_value,
                                  DescriptorPool::ErrorCollector::NAME);
    DO(ConsumeIdentifier(enum_value->mutable_name(),
                         "Expected enum constant name."));
  }

  DO(Consume("=", "Missing numeric value for enum constant."));

  // Parse value.
  {
    LocationRecorder location(enum_value_location,
                              EnumValueDescriptorProto::kNumberFieldNumber);
    location.RecordLegacyLocation(enum_value,
                                  DescriptorPool::ErrorCollector::NUMBER);

    int number;
    DO(ConsumeSignedInteger(&number, "Expected integer."));
    enum_value->set_number(number);
  }

  DO(ParseEnumConstantOptions(enum_value, enum_value_location,
                              containing_file));

  DO(ConsumeEndOfDeclaration(";", &enum_value_location));

  return true;
}

bool Parser::ParseEnumConstantOptions(
    EnumValueDescriptorProto* value,
    const LocationRecorder& enum_value_location,
    const FileDescriptorProto* containing_file) {
  if (!LookingAt("[")) return true;

  LocationRecorder location(enum_value_location,
                            EnumValueDescriptorProto::kOptionsFieldNumber);

  DO(Consume("["));

  do {
    DO(ParseOption(value->mutable_options(), location, containing_file,
                   OPTION_ASSIGNMENT));
  } while (TryConsume(","));

  DO(Consume("]"));
  return true;
}

// -------------------------------------------------------------------
// Services

bool Parser::ParseServiceDefinition(
    ServiceDescriptorProto* service, const LocationRecorder& service_location,
    const FileDescriptorProto* containing_file) {
  DO(Consume("service"));

  {
    LocationRecorder location(service_location,
                              ServiceDescriptorProto::kNameFieldNumber);
    location.RecordLegacyLocation(service,
                                  DescriptorPool::ErrorCollector::NAME);
    DO(ConsumeIdentifier(service->mutable_name(), "Expected service name."));
  }

  DO(ParseServiceBlock(service, service_location, containing_file));
  return true;
}

bool Parser::ParseServiceBlock(ServiceDescriptorProto* service,
                               const LocationRecorder& service_location,
                               const FileDescriptorProto* containing_file) {
  DO(ConsumeEndOfDeclaration("{", &service_location));

  while (!TryConsumeEndOfDeclaration("}", nullptr)) {
    if (AtEnd()) {
      RecordError("Reached end of input in service definition (missing '}').");
      return false;
    }

    if (!ParseServiceStatement(service, service_location, containing_file)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  }

  return true;
}

bool Parser::ParseServiceStatement(ServiceDescriptorProto* service,
                                   const LocationRecorder& service_location,
                                   const FileDescriptorProto* containing_file) {
  if (TryConsumeEndOfDeclaration(";", nullptr)) {
    // empty statement; ignore
    return true;
  } else if (LookingAt("option")) {
    LocationRecorder location(service_location,
                              ServiceDescriptorProto::kOptionsFieldNumber);
    return ParseOption(service->mutable_options(), location, containing_file,
                       OPTION_STATEMENT);
  } else {
    LocationRecorder location(service_location,
                              ServiceDescriptorProto::kMethodFieldNumber,
                              service->method_size());
    return ParseServiceMethod(service->add_method(), location, containing_file);
  }
}

bool Parser::ParseServiceMethod(MethodDescriptorProto* method,
                                const LocationRecorder& method_location,
                                const FileDescriptorProto* containing_file) {
  DO(Consume("rpc"));

  {
    LocationRecorder location(method_location,
                              MethodDescriptorProto::kNameFieldNumber);
    location.RecordLegacyLocation(method, DescriptorPool::ErrorCollector::NAME);
    DO(ConsumeIdentifier(method->mutable_name(), "Expected method name."));
  }

  // Parse input type.
  DO(Consume("("));
  {
    if (LookingAt("stream")) {
      LocationRecorder location(
          method_location, MethodDescriptorProto::kClientStreamingFieldNumber);
      location.RecordLegacyLocation(method,
                                    DescriptorPool::ErrorCollector::OTHER);
      method->set_client_streaming(true);
      DO(Consume("stream"));
    }
    LocationRecorder location(method_location,
                              MethodDescriptorProto::kInputTypeFieldNumber);
    location.RecordLegacyLocation(method,
                                  DescriptorPool::ErrorCollector::INPUT_TYPE);
    DO(ParseUserDefinedType(method->mutable_input_type()));
  }
  DO(Consume(")"));

  // Parse output type.
  DO(Consume("returns"));
  DO(Consume("("));
  {
    if (LookingAt("stream")) {
      LocationRecorder location(
          method_location, MethodDescriptorProto::kServerStreamingFieldNumber);
      location.RecordLegacyLocation(method,
                                    DescriptorPool::ErrorCollector::OTHER);
      DO(Consume("stream"));
      method->set_server_streaming(true);
    }
    LocationRecorder location(method_location,
                              MethodDescriptorProto::kOutputTypeFieldNumber);
    location.RecordLegacyLocation(method,
                                  DescriptorPool::ErrorCollector::OUTPUT_TYPE);
    DO(ParseUserDefinedType(method->mutable_output_type()));
  }
  DO(Consume(")"));

  if (LookingAt("{")) {
    // Options!
    DO(ParseMethodOptions(method_location, containing_file,
                          MethodDescriptorProto::kOptionsFieldNumber,
                          method->mutable_options()));
  } else {
    DO(ConsumeEndOfDeclaration(";", &method_location));
  }

  return true;
}

bool Parser::ParseMethodOptions(const LocationRecorder& parent_location,
                                const FileDescriptorProto* containing_file,
                                const int optionsFieldNumber,
                                Message* mutable_options) {
  // Options!
  ConsumeEndOfDeclaration("{", &parent_location);
  while (!TryConsumeEndOfDeclaration("}", nullptr)) {
    if (AtEnd()) {
      RecordError("Reached end of input in method options (missing '}').");
      return false;
    }

    if (TryConsumeEndOfDeclaration(";", nullptr)) {
      // empty statement; ignore
    } else {
      LocationRecorder location(parent_location, optionsFieldNumber);
      if (!ParseOption(mutable_options, location, containing_file,
                       OPTION_STATEMENT)) {
        // This statement failed to parse.  Skip it, but keep looping to
        // parse other statements.
        SkipStatement();
      }
    }
  }

  return true;
}

// -------------------------------------------------------------------

bool Parser::ParseLabel(FieldDescriptorProto::Label* label,
                        const LocationRecorder& field_location) {
  if (!LookingAt("optional") && !LookingAt("repeated") &&
      !LookingAt("required")) {
    return false;
  }
  if (LookingAt("optional") && syntax_identifier_ == "editions") {
    RecordError(
        "Label \"optional\" is not supported in editions. By default, all "
        "singular fields have presence unless features.field_presence is set.");
  }
  if (LookingAt("required") && syntax_identifier_ == "editions") {
    RecordError(
        "Label \"required\" is not supported in editions, use "
        "features.field_presence = LEGACY_REQUIRED.");
  }

  LocationRecorder location(field_location,
                            FieldDescriptorProto::kLabelFieldNumber);
  if (TryConsume("optional")) {
    *label = FieldDescriptorProto::LABEL_OPTIONAL;
  } else if (TryConsume("repeated")) {
    *label = FieldDescriptorProto::LABEL_REPEATED;
  } else {
    Consume("required");
    *label = FieldDescriptorProto::LABEL_REQUIRED;
  }
  return true;
}

bool Parser::ParseType(FieldDescriptorProto::Type* type,
                       std::string* type_name) {
  const auto& type_names_table = GetTypeNameTable();
  auto iter = type_names_table.find(input_->current().text);
  if (iter != type_names_table.end()) {
    if (syntax_identifier_ == "editions" &&
        iter->second == FieldDescriptorProto::TYPE_GROUP) {
      RecordError(
          "Group syntax is no longer supported in editions. To get group "
          "behavior you can specify features.message_encoding = DELIMITED on a "
          "message field.");
    }
    *type = iter->second;
    input_->Next();
  } else {
    DO(ParseUserDefinedType(type_name));
  }
  return true;
}

bool Parser::ParseUserDefinedType(std::string* type_name) {
  type_name->clear();

  const auto& type_names_table = GetTypeNameTable();
  auto iter = type_names_table.find(input_->current().text);
  if (iter != type_names_table.end()) {
    // Note:  The only place enum types are allowed is for field types, but
    //   if we are parsing a field type then we would not get here because
    //   primitives are allowed there as well.  So this error message doesn't
    //   need to account for enums.
    RecordError("Expected message type.");

    // Pretend to accept this type so that we can go on parsing.
    *type_name = input_->current().text;
    input_->Next();
    return true;
  }

  // A leading "." means the name is fully-qualified.
  if (TryConsume(".")) type_name->append(".");

  // Consume the first part of the name.
  std::string identifier;
  DO(ConsumeIdentifier(&identifier, "Expected type name."));
  type_name->append(identifier);

  // Consume more parts.
  while (TryConsume(".")) {
    type_name->append(".");
    DO(ConsumeIdentifier(&identifier, "Expected identifier."));
    type_name->append(identifier);
  }

  return true;
}

// ===================================================================

bool Parser::ParsePackage(FileDescriptorProto* file,
                          const LocationRecorder& root_location,
                          const FileDescriptorProto* containing_file) {
  if (file->has_package()) {
    RecordError("Multiple package definitions.");
    // Don't append the new package to the old one.  Just replace it.  Not
    // that it really matters since this is an error anyway.
    file->clear_package();
  }

  LocationRecorder location(root_location,
                            FileDescriptorProto::kPackageFieldNumber);
  location.RecordLegacyLocation(file, DescriptorPool::ErrorCollector::NAME);

  DO(Consume("package"));

  while (true) {
    std::string identifier;
    DO(ConsumeIdentifier(&identifier, "Expected identifier."));
    file->mutable_package()->append(identifier);
    if (!TryConsume(".")) break;
    file->mutable_package()->append(".");
  }

  DO(ConsumeEndOfDeclaration(";", &location));

  return true;
}

bool Parser::ParseImport(RepeatedPtrField<std::string>* dependency,
                         RepeatedField<int32_t>* public_dependency,
                         RepeatedField<int32_t>* weak_dependency,
                         const LocationRecorder& root_location,
                         const FileDescriptorProto* containing_file) {
  LocationRecorder location(root_location,
                            FileDescriptorProto::kDependencyFieldNumber,
                            dependency->size());

  DO(Consume("import"));

  if (LookingAt("public")) {
    LocationRecorder public_location(
        root_location, FileDescriptorProto::kPublicDependencyFieldNumber,
        public_dependency->size());
    DO(Consume("public"));
    *public_dependency->Add() = dependency->size();
  } else if (LookingAt("weak")) {
    LocationRecorder weak_location(
        root_location, FileDescriptorProto::kWeakDependencyFieldNumber,
        weak_dependency->size());
    weak_location.RecordLegacyImportLocation(containing_file, "weak");
    DO(Consume("weak"));
    *weak_dependency->Add() = dependency->size();
  }

  std::string import_file;
  DO(ConsumeString(&import_file,
                   "Expected a string naming the file to import."));
  *dependency->Add() = import_file;
  location.RecordLegacyImportLocation(containing_file, import_file);

  DO(ConsumeEndOfDeclaration(";", &location));

  return true;
}

// ===================================================================

SourceLocationTable::SourceLocationTable() {}
SourceLocationTable::~SourceLocationTable() {}

bool SourceLocationTable::Find(
    const Message* descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location, int* line,
    int* column) const {
  auto it = location_map_.find({descriptor, location});
  if (it == location_map_.end()) {
    *line = -1;
    *column = 0;
    return false;
  }
  std::tie(*line, *column) = it->second;
  return true;
}

bool SourceLocationTable::FindImport(const Message* descriptor,
                                     absl::string_view name, int* line,
                                     int* column) const {
  auto it = import_location_map_.find({descriptor, std::string(name)});
  if (it == import_location_map_.end()) {
    *line = -1;
    *column = 0;
    return false;
  }
  std::tie(*line, *column) = it->second;
  return true;
}

void SourceLocationTable::Add(
    const Message* descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location, int line,
    int column) {
  location_map_[std::make_pair(descriptor, location)] =
      std::make_pair(line, column);
}

void SourceLocationTable::AddImport(const Message* descriptor,
                                    const std::string& name, int line,
                                    int column) {
  import_location_map_[std::make_pair(descriptor, name)] =
      std::make_pair(line, column);
}

void SourceLocationTable::Clear() { location_map_.clear(); }

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
