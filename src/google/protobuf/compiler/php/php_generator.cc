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

const std::string kDescriptorFile = "google/protobuf/descriptor.proto";
const std::string kEmptyFile = "google/protobuf/empty.proto";
const std::string kEmptyMetadataFile = "GPBMetadata/Google/Protobuf/GPBEmpty.php";
const std::string kDescriptorMetadataFile =
    "GPBMetadata/Google/Protobuf/Internal/Descriptor.php";
const std::string kDescriptorDirName = "Google/Protobuf/Internal";
const std::string kDescriptorPackageName = "Google\\Protobuf\\Internal";
const char* const kReservedNames[] = {
    "abstract",   "and",        "array",        "as",           "break",
    "callable",   "case",       "catch",        "class",        "clone",
    "const",      "continue",   "declare",      "default",      "die",
    "do",         "echo",       "else",         "elseif",       "empty",
    "enddeclare", "endfor",     "endforeach",   "endif",        "endswitch",
    "endwhile",   "eval",       "exit",         "extends",      "final",
    "for",        "foreach",    "function",     "global",       "goto",
    "if",         "implements", "include",      "include_once", "instanceof",
    "insteadof",  "interface",  "isset",        "list",         "namespace",
    "new",        "or",         "print",        "private",      "protected",
    "public",     "require",    "require_once", "return",       "static",
    "switch",     "throw",      "trait",        "try",          "unset",
    "use",        "var",        "while",        "xor",          "int",
    "float",      "bool",       "string",       "true",         "false",
    "null",       "void",       "iterable"};
const char* const kValidConstantNames[] = {
    "int",   "float", "bool", "string",   "true",
    "false", "null",  "void", "iterable",
};
const int kReservedNamesSize = 73;
const int kValidConstantNamesSize = 9;
const int kFieldSetter = 1;
const int kFieldGetter = 2;
const int kFieldProperty = 3;

