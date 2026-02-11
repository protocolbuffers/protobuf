// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/ruby/rbs_generator.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/code_generator.h"
#include "google/protobuf/compiler/ruby/ruby_generator.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace ruby {

std::string GetRBSOutputFilename(absl::string_view proto_file) {
  return absl::StrCat(GetRequireName(proto_file), ".rbs");
}

std::string WrapUnion(const std::string& type) {
  // This is known not to work as intended for ::Array[::Integer | ::Float].
  // Maybe we need an AST
  if (absl::StrContains(type, '|')) {
    return absl::StrCat("(", type, ")");
  } else {
    return absl::StrCat(type);
  }
}

std::string MakeOptionalType(const std::string& type) {
  if (type.back() == '?') {
    return type;
  } else {
    return absl::StrCat(WrapUnion(type), "?");
  }
}

void PrintComment(const std::string& comment, io::Printer* printer) {
  if (comment.empty()) {
    return;
  }
  std::vector<std::string> lines =
      absl::StrSplit(comment, '\n', absl::SkipEmpty());
  if (!lines.empty() && lines.back().empty()) {
    lines.pop_back();
  }
  for (const std::string& line : lines) {
    // Spaces after `#` are usually included in the comment itself
    printer->Print("#$line$\n", "line", line);
  }
}

void PrintDeprecationComment(bool deprecated, io::Printer* printer) {
  if (deprecated) {
    printer->Print("# @deprecated\n");
  }
}

void InsertMidLine(bool& initial, io::Printer* printer) {
  if (initial) {
    initial = false;
  } else {
    printer->Print("\n");
  }
}

std::string UnionSeparator(bool& initial) {
  if (initial) {
    initial = false;
    return "  ";
  } else {
    return "| ";
  }
}

std::string ModulePath(const FileDescriptor* file) {
  // Mirror implementation of GeneratePackageModules
  bool need_change_to_module = true;
  std::string package_name;
  if (file->options().has_ruby_package()) {
    package_name = file->options().ruby_package();

    if (absl::StrContains(package_name, "::")) {
      need_change_to_module = false;
    }
  } else {
    package_name = std::string(file->package());
  }
  if (need_change_to_module) {
    std::vector<std::string> parts = absl::StrSplit(package_name, '.');
    package_name = "";
    for (const std::string& part : parts) {
      if (!package_name.empty()) {
        package_name += "::";
      }
      package_name += PackageToModule(part);
    }
  }
  if (!package_name.empty()) {
    // Absolutify
    package_name = absl::StrCat("::", package_name);
  }

  return package_name;
}
std::string RBSMessageFullName(const Descriptor* message) {
  if (message->containing_type() != nullptr) {
    return absl::StrCat(RBSMessageFullName(message->containing_type()),
                        "::", RubifyConstant(message->name()));
  } else {
    return absl::StrCat(ModulePath(message->file()),
                        "::", RubifyConstant(message->name()));
  }
}
std::string RBSEnumFullName(const EnumDescriptor* enum_) {
  if (enum_->containing_type() != nullptr) {
    return absl::StrCat(RBSMessageFullName(enum_->containing_type()),
                        "::", RubifyConstant(enum_->name()));
  } else {
    return absl::StrCat(ModulePath(enum_->file()),
                        "::", RubifyConstant(enum_->name()));
  }
}

// Corresponds with:
// - Convert_UpbToRuby in convert.c (protobuf_c)
// - convert_upb_to_ruby in convert.rb (protobuf_ffi)
// - wrapField in RubyMessage.java (protobuf_java)
std::string ScalarReadType(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
      return "::Float";
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
      return "::Integer";
    case FieldDescriptor::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return "::String";

    case FieldDescriptor::TYPE_ENUM:
      // ::MyEnum::names | ::Integer
      // Integer is for unknown enum values
      return absl::StrCat(RBSEnumFullName(field->enum_type()),
                          "::names | ::Integer");
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
      return RBSMessageFullName(field->message_type());
    default:
      return "untyped";
  }
}

