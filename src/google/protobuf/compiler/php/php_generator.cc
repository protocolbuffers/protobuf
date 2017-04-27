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
const std::string kEmptyFile = "google/protobuf/empty.proto";
const std::string kEmptyMetadataFile = "GPBMetadata/Google/Protobuf/GPBEmpty.php";
const std::string kDescriptorMetadataFile =
    "GPBMetadata/Google/Protobuf/Internal/Descriptor.php";
const std::string kDescriptorDirName = "Google/Protobuf/Internal";
const std::string kDescriptorPackageName = "Google\\Protobuf\\Internal";
const char* const kReservedNames[] = {"Empty", "ECHO"};
const int kReservedNamesSize = 2;

namespace google {
namespace protobuf {
namespace compiler {
namespace php {

// Forward decls.
std::string PhpName(const std::string& full_name, bool is_descriptor);
std::string DefaultForField(FieldDescriptor* field);
std::string IntToString(int32 value);
std::string FilenameToClassname(const string& filename);
std::string GeneratedMetadataFileName(const std::string& proto_file,
                                      bool is_descriptor);
std::string LabelForField(FieldDescriptor* field);
std::string TypeName(FieldDescriptor* field);
std::string UnderscoresToCamelCase(const string& name, bool cap_first_letter);
std::string EscapeDollor(const string& to_escape);
std::string BinaryToHex(const string& binary);
void Indent(io::Printer* printer);
void Outdent(io::Printer* printer);
void GenerateMessageDocComment(io::Printer* printer, const Descriptor* message);
void GenerateFieldDocComment(io::Printer* printer,
                             const FieldDescriptor* field);
void GenerateEnumDocComment(io::Printer* printer, const EnumDescriptor* enum_);
void GenerateEnumValueDocComment(io::Printer* printer,
                                 const EnumValueDescriptor* value);

std::string RenameEmpty(const std::string& name) {
  if (name == "Empty") {
    return "GPBEmpty";
  } else {
    return name;
  }
}

std::string MessageFullName(const Descriptor* message, bool is_descriptor) {
  if (is_descriptor) {
    return StringReplace(message->full_name(),
                         "google.protobuf",
                         "google.protobuf.internal", false);
  } else {
    return message->full_name();
  }
}

std::string EnumFullName(const EnumDescriptor* envm, bool is_descriptor) {
  if (is_descriptor) {
    return StringReplace(envm->full_name(),
                         "google.protobuf",
                         "google.protobuf.internal", false);
  } else {
    return envm->full_name();
  }
}

template <typename DescriptorType>
std::string ClassNamePrefix(const string& classname,
                            const DescriptorType* desc) {
  const string& prefix = (desc->file()->options()).php_class_prefix();
  if (prefix != "") {
    return prefix;
  }

  bool is_reserved = false;

  for (int i = 0; i < kReservedNamesSize; i++) {
    if (classname == kReservedNames[i]) {
      is_reserved = true;
      break;
    }
  }

  if (is_reserved) {
    if (desc->file()->package() == "google.protobuf") {
      return "GPB";
    } else {
      return "PB";
    }
  }

  return "";
}


template <typename DescriptorType>
std::string FullClassName(const DescriptorType* desc, bool is_descriptor) {
  string classname = desc->name();
  const Descriptor* containing = desc->containing_type();
  while (containing != NULL) {
    classname = containing->name() + '_' + classname;
    containing = containing->containing_type();
  }
  classname = ClassNamePrefix(classname, desc) + classname;

  if (desc->file()->package() == "") {
    return classname;
  } else {
    return PhpName(desc->file()->package(), is_descriptor) + '\\' +
           classname;
  }
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

std::string DefaultForField(const FieldDescriptor* field) {
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

std::string GeneratedMetadataFileName(const std::string& proto_file,
                                      bool is_descriptor) {
  int start_index = 0;
  int first_index = proto_file.find_first_of("/", start_index);
  std::string result = "GPBMetadata/";

  if (proto_file == kEmptyFile) {
    return kEmptyMetadataFile;
  }
  if (is_descriptor) {
    return kDescriptorMetadataFile;
  }

  // Append directory name.
  std::string file_no_suffix;
  int lastindex = proto_file.find_last_of(".");
  if (proto_file == kEmptyFile) {
    return kEmptyMetadataFile;
  } else {
    file_no_suffix = proto_file.substr(0, lastindex);
  }

  while (first_index != string::npos) {
    result += UnderscoresToCamelCase(
        file_no_suffix.substr(start_index, first_index - start_index), true);
    result += "/";
    start_index = first_index + 1;
    first_index = file_no_suffix.find_first_of("/", start_index);
  }

  // Append file name.
  result += RenameEmpty(UnderscoresToCamelCase(
      file_no_suffix.substr(start_index, first_index - start_index), true));

  return result += ".php";
}

std::string GeneratedMessageFileName(const Descriptor* message,
                                     bool is_descriptor) {
  std::string result = FullClassName(message, is_descriptor);
  for (int i = 0; i < result.size(); i++) {
    if (result[i] == '\\') {
      result[i] = '/';
    }
  }
  return result + ".php";
}

std::string GeneratedEnumFileName(const EnumDescriptor* en,
                                  bool is_descriptor) {
  std::string result = FullClassName(en, is_descriptor);
  for (int i = 0; i < result.size(); i++) {
    if (result[i] == '\\') {
      result[i] = '/';
    }
  }
  return result + ".php";
}

std::string IntToString(int32 value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

std::string LabelForField(const FieldDescriptor* field) {
  switch (field->label()) {
    case FieldDescriptor::LABEL_OPTIONAL: return "optional";
    case FieldDescriptor::LABEL_REQUIRED: return "required";
    case FieldDescriptor::LABEL_REPEATED: return "repeated";
    default: assert(false); return "";
  }
}

std::string TypeName(const FieldDescriptor* field) {
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
    const FieldDescriptor* field, bool is_descriptor) {
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

void Indent(io::Printer* printer) {
  printer->Indent();
  printer->Indent();
}
void Outdent(io::Printer* printer) {
  printer->Outdent();
  printer->Outdent();
}

void GenerateField(const FieldDescriptor* field, io::Printer* printer,
                   bool is_descriptor) {
  if (field->is_repeated()) {
    GenerateFieldDocComment(printer, field);
    printer->Print(
        "private $^name^;\n",
        "name", field->name());
  } else if (field->containing_oneof()) {
    // Oneof fields are handled by GenerateOneofField.
    return;
  } else {
    GenerateFieldDocComment(printer, field);
    printer->Print(
        "private $^name^ = ^default^;\n",
        "name", field->name(),
        "default", DefaultForField(field));
  }

  if (is_descriptor) {
    printer->Print(
        "private $has_^name^ = false;\n",
        "name", field->name());
  }
}

void GenerateOneofField(const OneofDescriptor* oneof, io::Printer* printer) {
  // Oneof property needs to be protected in order to be accessed by parent
  // class in implementation.
  printer->Print(
      "protected $^name^;\n",
      "name", oneof->name());
}

void GenerateFieldAccessor(const FieldDescriptor* field, bool is_descriptor,
                           io::Printer* printer) {
  const OneofDescriptor* oneof = field->containing_oneof();

  // Generate getter.
  if (oneof != NULL) {
    GenerateFieldDocComment(printer, field);
    printer->Print(
        "public function get^camel_name^()\n"
        "{\n"
        "    return $this->readOneof(^number^);\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true),
        "number", IntToString(field->number()));
  } else {
    GenerateFieldDocComment(printer, field);
    printer->Print(
        "public function get^camel_name^()\n"
        "{\n"
        "    return $this->^name^;\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true), "name",
        field->name());
  }

  // Generate setter.
  GenerateFieldDocComment(printer, field);
  printer->Print(
      "public function set^camel_name^(^var^)\n"
      "{\n",
      "camel_name", UnderscoresToCamelCase(field->name(), true),
      "var", (field->is_repeated() ||
              field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) ?
             "&$var": "$var");

  Indent(printer);

  // Type check.
  if (field->is_map()) {
    const Descriptor* map_entry = field->message_type();
    const FieldDescriptor* key = map_entry->FindFieldByName("key");
    const FieldDescriptor* value = map_entry->FindFieldByName("value");
    printer->Print(
        "$arr = GPBUtil::checkMapField($var, "
        "\\Google\\Protobuf\\Internal\\GPBType::^key_type^, "
        "\\Google\\Protobuf\\Internal\\GPBType::^value_type^",
        "key_type", ToUpper(key->type_name()),
        "value_type", ToUpper(value->type_name()));
    if (value->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      printer->Print(
          ", \\^class_name^);\n",
          "class_name",
          FullClassName(value->message_type(), is_descriptor) + "::class");
    } else if (value->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      printer->Print(
          ", \\^class_name^);\n",
          "class_name",
          FullClassName(value->enum_type(), is_descriptor) + "::class");
    } else {
      printer->Print(");\n");
    }
  } else if (field->is_repeated()) {
    printer->Print(
        "$arr = GPBUtil::checkRepeatedField($var, "
        "\\Google\\Protobuf\\Internal\\GPBType::^type^",
        "type", ToUpper(field->type_name()));
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      printer->Print(
          ", \\^class_name^);\n",
          "class_name",
          FullClassName(field->message_type(), is_descriptor) + "::class");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      printer->Print(
          ", \\^class_name^);\n",
          "class_name",
          FullClassName(field->enum_type(), is_descriptor) + "::class");
    } else {
      printer->Print(");\n");
    }
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    printer->Print(
        "GPBUtil::checkMessage($var, \\^class_name^::class);\n",
        "class_name", FullClassName(field->message_type(), is_descriptor));
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
    printer->Print(
        "GPBUtil::checkEnum($var, \\^class_name^::class);\n",
        "class_name", FullClassName(field->enum_type(), is_descriptor));
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_STRING) {
    printer->Print(
        "GPBUtil::checkString($var, ^utf8^);\n",
        "utf8",
        field->type() == FieldDescriptor::TYPE_STRING ? "True": "False");
  } else {
    printer->Print(
        "GPBUtil::check^type^($var);\n",
        "type", UnderscoresToCamelCase(field->cpp_type_name(), true));
  }

  if (oneof != NULL) {
    printer->Print(
        "$this->writeOneof(^number^, $var);\n",
        "number", IntToString(field->number()));
  } else if (field->is_repeated()) {
    printer->Print(
        "$this->^name^ = $arr;\n",
        "name", field->name());
  } else {
    printer->Print(
        "$this->^name^ = $var;\n",
        "name", field->name());
  }

  // Set has bit for proto2 only.
  if (is_descriptor) {
    printer->Print(
        "$this->has_^field_name^ = true;\n",
        "field_name", field->name());
  }

  Outdent(printer);

  printer->Print(
      "}\n\n");

  // Generate has method for proto2 only.
  if (is_descriptor) {
    printer->Print(
        "public function has^camel_name^()\n"
        "{\n"
        "    return $this->has_^field_name^;\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true),
        "field_name", field->name());
  }
}

void GenerateEnumToPool(const EnumDescriptor* en, io::Printer* printer) {
  printer->Print(
      "$pool->addEnum('^name^', "
      "\\Google\\Protobuf\\Internal\\^class_name^::class)\n",
      "name", EnumFullName(en, true),
      "class_name", en->name());
  Indent(printer);

  for (int i = 0; i < en->value_count(); i++) {
    const EnumValueDescriptor* value = en->value(i);
    printer->Print(
        "->value(\"^name^\", ^number^)\n",
        "name", ClassNamePrefix(value->name(), en) + value->name(),
        "number", IntToString(value->number()));
  }
  printer->Print("->finalizeToPool();\n\n");
  Outdent(printer);
}

void GenerateMessageToPool(const string& name_prefix, const Descriptor* message,
                           io::Printer* printer) {
  // Don't generate MapEntry messages -- we use the PHP extension's native
  // support for map fields instead.
  if (message->options().map_entry()) {
    return;
  }
  string class_name = name_prefix.empty()?
      message->name() : name_prefix + "_" + message->name();

  printer->Print(
      "$pool->addMessage('^message^', "
      "\\Google\\Protobuf\\Internal\\^class_name^::class)\n",
      "message", MessageFullName(message, true),
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
          "->map('^field^', \\Google\\Protobuf\\Internal\\GPBType::^key^, "
          "\\Google\\Protobuf\\Internal\\GPBType::^value^, ^number^^other^)\n",
          "field", field->name(),
          "key", ToUpper(key->type_name()),
          "value", ToUpper(val->type_name()),
          "number", SimpleItoa(field->number()),
          "other", EnumOrMessageSuffix(val, true));
    } else if (!field->containing_oneof()) {
      printer->Print(
          "->^label^('^field^', "
          "\\Google\\Protobuf\\Internal\\GPBType::^type^, ^number^^other^)\n",
          "field", field->name(),
          "label", LabelForField(field),
          "type", ToUpper(field->type_name()),
          "number", SimpleItoa(field->number()),
          "other", EnumOrMessageSuffix(field, true));
    }
  }

  // oneofs.
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    printer->Print("->oneof(^name^)\n",
                   "name", oneof->name());
    Indent(printer);
    for (int index = 0; index < oneof->field_count(); index++) {
      const FieldDescriptor* field = oneof->field(index);
      printer->Print(
          "->value('^field^', "
          "\\Google\\Protobuf\\Internal\\GPBType::^type^, ^number^^other^)\n",
          "field", field->name(),
          "type", ToUpper(field->type_name()),
          "number", SimpleItoa(field->number()),
          "other", EnumOrMessageSuffix(field, true));
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
    GenerateMessageToPool(class_name, message->nested_type(i), printer);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnumToPool(message->enum_type(i), printer);
  }
}

void GenerateAddFileToPool(const FileDescriptor* file, bool is_descriptor,
                           io::Printer* printer) {
    printer->Print(
        "public static $is_initialized = false;\n\n"
        "public static function initOnce() {\n");
    Indent(printer);

    printer->Print(
        "$pool = \\Google\\Protobuf\\Internal\\"
        "DescriptorPool::getGeneratedPool();\n\n"
        "if (static::$is_initialized == true) {\n"
        "  return;\n"
        "}\n");

  if (is_descriptor) {
    for (int i = 0; i < file->message_type_count(); i++) {
      GenerateMessageToPool("", file->message_type(i), printer);
    }
    for (int i = 0; i < file->enum_type_count(); i++) {
      GenerateEnumToPool(file->enum_type(i), printer);
    }

    printer->Print(
        "$pool->finish();\n");
  } else {
    for (int i = 0; i < file->dependency_count(); i++) {
      const std::string& name = file->dependency(i)->name();
      // Currently, descriptor.proto is not ready for external usage. Skip to
      // import it for now, so that its dependencies can still work as long as
      // they don't use protos defined in descriptor.proto.
      if (name == kDescriptorFile) {
        continue;
      }
      std::string dependency_filename =
          GeneratedMetadataFileName(name, is_descriptor);
      printer->Print(
          "\\^name^::initOnce();\n",
          "name", FilenameToClassname(dependency_filename));
    }

    // Add messages and enums to descriptor pool.
    FileDescriptorSet files;
    FileDescriptorProto* file_proto = files.add_file();
    file->CopyTo(file_proto);

    // Filter out descriptor.proto as it cannot be depended on for now.
    RepeatedPtrField<string>* dependency = file_proto->mutable_dependency();
    for (RepeatedPtrField<string>::iterator it = dependency->begin();
         it != dependency->end(); ++it) {
      if (*it != kDescriptorFile) {
        dependency->erase(it);
        break;
      }
    }

    // Filter out all extensions, since we do not support extension yet.
    file_proto->clear_extension();
    RepeatedPtrField<DescriptorProto>* message_type =
        file_proto->mutable_message_type();
    for (RepeatedPtrField<DescriptorProto>::iterator it = message_type->begin();
         it != message_type->end(); ++it) {
      it->clear_extension();
    }

    string files_data;
    files.SerializeToString(&files_data);

    printer->Print("$pool->internalAddGeneratedFile(hex2bin(\n");
    Indent(printer);

    // Only write 30 bytes per line.
    static const int kBytesPerLine = 30;
    for (int i = 0; i < files_data.size(); i += kBytesPerLine) {
      printer->Print(
          "\"^data^\"^dot^\n",
          "data", BinaryToHex(files_data.substr(i, kBytesPerLine)),
          "dot", i + kBytesPerLine < files_data.size() ? " ." : "");
    }

    Outdent(printer);
    printer->Print(
        "));\n\n");
  }
  printer->Print(
      "static::$is_initialized = true;\n");
  Outdent(printer);
  printer->Print("}\n");
}

void GenerateUseDeclaration(bool is_descriptor, io::Printer* printer) {
  if (!is_descriptor) {
    printer->Print(
        "use Google\\Protobuf\\Internal\\GPBType;\n"
        "use Google\\Protobuf\\Internal\\RepeatedField;\n"
        "use Google\\Protobuf\\Internal\\GPBUtil;\n\n");
  } else {
    printer->Print(
        "use Google\\Protobuf\\Internal\\GPBType;\n"
        "use Google\\Protobuf\\Internal\\GPBWire;\n"
        "use Google\\Protobuf\\Internal\\RepeatedField;\n"
        "use Google\\Protobuf\\Internal\\InputStream;\n\n"
        "use Google\\Protobuf\\Internal\\GPBUtil;\n\n");
  }
}

void GenerateHead(const FileDescriptor* file, io::Printer* printer) {
  printer->Print(
    "<?php\n"
    "# Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "# source: ^filename^\n"
    "\n",
    "filename", file->name());
}

std::string FilenameToClassname(const string& filename) {
  int lastindex = filename.find_last_of(".");
  std::string result = filename.substr(0, lastindex);
  for (int i = 0; i < result.size(); i++) {
    if (result[i] == '/') {
      result[i] = '\\';
    }
  }
  return result;
}

void GenerateMetadataFile(const FileDescriptor* file,
                          bool is_descriptor,
                          GeneratorContext* generator_context) {
  std::string filename = GeneratedMetadataFileName(file->name(), is_descriptor);
  scoped_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '^');

  GenerateHead(file, &printer);

  std::string fullname = FilenameToClassname(filename);
  int lastindex = fullname.find_last_of("\\");

  printer.Print(
      "namespace ^name^;\n\n",
      "name", fullname.substr(0, lastindex));

  if (lastindex != string::npos) {
    printer.Print(
        "class ^name^\n"
        "{\n",
        "name", fullname.substr(lastindex + 1));
  } else {
    printer.Print(
        "class ^name^\n"
        "{\n",
        "name", fullname);
  }
  Indent(&printer);

  GenerateAddFileToPool(file, is_descriptor, &printer);

  Outdent(&printer);
  printer.Print("}\n\n");
}

void GenerateEnumFile(const FileDescriptor* file, const EnumDescriptor* en,
                      bool is_descriptor, GeneratorContext* generator_context) {
  std::string filename = GeneratedEnumFileName(en, is_descriptor);
  scoped_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '^');