namespace google {
namespace protobuf {
namespace compiler {
namespace php {

struct Options {
  bool is_descriptor = false;
  bool aggregate_metadata = false;
  bool gen_c_wkt = false;
  std::set<string> aggregate_metadata_prefixes;
};

namespace {

// Forward decls.
std::string PhpName(const std::string& full_name, const Options& options);
std::string IntToString(int32 value);
std::string FilenameToClassname(const std::string& filename);
std::string GeneratedMetadataFileName(const FileDescriptor* file,
                                      const Options& options);
std::string UnderscoresToCamelCase(const std::string& name,
                                   bool cap_first_letter);
std::string BinaryToHex(const std::string& binary);
void Indent(io::Printer* printer);
void Outdent(io::Printer* printer);
void GenerateAddFilesToPool(const FileDescriptor* file, const Options& options,
                            io::Printer* printer);
void GenerateMessageDocComment(io::Printer* printer, const Descriptor* message,
                               const Options& options);
void GenerateMessageConstructorDocComment(io::Printer* printer,
                                          const Descriptor* message,
                                          const Options& options);
void GenerateFieldDocComment(io::Printer* printer, const FieldDescriptor* field,
                             const Options& options, int function_type);
void GenerateWrapperFieldGetterDocComment(io::Printer* printer,
                                          const FieldDescriptor* field);
void GenerateWrapperFieldSetterDocComment(io::Printer* printer,
                                          const FieldDescriptor* field);
void GenerateEnumDocComment(io::Printer* printer, const EnumDescriptor* enum_,
                            const Options& options);
void GenerateEnumValueDocComment(io::Printer* printer,
                                 const EnumValueDescriptor* value);
void GenerateServiceDocComment(io::Printer* printer,
                               const ServiceDescriptor* service);
void GenerateServiceMethodDocComment(io::Printer* printer,
                              const MethodDescriptor* method);

std::string ReservedNamePrefix(const std::string& classname,
                                const FileDescriptor* file) {
  bool is_reserved = false;

  std::string lower = classname;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  for (int i = 0; i < kReservedNamesSize; i++) {
    if (lower == kReservedNames[i]) {
      is_reserved = true;
      break;
    }
  }

  if (is_reserved) {
    if (file->package() == "google.protobuf") {
      return "GPB";
    } else {
      return "PB";
    }
  }

  return "";
}

template <typename DescriptorType>
std::string DescriptorFullName(const DescriptorType* desc, bool is_internal) {
  if (is_internal) {
    return StringReplace(desc->full_name(),
                         "google.protobuf",
                         "google.protobuf.internal", false);
  } else {
    return desc->full_name();
  }
}

template <typename DescriptorType>
std::string ClassNamePrefix(const std::string& classname,
                            const DescriptorType* desc) {
  const std::string& prefix = (desc->file()->options()).php_class_prefix();
  if (!prefix.empty()) {
    return prefix;
  }

  return ReservedNamePrefix(classname, desc->file());
}

template <typename DescriptorType>
std::string GeneratedClassNameImpl(const DescriptorType* desc) {
  std::string classname = ClassNamePrefix(desc->name(), desc) + desc->name();
  const Descriptor* containing = desc->containing_type();
  while (containing != NULL) {
    classname = ClassNamePrefix(containing->name(), desc) + containing->name()
       + '\\' + classname;
    containing = containing->containing_type();
  }
  return classname;
}

std::string GeneratedClassNameImpl(const ServiceDescriptor* desc) {
  std::string classname = desc->name();
  return ClassNamePrefix(classname, desc) + classname;
}

template <typename DescriptorType>
std::string LegacyGeneratedClassName(const DescriptorType* desc) {
  std::string classname = desc->name();
  const Descriptor* containing = desc->containing_type();
  while (containing != NULL) {
    classname = containing->name() + '_' + classname;
    containing = containing->containing_type();
  }
  return ClassNamePrefix(classname, desc) + classname;
}

std::string ClassNamePrefix(const std::string& classname) {
  std::string lower = classname;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  for (int i = 0; i < kReservedNamesSize; i++) {
    if (lower == kReservedNames[i]) {
      return "PB";
    }
  }

  return "";
}

std::string ConstantNamePrefix(const std::string& classname) {
  bool is_reserved = false;

  std::string lower = classname;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  for (int i = 0; i < kReservedNamesSize; i++) {
    if (lower == kReservedNames[i]) {
      is_reserved = true;
      break;
    }
  }

  for (int i = 0; i < kValidConstantNamesSize; i++) {
    if (lower == kValidConstantNames[i]) {
      is_reserved = false;
      break;
    }
  }

  if (is_reserved) {
    return "PB";
  }

  return "";
}

template <typename DescriptorType>
std::string RootPhpNamespace(const DescriptorType* desc,
                             const Options& options) {
  if (desc->file()->options().has_php_namespace()) {
    const std::string& php_namespace = desc->file()->options().php_namespace();
    if (!php_namespace.empty()) {
      return php_namespace;
    }
    return "";
  }

  if (!desc->file()->package().empty()) {
    return PhpName(desc->file()->package(), options);
  }
  return "";
}

template <typename DescriptorType>
std::string FullClassName(const DescriptorType* desc, const Options& options) {
  std::string classname = GeneratedClassNameImpl(desc);
  std::string php_namespace = RootPhpNamespace(desc, options);
  if (!php_namespace.empty()) {
    return php_namespace + "\\" + classname;
  }
  return classname;
}

template <typename DescriptorType>
std::string FullClassName(const DescriptorType* desc, bool is_descriptor) {
  Options options;
  options.is_descriptor = is_descriptor;
  return FullClassName(desc, options);
}

template <typename DescriptorType>
std::string LegacyFullClassName(const DescriptorType* desc,
                                const Options& options) {
  std::string classname = LegacyGeneratedClassName(desc);
  std::string php_namespace = RootPhpNamespace(desc, options);
  if (!php_namespace.empty()) {
    return php_namespace + "\\" + classname;
  }
  return classname;
}

std::string PhpName(const std::string& full_name, const Options& options) {
  if (options.is_descriptor) {
    return kDescriptorPackageName;
  }

  std::string segment;
  std::string result;
  bool cap_next_letter = true;
  for (int i = 0; i < full_name.size(); i++) {
    if ('a' <= full_name[i] && full_name[i] <= 'z' && cap_next_letter) {
      segment += full_name[i] + ('A' - 'a');
      cap_next_letter = false;
    } else if (full_name[i] == '.') {
      result += ClassNamePrefix(segment) + segment + '\\';
      segment = "";
      cap_next_letter = true;
    } else {
      segment += full_name[i];
      cap_next_letter = false;
    }
  }
  result += ClassNamePrefix(segment) + segment;
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

std::string GeneratedMetadataFileName(const FileDescriptor* file,
                                      const Options& options) {
  const std::string& proto_file = file->name();
  int start_index = 0;
  int first_index = proto_file.find_first_of("/", start_index);
  std::string result = "";
  std::string segment = "";

  if (proto_file == kEmptyFile) {
    return kEmptyMetadataFile;
  }
  if (options.is_descriptor) {
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

  if (file->options().has_php_metadata_namespace()) {
    const std::string& php_metadata_namespace =
        file->options().php_metadata_namespace();
    if (!php_metadata_namespace.empty() && php_metadata_namespace != "\\") {
      result += php_metadata_namespace;
      std::replace(result.begin(), result.end(), '\\', '/');
      if (result.at(result.size() - 1) != '/') {
        result += "/";
      }
    }
  } else {
    result += "GPBMetadata/";
    while (first_index != std::string::npos) {
      segment = UnderscoresToCamelCase(
          file_no_suffix.substr(start_index, first_index - start_index), true);
      result += ReservedNamePrefix(segment, file) + segment + "/";
      start_index = first_index + 1;
      first_index = file_no_suffix.find_first_of("/", start_index);
    }
  }

  // Append file name.
  int file_name_start = file_no_suffix.find_last_of("/");
  if (file_name_start == std::string::npos) {
    file_name_start = 0;
  } else {
    file_name_start += 1;
  }
  segment = UnderscoresToCamelCase(
      file_no_suffix.substr(file_name_start, first_index - file_name_start), true);

  return result + ReservedNamePrefix(segment, file) + segment + ".php";
}

std::string GeneratedMetadataFileName(const FileDescriptor* file,
                                      bool is_descriptor) {
  Options options;
  options.is_descriptor = is_descriptor;
  return GeneratedMetadataFileName(file, options);
}

template <typename DescriptorType>
std::string GeneratedClassFileName(const DescriptorType* desc,
                                   const Options& options) {
  std::string result = FullClassName(desc, options);
  for (int i = 0; i < result.size(); i++) {
    if (result[i] == '\\') {
      result[i] = '/';
    }
  }
  return result + ".php";
}

template <typename DescriptorType>
std::string LegacyGeneratedClassFileName(const DescriptorType* desc,
                                         const Options& options) {
  std::string result = LegacyFullClassName(desc, options);

  for (int i = 0; i < result.size(); i++) {
    if (result[i] == '\\') {
      result[i] = '/';
    }
  }
  return result + ".php";
}

std::string GeneratedServiceFileName(const ServiceDescriptor* service,
                                     const Options& options) {
  std::string result = FullClassName(service, options) + "Interface";
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

std::string PhpSetterTypeName(const FieldDescriptor* field,
                              const Options& options) {
  if (field->is_map()) {
    return "array|\\Google\\Protobuf\\Internal\\MapField";
  }
  std::string type;
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_ENUM:
      type = "int";
      break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64:
      type = "int|string";
      break;
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT:
      type = "float";
      break;
    case FieldDescriptor::TYPE_BOOL:
      type = "bool";
      break;
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      type = "string";
      break;
    case FieldDescriptor::TYPE_MESSAGE:
      type = "\\" + FullClassName(field->message_type(), options);
      break;
    case FieldDescriptor::TYPE_GROUP:
      return "null";
    default: assert(false); return "";
  }
  if (field->is_repeated()) {
    // accommodate for edge case with multiple types.
    size_t start_pos = type.find("|");
    if (start_pos != std::string::npos) {
      type.replace(start_pos, 1, "[]|");
    }
    type += "[]|\\Google\\Protobuf\\Internal\\RepeatedField";
  }
  return type;
}

std::string PhpSetterTypeName(const FieldDescriptor* field,
                              bool is_descriptor) {
  Options options;
  options.is_descriptor = is_descriptor;
  return PhpSetterTypeName(field, options);
}

std::string PhpGetterTypeName(const FieldDescriptor* field,
                              const Options& options) {
  if (field->is_map()) {
    return "\\Google\\Protobuf\\Internal\\MapField";
  }
  if (field->is_repeated()) {
    return "\\Google\\Protobuf\\Internal\\RepeatedField";
  }
  switch (field->type()) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_ENUM: return "int";
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED64: return "int|string";
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_FLOAT: return "float";
    case FieldDescriptor::TYPE_BOOL: return "bool";
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES: return "string";
    case FieldDescriptor::TYPE_MESSAGE:
      return "\\" + FullClassName(field->message_type(), options);
    case FieldDescriptor::TYPE_GROUP: return "null";
    default: assert(false); return "";
  }
}

std::string PhpGetterTypeName(const FieldDescriptor* field,
                              bool is_descriptor) {
  Options options;
  options.is_descriptor = is_descriptor;
  return PhpGetterTypeName(field, options);
}

std::string EnumOrMessageSuffix(const FieldDescriptor* field,
                                const Options& options) {
  if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    return ", '" +
           DescriptorFullName(field->message_type(), options.is_descriptor) +
           "'";
  }
  if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
    return ", '" +
           DescriptorFullName(field->enum_type(), options.is_descriptor) + "'";
  }
  return "";
}

std::string EnumOrMessageSuffix(const FieldDescriptor* field,
                                bool is_descriptor) {
  Options options;
  options.is_descriptor = is_descriptor;
  return EnumOrMessageSuffix(field, options);
}

// Converts a name to camel-case. If cap_first_letter is true, capitalize the
// first letter.
std::string UnderscoresToCamelCase(const std::string& name,
                                   bool cap_first_letter) {
  std::string result;
  for (int i = 0; i < name.size(); i++) {
    if ('a' <= name[i] && name[i] <= 'z') {
      if (cap_first_letter) {
        result += name[i] + ('A' - 'a');
      } else {
        result += name[i];
      }
      cap_first_letter = false;
    } else if ('A' <= name[i] && name[i] <= 'Z') {
      if (i == 0 && !cap_first_letter) {
        // Force first letter to lower-case unless explicitly told to
        // capitalize it.
        result += name[i] + ('a' - 'A');
      } else {
        // Capital letters after the first are left as-is.
        result += name[i];
      }
      cap_first_letter = false;
    } else if ('0' <= name[i] && name[i] <= '9') {
      result += name[i];
      cap_first_letter = true;
    } else {
      cap_first_letter = true;
    }
  }
  // Add a trailing "_" if the name should be altered.
  if (name[name.size() - 1] == '#') {
    result += '_';
  }
  return result;
}

std::string BinaryToHex(const std::string& binary) {
  std::string dest;
  size_t i;
  unsigned char symbol[16] = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'a', 'b',
    'c', 'd', 'e', 'f',
  };

  dest.resize(binary.size() * 2);
  char* append_ptr = &dest[0];