// Corresponds with:
// - Convert_RubyToUpb in convert.c (protobuf_c)
// - convert_ruby_to_upb in convert.rb (protobuf_ffi)
// - checkType in Utils.java (protobuf_java)
std::string ScalarWriteType(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
      // - protobuf_c: Float | Integer
      // - protobuf_ffi: _ToF
      return "::Float | ::Integer";
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
      // It accepts Float as long as it's a whole number
      // - protobuf_c: Integer | Float
      // - protobuf_ffi: Numeric & _ToI
      return "::Integer | ::Float";
    case FieldDescriptor::TYPE_BOOL:
      // Not `boolish` as it only accepts true or false
      return "bool";
    case FieldDescriptor::TYPE_STRING:
      // string accepts Symbol and bytes not.
      // - protobuf_c: String | Symbol
      // - protobuf_ffi: String | Symbol but rejects subclasses of String
      return "::String | ::Symbol";
    case FieldDescriptor::TYPE_BYTES:
      return "::String";

    case FieldDescriptor::TYPE_ENUM: {
      // ::MyEnum::names | ::MyEnum::strings | ::Integer | ::Float
      // - protobuf_c: Integer | Float | String | Symbol where String and Symbol
      // must be known names
      // - protobuf_ffi: (Numeric & _ToI) | String | Symbol where String and
      // Symbol must be known names
      const std::string enum_name = RBSEnumFullName(field->enum_type());
      return absl::StrCat(enum_name, "::names | ", enum_name,
                          "::strings | ::Integer | ::Float");
    }
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE: {
      // - protobuf_c: TheMessageType | Time (for Timestamp) | (Numeric &
      // _ToInt) (for Duration)
      // - protobuf_ffi: TheMessageType | Hash | Time (for Timestamp) | (Numeric
      // & _ToInt) (for Duration)
      const Descriptor::WellKnownType wkt =
          field->message_type()->well_known_type();
      if (wkt == Descriptor::WELLKNOWNTYPE_TIMESTAMP) {
        return absl::StrCat(RBSMessageFullName(field->message_type()),
                            " | ::Time");
      } else if (wkt == Descriptor::WELLKNOWNTYPE_DURATION) {
        return absl::StrCat(RBSMessageFullName(field->message_type()),
                            " | ::int");
      } else {
        return RBSMessageFullName(field->message_type());
      }
    }
    default:
      return "untyped";
  }
}

// Corresponds with:
// - protobuf_c: Message_getfield in message.c
// - protobuf_ffi: get_field in message.rb
// - protobuf_java: getFieldInternal in RubyMessage.java
std::string FieldReadType(const FieldDescriptor* field) {
  // Things not covered in these branches:
  // - read optionalities: unfortunately, neither of them are handled in a
  // Ruby-friendly way:
  //   - oneof fields
  //   - proto2 optional
  //   - proto3 optional
  //   though you can manipulate optionality through `clear_*` and `has_*?`
  //   methods
  // - write optionalities: same as above, but real oneofs are handled as
  // exceptions
  if (field->is_map()) {
    const std::string key_read_type =
        ScalarReadType(field->message_type()->map_key());
    const std::string key_write_type =
        ScalarWriteType(field->message_type()->map_key());
    const std::string value_read_type =
        ScalarReadType(field->message_type()->map_value());
    const std::string value_write_type =
        ScalarWriteType(field->message_type()->map_value());
    return absl::StrCat("::Google::Protobuf::Map[", key_read_type, ", ",
                        value_read_type, ", ", key_write_type, ", ",
                        value_write_type, "]");
  } else if (field->is_repeated()) {
    const std::string element_read_type = ScalarReadType(field);
    const std::string element_write_type = ScalarWriteType(field);
    return absl::StrCat("::Google::Protobuf::RepeatedField[", element_read_type,
                        ", ", element_write_type, "]");
  } else if (field->type() == FieldDescriptor::TYPE_MESSAGE ||
             field->type() == FieldDescriptor::TYPE_GROUP) {
    return MakeOptionalType(ScalarReadType(field));
  } else {
    // Proto2 optionals, proto3 optionals, and real oneof fields don't return
    // nil. And that is why they are handled here. This is unfortunate, but you
    // can still manipulate optionality through `clear_*` and `has_*?` methods.
    return ScalarReadType(field);
  }
}