  GenerateHead(file, &printer);

  std::string fullname = FilenameToClassname(filename);
  int lastindex = fullname.find_last_of("\\");

  if (!file->package().empty()) {
    printer.Print(
        "namespace ^name^;\n\n",
        "name", fullname.substr(0, lastindex));
  }

  GenerateEnumDocComment(&printer, en);

  if (lastindex != string::npos) {
    printer.Print(
        "class ^name^\n"
        "{\n",
        "name", fullname.substr(lastindex + 1));
  } else {
    printer.Print(
        "class ^name^\n"
        "{\n",
        "name", fullname);
  }
  Indent(&printer);

  for (int i = 0; i < en->value_count(); i++) {
    const EnumValueDescriptor* value = en->value(i);
    GenerateEnumValueDocComment(&printer, value);
    printer.Print("const ^name^ = ^number^;\n",
                  "name", ClassNamePrefix(value->name(), en) + value->name(),
                  "number", IntToString(value->number()));
  }

  Outdent(&printer);
  printer.Print("}\n\n");
}

void GenerateMessageFile(const FileDescriptor* file, const Descriptor* message,
                         bool is_descriptor,
                         GeneratorContext* generator_context) {
  // Don't generate MapEntry messages -- we use the PHP extension's native
  // support for map fields instead.
  if (message->options().map_entry()) {
    return;
  }

  std::string filename = GeneratedMessageFileName(message, is_descriptor);
  scoped_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '^');