  for (i = 0; i < binary.size(); i++) {
    *append_ptr++ = symbol[(binary[i] & 0xf0) >> 4];
    *append_ptr++ = symbol[binary[i] & 0x0f];
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
                   const Options& options) {
  if (field->is_repeated()) {
    GenerateFieldDocComment(printer, field, options, kFieldProperty);
    printer->Print(
        "private $^name^;\n",
        "name", field->name());
  } else if (field->real_containing_oneof()) {
    // Oneof fields are handled by GenerateOneofField.
    return;
  } else {
    std::string initial_value =
        field->has_presence() ? "null" : DefaultForField(field);
    GenerateFieldDocComment(printer, field, options, kFieldProperty);
    printer->Print(
        "protected $^name^ = ^initial_value^;\n",
        "name", field->name(),
        "initial_value", initial_value);
  }
}

void GenerateOneofField(const OneofDescriptor* oneof, io::Printer* printer) {
  // Oneof property needs to be protected in order to be accessed by parent
  // class in implementation.
  printer->Print(
      "protected $^name^;\n",
      "name", oneof->name());
}

void GenerateFieldAccessor(const FieldDescriptor* field, const Options& options,
                           io::Printer* printer) {
  const OneofDescriptor* oneof = field->real_containing_oneof();

  // Generate getter.
  GenerateFieldDocComment(printer, field, options, kFieldGetter);

  if (oneof != NULL) {
    printer->Print(
        "public function get^camel_name^()\n"
        "{\n"
        "    return $this->readOneof(^number^);\n"
        "}\n\n"
        "public function has^camel_name^()\n"
        "{\n"
        "    return $this->hasOneof(^number^);\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true),
        "number", IntToString(field->number()));
  } else if (field->has_presence()) {
    printer->Print(
        "public function get^camel_name^()\n"
        "{\n"
        "    return isset($this->^name^) ? $this->^name^ : ^default_value^;\n"
        "}\n\n"
        "public function has^camel_name^()\n"
        "{\n"
        "    return isset($this->^name^);\n"
        "}\n\n"
        "public function clear^camel_name^()\n"
        "{\n"
        "    unset($this->^name^);\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true),
        "name", field->name(),
        "default_value", DefaultForField(field));
  } else {
    printer->Print(
        "public function get^camel_name^()\n"
        "{\n"
        "    return $this->^name^;\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true), "name",
        field->name());
  }

  // For wrapper types, generate an additional getXXXUnwrapped getter
  if (!field->is_map() &&
      !field->is_repeated() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      IsWrapperType(field)) {
    GenerateWrapperFieldGetterDocComment(printer, field);
    printer->Print(
        "public function get^camel_name^Unwrapped()\n"
        "{\n"
        "    return $this->readWrapperValue(\"^field_name^\");\n"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true),
        "field_name", field->name());
  }

  // Generate setter.
  GenerateFieldDocComment(printer, field, options, kFieldSetter);
  printer->Print(
      "public function set^camel_name^($var)\n"
      "{\n",
      "camel_name", UnderscoresToCamelCase(field->name(), true));

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
          FullClassName(value->message_type(), options) + "::class");
    } else if (value->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      printer->Print(
          ", \\^class_name^);\n",
          "class_name",
          FullClassName(value->enum_type(), options) + "::class");
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
          FullClassName(field->message_type(), options) + "::class");
    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
      printer->Print(
          ", \\^class_name^);\n",
          "class_name",
          FullClassName(field->enum_type(), options) + "::class");
    } else {
      printer->Print(");\n");
    }
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    printer->Print(
        "GPBUtil::checkMessage($var, \\^class_name^::class);\n",
        "class_name", FullClassName(field->message_type(), options));
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
    printer->Print(
        "GPBUtil::checkEnum($var, \\^class_name^::class);\n",
        "class_name", FullClassName(field->enum_type(), options));
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

  printer->Print("\nreturn $this;\n");

  Outdent(printer);

  printer->Print(
      "}\n\n");

  // For wrapper types, generate an additional setXXXValue getter
  if (!field->is_map() &&
      !field->is_repeated() &&
      field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
      IsWrapperType(field)) {
    GenerateWrapperFieldSetterDocComment(printer, field);
    printer->Print(
        "public function set^camel_name^Unwrapped($var)\n"
        "{\n"
        "    $this->writeWrapperValue(\"^field_name^\", $var);\n"
        "    return $this;"
        "}\n\n",
        "camel_name", UnderscoresToCamelCase(field->name(), true),
        "field_name", field->name());
  }
}

void GenerateEnumToPool(const EnumDescriptor* en, io::Printer* printer) {
  printer->Print(
      "$pool->addEnum('^name^', "
      "\\Google\\Protobuf\\Internal\\^class_name^::class)\n",
      "name", DescriptorFullName(en, true),
      "class_name", en->name());
  Indent(printer);

  for (int i = 0; i < en->value_count(); i++) {
    const EnumValueDescriptor* value = en->value(i);
    printer->Print(
        "->value(\"^name^\", ^number^)\n",
        "name", ConstantNamePrefix(value->name()) + value->name(),
        "number", IntToString(value->number()));
  }
  printer->Print("->finalizeToPool();\n\n");
  Outdent(printer);
}

void GenerateServiceMethod(const MethodDescriptor* method,
                           io::Printer* printer) {
  printer->Print(
        "public function ^camel_name^(\\^request_name^ $request);\n\n",
        "camel_name", UnderscoresToCamelCase(method->name(), false),
        "request_name", FullClassName(
          method->input_type(), false)
  );
}

