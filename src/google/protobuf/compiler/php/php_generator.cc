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

#include <google/protobuf/compiler/php/php_generator.h>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/strutil.h>

#include <sstream>

using google::protobuf::internal::scoped_ptr;

const std::string kDescriptorFile = "google/protobuf/descriptor.proto";
const std::string kDescriptorPackageName = "Google\\Protobuf\\Internal";

namespace google {
namespace protobuf {
namespace compiler {
namespace php {

// Forward decls.
std::string PhpName(const std::string& full_name, bool is_descriptor);
std::string DefaultForField(google::protobuf::FieldDescriptor* field);
std::string IntToString(int32 value);
std::string GeneratedFileName(const std::string& proto_file,
                              bool is_descriptor);
std::string LabelForField(google::protobuf::FieldDescriptor* field);
std::string TypeName(google::protobuf::FieldDescriptor* field);
std::string UnderscoresToCamelCase(const string& name, bool cap_first_letter);
std::string EscapeDollor(const string& to_escape);
std::string BinaryToHex(const string& binary);
void GenerateMessage(const string& name_prefix,
                     const google::protobuf::Descriptor* message,
                     bool is_descriptor,
                     google::protobuf::io::Printer* printer);
void GenerateEnum(const google::protobuf::EnumDescriptor* en,
                  google::protobuf::io::Printer* printer);
void Indent(google::protobuf::io::Printer* printer);
void Outdent(google::protobuf::io::Printer* printer);

std::string MessageName(const google::protobuf::Descriptor* message,
                        bool is_descriptor) {
  string message_name = message->name();
  const google::protobuf::Descriptor* descriptor = message->containing_type();
  while (descriptor != NULL) {
    message_name = descriptor->name() + '_' + message_name;
    descriptor = descriptor->containing_type();
  }
  return PhpName(message->file()->package(), is_descriptor) + '\\' +
         message_name;
}

std::string MessageFullName(const google::protobuf::Descriptor* message,
                            bool is_descriptor) {
  if (is_descriptor) {
    return StringReplace(message->full_name(),
                         "google.protobuf",
                         "google.protobuf.internal", false);
  } else {
    return message->full_name();
  }
}

std::string EnumFullName(const google::protobuf::EnumDescriptor* envm,
                         bool is_descriptor) {
  if (is_descriptor) {
    return StringReplace(envm->full_name(),
                         "google.protobuf",
                         "google.protobuf.internal", false);
  } else {
    return envm->full_name();
  }
}

std::string EnumClassName(const google::protobuf::EnumDescriptor* envm) {
  string enum_class_name = envm->name();
  const google::protobuf::Descriptor* descriptor = envm->containing_type();
  while (descriptor != NULL) {
    enum_class_name = descriptor->name() + '_' + enum_class_name;
    descriptor = descriptor->containing_type();
  }
  return enum_class_name;
}

std::string EnumName(const google::protobuf::EnumDescriptor* envm,
                     bool is_descriptor) {
  string enum_name = EnumClassName(envm);
  return PhpName(envm->file()->package(), is_descriptor) + '\\' + enum_name;
}

std::string PhpName(const std::string& full_name, bool is_descriptor) {
  if (is_descriptor) {
    return kDescriptorPackageName;
  }

  std::string result;
  bool cap_next_letter = true;
  for (int i = 0; i < full_name.size(); i++) {
    if ('a' <= full_name[i] && full_name[i] <= 'z' && cap_next_letter) {
      result += full_name[i] + ('A' - 'a');
      cap_next_letter = false;
    } else if (full_name[i] == '.') {
      result += '\\';
      cap_next_letter = true;
    } else {
      result += full_name[i];
      cap_next_letter = false;
    }
  }
  return result;
}

std::string DefaultForField(const google::protobuf::FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_ENUM: return "0";
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT: return "0.0";
    case FieldDescriptor::TYPE_BOOL: return "false";
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES: return "''";
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_GROUP: return "null";
    default: assert(false); return "";
  }
}

std::string GeneratedFileName(const std::string& proto_file,
                              bool is_descriptor) {
  if (is_descriptor) {
    return "descriptor_internal.pb.php";
  } else {
    int lastindex = proto_file.find_last_of(".");
    return proto_file.substr(0, lastindex) + ".pb.php";
  }
}

std::string IntToString(int32 value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

std::string LabelForField(const google::protobuf::FieldDescriptor* field) {
  switch (field->label()) {
    case FieldDescriptor::LABEL_OPTIONAL: return "optional";
    case FieldDescriptor::LABEL_REQUIRED: return "required";
    case FieldDescriptor::LABEL_REPEATED: return "repeated";
    default: assert(false); return "";
  }
}

std::string TypeName(const google::protobuf::FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32: return "int32";
    case FieldDescriptor::TYPE_INT64: return "int64";
    case FieldDescriptor::TYPE_UINT32: return "uint32";
    case FieldDescriptor::TYPE_UINT64: return "uint64";
    case FieldDescriptor::TYPE_SINT32: return "sint32";
    case FieldDescriptor::TYPE_SINT64: return "sint64";
    case FieldDescriptor::TYPE_FIXED32: return "fixed32";
    case FieldDescriptor::TYPE_FIXED64: return "fixed64";
    case FieldDescriptor::TYPE_SFIXED32: return "sfixed32";
    case FieldDescriptor::TYPE_SFIXED64: return "sfixed64";
    case FieldDescriptor::TYPE_DOUBLE: return "double";
    case FieldDescriptor::TYPE_FLOAT: return "float";
    case FieldDescriptor::TYPE_BOOL: return "bool";
    case FieldDescriptor::TYPE_ENUM: return "enum";
    case FieldDescriptor::TYPE_STRING: return "string";
    case FieldDescriptor::TYPE_BYTES: return "bytes";
    case FieldDescriptor::TYPE_MESSAGE: return "message";
    case FieldDescriptor::TYPE_GROUP: return "group";
    default: assert(false); return "";
  }
}

std::string EnumOrMessageSuffix(
    const google::protobuf::FieldDescriptor* field, bool is_descriptor) {
  if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    return ", '" + MessageFullName(field->message_type(), is_descriptor) + "'";
  }
  if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
    return ", '" + EnumFullName(field->enum_type(), is_descriptor) + "'";
  }
  return "";
}

// Converts a name to camel-case. If cap_first_letter is true, capitalize the
// first letter.
std::string UnderscoresToCamelCase(const string& input, bool cap_first_letter) {
  std::string result;
  for (int i = 0; i < input.size(); i++) {
    if ('a' <= input[i] && input[i] <= 'z') {
      if (cap_first_letter) {
        result += input[i] + ('A' - 'a');
      } else {
        result += input[i];
      }
      cap_first_letter = false;
    } else if ('A' <= input[i] && input[i] <= 'Z') {
      if (i == 0 && !cap_first_letter) {
        // Force first letter to lower-case unless explicitly told to
        // capitalize it.
        result += input[i] + ('a' - 'A');
      } else {
        // Capital letters after the first are left as-is.
        result += input[i];
      }
      cap_first_letter = false;
    } else if ('0' <= input[i] && input[i] <= '9') {
      result += input[i];
      cap_first_letter = true;
    } else {
      cap_first_letter = true;
    }
  }
  // Add a trailing "_" if the name should be altered.
  if (input[input.size() - 1] == '#') {
    result += '_';
  }
  return result;
}

std::string EscapeDollor(const string& to_escape) {
  return StringReplace(to_escape, "$", "\\$", true);
}

std::string BinaryToHex(const string& src) {
  string dest;
  size_t i;
  unsigned char symbol[16] = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'a', 'b',
    'c', 'd', 'e', 'f',
  };