  GenerateHead(file, &printer);

  std::string fullname = FilenameToClassname(filename);
  int lastindex = fullname.find_last_of("\\");

  if (!file->package().empty()) {
    printer.Print(
        "namespace ^name^;\n\n",
        "name", fullname.substr(0, lastindex));
  }

  GenerateUseDeclaration(is_descriptor, &printer);

  GenerateMessageDocComment(&printer, message);
  if (lastindex != string::npos) {
    printer.Print(
        "class ^name^ extends \\Google\\Protobuf\\Internal\\Message\n"
        "{\n",
        "name", fullname.substr(lastindex + 1));
  } else {
    printer.Print(
        "class ^name^ extends \\Google\\Protobuf\\Internal\\Message\n"
        "{\n",
        "name", fullname);
  }
  Indent(&printer);

  // Field and oneof definitions.
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    GenerateField(field, &printer, is_descriptor);
  }
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    GenerateOneofField(oneof, &printer);
  }
  printer.Print("\n");

  printer.Print(
      "public function __construct() {\n");
  Indent(&printer);

  std::string metadata_filename =
      GeneratedMetadataFileName(file->name(), is_descriptor);
  std::string metadata_fullname = FilenameToClassname(metadata_filename);
  printer.Print(
      "\\^fullname^::initOnce();\n"
      "parent::__construct();\n",
      "fullname", metadata_fullname);