// Corresponds with:
// - protobuf_c: Message_setfield in message.c
// - protobuf_ffi: set_value_on_message in field_descriptor.rb
// - protobuf_java: setFieldInternal in RubyMessage.java
std::string FieldWriteType(const FieldDescriptor* field) {
  // Things not covered in these branches:
  // - read optionalities: unfortunately, neither of them are handled in a
  // Ruby-friendly way:
  //   - oneof fields
  //   - proto2 optional
  //   - proto3 optional
  //   though you can manipulate optionality through `clear_*` and `has_*?`
  //   methods
  // - write optionalities: same as above, but real oneofs are handled as
  // exceptions
  if (field->is_map() || field->is_repeated()) {
    return FieldReadType(field);
  } else if (field->type() == FieldDescriptor::TYPE_MESSAGE ||
             field->type() == FieldDescriptor::TYPE_GROUP ||
             field->real_containing_oneof()) {
    // In addition to sub-messages, real oneofs accept nil (but won't return
    // it).
    return MakeOptionalType(ScalarWriteType(field));
  } else {
    // Proto2 optionals and proto3 optionals don't accept nil.
    // And that is why they are handled here.
    // This is unfortunate, but you can still manipulate optionality through
    // `clear_*` and `has_*?` methods.
    return ScalarWriteType(field);
  }
}

// Corresponds with:
// - protobuf_c: Message_InitFieldFromValue in message.c
// - protobuf_ffi: initialize and index_assign_internal in message.rb and
// - protobuf_java: initialize in RubyMessage.java
std::string FieldInitType(const FieldDescriptor* field) {
  // All of them are optional here, as nil means to just skip initialization for
  // the field.
  if (field->is_map()) {
    const std::string key_write_type =
        ScalarWriteType(field->message_type()->map_key());
    const std::string value_write_type =
        ScalarWriteType(field->message_type()->map_value());
    return absl::StrCat("::Hash[", key_write_type, ", ", value_write_type,
                        "]?");
  } else if (field->is_map() || field->is_repeated()) {
    const std::string element_write_type = ScalarWriteType(field);
    return absl::StrCat("::Array[", element_write_type, "]?");
  } else if (field->type() == FieldDescriptor::TYPE_MESSAGE ||
             field->type() == FieldDescriptor::TYPE_GROUP) {
    const std::string full_name = RBSMessageFullName(field->message_type());
    // ::MyMessage | ::MyMessage::init_map
    return MakeOptionalType(
        absl::StrCat(full_name, " | ", full_name, "::init_map"));
  } else {
    return MakeOptionalType(ScalarWriteType(field));
  }
}

bool IsWrapper(const FieldDescriptor* field) {
  const Descriptor* message = field->message_type();
  if (message == nullptr) {
    return false;
  }
  switch (message->well_known_type()) {
    case Descriptor::WELLKNOWNTYPE_DOUBLEVALUE:
    case Descriptor::WELLKNOWNTYPE_FLOATVALUE:
    case Descriptor::WELLKNOWNTYPE_INT64VALUE:
    case Descriptor::WELLKNOWNTYPE_UINT64VALUE:
    case Descriptor::WELLKNOWNTYPE_INT32VALUE:
    case Descriptor::WELLKNOWNTYPE_UINT32VALUE:
    case Descriptor::WELLKNOWNTYPE_STRINGVALUE:
    case Descriptor::WELLKNOWNTYPE_BYTESVALUE:
    case Descriptor::WELLKNOWNTYPE_BOOLVALUE:
      return true;
    default:
      return false;
  }
}

