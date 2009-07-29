// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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
//
// Recursive descent FTW.

#include <google/protobuf/stubs/hash.h>
#include <float.h>


#include <google/protobuf/compiler/parser.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/io/tokenizer.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/stubs/map-util.h>

namespace google {
namespace protobuf {
namespace compiler {

using internal::WireFormat;

namespace {

typedef hash_map<string, FieldDescriptorProto::Type> TypeNameMap;

TypeNameMap MakeTypeNameTable() {
  TypeNameMap result;

  result["double"  ] = FieldDescriptorProto::TYPE_DOUBLE;
  result["float"   ] = FieldDescriptorProto::TYPE_FLOAT;
  result["uint64"  ] = FieldDescriptorProto::TYPE_UINT64;
  result["fixed64" ] = FieldDescriptorProto::TYPE_FIXED64;
  result["fixed32" ] = FieldDescriptorProto::TYPE_FIXED32;
  result["bool"    ] = FieldDescriptorProto::TYPE_BOOL;
  result["string"  ] = FieldDescriptorProto::TYPE_STRING;
  result["group"   ] = FieldDescriptorProto::TYPE_GROUP;

  result["bytes"   ] = FieldDescriptorProto::TYPE_BYTES;
  result["uint32"  ] = FieldDescriptorProto::TYPE_UINT32;
  result["sfixed32"] = FieldDescriptorProto::TYPE_SFIXED32;
  result["sfixed64"] = FieldDescriptorProto::TYPE_SFIXED64;
  result["int32"   ] = FieldDescriptorProto::TYPE_INT32;
  result["int64"   ] = FieldDescriptorProto::TYPE_INT64;
  result["sint32"  ] = FieldDescriptorProto::TYPE_SINT32;
  result["sint64"  ] = FieldDescriptorProto::TYPE_SINT64;

  return result;
}

const TypeNameMap kTypeNames = MakeTypeNameTable();

}  // anonymous namespace

// Makes code slightly more readable.  The meaning of "DO(foo)" is
// "Execute foo and fail if it fails.", where failure is indicated by
// returning false.
#define DO(STATEMENT) if (STATEMENT) {} else return false

// ===================================================================

Parser::Parser()
  : input_(NULL),
    error_collector_(NULL),
    source_location_table_(NULL),
    had_errors_(false),
    require_syntax_identifier_(false),
    stop_after_syntax_identifier_(false) {
}

Parser::~Parser() {
}

// ===================================================================

inline bool Parser::LookingAt(const char* text) {
  return input_->current().text == text;
}

inline bool Parser::LookingAtType(io::Tokenizer::TokenType token_type) {
  return input_->current().type == token_type;
}

inline bool Parser::AtEnd() {
  return LookingAtType(io::Tokenizer::TYPE_END);
}

bool Parser::TryConsume(const char* text) {
  if (LookingAt(text)) {
    input_->Next();
    return true;
  } else {
    return false;
  }
}

bool Parser::Consume(const char* text, const char* error) {
  if (TryConsume(text)) {
    return true;
  } else {
    AddError(error);
    return false;
  }
}

bool Parser::Consume(const char* text) {
  if (TryConsume(text)) {
    return true;
  } else {
    AddError("Expected \"" + string(text) + "\".");
    return false;
  }
}

bool Parser::ConsumeIdentifier(string* output, const char* error) {
  if (LookingAtType(io::Tokenizer::TYPE_IDENTIFIER)) {
    *output = input_->current().text;
    input_->Next();
    return true;
  } else {
    AddError(error);
    return false;
  }
}

bool Parser::ConsumeInteger(int* output, const char* error) {
  if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
    uint64 value = 0;
    if (!io::Tokenizer::ParseInteger(input_->current().text,
                                     kint32max, &value)) {
      AddError("Integer out of range.");
      // We still return true because we did, in fact, parse an integer.
    }
    *output = value;
    input_->Next();
    return true;
  } else {
    AddError(error);
    return false;
  }
}

bool Parser::ConsumeInteger64(uint64 max_value, uint64* output,
                              const char* error) {
  if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
    if (!io::Tokenizer::ParseInteger(input_->current().text, max_value,
                                     output)) {
      AddError("Integer out of range.");
      // We still return true because we did, in fact, parse an integer.
      *output = 0;
    }
    input_->Next();
    return true;
  } else {
    AddError(error);
    return false;
  }
}

bool Parser::ConsumeNumber(double* output, const char* error) {
  if (LookingAtType(io::Tokenizer::TYPE_FLOAT)) {
    *output = io::Tokenizer::ParseFloat(input_->current().text);
    input_->Next();
    return true;
  } else if (LookingAtType(io::Tokenizer::TYPE_INTEGER)) {
    // Also accept integers.
    uint64 value = 0;
    if (!io::Tokenizer::ParseInteger(input_->current().text,
                                     kuint64max, &value)) {
      AddError("Integer out of range.");
      // We still return true because we did, in fact, parse a number.
    }
    *output = value;
    input_->Next();
    return true;
  } else {
    AddError(error);
    return false;
  }
}

bool Parser::ConsumeString(string* output, const char* error) {
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
    AddError(error);
    return false;
  }
}