void GenerateMessageToPool(const std::string& name_prefix,
                           const Descriptor* message, io::Printer* printer) {
  // Don't generate MapEntry messages -- we use the PHP extension's native
  // support for map fields instead.
  if (message->options().map_entry()) {
    return;
  }
  std::string class_name =
      (name_prefix.empty() ? "" : name_prefix + "\\") +
      ReservedNamePrefix(message->name(), message->file()) + message->name();

  printer->Print(
      "$pool->addMessage('^message^', "
      "\\Google\\Protobuf\\Internal\\^class_name^::class)\n",
      "message", DescriptorFullName(message, true),
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
          "number", StrCat(field->number()),
          "other", EnumOrMessageSuffix(val, true));
    } else if (!field->real_containing_oneof()) {
      printer->Print(
          "->^label^('^field^', "
          "\\Google\\Protobuf\\Internal\\GPBType::^type^, ^number^^other^)\n",
          "field", field->name(),
          "label", LabelForField(field),
          "type", ToUpper(field->type_name()),
          "number", StrCat(field->number()),
          "other", EnumOrMessageSuffix(field, true));
    }
  }

  // oneofs.
  for (int i = 0; i < message->real_oneof_decl_count(); i++) {
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
          "number", StrCat(field->number()),
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

void GenerateAddFileToPool(const FileDescriptor* file, const Options& options,
                           io::Printer* printer) {
  printer->Print(
      "public static $is_initialized = false;\n\n"
      "public static function initOnce() {\n");
  Indent(printer);

  if (options.aggregate_metadata) {
    GenerateAddFilesToPool(file, options, printer);
  } else {
    printer->Print(
        "$pool = \\Google\\Protobuf\\Internal\\"
        "DescriptorPool::getGeneratedPool();\n\n"
        "if (static::$is_initialized == true) {\n"
        "  return;\n"
        "}\n");

    if (options.is_descriptor) {
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
            GeneratedMetadataFileName(file->dependency(i), options);
        printer->Print(
            "\\^name^::initOnce();\n",
            "name", FilenameToClassname(dependency_filename));
      }

      // Add messages and enums to descriptor pool.
      FileDescriptorSet files;
      FileDescriptorProto* file_proto = files.add_file();
      file->CopyTo(file_proto);

      // Filter out descriptor.proto as it cannot be depended on for now.
      RepeatedPtrField<std::string>* dependency =
          file_proto->mutable_dependency();
      for (RepeatedPtrField<std::string>::iterator it = dependency->begin();
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

      std::string files_data;
      files.SerializeToString(&files_data);

      printer->Print("$pool->internalAddGeneratedFile(\n");
      Indent(printer);
      printer->Print("'");

      for (auto ch : files_data) {
        switch (ch) {
          case '\\':
            printer->Print(R"(\\)");
            break;
          case '\'':
            printer->Print(R"(\')");
            break;
          default:
            printer->Print("^char^", "char", std::string(1, ch));
            break;
        }
      }

      printer->Print("'\n");
      Outdent(printer);
      printer->Print(
          ", true);\n\n");
    }
    printer->Print(
        "static::$is_initialized = true;\n");
  }

  Outdent(printer);
  printer->Print("}\n");
}

static void AnalyzeDependencyForFile(
    const FileDescriptor* file,
    std::set<const FileDescriptor*>* nodes_without_dependency,
    std::map<const FileDescriptor*, std::set<const FileDescriptor*>>* deps,
    std::map<const FileDescriptor*, int>* dependency_count) {
  int count = file->dependency_count();
  for (int i = 0; i < file->dependency_count(); i++) {
      const FileDescriptor* dependency = file->dependency(i);
      if (dependency->name() == kDescriptorFile) {
        count--;
        break;
      }
  }

  if (count == 0) {
    nodes_without_dependency->insert(file);
  } else {
    (*dependency_count)[file] = count;
    for (int i = 0; i < file->dependency_count(); i++) {
      const FileDescriptor* dependency = file->dependency(i);
      if (dependency->name() == kDescriptorFile) {
        continue;
      }
      if (deps->find(dependency) == deps->end()) {
        (*deps)[dependency] = std::set<const FileDescriptor*>();
      }
      (*deps)[dependency].insert(file);
      AnalyzeDependencyForFile(
          dependency, nodes_without_dependency, deps, dependency_count);
    }
  }
}

static bool NeedsUnwrapping(const FileDescriptor* file,
                            const Options& options) {
  bool has_aggregate_metadata_prefix = false;
  if (options.aggregate_metadata_prefixes.empty()) {
    has_aggregate_metadata_prefix = true;
  } else {
    for (const auto& prefix : options.aggregate_metadata_prefixes) {
      if (HasPrefixString(file->package(), prefix)) {
        has_aggregate_metadata_prefix = true;
        break;
      }
    }
  }

  return has_aggregate_metadata_prefix;
}

void GenerateAddFilesToPool(const FileDescriptor* file, const Options& options,
                            io::Printer* printer) {
  printer->Print(
      "$pool = \\Google\\Protobuf\\Internal\\"
      "DescriptorPool::getGeneratedPool();\n"
      "if (static::$is_initialized == true) {\n"
      "  return;\n"
      "}\n");

  // Sort files according to dependency
  std::map<const FileDescriptor*, std::set<const FileDescriptor*>> deps;
  std::map<const FileDescriptor*, int> dependency_count;
  std::set<const FileDescriptor*> nodes_without_dependency;
  FileDescriptorSet sorted_file_set;
  
  AnalyzeDependencyForFile(
      file, &nodes_without_dependency, &deps, &dependency_count);

  while (!nodes_without_dependency.empty()) {
    auto file = *nodes_without_dependency.begin();
    nodes_without_dependency.erase(file);
    for (auto dependent : deps[file]) {
      if (dependency_count[dependent] == 1) {
        dependency_count.erase(dependent);
        nodes_without_dependency.insert(dependent);
      } else {
        dependency_count[dependent] -= 1;
      }
    }

    bool needs_aggregate = NeedsUnwrapping(file, options);

    if (needs_aggregate) {
      auto file_proto = sorted_file_set.add_file();
      file->CopyTo(file_proto);

      // Filter out descriptor.proto as it cannot be depended on for now.
      RepeatedPtrField<std::string>* dependency =
          file_proto->mutable_dependency();
      for (RepeatedPtrField<std::string>::iterator it = dependency->begin();
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
    } else {
      std::string dependency_filename = GeneratedMetadataFileName(file, false);
      printer->Print(
          "\\^name^::initOnce();\n",
          "name", FilenameToClassname(dependency_filename));
    }
  }

  std::string files_data;
  sorted_file_set.SerializeToString(&files_data);

  printer->Print("$pool->internalAddGeneratedFile(\n");
  Indent(printer);
  printer->Print("'");

  for (auto ch : files_data) {
    switch (ch) {
      case '\\':
        printer->Print(R"(\\)");
        break;
      case '\'':
        printer->Print(R"(\')");
        break;
      default:
        printer->Print("^char^", "char", std::string(1, ch));
        break;
    }
  }

  printer->Print("'\n");
  Outdent(printer);
  printer->Print(
      ", true);\n");

  printer->Print(
      "static::$is_initialized = true;\n");
}

void GenerateUseDeclaration(const Options& options, io::Printer* printer) {
  if (!options.is_descriptor) {
    printer->Print(
        "use Google\\Protobuf\\Internal\\GPBType;\n"
        "use Google\\Protobuf\\Internal\\RepeatedField;\n"
        "use Google\\Protobuf\\Internal\\GPBUtil;\n\n");
  } else {
    printer->Print(
        "use Google\\Protobuf\\Internal\\GPBType;\n"
        "use Google\\Protobuf\\Internal\\GPBWire;\n"
        "use Google\\Protobuf\\Internal\\RepeatedField;\n"
        "use Google\\Protobuf\\Internal\\InputStream;\n"
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

std::string FilenameToClassname(const std::string& filename) {
  int lastindex = filename.find_last_of(".");
  std::string result = filename.substr(0, lastindex);
  for (int i = 0; i < result.size(); i++) {
    if (result[i] == '/') {
      result[i] = '\\';
    }
  }
  return result;
}

void GenerateMetadataFile(const FileDescriptor* file, const Options& options,
                          GeneratorContext* generator_context) {
  std::string filename = GeneratedMetadataFileName(file, options);
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '^');

  GenerateHead(file, &printer);

  std::string fullname = FilenameToClassname(filename);
  int lastindex = fullname.find_last_of("\\");

  if (lastindex != std::string::npos) {
    printer.Print(
        "namespace ^name^;\n\n",
        "name", fullname.substr(0, lastindex));

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

  GenerateAddFileToPool(file, options, &printer);

  Outdent(&printer);
  printer.Print("}\n\n");
}

template <typename DescriptorType>
void LegacyGenerateClassFile(const FileDescriptor* file,
                             const DescriptorType* desc, const Options& options,
                             GeneratorContext* generator_context) {
  std::string filename = LegacyGeneratedClassFileName(desc, options);
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '^');

  GenerateHead(file, &printer);

  std::string php_namespace = RootPhpNamespace(desc, options);
  if (!php_namespace.empty()) {
    printer.Print(
        "namespace ^name^;\n\n",
        "name", php_namespace);
  }
  std::string newname = FullClassName(desc, options);
  printer.Print("if (false) {\n");
  Indent(&printer);
  printer.Print("/**\n");
  printer.Print(" * This class is deprecated. Use ^new^ instead.\n",
      "new", newname);
  printer.Print(" * @deprecated\n");
  printer.Print(" */\n");
  printer.Print("class ^old^ {}\n",
      "old", LegacyGeneratedClassName(desc));
  Outdent(&printer);
  printer.Print("}\n");
  printer.Print("class_exists(^new^::class);\n",
      "new", GeneratedClassNameImpl(desc));
  printer.Print("@trigger_error('^old^ is deprecated and will be removed in "
      "the next major release. Use ^fullname^ instead', E_USER_DEPRECATED);\n\n",
      "old", LegacyFullClassName(desc, options),
      "fullname", newname);
}

void GenerateEnumFile(const FileDescriptor* file, const EnumDescriptor* en,
                      const Options& options,
                      GeneratorContext* generator_context) {
  std::string filename = GeneratedClassFileName(en, options);
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '^');

  GenerateHead(file, &printer);

  std::string fullname = FilenameToClassname(filename);
  int lastindex = fullname.find_last_of("\\");

  if (lastindex != std::string::npos) {
    printer.Print(
        "namespace ^name^;\n\n",
        "name", fullname.substr(0, lastindex));

    // We only need this 'use' statement if the enum has a namespace.
    // Otherwise, we get a warning that the use statement has no effect.
    printer.Print("use UnexpectedValueException;\n\n");
  }

  GenerateEnumDocComment(&printer, en, options);

  if (lastindex != std::string::npos) {
    fullname = fullname.substr(lastindex + 1);
  }

  printer.Print(
      "class ^name^\n"
      "{\n",
      "name", fullname);
  Indent(&printer);

  for (int i = 0; i < en->value_count(); i++) {
    const EnumValueDescriptor* value = en->value(i);
    GenerateEnumValueDocComment(&printer, value);
    printer.Print("const ^name^ = ^number^;\n",
                  "name", ConstantNamePrefix(value->name()) + value->name(),
                  "number", IntToString(value->number()));
  }

  printer.Print("\nprivate static $valueToName = [\n");
  Indent(&printer);
  for (int i = 0; i < en->value_count(); i++) {
    const EnumValueDescriptor* value = en->value(i);
    printer.Print("self::^name^ => '^name^',\n",
                  "name", ConstantNamePrefix(value->name()) + value->name());
  }
  Outdent(&printer);
  printer.Print("];\n");

  printer.Print(
      "\npublic static function name($value)\n"
      "{\n");
  Indent(&printer);
  printer.Print("if (!isset(self::$valueToName[$value])) {\n");
  Indent(&printer);
  printer.Print("throw new UnexpectedValueException(sprintf(\n");
  Indent(&printer);
  Indent(&printer);
  printer.Print("'Enum %s has no name defined for value %s', __CLASS__, $value));\n");
  Outdent(&printer);
  Outdent(&printer);
  Outdent(&printer);
  printer.Print("}\n"
                "return self::$valueToName[$value];\n");
  Outdent(&printer);
  printer.Print("}\n\n");

  printer.Print(
      "\npublic static function value($name)\n"
      "{\n");
  Indent(&printer);
  printer.Print("$const = __CLASS__ . '::' . strtoupper($name);\n"
                "if (!defined($const)) {\n");
  Indent(&printer);
  printer.Print("throw new UnexpectedValueException(sprintf(\n");
  Indent(&printer);
  Indent(&printer);
  printer.Print("'Enum %s has no value defined for name %s', __CLASS__, $name));\n");
  Outdent(&printer);
  Outdent(&printer);
  Outdent(&printer);
  printer.Print("}\n"
                "return constant($const);\n");
  Outdent(&printer);
  printer.Print("}\n");

  Outdent(&printer);
  printer.Print("}\n\n");

  // write legacy file for backwards compatibility with nested messages and enums
  if (en->containing_type() != NULL) {
    printer.Print(
        "// Adding a class alias for backwards compatibility with the previous class name.\n");
    printer.Print(
        "class_alias(^new^::class, \\^old^::class);\n\n",
        "new", fullname,
        "old", LegacyFullClassName(en, options));
    LegacyGenerateClassFile(file, en, options, generator_context);
  }
}

void GenerateMessageFile(const FileDescriptor* file, const Descriptor* message,
                         const Options& options,
                         GeneratorContext* generator_context) {
  // Don't generate MapEntry messages -- we use the PHP extension's native
  // support for map fields instead.
  if (message->options().map_entry()) {
    return;
  }

  std::string filename = GeneratedClassFileName(message, options);
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '^');

  GenerateHead(file, &printer);

  std::string fullname = FilenameToClassname(filename);
  int lastindex = fullname.find_last_of("\\");

  if (lastindex != std::string::npos) {
    printer.Print(
        "namespace ^name^;\n\n",
        "name", fullname.substr(0, lastindex));
  }

  GenerateUseDeclaration(options, &printer);

  GenerateMessageDocComment(&printer, message, options);
  if (lastindex != std::string::npos) {
    fullname = fullname.substr(lastindex + 1);
  }

  std::string base;

  switch (message->well_known_type()) {
    case Descriptor::WELLKNOWNTYPE_ANY:
      base = "\\Google\\Protobuf\\Internal\\AnyBase";
      break;
    case Descriptor::WELLKNOWNTYPE_TIMESTAMP:
      base = "\\Google\\Protobuf\\Internal\\TimestampBase";
      break;
    default:
      base = "\\Google\\Protobuf\\Internal\\Message";
      break;
  }

  printer.Print(
      "class ^name^ extends ^base^\n"
      "{\n",
      "base", base,
      "name", fullname);
  Indent(&printer);

  // Field and oneof definitions.
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    GenerateField(field, &printer, options);
  }
  for (int i = 0; i < message->real_oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    GenerateOneofField(oneof, &printer);
  }
  printer.Print("\n");

  GenerateMessageConstructorDocComment(&printer, message, options);
  printer.Print(
      "public function __construct($data = NULL) {\n");
  Indent(&printer);

  std::string metadata_filename = GeneratedMetadataFileName(file, options);
  std::string metadata_fullname = FilenameToClassname(metadata_filename);
  printer.Print(
      "\\^fullname^::initOnce();\n",
      "fullname", metadata_fullname);

  printer.Print(
      "parent::__construct($data);\n");

  Outdent(&printer);
  printer.Print("}\n\n");

  // Field and oneof accessors.
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    GenerateFieldAccessor(field, options, &printer);
  }
  for (int i = 0; i < message->real_oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    printer.Print(
      "/**\n"
      " * @return string\n"
      " */\n"
      "public function get^camel_name^()\n"
      "{\n"
      "    return $this->whichOneof(\"^name^\");\n"
      "}\n\n",
      "camel_name", UnderscoresToCamelCase(oneof->name(), true), "name",
      oneof->name());
  }

  Outdent(&printer);
  printer.Print("}\n\n");

  // write legacy file for backwards compatibility with nested messages and enums
  if (message->containing_type() != NULL) {
    printer.Print(
        "// Adding a class alias for backwards compatibility with the previous class name.\n");
    printer.Print(
        "class_alias(^new^::class, \\^old^::class);\n\n",
        "new", fullname,
        "old", LegacyFullClassName(message, options));
    LegacyGenerateClassFile(file, message, options, generator_context);
  }

  // Nested messages and enums.
  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateMessageFile(file, message->nested_type(i), options,
                        generator_context);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnumFile(file, message->enum_type(i), options, generator_context);
  }
}