  dest.resize(src.size() * 2);
  char* append_ptr = &dest[0];

  for (i = 0; i < src.size(); i++) {
    *append_ptr++ = symbol[(src[i] & 0xf0) >> 4];
    *append_ptr++ = symbol[src[i] & 0x0f];
  }

  return dest;
}

void Indent(google::protobuf::io::Printer* printer) {
  printer->Indent();
  printer->Indent();
}
void Outdent(google::protobuf::io::Printer* printer) {
  printer->Outdent();
  printer->Outdent();
}

void GenerateField(const google::protobuf::FieldDescriptor* field,
                   google::protobuf::io::Printer* printer, bool is_descriptor) {
  if (field->is_repeated()) {
    printer->Print(
        "private $@name@;\n",
        "name", field->name());
  } else if (field->containing_oneof()) {
    // Oneof fields are handled by GenerateOneofField.
    return;
  } else {
    printer->Print(
        "private $@name@ = @default@;\n",
        "name", field->name(),
        "default", DefaultForField(field));
  }

  if (is_descriptor) {
    printer->Print(
        "private $has_@name@ = false;\n",
        "name", field->name());
  }
}

void GenerateOneofField(const google::protobuf::OneofDescriptor* oneof,
                        google::protobuf::io::Printer* printer) {
  // Oneof property needs to be protected in order to be accessed by parent
  // class in implementation.
  printer->Print(
      "protected $@name@;\n",
      "name", oneof->name());
}