  Outdent(&printer);
  printer.Print("}\n\n");

  // Field and oneof accessors.
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    GenerateFieldAccessor(field, is_descriptor, &printer);
  }
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    printer.Print(
      "public function get^camel_name^()\n"
      "{\n"
      "    return $this->whichOneof(\"^name^\");\n"
      "}\n\n",
      "camel_name", UnderscoresToCamelCase(oneof->name(), true), "name",
      oneof->name());
  }

  Outdent(&printer);
  printer.Print("}\n\n");

  // Nested messages and enums.
  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateMessageFile(file, message->nested_type(i), is_descriptor,
                        generator_context);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnumFile(file, message->enum_type(i), is_descriptor,
                     generator_context);
  }
}

void GenerateFile(const FileDescriptor* file, bool is_descriptor,
                  GeneratorContext* generator_context) {
  GenerateMetadataFile(file, is_descriptor, generator_context);
  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateMessageFile(file, file->message_type(i), is_descriptor,
                        generator_context);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    GenerateEnumFile(file, file->enum_type(i), is_descriptor,
                     generator_context);
  }
}

static string EscapePhpdoc(const string& input) {
  string result;
  result.reserve(input.size() * 2);

  char prev = '*';

  for (string::size_type i = 0; i < input.size(); i++) {
    char c = input[i];
    switch (c) {
      case '*':
        // Avoid "/*".
        if (prev == '/') {
          result.append("&#42;");
        } else {
          result.push_back(c);
        }
        break;
      case '/':
        // Avoid "*/".
        if (prev == '*') {
          result.append("&#47;");
        } else {
          result.push_back(c);
        }
        break;
      case '@':
        // '@' starts phpdoc tags including the @deprecated tag, which will
        // cause a compile-time error if inserted before a declaration that
        // does not have a corresponding @Deprecated annotation.
        result.append("&#64;");
        break;
      case '<':
        // Avoid interpretation as HTML.
        result.append("&lt;");
        break;
      case '>':
        // Avoid interpretation as HTML.
        result.append("&gt;");
        break;
      case '&':
        // Avoid interpretation as HTML.
        result.append("&amp;");
        break;
      case '\\':
        // Java interprets Unicode escape sequences anywhere!
        result.append("&#92;");
        break;
      default:
        result.push_back(c);
        break;
    }

    prev = c;
  }

  return result;
}