void GenerateEnumValueTypeDefinition(const EnumValueDescriptor* value,
                                     io::Printer* printer) {
  SourceLocation location;
  bool has_location = value->GetSourceLocation(&location);
  if (has_location) {
    PrintComment(location.leading_comments, printer);
  }
  PrintDeprecationComment(value->options().deprecated(), printer);

  std::string name{value->name()};

  if (name[0] < 'A' || name[0] > 'Z') {
    if ('a' <= name[0] && name[0] <= 'z') {
      // auto capitalize
      name[0] = name[0] - 'a' + 'A';
    } else {
      printer->Print(
          "# Enum value '$name$' does not start with an uppercase letter "
          "as is required for Ruby constants.\n"
          "# $name$: $number$\n",
          "name", value->name(), "number", absl::StrCat(value->number()));
      return;
    }
  }

  printer->Print("$name$: $number$\n", "name", name, "number",
                 absl::StrCat(value->number()));
}

void GenerateEnumTypeDefinition(const EnumDescriptor* enum_,
                                io::Printer* printer) {
  SourceLocation location;
  bool has_location = enum_->GetSourceLocation(&location);
  if (has_location) {
    PrintComment(location.leading_comments, printer);
  }
  PrintDeprecationComment(enum_->options().deprecated(), printer);

  bool initial = true;

  printer->Print("module $name$\n", "name", RubifyConstant(enum_->name()));
  printer->Indent();
  InsertMidLine(initial, printer);
  printer->Print("extend ::Google::Protobuf::_EnumModule\n");

  for (int i = 0; i < enum_->value_count(); i++) {
    const EnumValueDescriptor* value = enum_->value(i);
    InsertMidLine(initial, printer);
    GenerateEnumValueTypeDefinition(value, printer);
  }

  std::vector<int> unique_numbers;
  absl::flat_hash_map<int, std::vector<std::string>> names_by_number;
  for (int i = 0; i < enum_->value_count(); i++) {
    const EnumValueDescriptor* value = enum_->value(i);
    // Check if there is an entry from names_by_number and if so, get the
    // position
    auto it = names_by_number.find(value->number());
    if (it == names_by_number.end()) {
      names_by_number[value->number()].push_back(std::string{value->name()});
      unique_numbers.push_back(value->number());
    } else {
      it->second.push_back(std::string{value->name()});
    }
  }

  InsertMidLine(initial, printer);
  printer->Print("def self.lookup:\n");
  printer->Indent();
  bool overload_initial = true;
  for (int number : unique_numbers) {
    const std::vector<std::string>& names = names_by_number[number];
    std::string name_union = absl::StrCat(":", names[0]);
    for (size_t i = 1; i < names.size(); i++) {
      absl::StrAppend(&name_union, " | :", names[i]);
    }
    printer->Print("$sep$($number$ number) -> $name_union$\n", "sep",
                   UnionSeparator(overload_initial), "number",
                   absl::StrCat(number), "name_union", WrapUnion(name_union));
  }
  printer->Print("$sep$(::int number) -> names?\n", "sep",
                 UnionSeparator(overload_initial));
  printer->Print("$sep$...\n", "sep", UnionSeparator(overload_initial));
  printer->Outdent();

  InsertMidLine(initial, printer);
  printer->Print("def self.resolve:\n");
  printer->Indent();
  overload_initial = true;
  for (int i = 0; i < enum_->value_count(); i++) {
    const EnumValueDescriptor* value = enum_->value(i);
    printer->Print("$sep$(:$name$ name) -> $number$\n", "sep",
                   UnionSeparator(overload_initial), "name", value->name(),
                   "number", absl::StrCat(value->number()));
  }
  printer->Print("$sep$(::Symbol name) -> numbers?\n", "sep",
                 UnionSeparator(overload_initial));
  printer->Print("$sep$...\n", "sep", UnionSeparator(overload_initial));
  printer->Outdent();

  std::string all_name_union = absl::StrCat(":", enum_->value(0)->name());
  for (int i = 1; i < enum_->value_count(); i++) {
    absl::StrAppend(&all_name_union, " | :", enum_->value(i)->name());
  }
  InsertMidLine(initial, printer);
  printer->Print("type names = $name_union$\n", "name_union", all_name_union);

  std::string all_string_union =
      absl::StrCat("\"", enum_->value(0)->name(), "\"");
  for (int i = 1; i < enum_->value_count(); i++) {
    absl::StrAppend(&all_string_union, " | \"", enum_->value(i)->name(), "\"");
  }
  InsertMidLine(initial, printer);
  printer->Print("type strings = $string_union$\n", "string_union",
                 all_string_union);

  std::string all_number_union = absl::StrCat(unique_numbers[0]);
  for (size_t i = 1; i < unique_numbers.size(); i++) {
    absl::StrAppend(&all_number_union, " | ", unique_numbers[i]);
  }
  InsertMidLine(initial, printer);
  printer->Print("type numbers = $number_union$\n", "number_union",
                 all_number_union);

  printer->Outdent();
  printer->Print("end\n");
}