void GenerateFieldAccessor(const google::protobuf::FieldDescriptor* field,
                           bool is_descriptor,
                           google::protobuf::io::Printer* printer) {
  const OneofDescriptor* oneof = field->containing_oneof();

  // Generate getter.
  if (oneof != NULL) {
    printer->Print(
        "public function get@camel_name@()\n"
        "{\n"
        "    return $this->readOneof(@number@);\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true),
        "number", IntToString(field->number()));
  } else {
    printer->Print(
        "public function get@camel_name@()\n"
        "{\n"
        "    return $this->@name@;\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true), "name",
        field->name());
  }

  // Generate setter.
  printer->Print(
      "public function set@camel_name@(@var@)\n"
      "{\n",
      "camel_name", UnderscoresToCamelCase(field->name(), true),
      "var", (field->is_repeated() ||
              field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) ?
             "&$var": "$var");

  Indent(printer);

  // Type check.
  if (field->is_map()) {
  } else if (field->is_repeated()) {
    printer->Print(
        "GPBUtil::checkRepeatedField($var, GPBType::@type@",
        "type", ToUpper(field->type_name()));
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      printer->Print(
          ", \\@class_name@);\n",
          "class_name",
          MessageName(field->message_type(), is_descriptor) + "::class");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      printer->Print(
          ", @class_name@);\n",
          "class_name",
          EnumName(field->enum_type(), is_descriptor) + "::class");
    } else {
      printer->Print(");\n");
    }
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    printer->Print(
        "GPBUtil::checkMessage($var, \\@class_name@::class);\n",
        "class_name", MessageName(field->message_type(), is_descriptor));
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
    printer->Print(
        "GPBUtil::checkEnum($var, \\@class_name@::class);\n",
        "class_name", EnumName(field->enum_type(), is_descriptor));
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
    printer->Print(
        "GPBUtil::checkString($var, @utf8@);\n",
        "utf8",
        field->type() == FieldDescriptor::TYPE_STRING ? "True": "False");
  } else {
    printer->Print(
        "GPBUtil::check@type@($var);\n",
        "type", UnderscoresToCamelCase(field->cpp_type_name(), true));
  }

  if (oneof != NULL) {
    printer->Print(
        "$this->writeOneof(@number@, $var);\n",
        "number", IntToString(field->number()));
  } else {
    printer->Print(
        "$this->@name@ = $var;\n",
        "name", field->name());
  }

  // Set has bit for proto2 only.
  if (is_descriptor) {
    printer->Print(
        "$this->has_@field_name@ = true;\n",
        "field_name", field->name());
  }

  Outdent(printer);

  printer->Print(
      "}\n\n");

  // Generate has method for proto2 only.
  if (is_descriptor) {
    printer->Print(
        "public function has@camel_name@()\n"
        "{\n"
        "    return $this->has_@field_name@;\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true),
        "field_name", field->name());
  }
}

void GenerateRepeatedFieldDecode(
    const google::protobuf::FieldDescriptor* field,
    google::protobuf::io::Printer* printer) {
  printer->Print(
      "if ($input->read@cap_wire_type@($var)) return False;\n"
      "$this->get@cap_field_name@() []= $var;\n",
      "cap_field_name", UnderscoresToCamelCase(field->name(), true),
      "cap_wire_type", UnderscoresToCamelCase(field->type_name(), true));
}

void GeneratePrimitiveFieldDecode(
    const google::protobuf::FieldDescriptor* field,
    google::protobuf::io::Printer* printer) {
  printer->Print(
      "if ($input->read@cap_wire_type@($var)) return False;\n"
      "$this->set@cap_field_name@($var);\n",
      "cap_field_name", UnderscoresToCamelCase(field->name(), true),
      "cap_wire_type", UnderscoresToCamelCase(field->type_name(), true));
}

void GenerateFieldDecode(const google::protobuf::FieldDescriptor* field,
                         google::protobuf::io::Printer* printer) {
  printer->Print(
      "case @number@:\n",
      "number", IntToString(field->number()));
  Indent(printer);

  if (field->is_repeated()) {
    GenerateRepeatedFieldDecode(field, printer);
  } else {
    GeneratePrimitiveFieldDecode(field, printer);
  }

  printer->Print(
      "break;\n");
  Outdent(printer);
}