// -------------------------------------------------------------------

void Parser::AddError(int line, int column, const string& error) {
  if (error_collector_ != NULL) {
    error_collector_->AddError(line, column, error);
  }
  had_errors_ = true;
}

void Parser::AddError(const string& error) {
  AddError(input_->current().line, input_->current().column, error);
}

void Parser::RecordLocation(
    const Message* descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    int line, int column) {
  if (source_location_table_ != NULL) {
    source_location_table_->Add(descriptor, location, line, column);
  }
}

void Parser::RecordLocation(
    const Message* descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location) {
  RecordLocation(descriptor, location,
                 input_->current().line, input_->current().column);
}

// -------------------------------------------------------------------

void Parser::SkipStatement() {
  while (true) {
    if (AtEnd()) {
      return;
    } else if (LookingAtType(io::Tokenizer::TYPE_SYMBOL)) {
      if (TryConsume(";")) {
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
  while (true) {
    if (AtEnd()) {
      return;
    } else if (LookingAtType(io::Tokenizer::TYPE_SYMBOL)) {
      if (TryConsume("}")) {
        return;
      } else if (TryConsume("{")) {
        SkipRestOfBlock();
      }
    }
    input_->Next();
  }
}

// ===================================================================

bool Parser::Parse(io::Tokenizer* input, FileDescriptorProto* file) {
  input_ = input;
  had_errors_ = false;
  syntax_identifier_.clear();

  if (LookingAtType(io::Tokenizer::TYPE_START)) {
    // Advance to first token.
    input_->Next();
  }

  if (require_syntax_identifier_ || LookingAt("syntax")) {
    if (!ParseSyntaxIdentifier()) {
      // Don't attempt to parse the file if we didn't recognize the syntax
      // identifier.
      return false;
    }
  } else if (!stop_after_syntax_identifier_) {
    syntax_identifier_ = "proto2";
  }

  if (stop_after_syntax_identifier_) return !had_errors_;

  // Repeatedly parse statements until we reach the end of the file.
  while (!AtEnd()) {
    if (!ParseTopLevelStatement(file)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();

      if (LookingAt("}")) {
        AddError("Unmatched \"}\".");
        input_->Next();
      }
    }
  }

  input_ = NULL;
  return !had_errors_;
}

bool Parser::ParseSyntaxIdentifier() {
  DO(Consume("syntax", "File must begin with 'syntax = \"proto2\";'."));
  DO(Consume("="));
  io::Tokenizer::Token syntax_token = input_->current();
  string syntax;
  DO(ConsumeString(&syntax, "Expected syntax identifier."));
  DO(Consume(";"));

  syntax_identifier_ = syntax;

  if (syntax != "proto2" && !stop_after_syntax_identifier_) {
    AddError(syntax_token.line, syntax_token.column,
      "Unrecognized syntax identifier \"" + syntax + "\".  This parser "
      "only recognizes \"proto2\".");
    return false;
  }

  return true;
}

bool Parser::ParseTopLevelStatement(FileDescriptorProto* file) {
  if (TryConsume(";")) {
    // empty statement; ignore
    return true;
  } else if (LookingAt("message")) {
    return ParseMessageDefinition(file->add_message_type());
  } else if (LookingAt("enum")) {
    return ParseEnumDefinition(file->add_enum_type());
  } else if (LookingAt("service")) {
    return ParseServiceDefinition(file->add_service());
  } else if (LookingAt("extend")) {
    return ParseExtend(file->mutable_extension(),
                       file->mutable_message_type());
  } else if (LookingAt("import")) {
    return ParseImport(file->add_dependency());
  } else if (LookingAt("package")) {
    return ParsePackage(file);
  } else if (LookingAt("option")) {
    return ParseOption(file->mutable_options());
  } else {
    AddError("Expected top-level statement (e.g. \"message\").");
    return false;
  }
}

// -------------------------------------------------------------------
// Messages

bool Parser::ParseMessageDefinition(DescriptorProto* message) {
  DO(Consume("message"));
  RecordLocation(message, DescriptorPool::ErrorCollector::NAME);
  DO(ConsumeIdentifier(message->mutable_name(), "Expected message name."));
  DO(ParseMessageBlock(message));
  return true;
}

bool Parser::ParseMessageBlock(DescriptorProto* message) {
  DO(Consume("{"));

  while (!TryConsume("}")) {
    if (AtEnd()) {
      AddError("Reached end of input in message definition (missing '}').");
      return false;
    }

    if (!ParseMessageStatement(message)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  }

  return true;
}

bool Parser::ParseMessageStatement(DescriptorProto* message) {
  if (TryConsume(";")) {
    // empty statement; ignore
    return true;
  } else if (LookingAt("message")) {
    return ParseMessageDefinition(message->add_nested_type());
  } else if (LookingAt("enum")) {
    return ParseEnumDefinition(message->add_enum_type());
  } else if (LookingAt("extensions")) {
    return ParseExtensions(message);
  } else if (LookingAt("extend")) {
    return ParseExtend(message->mutable_extension(),
                       message->mutable_nested_type());
  } else if (LookingAt("option")) {
    return ParseOption(message->mutable_options());
  } else {
    return ParseMessageField(message->add_field(),
                             message->mutable_nested_type());
  }
}

bool Parser::ParseMessageField(FieldDescriptorProto* field,
                               RepeatedPtrField<DescriptorProto>* messages) {
  // Parse label and type.
  FieldDescriptorProto::Label label;
  DO(ParseLabel(&label));
  field->set_label(label);

  RecordLocation(field, DescriptorPool::ErrorCollector::TYPE);
  FieldDescriptorProto::Type type = FieldDescriptorProto::TYPE_INT32;
  string type_name;
  DO(ParseType(&type, &type_name));
  if (type_name.empty()) {
    field->set_type(type);
  } else {
    field->set_type_name(type_name);
  }

  // Parse name and '='.
  RecordLocation(field, DescriptorPool::ErrorCollector::NAME);
  io::Tokenizer::Token name_token = input_->current();
  DO(ConsumeIdentifier(field->mutable_name(), "Expected field name."));
  DO(Consume("=", "Missing field number."));

  // Parse field number.
  RecordLocation(field, DescriptorPool::ErrorCollector::NUMBER);
  int number;
  DO(ConsumeInteger(&number, "Expected field number."));
  field->set_number(number);

  // Parse options.
  DO(ParseFieldOptions(field));

  // Deal with groups.
  if (type_name.empty() && type == FieldDescriptorProto::TYPE_GROUP) {
    DescriptorProto* group = messages->Add();
    group->set_name(field->name());
    // Record name location to match the field name's location.
    RecordLocation(group, DescriptorPool::ErrorCollector::NAME,
                   name_token.line, name_token.column);

    // As a hack for backwards-compatibility, we force the group name to start
    // with a capital letter and lower-case the field name.  New code should
    // not use groups; it should use nested messages.
    if (group->name()[0] < 'A' || 'Z' < group->name()[0]) {
      AddError(name_token.line, name_token.column,
        "Group names must start with a capital letter.");
    }
    LowerString(field->mutable_name());

    field->set_type_name(group->name());
    if (LookingAt("{")) {
      DO(ParseMessageBlock(group));
    } else {
      AddError("Missing group body.");
      return false;
    }
  } else {
    DO(Consume(";"));
  }

  return true;
}

bool Parser::ParseFieldOptions(FieldDescriptorProto* field) {
  if (!TryConsume("[")) return true;

  // Parse field options.
  do {
    if (LookingAt("default")) {
      DO(ParseDefaultAssignment(field));
    } else {
      DO(ParseOptionAssignment(field->mutable_options()));
    }
  } while (TryConsume(","));

  DO(Consume("]"));
  return true;
}

bool Parser::ParseDefaultAssignment(FieldDescriptorProto* field) {
  if (field->has_default_value()) {
    AddError("Already set option \"default\".");
    field->clear_default_value();
  }

  DO(Consume("default"));
  DO(Consume("="));

  RecordLocation(field, DescriptorPool::ErrorCollector::DEFAULT_VALUE);
  string* default_value = field->mutable_default_value();

  if (!field->has_type()) {
    // The field has a type name, but we don't know if it is a message or an
    // enum yet.  Assume an enum for now.
    DO(ConsumeIdentifier(default_value, "Expected identifier."));
    return true;
  }

  switch (field->type()) {
    case FieldDescriptorProto::TYPE_INT32:
    case FieldDescriptorProto::TYPE_INT64:
    case FieldDescriptorProto::TYPE_SINT32:
    case FieldDescriptorProto::TYPE_SINT64:
    case FieldDescriptorProto::TYPE_SFIXED32:
    case FieldDescriptorProto::TYPE_SFIXED64: {
      uint64 max_value = kint64max;
      if (field->type() == FieldDescriptorProto::TYPE_INT32 ||
          field->type() == FieldDescriptorProto::TYPE_SINT32 ||
          field->type() == FieldDescriptorProto::TYPE_SFIXED32) {
        max_value = kint32max;
      }

      // These types can be negative.
      if (TryConsume("-")) {
        default_value->append("-");
        // Two's complement always has one more negative value than positive.
        ++max_value;
      }
      // Parse the integer to verify that it is not out-of-range.
      uint64 value;
      DO(ConsumeInteger64(max_value, &value, "Expected integer."));
      // And stringify it again.
      default_value->append(SimpleItoa(value));
      break;
    }

    case FieldDescriptorProto::TYPE_UINT32:
    case FieldDescriptorProto::TYPE_UINT64:
    case FieldDescriptorProto::TYPE_FIXED32:
    case FieldDescriptorProto::TYPE_FIXED64: {
      uint64 max_value = kuint64max;
      if (field->type() == FieldDescriptorProto::TYPE_UINT32 ||
          field->type() == FieldDescriptorProto::TYPE_FIXED32) {
        max_value = kuint32max;
      }

      // Numeric, not negative.
      if (TryConsume("-")) {
        AddError("Unsigned field can't have negative default value.");
      }
      // Parse the integer to verify that it is not out-of-range.
      uint64 value;
      DO(ConsumeInteger64(max_value, &value, "Expected integer."));
      // And stringify it again.
      default_value->append(SimpleItoa(value));
      break;
    }

    case FieldDescriptorProto::TYPE_FLOAT:
    case FieldDescriptorProto::TYPE_DOUBLE:
      // These types can be negative.
      if (TryConsume("-")) {
        default_value->append("-");
      }
      // Parse the integer because we have to convert hex integers to decimal
      // floats.
      double value;
      DO(ConsumeNumber(&value, "Expected number."));
      // And stringify it again.
      default_value->append(SimpleDtoa(value));
      break;

    case FieldDescriptorProto::TYPE_BOOL:
      if (TryConsume("true")) {
        default_value->assign("true");
      } else if (TryConsume("false")) {
        default_value->assign("false");
      } else {
        AddError("Expected \"true\" or \"false\".");
        return false;
      }
      break;

    case FieldDescriptorProto::TYPE_STRING:
      DO(ConsumeString(default_value, "Expected string."));
      break;

    case FieldDescriptorProto::TYPE_BYTES:
      DO(ConsumeString(default_value, "Expected string."));
      *default_value = CEscape(*default_value);
      break;

    case FieldDescriptorProto::TYPE_ENUM:
      DO(ConsumeIdentifier(default_value, "Expected identifier."));
      break;

    case FieldDescriptorProto::TYPE_MESSAGE:
    case FieldDescriptorProto::TYPE_GROUP:
      AddError("Messages can't have default values.");
      return false;
  }

  return true;
}

bool Parser::ParseOptionNamePart(UninterpretedOption* uninterpreted_option) {
  UninterpretedOption::NamePart* name = uninterpreted_option->add_name();
  string identifier;  // We parse identifiers into this string.
  if (LookingAt("(")) {  // This is an extension.
    DO(Consume("("));
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
    DO(Consume(")"));
    name->set_is_extension(true);
  } else {  // This is a regular field.
    DO(ConsumeIdentifier(&identifier, "Expected identifier."));
    name->mutable_name_part()->append(identifier);
    name->set_is_extension(false);
  }
  return true;
}

// We don't interpret the option here. Instead we store it in an
// UninterpretedOption, to be interpreted later.
bool Parser::ParseOptionAssignment(Message* options) {
  // Create an entry in the uninterpreted_option field.
  const FieldDescriptor* uninterpreted_option_field = options->GetDescriptor()->
      FindFieldByName("uninterpreted_option");
  GOOGLE_CHECK(uninterpreted_option_field != NULL)
      << "No field named \"uninterpreted_option\" in the Options proto.";

  UninterpretedOption* uninterpreted_option = down_cast<UninterpretedOption*>(
      options->GetReflection()->AddMessage(options,
                                           uninterpreted_option_field));

  // Parse dot-separated name.
  RecordLocation(uninterpreted_option,
                 DescriptorPool::ErrorCollector::OPTION_NAME);

  DO(ParseOptionNamePart(uninterpreted_option));

  while (LookingAt(".")) {
    DO(Consume("."));
    DO(ParseOptionNamePart(uninterpreted_option));
  }

  DO(Consume("="));

  RecordLocation(uninterpreted_option,
                 DescriptorPool::ErrorCollector::OPTION_VALUE);

  // All values are a single token, except for negative numbers, which consist
  // of a single '-' symbol, followed by a positive number.
  bool is_negative = TryConsume("-");

  switch (input_->current().type) {
    case io::Tokenizer::TYPE_START:
      GOOGLE_LOG(FATAL) << "Trying to read value before any tokens have been read.";
      return false;

    case io::Tokenizer::TYPE_END:
      AddError("Unexpected end of stream while parsing option value.");
      return false;

    case io::Tokenizer::TYPE_IDENTIFIER: {
      if (is_negative) {
        AddError("Invalid '-' symbol before identifier.");
        return false;
      }
      string value;
      DO(ConsumeIdentifier(&value, "Expected identifier."));
      uninterpreted_option->set_identifier_value(value);
      break;
    }

    case io::Tokenizer::TYPE_INTEGER: {
      uint64 value;
      uint64 max_value =
          is_negative ? static_cast<uint64>(kint64max) + 1 : kuint64max;
      DO(ConsumeInteger64(max_value, &value, "Expected integer."));
      if (is_negative) {
        uninterpreted_option->set_negative_int_value(-value);
      } else {
        uninterpreted_option->set_positive_int_value(value);
      }
      break;
    }

    case io::Tokenizer::TYPE_FLOAT: {
      double value;
      DO(ConsumeNumber(&value, "Expected number."));
      uninterpreted_option->set_double_value(is_negative ? -value : value);
      break;
    }

    case io::Tokenizer::TYPE_STRING: {
      if (is_negative) {
        AddError("Invalid '-' symbol before string.");
        return false;
      }
      string value;
      DO(ConsumeString(&value, "Expected string."));
      uninterpreted_option->set_string_value(value);
      break;
    }

    case io::Tokenizer::TYPE_SYMBOL:
      AddError("Expected option value.");
      return false;
  }

  return true;
}

bool Parser::ParseExtensions(DescriptorProto* message) {
  // Parse the declaration.
  DO(Consume("extensions"));

  do {
    DescriptorProto::ExtensionRange* range = message->add_extension_range();
    RecordLocation(range, DescriptorPool::ErrorCollector::NUMBER);

    int start, end;
    DO(ConsumeInteger(&start, "Expected field number range."));

    if (TryConsume("to")) {
      if (TryConsume("max")) {
        end = FieldDescriptor::kMaxNumber;
      } else {
        DO(ConsumeInteger(&end, "Expected integer."));
      }
    } else {
      end = start;
    }

    // Users like to specify inclusive ranges, but in code we like the end
    // number to be exclusive.
    ++end;

    range->set_start(start);
    range->set_end(end);
  } while (TryConsume(","));

  DO(Consume(";"));
  return true;
}

bool Parser::ParseExtend(RepeatedPtrField<FieldDescriptorProto>* extensions,
                         RepeatedPtrField<DescriptorProto>* messages) {
  DO(Consume("extend"));

  // We expect to see at least one extension field defined in the extend block.
  // We need to create it now so we can record the extendee's location.
  FieldDescriptorProto* first_field = extensions->Add();

  // Parse the extendee type.
  RecordLocation(first_field, DescriptorPool::ErrorCollector::EXTENDEE);
  DO(ParseUserDefinedType(first_field->mutable_extendee()));

  // Parse the block.
  DO(Consume("{"));

  bool is_first = true;

  do {
    if (AtEnd()) {
      AddError("Reached end of input in extend definition (missing '}').");
      return false;
    }

    FieldDescriptorProto* field;
    if (is_first) {
      field = first_field;
      is_first = false;
    } else {
      field = extensions->Add();
      field->set_extendee(first_field->extendee());
    }

    if (!ParseMessageField(field, messages)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  } while(!TryConsume("}"));

  return true;
}

// -------------------------------------------------------------------
// Enums

bool Parser::ParseEnumDefinition(EnumDescriptorProto* enum_type) {
  DO(Consume("enum"));
  RecordLocation(enum_type, DescriptorPool::ErrorCollector::NAME);
  DO(ConsumeIdentifier(enum_type->mutable_name(), "Expected enum name."));
  DO(ParseEnumBlock(enum_type));
  return true;
}

bool Parser::ParseEnumBlock(EnumDescriptorProto* enum_type) {
  DO(Consume("{"));

  while (!TryConsume("}")) {
    if (AtEnd()) {
      AddError("Reached end of input in enum definition (missing '}').");
      return false;
    }

    if (!ParseEnumStatement(enum_type)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  }

  return true;
}

bool Parser::ParseEnumStatement(EnumDescriptorProto* enum_type) {
  if (TryConsume(";")) {
    // empty statement; ignore
    return true;
  } else if (LookingAt("option")) {
    return ParseOption(enum_type->mutable_options());
  } else {
    return ParseEnumConstant(enum_type->add_value());
  }
}

bool Parser::ParseEnumConstant(EnumValueDescriptorProto* enum_value) {
  RecordLocation(enum_value, DescriptorPool::ErrorCollector::NAME);
  DO(ConsumeIdentifier(enum_value->mutable_name(),
                       "Expected enum constant name."));
  DO(Consume("=", "Missing numeric value for enum constant."));

  bool is_negative = TryConsume("-");
  int number;
  DO(ConsumeInteger(&number, "Expected integer."));
  if (is_negative) number *= -1;
  enum_value->set_number(number);

  DO(ParseEnumConstantOptions(enum_value));

  DO(Consume(";"));

  return true;
}

bool Parser::ParseEnumConstantOptions(EnumValueDescriptorProto* value) {
  if (!TryConsume("[")) return true;

  do {
    DO(ParseOptionAssignment(value->mutable_options()));
  } while (TryConsume(","));

  DO(Consume("]"));
  return true;
}

// -------------------------------------------------------------------
// Services

bool Parser::ParseServiceDefinition(ServiceDescriptorProto* service) {
  DO(Consume("service"));
  RecordLocation(service, DescriptorPool::ErrorCollector::NAME);
  DO(ConsumeIdentifier(service->mutable_name(), "Expected service name."));
  DO(ParseServiceBlock(service));
  return true;
}

bool Parser::ParseServiceBlock(ServiceDescriptorProto* service) {
  DO(Consume("{"));

  while (!TryConsume("}")) {
    if (AtEnd()) {
      AddError("Reached end of input in service definition (missing '}').");
      return false;
    }

    if (!ParseServiceStatement(service)) {
      // This statement failed to parse.  Skip it, but keep looping to parse
      // other statements.
      SkipStatement();
    }
  }

  return true;
}

bool Parser::ParseServiceStatement(ServiceDescriptorProto* service) {
  if (TryConsume(";")) {
    // empty statement; ignore
    return true;
  } else if (LookingAt("option")) {
    return ParseOption(service->mutable_options());
  } else {
    return ParseServiceMethod(service->add_method());
  }
}

bool Parser::ParseServiceMethod(MethodDescriptorProto* method) {
  DO(Consume("rpc"));
  RecordLocation(method, DescriptorPool::ErrorCollector::NAME);
  DO(ConsumeIdentifier(method->mutable_name(), "Expected method name."));

  // Parse input type.
  DO(Consume("("));
  RecordLocation(method, DescriptorPool::ErrorCollector::INPUT_TYPE);
  DO(ParseUserDefinedType(method->mutable_input_type()));
  DO(Consume(")"));

  // Parse output type.
  DO(Consume("returns"));
  DO(Consume("("));
  RecordLocation(method, DescriptorPool::ErrorCollector::OUTPUT_TYPE);
  DO(ParseUserDefinedType(method->mutable_output_type()));
  DO(Consume(")"));

  if (TryConsume("{")) {
    // Options!
    while (!TryConsume("}")) {
      if (AtEnd()) {
        AddError("Reached end of input in method options (missing '}').");
        return false;
      }

      if (TryConsume(";")) {
        // empty statement; ignore
      } else {
        if (!ParseOption(method->mutable_options())) {
          // This statement failed to parse.  Skip it, but keep looping to
          // parse other statements.
          SkipStatement();
        }
      }
    }
  } else {
    DO(Consume(";"));
  }

  return true;
}

// -------------------------------------------------------------------

bool Parser::ParseLabel(FieldDescriptorProto::Label* label) {
  if (TryConsume("optional")) {
    *label = FieldDescriptorProto::LABEL_OPTIONAL;
    return true;
  } else if (TryConsume("repeated")) {
    *label = FieldDescriptorProto::LABEL_REPEATED;
    return true;
  } else if (TryConsume("required")) {
    *label = FieldDescriptorProto::LABEL_REQUIRED;
    return true;
  } else {
    AddError("Expected \"required\", \"optional\", or \"repeated\".");
    // We can actually reasonably recover here by just assuming the user
    // forgot the label altogether.
    *label = FieldDescriptorProto::LABEL_OPTIONAL;
    return true;
  }
}

bool Parser::ParseType(FieldDescriptorProto::Type* type,
                       string* type_name) {
  TypeNameMap::const_iterator iter = kTypeNames.find(input_->current().text);
  if (iter != kTypeNames.end()) {
    *type = iter->second;
    input_->Next();
  } else {
    DO(ParseUserDefinedType(type_name));
  }
  return true;
}

bool Parser::ParseUserDefinedType(string* type_name) {
  type_name->clear();

  TypeNameMap::const_iterator iter = kTypeNames.find(input_->current().text);
  if (iter != kTypeNames.end()) {
    // Note:  The only place enum types are allowed is for field types, but
    //   if we are parsing a field type then we would not get here because
    //   primitives are allowed there as well.  So this error message doesn't
    //   need to account for enums.
    AddError("Expected message type.");

    // Pretend to accept this type so that we can go on parsing.
    *type_name = input_->current().text;
    input_->Next();
    return true;
  }

  // A leading "." means the name is fully-qualified.
  if (TryConsume(".")) type_name->append(".");

  // Consume the first part of the name.
  string identifier;
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

bool Parser::ParsePackage(FileDescriptorProto* file) {
  if (file->has_package()) {
    AddError("Multiple package definitions.");
    // Don't append the new package to the old one.  Just replace it.  Not
    // that it really matters since this is an error anyway.
    file->clear_package();
  }

  DO(Consume("package"));

  RecordLocation(file, DescriptorPool::ErrorCollector::NAME);

  while (true) {
    string identifier;
    DO(ConsumeIdentifier(&identifier, "Expected identifier."));
    file->mutable_package()->append(identifier);
    if (!TryConsume(".")) break;
    file->mutable_package()->append(".");
  }

  DO(Consume(";"));
  return true;
}

bool Parser::ParseImport(string* import_filename) {
  DO(Consume("import"));
  DO(ConsumeString(import_filename,
    "Expected a string naming the file to import."));
  DO(Consume(";"));
  return true;
}

bool Parser::ParseOption(Message* options) {
  DO(Consume("option"));
  DO(ParseOptionAssignment(options));
  DO(Consume(";"));
  return true;
}

// ===================================================================

SourceLocationTable::SourceLocationTable() {}
SourceLocationTable::~SourceLocationTable() {}

bool SourceLocationTable::Find(
    const Message* descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    int* line, int* column) const {
  const pair<int, int>* result =
    FindOrNull(location_map_, make_pair(descriptor, location));
  if (result == NULL) {
    *line   = -1;
    *column = 0;
    return false;
  } else {
    *line   = result->first;
    *column = result->second;
    return true;
  }
}

void SourceLocationTable::Add(
    const Message* descriptor,
    DescriptorPool::ErrorCollector::ErrorLocation location,
    int line, int column) {
  location_map_[make_pair(descriptor, location)] = make_pair(line, column);
}

void SourceLocationTable::Clear() {
  location_map_.clear();
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