void GenerateServiceFile(
    const FileDescriptor* file, const ServiceDescriptor* service,
    const Options& options, GeneratorContext* generator_context) {
  std::string filename = GeneratedServiceFileName(service, options);
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      generator_context->Open(filename));
  io::Printer printer(output.get(), '^');

  GenerateHead(file, &printer);

  std::string fullname = FilenameToClassname(filename);
  int lastindex = fullname.find_last_of("\\");

  if (!file->options().php_namespace().empty() ||
      (!file->options().has_php_namespace() && !file->package().empty()) ||
      lastindex != std::string::npos) {
    printer.Print(
        "namespace ^name^;\n\n",
        "name", fullname.substr(0, lastindex));
  }

  GenerateServiceDocComment(&printer, service);

  if (lastindex != std::string::npos) {
    printer.Print(
        "interface ^name^\n"
        "{\n",
        "name", fullname.substr(lastindex + 1));
  } else {
    printer.Print(
        "interface ^name^\n"
        "{\n",
        "name", fullname);
  }

  Indent(&printer);

  for (int i = 0; i < service->method_count(); i++) {
    const MethodDescriptor* method = service->method(i);
    GenerateServiceMethodDocComment(&printer, method);
    GenerateServiceMethod(method, &printer);
  }

  Outdent(&printer);
  printer.Print("}\n\n");
}

void GenerateFile(const FileDescriptor* file, const Options& options,
                  GeneratorContext* generator_context) {
  GenerateMetadataFile(file, options, generator_context);

  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateMessageFile(file, file->message_type(i), options,
                        generator_context);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    GenerateEnumFile(file, file->enum_type(i), options, generator_context);
  }
  if (file->options().php_generic_services()) {
    for (int i = 0; i < file->service_count(); i++) {
      GenerateServiceFile(file, file->service(i), options, generator_context);
    }
  }
}

static std::string EscapePhpdoc(const std::string& input) {
  std::string result;
  result.reserve(input.size() * 2);

  char prev = '*';

  for (std::string::size_type i = 0; i < input.size(); i++) {
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
      default:
        result.push_back(c);
        break;
    }

    prev = c;
  }

  return result;
}