void GenerateMessage(const string& name_prefix,
                     const google::protobuf::Descriptor* message,
                     bool is_descriptor,
                     google::protobuf::io::Printer* printer) {
  // Don't generate MapEntry messages -- we use the PHP extension's native
  // support for map fields instead.
  if (message->options().map_entry()) {
    return;
  }

  string message_name = name_prefix.empty()?
      message->name() : name_prefix + "_" + message->name();

  printer->Print(
      "class @name@ extends \\Google\\Protobuf\\Internal\\Message\n"
      "{\n",
      "name", message_name);
  Indent(printer);

  // Field and oneof definitions.
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    GenerateField(field, printer, is_descriptor);
  }
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    GenerateOneofField(oneof, printer);
  }
  printer->Print("\n");

  // Field and oneof accessors.
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    GenerateFieldAccessor(field, is_descriptor, printer);
  }
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const google::protobuf::OneofDescriptor* oneof = message->oneof_decl(i);
    printer->Print(
      "public function get@camel_name@()\n"
      "{\n"
      "    return $this->@name@;\n"
      "}\n\n",
      "camel_name", UnderscoresToCamelCase(oneof->name(), true), "name",
      oneof->name());
  }

  Outdent(printer);
  printer->Print("}\n\n");

  // Nested messages and enums.
  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateMessage(message_name, message->nested_type(i), is_descriptor,
                    printer);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnum(message->enum_type(i), printer);
  }
}

void GenerateEnumToPool(const google::protobuf::EnumDescriptor* en,
                        bool is_descriptor,
                        google::protobuf::io::Printer* printer) {
  printer->Print(
      "$pool->addEnum('@name@', @class_name@::class)\n",
      "name", EnumFullName(en, is_descriptor),
      "class_name", en->name());
  Indent(printer);

  for (int i = 0; i < en->value_count(); i++) {
    const EnumValueDescriptor* value = en->value(i);
    printer->Print(
        "->value(\"@name@\", @number@)\n",
        "name", value->name(),
        "number", IntToString(value->number()));
  }
  printer->Print("->finalizeToPool();\n\n");
  Outdent(printer);
}

void GenerateMessageToPool(const string& name_prefix,
                           const google::protobuf::Descriptor* message,
                           bool is_descriptor,
                           google::protobuf::io::Printer* printer) {
  // Don't generate MapEntry messages -- we use the PHP extension's native
  // support for map fields instead.
  if (message->options().map_entry()) {
    return;
  }
  string class_name = name_prefix.empty()?
      message->name() : name_prefix + "_" + message->name();

  printer->Print(
      "$pool->addMessage('@message@', @class_name@::class)\n",
      "message", MessageFullName(message, is_descriptor),
      "class_name", class_name);

  Indent(printer);

  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    if (field->is_map()) {
      const FieldDescriptor* key =
          field->message_type()->FindFieldByName("key");
      const FieldDescriptor* val =
          field->message_type()->FindFieldByName("value");
      printer->Print(
          "->map('@field@', GPBType::@key@, "
          "GPBType::@value@, @number@@other@)\n",
          "field", field->name(),
          "key", ToUpper(key->type_name()),
          "value", ToUpper(val->type_name()),
          "number", SimpleItoa(field->number()),
          "other", EnumOrMessageSuffix(val, is_descriptor));
    } else if (!field->containing_oneof()) {
      printer->Print(
          "->@label@('@field@', GPBType::@type@, @number@@other@)\n",
          "field", field->name(),
          "label", LabelForField(field),
          "type", ToUpper(field->type_name()),
          "number", SimpleItoa(field->number()),
          "other", EnumOrMessageSuffix(field, is_descriptor));
    }
  }

  // oneofs.
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    printer->Print("->oneof(@name@)\n",
                   "name", oneof->name());
    Indent(printer);
    for (int index = 0; index < oneof->field_count(); index++) {
      const FieldDescriptor* field = oneof->field(index);
      printer->Print(
          "->value('@field@', GPBType::@type@, @number@@other@)\n",
          "field", field->name(),
          "type", ToUpper(field->type_name()),
          "number", SimpleItoa(field->number()),
          "other", EnumOrMessageSuffix(field, is_descriptor));
    }
    printer->Print("->finish()\n");
    Outdent(printer);
  }

  printer->Print(
      "->finalizeToPool();\n");

  Outdent(printer);

  printer->Print(
      "\n");

  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateMessageToPool(class_name, message->nested_type(i), is_descriptor,
                          printer);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnumToPool(message->enum_type(i), is_descriptor, printer);
  }
}