void GenerateFieldTypeDefinition(const FieldDescriptor* field,
                                 io::Printer* printer) {
  SourceLocation location;
  bool has_location = field->GetSourceLocation(&location);
  if (has_location) {
    PrintComment(location.leading_comments, printer);
  }
  PrintDeprecationComment(field->options().deprecated(), printer);

  // attr_accessor my_field(): ::Integer

  std::string read_type = FieldReadType(field);
  std::string write_type = FieldWriteType(field);
  if (read_type == write_type) {
    printer->Print("attr_accessor $name$(): $read_type$\n", "name",
                   field->name(), "read_type", read_type);
  } else {
    printer->Print(
        "attr_reader $name$(): $read_type$\n"
        "attr_writer $name$(): $write_type$\n",
        "name", field->name(), "read_type", read_type, "write_type",
        write_type);
  }

  if (!field->is_repeated() && IsWrapper(field)) {
    // field is already known to be a message
    const FieldDescriptor* wrapped_field =
        field->message_type()->FindFieldByNumber(1);
    if (wrapped_field != nullptr) {
      // attr_accessor my_field_as_value(): ::Integer?
      std::string wrapped_read_type =
          MakeOptionalType(ScalarReadType(wrapped_field));
      std::string wrapped_write_type =
          MakeOptionalType(ScalarWriteType(wrapped_field));
      if (wrapped_read_type == wrapped_write_type) {
        printer->Print("attr_accessor $name$_as_value(): $read_type$\n", "name",
                       field->name(), "read_type", wrapped_read_type);
      } else {
        printer->Print(
            "attr_reader $name$_as_value(): $read_type$\n"
            "attr_writer $name$_as_value(): $write_type$\n",
            "name", field->name(), "read_type", wrapped_read_type, "write_type",
            wrapped_write_type);
      }
    }
  }

  if (field->type() == FieldDescriptor::TYPE_ENUM) {
    // attr_accessor my_field_const(): ::Integer
    if (field->is_repeated()) {
      printer->Print("attr_reader $name$_const(): ::Array[::Integer]\n", "name",
                     field->name());
    } else {
      // Always non-optional
      printer->Print("attr_reader $name$_const(): ::Integer\n", "name",
                     field->name());
    }
  }

  if (field->has_presence()) {
    // def has_my_field?: () -> bool
    printer->Print("def has_$name$?: () -> bool\n", "name", field->name());
  }

  // def clear_my_field: () -> void
  printer->Print("def clear_$name$: () -> void\n", "name", field->name());
}

void GenerateOneofDeclTypeDefinition(const OneofDescriptor* oneof,
                                     io::Printer* printer) {
  SourceLocation location;
  bool has_location = oneof->GetSourceLocation(&location);
  if (has_location) {
    PrintComment(location.leading_comments, printer);
  }

  // attr_reader my_oneof(): ::Integer?
  std::vector<std::string> oneof_scalar_types;
  std::unordered_set<std::string> oneof_scalar_types_set;
  for (int i = 0; i < oneof->field_count(); i++) {
    const FieldDescriptor* field = oneof->field(i);
    const std::string read_type = ScalarReadType(field);
    if (oneof_scalar_types_set.insert(read_type).second) {
      oneof_scalar_types.push_back(read_type);
    }
  }
  std::string oneof_scalar_type =
      MakeOptionalType(absl::StrJoin(oneof_scalar_types, " | "));

  printer->Print("attr_reader $name$(): $type$\n", "name", oneof->name(),
                 "type", oneof_scalar_type);

  // def has_my_field?: () -> bool
  printer->Print("def has_$name$?: () -> bool\n", "name", oneof->name());

  // def clear_my_field: () -> void
  printer->Print("def clear_$name$: () -> void\n", "name", oneof->name());
}