static void GenerateDocCommentBodyForLocation(
    io::Printer* printer, const SourceLocation& location, bool trailingNewline,
    int indentCount) {
  std::string comments = location.leading_comments.empty()
                             ? location.trailing_comments
                             : location.leading_comments;
  if (!comments.empty()) {
    // TODO(teboring):  Ideally we should parse the comment text as Markdown and
    //   write it back as HTML, but this requires a Markdown parser.  For now
    //   we just use the proto comments unchanged.

    // If the comment itself contains block comment start or end markers,
    // HTML-escape them so that they don't accidentally close the doc comment.
    comments = EscapePhpdoc(comments);

    std::vector<std::string> lines = Split(comments, "\n", true);
    while (!lines.empty() && lines.back().empty()) {
      lines.pop_back();
    }

    for (int i = 0; i < lines.size(); i++) {
      // Most lines should start with a space.  Watch out for lines that start
      // with a /, since putting that right after the leading asterisk will
      // close the comment.
      if (indentCount == 0 && !lines[i].empty() && lines[i][0] == '/') {
        printer->Print(" * ^line^\n", "line", lines[i]);
      } else {
        std::string indent = std::string(indentCount, ' ');
        printer->Print(" *^ind^^line^\n", "ind", indent, "line", lines[i]);
      }
    }
    if (trailingNewline) {
      printer->Print(" *\n");
    }
  }
}

template <typename DescriptorType>
static void GenerateDocCommentBody(
    io::Printer* printer, const DescriptorType* descriptor) {
  SourceLocation location;
  if (descriptor->GetSourceLocation(&location)) {
    GenerateDocCommentBodyForLocation(printer, location, true, 0);
  }
}

static std::string FirstLineOf(const std::string& value) {
  std::string result = value;

  std::string::size_type pos = result.find_first_of('\n');
  if (pos != std::string::npos) {
    result.erase(pos);
  }

  return result;
}

void GenerateMessageDocComment(io::Printer* printer, const Descriptor* message,
                               const Options& options) {
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, message);
  printer->Print(
    " * Generated from protobuf message <code>^messagename^</code>\n"
    " */\n",
    "fullname", EscapePhpdoc(FullClassName(message, options)),
    "messagename", EscapePhpdoc(message->full_name()));
}

void GenerateMessageConstructorDocComment(io::Printer* printer,
                                          const Descriptor* message,
                                          const Options& options) {
  // In theory we should have slightly different comments for setters, getters,
  // etc., but in practice everyone already knows the difference between these
  // so it's redundant information.

  // We start the comment with the main body based on the comments from the
  // .proto file (if present). We then end with the field declaration, e.g.:
  //   optional string foo = 5;
  // If the field is a group, the debug string might end with {.
  printer->Print("/**\n");
  printer->Print(" * Constructor.\n");
  printer->Print(" *\n");
  printer->Print(" * @param array $data {\n");
  printer->Print(" *     Optional. Data for populating the Message object.\n");
  printer->Print(" *\n");
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    printer->Print(" *     @type ^php_type^ $^var^\n",
      "php_type", PhpSetterTypeName(field, options),
      "var", field->name());
    SourceLocation location;
    if (field->GetSourceLocation(&location)) {
      GenerateDocCommentBodyForLocation(printer, location, false, 10);
    }
  }
  printer->Print(" * }\n");
  printer->Print(" */\n");
}

void GenerateServiceDocComment(io::Printer* printer,
                               const ServiceDescriptor* service) {
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, service);
  printer->Print(
    " * Protobuf type <code>^fullname^</code>\n"
    " */\n",
    "fullname", EscapePhpdoc(service->full_name()));
}

void GenerateFieldDocComment(io::Printer* printer, const FieldDescriptor* field,
                             const Options& options, int function_type) {
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
    " * Generated from protobuf field <code>^def^</code>\n",
    "def", EscapePhpdoc(FirstLineOf(field->DebugString())));
  if (function_type == kFieldSetter) {
    printer->Print(" * @param ^php_type^ $var\n",
      "php_type", PhpSetterTypeName(field, options));
    printer->Print(" * @return $this\n");
  } else if (function_type == kFieldGetter) {
    printer->Print(" * @return ^php_type^\n",
      "php_type", PhpGetterTypeName(field, options));
  }
  printer->Print(" */\n");
}

void GenerateWrapperFieldGetterDocComment(io::Printer* printer, const FieldDescriptor* field) {
  // Generate a doc comment for the special getXXXValue methods that are
  // generated for wrapper types.
  const FieldDescriptor* primitiveField = field->message_type()->FindFieldByName("value");
  printer->Print("/**\n");
  printer->Print(
      " * Returns the unboxed value from <code>get^camel_name^()</code>\n\n",
      "camel_name", UnderscoresToCamelCase(field->name(), true));
  GenerateDocCommentBody(printer, field);
  printer->Print(
    " * Generated from protobuf field <code>^def^</code>\n",
    "def", EscapePhpdoc(FirstLineOf(field->DebugString())));
  printer->Print(" * @return ^php_type^|null\n",
        "php_type", PhpGetterTypeName(primitiveField, false));
  printer->Print(" */\n");
}

void GenerateWrapperFieldSetterDocComment(io::Printer* printer, const FieldDescriptor* field) {
  // Generate a doc comment for the special setXXXValue methods that are
  // generated for wrapper types.
  const FieldDescriptor* primitiveField = field->message_type()->FindFieldByName("value");
  printer->Print("/**\n");
  printer->Print(
      " * Sets the field by wrapping a primitive type in a ^message_name^ object.\n\n",
      "message_name", FullClassName(field->message_type(), false));
  GenerateDocCommentBody(printer, field);
  printer->Print(
    " * Generated from protobuf field <code>^def^</code>\n",
    "def", EscapePhpdoc(FirstLineOf(field->DebugString())));
  printer->Print(" * @param ^php_type^|null $var\n",
        "php_type", PhpSetterTypeName(primitiveField, false));
  printer->Print(" * @return $this\n");
  printer->Print(" */\n");
}

void GenerateEnumDocComment(io::Printer* printer, const EnumDescriptor* enum_,
                            const Options& options) {
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, enum_);
  printer->Print(
    " * Protobuf type <code>^fullname^</code>\n"
    " */\n",
    "fullname", EscapePhpdoc(enum_->full_name()));
}

void GenerateEnumValueDocComment(io::Printer* printer,
                                 const EnumValueDescriptor* value) {
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, value);
  printer->Print(
    " * Generated from protobuf enum <code>^def^</code>\n"
    " */\n",
    "def", EscapePhpdoc(FirstLineOf(value->DebugString())));
}

void GenerateServiceMethodDocComment(io::Printer* printer,
                                     const MethodDescriptor* method) {
  printer->Print("/**\n");
  GenerateDocCommentBody(printer, method);
  printer->Print(
    " * Method <code>^method_name^</code>\n"
    " *\n",
    "method_name", EscapePhpdoc(UnderscoresToCamelCase(method->name(), false)));
  printer->Print(
    " * @param \\^input_type^ $request\n",
    "input_type", EscapePhpdoc(FullClassName(method->input_type(), false)));
  printer->Print(
    " * @return \\^return_type^\n"
    " */\n",
    "return_type", EscapePhpdoc(FullClassName(method->output_type(), false)));
}

std::string FilenameCName(const FileDescriptor* file) {
  std::string c_name = file->name();
  c_name = StringReplace(c_name, ".", "_", true);
  c_name = StringReplace(c_name, "/", "_", true);
  return c_name;
}