void GenerateAddFileToPool(const google::protobuf::FileDescriptor* file,
                           bool is_descriptor,
                           google::protobuf::io::Printer* printer) {
  if (is_descriptor) {
    printer->Print("$pool = DescriptorPool::getGeneratedPool();\n\n");

    for (int i = 0; i < file->message_type_count(); i++) {
      GenerateMessageToPool("", file->message_type(i), is_descriptor, printer);
    }
    for (int i = 0; i < file->enum_type_count(); i++) {
      GenerateEnumToPool(file->enum_type(i), is_descriptor, printer);
    }

    printer->Print(
        "$pool->finish();\n");
  } else {
    // Add messages and enums to descriptor pool.
    printer->Print("$pool = DescriptorPool::getGeneratedPool();\n\n");

    FileDescriptorSet files;
    FileDescriptorProto* file_proto = files.add_file();
    file->CopyTo(file_proto);
    string files_data;
    files.SerializeToString(&files_data);

    printer->Print("$pool->internalAddGeneratedFile(hex2bin(\n");
    Indent(printer);

    // Only write 30 bytes per line.
    static const int kBytesPerLine = 30;
    for (int i = 0; i < files_data.size(); i += kBytesPerLine) {
      printer->Print(
          "\"@data@\"@dot@\n",
          "data", BinaryToHex(files_data.substr(i, kBytesPerLine)),
          "dot", i + kBytesPerLine < files_data.size() ? " ." : "");
    }

    Outdent(printer);
    printer->Print(
        "));\n\n");
  }

}

void GenerateEnum(const google::protobuf::EnumDescriptor* en,
                  google::protobuf::io::Printer* printer) {
  printer->Print(
      "class @name@\n"
      "{\n",
      "name", EnumClassName(en));
  Indent(printer);

  for (int i = 0; i < en->value_count(); i++) {
    const EnumValueDescriptor* value = en->value(i);
    printer->Print("const @name@ = @number@;\n",
                   "name", value->name(),
                   "number", IntToString(value->number()));
  }
  Outdent(printer);
  printer->Print("}\n\n");
}

void GenerateUseDeclaration(bool is_descriptor,
                            google::protobuf::io::Printer* printer) {
  if (!is_descriptor) {
    printer->Print(
        "use Google\\Protobuf\\Internal\\DescriptorPool;\n"
        "use Google\\Protobuf\\Internal\\GPBType;\n"
        "use Google\\Protobuf\\Internal\\RepeatedField;\n"
        "use Google\\Protobuf\\Internal\\GPBUtil;\n\n");
  } else {
    printer->Print(
        "use Google\\Protobuf\\Internal\\DescriptorPool;\n"
        "use Google\\Protobuf\\Internal\\GPBType;\n"
        "use Google\\Protobuf\\Internal\\GPBWire;\n"
        "use Google\\Protobuf\\Internal\\RepeatedField;\n"
        "use Google\\Protobuf\\Internal\\InputStream;\n\n"
        "use Google\\Protobuf\\Internal\\GPBUtil;\n\n");
  }
}

void GenerateFile(const google::protobuf::FileDescriptor* file,
                  bool is_descriptor, google::protobuf::io::Printer* printer) {
  printer->Print(
    "<?php\n"
    "# Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "# source: @filename@\n"
    "\n",
    "filename", file->name());
  if (!file->package().empty()) {
    printer->Print("namespace @name@;\n\n",
                   "name", PhpName(file->package(), is_descriptor));
  }

  for (int i = 0; i < file->dependency_count(); i++) {
    const std::string& name = file->dependency(i)->name();
    printer->Print("require_once('@name@');\n", "name",
                   GeneratedFileName(name, is_descriptor));
  }

  GenerateUseDeclaration(is_descriptor, printer);

  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateMessage("", file->message_type(i), is_descriptor, printer);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    GenerateEnum(file->enum_type(i), printer);
  }

  GenerateAddFileToPool(file, is_descriptor, printer);
}

bool Generator::Generate(
    const FileDescriptor* file,
    const string& parameter,
    GeneratorContext* generator_context,
    string* error) const {
  bool is_descriptor = parameter == "internal";

  if (is_descriptor && file->name() != kDescriptorFile) {
    *error =
        "Can only generate PHP code for google/protobuf/descriptor.proto.\n";
    return false;
  }

  if (!is_descriptor && file->syntax() != FileDescriptor::SYNTAX_PROTO3) {
    *error =
        "Can only generate PHP code for proto3 .proto files.\n"
        "Please add 'syntax = \"proto3\";' to the top of your .proto file.\n";
    return false;
  }

  std::string filename = GeneratedFileName(file->name(), is_descriptor);
  scoped_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '@');

  GenerateFile(file, is_descriptor, &printer);

  return true;
}

}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