void GenerateMessageInitMap(const Descriptor* message, io::Printer* printer) {
  printer->Print("type init_map = {\n");
  printer->Indent();
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);

    std::string init_type = FieldInitType(field);
    // We may add support for https://github.com/ruby/rbs/pull/1717
    // when it is shipped in Sorbet and RubyMine.
    printer->Print(
        // "?$name$: $type$,\n", // Wait for
        // https://github.com/ruby/rbs/pull/1717
        "$name$: $type$,\n", "name", field->name(), "type", init_type);
    printer->Print(
        // "\"$name$\" => $type$,\n", // Wait for
        // https://github.com/ruby/rbs/pull/1717
        "\"$name$\" => $type$,\n", "name", field->name(), "type", init_type);
  }
  printer->Outdent();
  printer->Print("}\n");
}

void GenerateIndexReaderDefinition(const Descriptor* message,
                                   io::Printer* printer) {
  printer->Print("def []:\n");
  printer->Indent();
  bool overload_initial = true;
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    printer->Print("$sep$(\"$name$\" name) -> $type$\n", "sep",
                   UnionSeparator(overload_initial), "name", field->name(),
                   "type", WrapUnion(FieldReadType(field)));
  }
  printer->Outdent();
}

void GenerateIndexWriterDefinition(const Descriptor* message,
                                   io::Printer* printer) {
  printer->Print("def []=:\n");
  printer->Indent();
  bool overload_initial = true;
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    printer->Print("$sep$(\"$name$\" name, $type$ value) -> void\n", "sep",
                   UnionSeparator(overload_initial), "name", field->name(),
                   "type", WrapUnion(FieldWriteType(field)));
  }
  printer->Outdent();
}

void GenerateMessageTypeDefinition(const Descriptor* message,
                                   io::Printer* printer) {
  SourceLocation location;
  bool has_location = message->GetSourceLocation(&location);
  if (has_location) {
    PrintComment(location.leading_comments, printer);
  }
  PrintDeprecationComment(message->options().deprecated(), printer);

  printer->Print("class $classname$ < ::Google::Protobuf::AbstractMessage\n",
                 "classname", RubifyConstant(message->name()));
  printer->Indent();

  bool initial = true;

  for (int i = 0; i < message->nested_type_count(); i++) {
    if (message->nested_type(i)->map_key() != nullptr) {
      continue;
    }
    InsertMidLine(initial, printer);
    GenerateMessageTypeDefinition(message->nested_type(i), printer);
  }

  for (int i = 0; i < message->enum_type_count(); i++) {
    InsertMidLine(initial, printer);
    GenerateEnumTypeDefinition(message->enum_type(i), printer);
  }

  InsertMidLine(initial, printer);
  printer->Print("include ::Google::Protobuf::_MessageClass[$name$]\n", "name",
                 RBSMessageFullName(message));

  for (int i = 0; i < message->field_count(); i++) {
    InsertMidLine(initial, printer);
    const FieldDescriptor* field = message->field(i);
    GenerateFieldTypeDefinition(field, printer);
  }

  for (int i = 0; i < message->oneof_decl_count(); i++) {
    InsertMidLine(initial, printer);
    // Note: Ruby PB impl currently treats synthetic oneofs indifferently.
    const OneofDescriptor* oneof = message->oneof_decl(i);
    GenerateOneofDeclTypeDefinition(oneof, printer);
  }

  InsertMidLine(initial, printer);
  GenerateMessageInitMap(message, printer);

  InsertMidLine(initial, printer);
  printer->Print("def initialize: (?init_map initial_value) -> void\n");

  const Descriptor::WellKnownType wkt = message->well_known_type();
  if (message->field_count() > 0
      // These two redefine `[]` and `[]=`
      && wkt != Descriptor::WELLKNOWNTYPE_LISTVALUE &&
      wkt != Descriptor::WELLKNOWNTYPE_STRUCT) {
    InsertMidLine(initial, printer);
    GenerateIndexReaderDefinition(message, printer);
    InsertMidLine(initial, printer);
    GenerateIndexWriterDefinition(message, printer);
  }

  printer->Outdent();
  printer->Print("end\n");
}