void GenerateCEnum(const EnumDescriptor* desc, io::Printer* printer) {
  std::string c_name = desc->full_name();
  c_name = StringReplace(c_name, ".", "_", true);
  std::string php_name = FullClassName(desc, Options());
  php_name = StringReplace(php_name, "\\", "\\\\", true);
  printer->Print(
      "/* $c_name$ */\n"
      "\n"
      "zend_class_entry* $c_name$_ce;\n"
      "\n"
      "PHP_METHOD($c_name$, name) {\n"
      "  $file_c_name$_AddDescriptor();\n"
      "  const upb_symtab *symtab = DescriptorPool_GetSymbolTable();\n"
      "  const upb_enumdef *e = upb_symtab_lookupenum(symtab, \"$name$\");\n"
      "  const char *name;\n"
      "  zend_long value;\n"
      "  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, \"l\", &value) ==\n"
      "      FAILURE) {\n"
      "    return;\n"
      "  }\n"
      "  name = upb_enumdef_iton(e, value);\n"
      "  if (!name) {\n"
      "    zend_throw_exception_ex(NULL, 0,\n"
      "                            \"$php_name$ has no name \"\n"
      "                            \"defined for value \" ZEND_LONG_FMT \".\",\n"
      "                            value);\n"
      "    return;\n"
      "  }\n"
      "  RETURN_STRING(name);\n"
      "}\n"
      "\n"
      "PHP_METHOD($c_name$, value) {\n"
      "  $file_c_name$_AddDescriptor();\n"
      "  const upb_symtab *symtab = DescriptorPool_GetSymbolTable();\n"
      "  const upb_enumdef *e = upb_symtab_lookupenum(symtab, \"$name$\");\n"
      "  char *name = NULL;\n"
      "  size_t name_len;\n"
      "  int32_t num;\n"
      "  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, \"s\", &name,\n"
      "                            &name_len) == FAILURE) {\n"
      "    return;\n"
      "  }\n"
      "  if (!upb_enumdef_ntoi(e, name, name_len, &num)) {\n"
      "    zend_throw_exception_ex(NULL, 0,\n"
      "                            \"$php_name$ has no value \"\n"
      "                            \"defined for name %s.\",\n"
      "                            name);\n"
      "    return;\n"
      "  }\n"
      "  RETURN_LONG(num);\n"
      "}\n"
      "\n"
      "static zend_function_entry $c_name$_phpmethods[] = {\n"
      "  PHP_ME($c_name$, name, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)\n"
      "  PHP_ME($c_name$, value, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)\n"
      "  ZEND_FE_END\n"
      "};\n"
      "\n"
      "static void $c_name$_ModuleInit() {\n"
      "  zend_class_entry tmp_ce;\n"
      "\n"
      "  INIT_CLASS_ENTRY(tmp_ce, \"$php_name$\",\n"
      "                   $c_name$_phpmethods);\n"
      "\n"
      "  $c_name$_ce = zend_register_internal_class(&tmp_ce);\n",
      "name", desc->full_name(),
      "file_c_name", FilenameCName(desc->file()),
      "c_name", c_name,
      "php_name", php_name);

  for (int i = 0; i < desc->value_count(); i++) {
    const EnumValueDescriptor* value = desc->value(i);
    printer->Print(
        "  zend_declare_class_constant_long($c_name$_ce, \"$name$\",\n"
        "                                   strlen(\"$name$\"), $num$);\n",
        "c_name", c_name,
        "name", value->name(),
        "num", std::to_string(value->number()));
  }

  printer->Print(
      "}\n"
      "\n");
}

void GenerateCMessage(const Descriptor* message, io::Printer* printer) {
  std::string c_name = message->full_name();
  c_name = StringReplace(c_name, ".", "_", true);
  std::string php_name = FullClassName(message, Options());
  php_name = StringReplace(php_name, "\\", "\\\\", true);
  printer->Print(
      "/* $c_name$ */\n"
      "\n"
      "zend_class_entry* $c_name$_ce;\n"
      "\n"
      "static PHP_METHOD($c_name$, __construct) {\n"
      "  $file_c_name$_AddDescriptor();\n"
      "  zim_Message___construct(INTERNAL_FUNCTION_PARAM_PASSTHRU);\n"
      "}\n"
      "\n",
      "file_c_name", FilenameCName(message->file()),
      "c_name", c_name);

  for (int i = 0; i < message->field_count(); i++) {
    auto field = message->field(i);
    printer->Print(
      "static PHP_METHOD($c_name$, get$camel_name$) {\n"
      "  Message* intern = (Message*)Z_OBJ_P(getThis());\n"
      "  const upb_fielddef *f = upb_msgdef_ntofz(intern->desc->msgdef,\n"
      "                                           \"$name$\");\n"
      "  zval ret;\n"
      "  Message_get(intern, f, &ret);\n"
      "  RETURN_ZVAL(&ret, 1, 0);\n"
      "}\n"
      "\n"
      "static PHP_METHOD($c_name$, set$camel_name$) {\n"
      "  Message* intern = (Message*)Z_OBJ_P(getThis());\n"
      "  const upb_fielddef *f = upb_msgdef_ntofz(intern->desc->msgdef,\n"
      "                                           \"$name$\");\n"
      "  zval *val;\n"
      "  if (zend_parse_parameters(ZEND_NUM_ARGS(), \"z\", &val)\n"
      "      == FAILURE) {\n"
      "    return;\n"
      "  }\n"
      "  Message_set(intern, f, val);\n"
      "  RETURN_ZVAL(getThis(), 1, 0);\n"
      "}\n"
      "\n",
      "c_name", c_name,
      "name", field->name(),
      "camel_name", UnderscoresToCamelCase(field->name(), true));
  }

  for (int i = 0; i < message->real_oneof_decl_count(); i++) {
    auto oneof = message->oneof_decl(i);
    printer->Print(
      "static PHP_METHOD($c_name$, get$camel_name$) {\n"
      "  Message* intern = (Message*)Z_OBJ_P(getThis());\n"
      "  const upb_oneofdef *oneof = upb_msgdef_ntooz(intern->desc->msgdef,\n"
      "                                              \"$name$\");\n"
      "  const upb_fielddef *field = upb_msg_whichoneof(intern->msg, oneof);\n"
      "  RETURN_STRING(field ? upb_fielddef_name(field) : \"\");\n"
      "}\n",
      "c_name", c_name,
      "name", oneof->name(),
      "camel_name", UnderscoresToCamelCase(oneof->name(), true));
  }

  printer->Print(
      "static zend_function_entry $c_name$_phpmethods[] = {\n"
      "  PHP_ME($c_name$, __construct, NULL, ZEND_ACC_PUBLIC)\n",
      "c_name", c_name);

  for (int i = 0; i < message->field_count(); i++) {
    auto field = message->field(i);
    printer->Print(
      "  PHP_ME($c_name$, get$camel_name$, NULL, ZEND_ACC_PUBLIC)\n"
      "  PHP_ME($c_name$, set$camel_name$, NULL, ZEND_ACC_PUBLIC)\n",
      "c_name", c_name,
      "camel_name", UnderscoresToCamelCase(field->name(), true));
  }

  for (int i = 0; i < message->real_oneof_decl_count(); i++) {
    auto oneof = message->oneof_decl(i);
    printer->Print(
      "  PHP_ME($c_name$, get$camel_name$, NULL, ZEND_ACC_PUBLIC)\n",
      "c_name", c_name,
      "camel_name", UnderscoresToCamelCase(oneof->name(), true));
  }

  // Extra hand-written functions added to the well-known types.
  switch (message->well_known_type()) {
    case Descriptor::WELLKNOWNTYPE_ANY:
      printer->Print(
        "  PHP_ME($c_name$, is, NULL, ZEND_ACC_PUBLIC)\n"
        "  PHP_ME($c_name$, pack, NULL, ZEND_ACC_PUBLIC)\n"
        "  PHP_ME($c_name$, unpack, NULL, ZEND_ACC_PUBLIC)\n",
        "c_name", c_name);
      break;
    case Descriptor::WELLKNOWNTYPE_TIMESTAMP:
      printer->Print(
        "  PHP_ME($c_name$, fromDateTime, NULL, ZEND_ACC_PUBLIC)\n"
        "  PHP_ME($c_name$, toDateTime, NULL, ZEND_ACC_PUBLIC)\n",
        "c_name", c_name);
      break;
    default:
      break;
  } 

  printer->Print(
      "  ZEND_FE_END\n"
      "};\n"
      "\n"
      "static void $c_name$_ModuleInit() {\n"
      "  zend_class_entry tmp_ce;\n"
      "\n"
      "  INIT_CLASS_ENTRY(tmp_ce, \"$php_name$\",\n"
      "                   $c_name$_phpmethods);\n"
      "\n"
      "  $c_name$_ce = zend_register_internal_class(&tmp_ce);\n"
      "  $c_name$_ce->ce_flags |= ZEND_ACC_FINAL;\n"
      "  $c_name$_ce->create_object = Message_create;\n"
      "  zend_do_inheritance($c_name$_ce, message_ce);\n"
      "}\n"
      "\n",
      "c_name", c_name,
      "php_name", php_name);

  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateCMessage(message->nested_type(i), printer);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateCEnum(message->enum_type(i), printer);
  }
}