static void GenerateDocCommentBodyForLocation(
    io::Printer* printer, const SourceLocation& location) {
  string comments = location.leading_comments.empty() ?
      location.trailing_comments : location.leading_comments;
  if (!comments.empty()) {
    // TODO(teboring):  Ideally we should parse the comment text as Markdown and
    //   write it back as HTML, but this requires a Markdown parser.  For now
    //   we just use <pre> to get fixed-width text formatting.

    // If the comment itself contains block comment start or end markers,
    // HTML-escape them so that they don't accidentally close the doc comment.
    comments = EscapePhpdoc(comments);

    vector<string> lines = Split(comments, "\n");
    while (!lines.empty() && lines.back().empty()) {
      lines.pop_back();
    }

    printer->Print(" * <pre>\n");
    for (int i = 0; i < lines.size(); i++) {
      // Most lines should start with a space.  Watch out for lines that start
      // with a /, since putting that right after the leading asterisk will
      // close the comment.
      if (!lines[i].empty() && lines[i][0] == '/') {
        printer->Print(" * ^line^\n", "line", lines[i]);
      } else {
        printer->Print(" *^line^\n", "line", lines[i]);
      }
    }
    printer->Print(
        " * </pre>\n"
        " *\n");
  }
}

template <typename DescriptorType>
static void GenerateDocCommentBody(
    io::Printer* printer, const DescriptorType* descriptor) {
  SourceLocation location;
  if (descriptor->GetSourceLocation(&location)) {
    GenerateDocCommentBodyForLocation(printer, location);
  }
}