void GenerateEnumLookup(const EnumDescriptor* enum_, io::Printer* printer,
                        bool& initial) {
  printer->Print(
      "$sep$(\"$full_name$\" name) -> (::Google::Protobuf::EnumDescriptor & "
      "::Google::Protobuf::_SpecificEnumDescriptor[singleton($ruby_name$)])\n",
      "sep", UnionSeparator(initial), "full_name", enum_->full_name(),
      "ruby_name", RBSEnumFullName(enum_));
}
void GenerateMessageLookup(const Descriptor* message, io::Printer* printer,
                           bool& initial) {
  printer->Print(
      "$sep$(\"$full_name$\" name) -> (::Google::Protobuf::Descriptor & "
      "::Google::Protobuf::_SpecificDescriptor[singleton($ruby_name$)])\n",
      "sep", UnionSeparator(initial), "full_name", message->full_name(),
      "ruby_name", RBSMessageFullName(message));

  for (int i = 0; i < message->nested_type_count(); i++) {
    if (message->nested_type(i)->map_key() != nullptr) {
      continue;
    }
    GenerateMessageLookup(message->nested_type(i), printer, initial);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnumLookup(message->enum_type(i), printer, initial);
  }
}

void GenerateDescriptorLookupOverride(const FileDescriptor* file,
                                      io::Printer* printer) {
  printer->Print("module Google\n");
  printer->Indent();
  printer->Print("module Protobuf\n");
  printer->Indent();
  printer->Print("class DescriptorPool\n");
  printer->Indent();
  printer->Print("def lookup:\n");
  printer->Indent();

  bool overload_initial = true;

  for (int i = 0; i < file->message_type_count(); i++) {
    const Descriptor* message = file->message_type(i);
    GenerateMessageLookup(message, printer, overload_initial);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    const EnumDescriptor* enum_ = file->enum_type(i);
    GenerateEnumLookup(enum_, printer, overload_initial);
  }

  printer->Print("$sep$...\n", "sep", UnionSeparator(overload_initial));

  printer->Outdent();
  printer->Outdent();
  printer->Print("end\n");
  printer->Outdent();
  printer->Print("end\n");
  printer->Outdent();
  printer->Print("end\n");
}

bool GenerateRBSFile(const FileDescriptor* file, io::Printer* printer,
                     std::string* error) {
  printer->Print(
      "# Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
      "# This RBS interface is provided for convenience, on a best-effort "
      "basis.\n"
      "# The library is the definitive source for the API contract; if the RBS "
      "file\n"
      "# and the library's behavior differ, the library behavior is "
      "authoritative.\n"
      "# We welcome fixes to change the RBS file to match.\n"
      "# source: $filename$\n"
      "\n",
      "filename", file->name());

  bool initial = true;

  int levels = GeneratePackageModules(file, printer);
  for (int i = 0; i < file->message_type_count(); i++) {
    InsertMidLine(initial, printer);
    GenerateMessageTypeDefinition(file->message_type(i), printer);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    InsertMidLine(initial, printer);
    GenerateEnumTypeDefinition(file->enum_type(i), printer);
  }
  EndPackageModules(levels, printer);

  printer->Print("\n");
  GenerateDescriptorLookupOverride(file, printer);

  return true;
}

bool RBSGenerator::Generate(const FileDescriptor* file,
                            const std::string& parameter,
                            GeneratorContext* generator_context,
                            std::string* error) const {
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(GetRBSOutputFilename(file->name())));
  io::Printer printer(output.get(), '$');

  return GenerateRBSFile(file, &printer, error);
}

}  // namespace ruby
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