void GenerateEnumCInit(const EnumDescriptor* desc, io::Printer* printer) {
  std::string c_name = desc->full_name();
  c_name = StringReplace(c_name, ".", "_", true);

  printer->Print(
      "  $c_name$_ModuleInit();\n",
      "c_name", c_name);
}

void GenerateCInit(const Descriptor* message, io::Printer* printer) {
  std::string c_name = message->full_name();
  c_name = StringReplace(c_name, ".", "_", true);

  printer->Print(
      "  $c_name$_ModuleInit();\n",
      "c_name", c_name);

  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateCInit(message->nested_type(i), printer);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnumCInit(message->enum_type(i), printer);
  }
}

void GenerateCWellKnownTypes(const std::vector<const FileDescriptor*>& files,
                             GeneratorContext* context) {
  std::unique_ptr<io::ZeroCopyOutputStream> output(
      context->Open("../ext/google/protobuf/wkt.inc"));
  io::Printer printer(output.get(), '$');

  printer.Print(
      "// This file is generated from the .proto files for the well-known\n"
      "// types. Do not edit!\n");

  for (auto file : files) {
    printer.Print(
        "static void $c_name$_AddDescriptor();\n",
        "c_name", FilenameCName(file));
  }

  for (auto file : files) {
    std::string c_name = FilenameCName(file);
    std::string metadata_filename = GeneratedMetadataFileName(file, Options());
    std::string metadata_classname = FilenameToClassname(metadata_filename);
    std::string metadata_c_name =
        StringReplace(metadata_classname, "\\", "_", true);
    metadata_classname = StringReplace(metadata_classname, "\\", "\\\\", true);
    FileDescriptorProto file_proto;
    file->CopyTo(&file_proto);
    std::string serialized;
    file_proto.SerializeToString(&serialized);
    printer.Print(
        "/* $filename$ */\n"
        "\n"
        "zend_class_entry* $metadata_c_name$_ce;\n"
        "\n"
        "const char $c_name$_descriptor [$size$] = {\n",
        "filename", file->name(),
        "c_name", c_name,
        "metadata_c_name", metadata_c_name,
        "size", std::to_string(serialized.size()));

    for (size_t i = 0; i < serialized.size();) {
      for (size_t j = 0; j < 25 && i < serialized.size(); ++i, ++j) {
        printer.Print("'$ch$', ", "ch", CEscape(serialized.substr(i, 1)));
      }
      printer.Print("\n");
    }

    printer.Print(
        "};\n"
        "\n"
        "static void $c_name$_AddDescriptor() {\n"
        "  if (DescriptorPool_HasFile(\"$filename$\")) return;\n",
        "filename", file->name(),
        "c_name", c_name,
        "metadata_c_name", metadata_c_name);

    for (int i = 0; i < file->dependency_count(); i++) {
      std::string dep_c_name = FilenameCName(file->dependency(i));
      printer.Print(
          "  $dep_c_name$_AddDescriptor();\n",
          "dep_c_name", dep_c_name);
    }

    printer.Print(
        "  DescriptorPool_AddDescriptor(\"$filename$\", $c_name$_descriptor,\n"
        "                               sizeof($c_name$_descriptor));\n"
        "}\n"
        "\n"
        "static PHP_METHOD($metadata_c_name$, initOnce) {\n"
        "  $c_name$_AddDescriptor();\n"
        "}\n"
        "\n"
        "static zend_function_entry $metadata_c_name$_methods[] = {\n"
        "  PHP_ME($metadata_c_name$, initOnce, NULL, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)\n"
        "  ZEND_FE_END\n"
        "};\n"
        "\n"
        "static void $metadata_c_name$_ModuleInit() {\n"
        "  zend_class_entry tmp_ce;\n"
        "\n"
        "  INIT_CLASS_ENTRY(tmp_ce, \"$metadata_classname$\",\n"
        "                   $metadata_c_name$_methods);\n"
        "\n"
        "  $metadata_c_name$_ce = zend_register_internal_class(&tmp_ce);\n"
        "}\n"
        "\n",
        "filename", file->name(),
        "c_name", c_name,
        "metadata_c_name", metadata_c_name,
        "metadata_classname", metadata_classname);
    for (int i = 0; i < file->message_type_count(); i++) {
      GenerateCMessage(file->message_type(i), &printer);
    }
    for (int i = 0; i < file->enum_type_count(); i++) {
      GenerateCEnum(file->enum_type(i), &printer);
    }
  }

  printer.Print(
      "static void WellKnownTypes_ModuleInit() {\n");

  for (auto file : files) {
    std::string metadata_filename = GeneratedMetadataFileName(file, Options());
    std::string metadata_classname = FilenameToClassname(metadata_filename);
    std::string metadata_c_name =
        StringReplace(metadata_classname, "\\", "_", true);
    printer.Print(
        "  $metadata_c_name$_ModuleInit();\n",
        "metadata_c_name", metadata_c_name);
    for (int i = 0; i < file->message_type_count(); i++) {
      GenerateCInit(file->message_type(i), &printer);
    }
    for (int i = 0; i < file->enum_type_count(); i++) {
      GenerateEnumCInit(file->enum_type(i), &printer);
    }
  }

  printer.Print(
      "}\n");
}

}  // namespace

std::string GeneratedClassName(const Descriptor* desc) {
  return GeneratedClassNameImpl(desc);
}

std::string GeneratedClassName(const EnumDescriptor* desc) {
  return GeneratedClassNameImpl(desc);
}

std::string GeneratedClassName(const ServiceDescriptor* desc) {
  return GeneratedClassNameImpl(desc);
}

bool Generator::Generate(const FileDescriptor* file,
                         const std::string& parameter,
                         GeneratorContext* generator_context,
                         std::string* error) const {
  return Generate(file, Options(), generator_context, error);
}

bool Generator::Generate(const FileDescriptor* file, const Options& options,
                         GeneratorContext* generator_context,
                         std::string* error) const {
  if (options.is_descriptor && file->name() != kDescriptorFile) {
    *error =
        "Can only generate PHP code for google/protobuf/descriptor.proto.\n";
    return false;
  }

  if (!options.is_descriptor && file->syntax() != FileDescriptor::SYNTAX_PROTO3) {
    *error =
        "Can only generate PHP code for proto3 .proto files.\n"
        "Please add 'syntax = \"proto3\";' to the top of your .proto file.\n";
    return false;
  }

  GenerateFile(file, options, generator_context);

  return true;
}

bool Generator::GenerateAll(const std::vector<const FileDescriptor*>& files,
                            const std::string& parameter,
                            GeneratorContext* generator_context,
                            std::string* error) const {
  Options options;

  for (const auto& option : Split(parameter, ",", true)) {
    const std::vector<std::string> option_pair = Split(option, "=", true);
    if (HasPrefixString(option_pair[0], "aggregate_metadata")) {
      options.aggregate_metadata = true;
      for (const auto& prefix : Split(option_pair[1], "#", false)) {
        options.aggregate_metadata_prefixes.emplace(prefix);
        GOOGLE_LOG(INFO) << prefix;
      }
    } else if (option_pair[0] == "internal") {
      options.is_descriptor = true;
    } else if (option_pair[0] == "internal_generate_c_wkt") {
      GenerateCWellKnownTypes(files, generator_context);
    } else {
      GOOGLE_LOG(FATAL) << "Unknown codegen option: " << option_pair[0];
    }
  }

  for (auto file : files) {
    if (!Generate(file, options, generator_context, error)) {
      return false;
    }
  }

  return true;
}

}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