static string FirstLineOf(const string& value) {
  string result = value;

  string::size_type pos = result.find_first_of('\n');
  if (pos != string::npos) {
    result.erase(pos);
  }

  return result;
}

void GenerateMessageDocComment(io::Printer* printer,
                               const Descriptor* message) {
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, message);
  printer->Print(
    " * Protobuf type <code>^fullname^</code>\n"
    " */\n",
    "fullname", EscapePhpdoc(message->full_name()));
}

void GenerateFieldDocComment(io::Printer* printer,
                             const FieldDescriptor* field) {
  // In theory we should have slightly different comments for setters, getters,
  // etc., but in practice everyone already knows the difference between these
  // so it's redundant information.

  // We start the comment with the main body based on the comments from the
  // .proto file (if present). We then end with the field declaration, e.g.:
  //   optional string foo = 5;
  // If the field is a group, the debug string might end with {.
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, field);
  printer->Print(
    " * <code>^def^</code>\n",
    "def", EscapePhpdoc(FirstLineOf(field->DebugString())));
  printer->Print(" */\n");
}

void GenerateEnumDocComment(io::Printer* printer, const EnumDescriptor* enum_) {
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, enum_);
  printer->Print(
    " * Protobuf enum <code>^fullname^</code>\n"
    " */\n",
    "fullname", EscapePhpdoc(enum_->full_name()));
}

void GenerateEnumValueDocComment(io::Printer* printer,
                                 const EnumValueDescriptor* value) {
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, value);
  printer->Print(
    " * <code>^def^</code>\n"
    " */\n",
    "def", EscapePhpdoc(FirstLineOf(value->DebugString())));
}

bool Generator::Generate(const FileDescriptor* file, const string& parameter,
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

  GenerateFile(file, is_descriptor, generator_context);

  return true;
}

}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
