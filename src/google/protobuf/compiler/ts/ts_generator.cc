// Protocol Buffers - Google's data interchange format
// Copyright 2020 Levi Behunin.  All rights reserved.
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

#include <assert.h>
#include <google/protobuf/compiler/scc.h>
#include <google/protobuf/compiler/ts/ts_generator.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/stringprintf.h>

#include <map>
#include <string>

namespace google {
namespace protobuf {
namespace compiler {
namespace ts {

bool StrEndsWith(StringPiece sp, StringPiece x) {
  return sp.size() >= x.size() && sp.substr(sp.size() - x.size()) == x;
}

// Returns a copy of |filename| with any trailing ".protodevel" or ".proto
// suffix stripped.
std::string StripProto(const std::string& filename) {
  const char* suffix =
      StrEndsWith(filename, ".protodevel") ? ".protodevel" : ".proto";

  return StripSuffixString(filename, suffix);
}

// Given a filename like foo/bar/baz.proto, returns the root directory
// path ../../
std::string GetRootPath(const std::string& from_filename,
                        const std::string& to_filename) {
  size_t slashes = std::count(from_filename.begin(), from_filename.end(), '/');
  if (slashes == 0) {
    return "./";
  }

  std::string result = "";
  for (size_t i = 0; i < slashes; i++) {
    result += "../";
  }

  return result;
}

// Returns the name of the message with a leading dot and taking into account
// nesting, for example ".OuterMessage.InnerMessage", or returns empty if
// descriptor is null. This function does not handle namespacing, only message
// nesting.
std::string GetNestedMessageName(const Descriptor* descriptor) {
  if (descriptor == nullptr) {
    return "";
  }

  return StripPrefixString(descriptor->full_name(),
                           descriptor->file()->package() + '.');
}

// Returns the fully normalized JavaScript path for the given
// message descriptor.
std::string GetMessagePath(const Descriptor* descriptor, bool type) {
  std::string parent = GetNestedMessageName(descriptor->containing_type());

  if (parent == "") {
    return descriptor->name();
  }

  if (type) {
    return "InstanceType<" + parent + "[\'" + descriptor->name() + "\']>";
  }

  return parent + '.' + descriptor->name();
}

// Returns the fully normalized JavaScript path for the given
// enumeration descriptor.
std::string GetEnumPath(const EnumDescriptor* enum_descriptor) {
  std::string parent = GetNestedMessageName(enum_descriptor->containing_type());
  if (parent == "") {
    return enum_descriptor->name();
  }

  return parent + "[\'" + enum_descriptor->name() + "\']" + "[\'" +
         enum_descriptor->value(0)->name() + "\']";
}

std::string TSTypeName(const FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_BOOL:
      return "boolean";
    case FieldDescriptor::CPPTYPE_INT32:
    case FieldDescriptor::CPPTYPE_UINT32:
    case FieldDescriptor::CPPTYPE_FLOAT:
    case FieldDescriptor::CPPTYPE_DOUBLE:
      return "number";
    case FieldDescriptor::CPPTYPE_INT64:
    case FieldDescriptor::CPPTYPE_UINT64:
      return "bigint";
    case FieldDescriptor::CPPTYPE_STRING:
      if (field->type() == FieldDescriptor::TYPE_BYTES) {
        return "Uint8Array";
      }
      return "string";
    case FieldDescriptor::CPPTYPE_ENUM:
      return GetEnumPath(field->enum_type());
    case FieldDescriptor::CPPTYPE_MESSAGE:
      return GetMessagePath(field->message_type(), true);
    default:
      return "";
  }
}

std::string TSFieldType(const FieldDescriptor* field) {
  if (field->is_repeated()) {
    if (field->type() == FieldDescriptor::TYPE_BYTES) {
      return "Array<Uint8Array>";
    } else if (field->type() == FieldDescriptor::TYPE_ENUM) {
      return "Array<" + GetEnumPath(field->enum_type()) + ">";
    } else if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
      return "Array<" + GetMessagePath(field->message_type(), true) + ">";
    }
    return "Array<" + TSTypeName(field) + ">";
  }

  if (field->type() == FieldDescriptor::TYPE_ENUM) {
    return GetEnumPath(field->enum_type());
  }

  if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
    return GetMessagePath(field->message_type(), true);
  }

  return TSTypeName(field);
}

std::string ProtoTypeName(const FieldDescriptor* field) {
  switch (field->type()) {
    case FieldDescriptor::TYPE_BOOL:
      return "bool";
    case FieldDescriptor::TYPE_INT32:
      return "int32";
    case FieldDescriptor::TYPE_UINT32:
      return "uint32";
    case FieldDescriptor::TYPE_SINT32:
      return "sint32";
    case FieldDescriptor::TYPE_FIXED32:
      return "fixed32";
    case FieldDescriptor::TYPE_SFIXED32:
      return "sfixed32";
    case FieldDescriptor::TYPE_INT64:
      return "int64";
    case FieldDescriptor::TYPE_UINT64:
      return "uint64";
    case FieldDescriptor::TYPE_SINT64:
      return "sint64";
    case FieldDescriptor::TYPE_FIXED64:
      return "fixed64";
    case FieldDescriptor::TYPE_SFIXED64:
      return "sfixed64";
    case FieldDescriptor::TYPE_FLOAT:
      return "float";
    case FieldDescriptor::TYPE_DOUBLE:
      return "double";
    case FieldDescriptor::TYPE_STRING:
      return "string";
    case FieldDescriptor::TYPE_BYTES:
      return "bytes";
    case FieldDescriptor::TYPE_ENUM:
      return GetEnumPath(field->enum_type());
    case FieldDescriptor::TYPE_MESSAGE:
      return GetMessagePath(field->message_type(), false);
    default:
      return "";
  }
}

std::string TSBinaryReadWriteMethodName(const FieldDescriptor* field,
                                        bool is_writer) {
  std::string name = field->type_name();
  if (name[0] >= 'a' && name[0] <= 'z') {
    name[0] = (name[0] - 'a') + 'A';
  }
  if (field->is_packed()) {
    name = "Packed" + name;
  } else if (is_writer && field->is_repeated()) {
    name = "Repeated" + name;
  }
  return name;
}

std::string RelativeTypeName(const FieldDescriptor* field) {
  assert(field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM ||
         field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE);
  // For a field with an enum or message type, compute a name relative to the
  // path name of the message type containing this field.
  std::string package = field->file()->package();
  std::string containing_type = field->containing_type()->full_name() + '.';
  std::string type = (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM)
                         ? field->enum_type()->full_name()
                         : field->message_type()->full_name();

  // |prefix| is advanced as we find separators '.' past the common package
  // prefix that yield common prefixes in the containing type's name and this
  // type's name.
  int prefix = 0;
  for (int i = 0; i < type.size() && i < containing_type.size(); i++) {
    if (type[i] != containing_type[i]) {
      break;
    }
    if (type[i] == '.' && i >= package.size()) {
      prefix = i + 1;
    }
  }

  return type.substr(prefix);
}

static const int kMapKeyField = 1;
static const int kMapValueField = 2;

const FieldDescriptor* MapFieldKey(const FieldDescriptor* field) {
  assert(field->is_map());
  return field->message_type()->FindFieldByNumber(kMapKeyField);
}

const FieldDescriptor* MapFieldValue(const FieldDescriptor* field) {
  assert(field->is_map());
  return field->message_type()->FindFieldByNumber(kMapValueField);
}

std::string FieldDefinition(const FieldDescriptor* field) {
  if (field->is_map()) {
    const FieldDescriptor* key_field = MapFieldKey(field);
    const FieldDescriptor* value_field = MapFieldValue(field);
    std::string key_type = ProtoTypeName(key_field);
    std::string value_type;
    if (value_field->type() == FieldDescriptor::TYPE_ENUM ||
        value_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      value_type = RelativeTypeName(value_field);
    } else {
      value_type = ProtoTypeName(value_field);
    }

    return StringPrintf("map<%s, %s> %s = %d;", key_type.c_str(),
                        value_type.c_str(), field->name().c_str(),
                        field->number());
  } else {
    std::string qualifier =
        field->is_repeated() ? "repeated"
                             : (field->is_optional() ? "optional" : "required");
    std::string type, name;
    if (field->type() == FieldDescriptor::TYPE_ENUM ||
        field->type() == FieldDescriptor::TYPE_MESSAGE) {
      type = RelativeTypeName(field);
      name = field->name();
    } else if (field->type() == FieldDescriptor::TYPE_GROUP) {
      type = "group";
      name = field->message_type()->name();
    } else {
      type = ProtoTypeName(field);
      name = field->name();
    }

    return StringPrintf("%s %s %s = %d;", qualifier.c_str(), type.c_str(),
                        name.c_str(), field->number());
  }
}

void SaveTheChildren(std::vector<const SCC*> children, const Descriptor* desc,
                     std::set<const Descriptor*>* messages,
                     std::set<const EnumDescriptor*>* enums) {
  for (auto one_scc : children) {
    for (auto des : one_scc->descriptors) {
      if (des == desc) continue;
      if (des->containing_type() == nullptr) messages->insert(des);
      for (int i = 0; i < des->field_count(); i++) {
        if (des->field(i)->type() == FieldDescriptor::TYPE_ENUM) {
          if (des->field(i)->enum_type()->containing_type() == nullptr) {
            enums->insert(des->field(i)->enum_type());
          }
        }
      }
    }

    if (!one_scc->children.empty()) {
      SaveTheChildren(one_scc->children, desc, messages, enums);
    }
  }
}

void Generator::Header(std::map<std::string, std::string>* vars,
                       io::Printer* printer) const {
  printer->Print(*vars, "// source: ~name~\n");
  printer->Print("// GENERATED CODE -- DO NOT EDIT!\n\n");
  printer->Print(
      "import { BinaryReader, BinaryWriter, Message } from 'grpc-web-ts';\n");
}

void Generator::ES6Imports(const FileDescriptor* file, io::Printer* printer,
                           std::map<std::string, std::string>* vars) const {
  for (int i = 0; i < file->dependency_count(); i++) {
    vars->insert_or_assign(
        "file", GetRootPath(file->name(), file->dependency(i)->name()) +
                    StripProto(file->dependency(i)->name()));

    if (file->dependency(i)->message_type_count() > 1) {
      const int last = file->dependency(i)->message_type_count() - 1;

      if (file->dependency(i)->message_type_count() > 4) {
        printer->Print("import {\n");

        for (int n = 0; n < file->dependency(i)->message_type_count(); n++) {
          vars->insert_or_assign("name",
                                 file->dependency(i)->message_type(n)->name());
          if (n == last) {
            printer->Print(*vars, "  ~name~\n");
          } else {
            printer->Print(*vars, "  ~name~,\n");
          }
        }

        printer->Print(*vars, "} from '~file~';\n");
      } else {
        std::string name;
        for (int n = 0; n < file->dependency(i)->message_type_count(); n++) {
          if (n == last) {
            name += file->dependency(i)->message_type(n)->name();
          } else {
            name += file->dependency(i)->message_type(n)->name() + ", ";
          }
        }

        vars->insert_or_assign("name", name);

        printer->Print(*vars, "import { ~name~ } from '~file~';\n");
      }
    } else {
      vars->insert_or_assign("name",
                             file->dependency(i)->message_type(0)->name());

      printer->Print(*vars, "import { ~name~ } from '~file~';\n");
    }
  }
}

void Generator::ClassesAndEnums(
    io::Printer* printer, const FileDescriptor* file,
    std::map<std::string, std::string>* vars) const {
  for (int i = 0; i < file->message_type_count(); i++) {
    vars->insert_or_assign("name", file->message_type(i)->name());

    ClassConstructor(printer, vars);

    printer->Indent();

    Class(printer, file->message_type(i), vars);

    printer->Print("\n\n");
  }

  for (int i = 0; i < file->enum_type_count(); i++) {
    Enum(printer, file->enum_type(i), vars);
  }
}

void Generator::Class(io::Printer* printer, const Descriptor* desc,
                      std::map<std::string, std::string>* vars) const {
  for (int i = 0; i < desc->nested_type_count(); i++) {
    if (desc->nested_type(i)->options().map_entry()) {
      continue;
    }
    Nested(printer, desc->nested_type(i), vars);
  }
  for (int i = 0; i < desc->enum_type_count(); i++) {
    Enum(printer, desc->enum_type(i), vars);
  }

  Fields(printer, desc, vars);
  SerializeBinary(printer, desc, vars);
  DeserializeBinary(printer, desc, vars);

  printer->Outdent();
  printer->Print("}");
}

void Generator::ClassConstructor(
    io::Printer* printer, std::map<std::string, std::string>* vars) const {
  printer->Print(*vars,
                 "/**\n"
                 " * Generated by TSPbCodeGenerator\n"
                 " */\n"
                 "export class ~name~ extends Message {\n");
}

void Generator::OneofCaseDefinition(
    io::Printer* printer, const OneofDescriptor* oneof,
    std::map<std::string, std::string>* vars) const {
  vars->insert_or_assign("name", oneof->name());

  printer->Print(*vars, "~name~ = class ~name~ {\n");
  printer->Indent();

  for (int i = 0; i < oneof->field_count(); i++) {
    if (oneof->field(i)->is_extension()) {
      continue;
    }
    vars->insert_or_assign("fielddef", FieldDefinition(oneof->field(i)));

    printer->Print(*vars,
                   "/**\n"
                   " * ~fielddef~\n"
                   " */\n");
    vars->insert_or_assign("oneof_name", oneof->field(i)->name());
    vars->insert_or_assign("type", TSFieldType(oneof->field(i)));

    printer->Print(*vars, "~oneof_name~?: ~type~;\n");
  }

  printer->Outdent();
  printer->Print("};\n");
  printer->Print(*vars,
                 "oneof_~name~ = new Proxy(new this.~name~(), {\n"
                 "  set: function(obj, prop, value) {\n"
                 "    for (let key in obj) {\n"
                 "      if (!Reflect.deleteProperty(obj, key)) {\n"
                 "        return false;\n"
                 "      }\n"
                 "    }\n"
                 "    return Reflect.set(obj, prop, value);\n"
                 "  }\n"
                 "});\n");
}

void Generator::Field(io::Printer* printer, const FieldDescriptor* field,
                      std::map<std::string, std::string>* vars) const {
  vars->insert_or_assign("fielddef", FieldDefinition(field));
  vars->insert_or_assign("name", field->name());

  printer->Print(*vars,
                 "/**\n"
                 " * ~fielddef~\n"
                 " */\n");
  printer->Print(*vars, "~name~?: ");

  if (field->is_map()) {
    vars->insert_or_assign("key_type", TSFieldType(MapFieldKey(field)));
    vars->insert_or_assign("value_type", TSFieldType(MapFieldValue(field)));

    printer->Print(*vars, "Map<~key_type~, ~value_type~>;\n");

    return;
  } else if (field->is_repeated()) {
    vars->insert_or_assign("type", TSFieldType(field));

    printer->Print(*vars, "~type~;\n");

    return;
  } else {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32:
      case FieldDescriptor::CPPTYPE_UINT32:
      case FieldDescriptor::CPPTYPE_FLOAT:
      case FieldDescriptor::CPPTYPE_DOUBLE:
        printer->Print("number;\n");
        break;
      case FieldDescriptor::CPPTYPE_INT64:
      case FieldDescriptor::CPPTYPE_UINT64:
        printer->Print("bigint;\n");
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
      case FieldDescriptor::CPPTYPE_ENUM:
        vars->insert_or_assign("type", TSFieldType(field));
        printer->Print(*vars, "~type~;\n");
        break;
      case FieldDescriptor::CPPTYPE_STRING:
        if (field->type() == field->TYPE_BYTES) {
          printer->Print("Uint8Array;\n");
          break;
        }
        printer->Print("string;\n");
        break;
      case FieldDescriptor::CPPTYPE_BOOL:
        printer->Print("boolean;\n");
        break;
      default:
        assert(false);
        break;
    }
  }
}

void Generator::Fields(io::Printer* printer, const Descriptor* desc,
                       std::map<std::string, std::string>* vars) const {
  for (int i = 0; i < desc->field_count(); i++) {
    if (!desc->field(i)->is_extension()) {
      if (desc->field(i)->containing_oneof() != nullptr) {
        OneofCaseDefinition(printer, desc->field(i)->containing_oneof(), vars);
        i += desc->field(i)->containing_oneof()->field_count() - 1;
      } else {
        Field(printer, desc->field(i), vars);
      }
    }
  }
}

void Generator::DeserializeBinary(
    io::Printer* printer, const Descriptor* desc,
    std::map<std::string, std::string>* vars) const {
  bool tmp = false;

  printer->Print("// deez cereal eyes\n");
  printer->Print("deserializeBinaryFromReader(reader: BinaryReader) {\n");
  printer->Indent();

  if (desc->nested_type_count() > 0) {
    printer->Print(
        "let tmp: any;\n"
        "let entry: Array<any>;\n");
    tmp = true;
  }

  printer->Print("while (reader.NextField) {\n");
  printer->Indent();

  printer->Print("switch (reader.FieldNumber) {\n");
  printer->Indent();

  for (int i = 0; i < desc->field_count(); i++) {
    if (!desc->field(i)->is_extension()) {
      if (desc->field(i)->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
          !tmp) {
        DeserializeBinaryField(printer, desc->field(i), vars, true);
        tmp = true;
      } else {
        DeserializeBinaryField(printer, desc->field(i), vars);
      }
    }
  }

  printer->Print(
      "default:\n"
      "  reader.skipField();\n"
      "  break;\n");
  printer->Outdent();
  printer->Print("}\n");
  printer->Outdent();

  printer->Print(
      "}\n"
      "return this;\n");
  printer->Outdent();
  printer->Print("}\n");
}

void Generator::DeserializeBinaryField(io::Printer* printer,
                                       const FieldDescriptor* field,
                                       std::map<std::string, std::string>* vars,
                                       bool tmp) const {
  vars->insert_or_assign("index", StrCat(field->number()));

  printer->Print(*vars, "case ~index~:\n");

  if (field->containing_oneof() != nullptr) {
    vars->insert_or_assign(
        "name",
        "oneof_" + field->containing_oneof()->name() + '.' + field->name());
  } else {
    vars->insert_or_assign("name", field->name());
  }

  printer->Indent();

  if (field->is_map()) {
    vars->insert_or_assign("key_type",
                           std::to_string(MapFieldKey(field)->type()));

    const FieldDescriptor* value_field = MapFieldValue(field);
    vars->insert_or_assign("value_type", std::to_string(value_field->type()));

    if (value_field->type() == FieldDescriptor::TYPE_MESSAGE) {
      if (value_field->message_type()->containing_type() != nullptr) {
        vars->insert_or_assign(
            "message",
            "this" +
                StripPrefixString(
                    GetMessagePath(value_field->message_type(), false),
                    value_field->message_type()->containing_type()->name()));

        printer->Print(*vars,
                       "tmp = new ~message~();\n"
                       "entry = reader.Map(~key_type~, ~value_type~, tmp);\n"
                       "if (this.~name~) {\n"
                       "  this.~name~.set(entry[0], entry[1]);\n"
                       "  break;\n"
                       "}\n"
                       "this.~name~ = new Map();\n"
                       "this.~name~.set(entry[0], entry[1]);\n");
      } else {
        vars->insert_or_assign(
            "message", GetMessagePath(value_field->message_type(), false));

        printer->Print(*vars,
                       "tmp = new ~message~();\n"
                       "entry = reader.Map(~key_type~, ~value_type~, tmp);\n"
                       "if (this.~name~) {\n"
                       "  this.~name~.set(entry[0], entry[1]);\n"
                       "  break;\n"
                       "}\n"
                       "this.~name~ = new Map();\n"
                       "this.~name~.set(entry[0], entry[1]);\n");
      }
    } else {
      printer->Print(*vars,
                     "entry = reader.Map(~key_type~, ~value_type~);\n"
                     "if (this.~name~) {\n"
                     "  this.~name~.set(entry[0], entry[1]);\n"
                     "  break;\n"
                     "}\n"
                     "this.~name~ = new Map();\n"
                     "this.~name~.set(entry[0], entry[1]);\n");
    }
  } else if (field->is_repeated() && !field->is_packed()) {
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (tmp) {
        printer->Print("var tmp: any;\n");
      }
      if (field->message_type()->containing_type() != nullptr) {
        vars->insert_or_assign(
            "message",
            "this" + StripPrefixString(
                         GetMessagePath(field->message_type(), false),
                         field->message_type()->containing_type()->name()));

        printer->Print(*vars,
                       "tmp = new ~message~();\n"
                       "reader.Message(tmp);\n"
                       "if (this.~name~) {\n"
                       "  this.~name~.push(tmp);\n"
                       "  break;\n"
                       "}\n"
                       "this.~name~ = new Array();\n"
                       "this.~name~.push(tmp);\n");
      } else {
        vars->insert_or_assign("message",
                               GetMessagePath(field->message_type(), false));

        printer->Print(*vars,
                       "tmp = new ~message~();\n"
                       "reader.Message(tmp);\n"
                       "if (this.~name~) {\n"
                       "  this.~name~.push(tmp);\n"
                       "  break;\n"
                       "}\n"
                       "this.~name~ = new Array();\n"
                       "this.~name~.push(tmp);\n");
      }
    } else {
      vars->insert_or_assign("method", TSBinaryReadWriteMethodName(
                                           field, /* is_writer = */ false));

      printer->Print(*vars,
                     "if (this.~name~) {\n"
                     "  this.~name~.push(reader.~method~());\n"
                     "  break;\n"
                     "}\n"
                     "this.~name~ = new Array();\n"
                     "this.~name~.push(reader.~method~());\n");
    }
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    if (field->message_type()->containing_type() != nullptr) {
      vars->insert_or_assign(
          "message",
          "this" + StripPrefixString(
                       GetMessagePath(field->message_type(), false),
                       field->message_type()->containing_type()->name()));

      printer->Print(*vars,
                     "if (this.~name~) {\n"
                     "  reader.Message(this.~name~);\n"
                     "  break;\n"
                     "}\n"
                     "this.~name~ = new ~message~();\n"
                     "reader.Message(this.~name~);\n");
    } else {
      vars->insert_or_assign("message",
                             GetMessagePath(field->message_type(), false));
      printer->Print(*vars,
                     "if (this.~name~) {\n"
                     "  reader.Message(this.~name~);\n"
                     "  break;\n"
                     "}\n"
                     "this.~name~ = new ~message~();\n"
                     "reader.Message(this.~name~);\n");
    }
  } else {
    vars->insert_or_assign(
        "method", TSBinaryReadWriteMethodName(field, /* is_writer = */ false));

    printer->Print(*vars, "this.~name~ = reader.~method~();\n");
  }

  printer->Print("break;\n");
  printer->Outdent();
}

void Generator::SerializeBinary(
    io::Printer* printer, const Descriptor* desc,
    std::map<std::string, std::string>* vars) const {
  printer->Print("// cereal eyes\n");
  printer->Print("serializeBinaryToWriter(writer: BinaryWriter) {\n");
  printer->Indent();

  for (int i = 0; i < desc->field_count(); i++) {
    if (!desc->field(i)->is_extension()) {
      SerializeBinaryField(printer, desc->field(i), vars);
    }
  }

  printer->Outdent();
  printer->Print("}\n");
}

void Generator::SerializeBinaryField(
    io::Printer* printer, const FieldDescriptor* field,
    std::map<std::string, std::string>* vars) const {
  if (field->containing_oneof() != nullptr) {
    vars->insert_or_assign(
        "name",
        "oneof_" + field->containing_oneof()->name() + '.' + field->name());
  } else {
    vars->insert_or_assign("name", field->name());
  }
  if (field->is_map()) {
    printer->Print(*vars, "if (this.~name~ && this.~name~.size > 0) {\n");
  } else if (field->is_repeated()) {
    printer->Print(*vars, "if (this.~name~ && this.~name~.length > 0) {\n");
  } else {
    switch (field->cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32:
      case FieldDescriptor::CPPTYPE_UINT32:
        printer->Print(*vars, "if (this.~name~ && this.~name~ !== 0) {\n");
        break;
      case FieldDescriptor::CPPTYPE_INT64:
      case FieldDescriptor::CPPTYPE_UINT64:
        printer->Print(*vars, "if (this.~name~ && this.~name~ !== 0n) {\n");
        break;
      case FieldDescriptor::CPPTYPE_FLOAT:
      case FieldDescriptor::CPPTYPE_DOUBLE:
        printer->Print(*vars, "if (this.~name~ && this.~name~ !== 0.0) {\n");
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
      case FieldDescriptor::CPPTYPE_BOOL:
      case FieldDescriptor::CPPTYPE_ENUM:
        printer->Print(*vars, "if (this.~name~) {\n");
        break;
      case FieldDescriptor::CPPTYPE_STRING:
        printer->Print(*vars, "if (this.~name~ && this.~name~.length > 0) {\n");
        break;
      default:
        assert(false);
        break;
    }
  }

  printer->Indent();

  vars->insert_or_assign("index", StrCat(field->number()));

  if (field->is_map()) {
    vars->insert_or_assign("key_type",
                           std::to_string(MapFieldKey(field)->type()));
    vars->insert_or_assign("value_type",
                           std::to_string(MapFieldValue(field)->type()));
    printer->Print(*vars,
                   "writer.Map(~index~, this.~name~, ~key_type~, "
                   "~value_type~);\n");
  } else {
    vars->insert_or_assign(
        "method", TSBinaryReadWriteMethodName(field, /* is_writer = */ true));
    printer->Print(*vars, "writer.~method~(~index~, this.~name~);\n");
  }

  printer->Outdent();
  printer->Print("}\n");
}

void Generator::Nested(io::Printer* printer, const Descriptor* desc,
                       std::map<std::string, std::string>* vars) const {
  vars->insert_or_assign("name", desc->name());

  printer->Print(*vars, "~name~ = class ~name~ extends Message {\n");
  printer->Indent();

  Class(printer, desc, vars);

  printer->Print(";\n");
}

void Generator::Enum(io::Printer* printer, const EnumDescriptor* enumdesc,
                     std::map<std::string, std::string>* vars) const {
  vars->insert_or_assign("name", enumdesc->name());

  const int last = enumdesc->value_count() - 1;

  if (enumdesc->containing_type() == nullptr) {
    printer->Print(*vars, "export enum ~name~ {\n");
    printer->Indent();

    for (int i = 0; i < enumdesc->value_count(); i++) {
      vars->insert_or_assign("name", enumdesc->value(i)->name());

      printer->Print(*vars, "~name~");
      if (i == enumdesc->value(i)->number()) {
        printer->Print("");
      } else {
        printer->PrintRaw(" = " + StrCat(enumdesc->value(i)->number()));
      }
      if (i == last) {
        printer->Print("\n");
      } else {
        printer->Print(",\n");
      }
    }

    printer->Outdent();
    printer->Print("}\n\n");
  } else {
    printer->Print(*vars, "~name~ = Object.freeze({\n");
    printer->Indent();

    for (int i = 0; i < enumdesc->value_count(); i++) {
      vars->insert_or_assign("name", enumdesc->value(i)->name());
      vars->insert_or_assign("value", StrCat(enumdesc->value(i)->number()));

      printer->Print(*vars,
                     "get ~name~() {\n"
                     "  return ~value~;\n"
                     "}");
      if (i == last) {
        printer->Print("\n");
      } else {
        printer->Print(",\n");
      }
    }

    printer->Outdent();
    printer->Print("});\n");
  }
}

void Generator::ServiceUtil(io::Printer* printer) const {
  printer->Print(
      "function frameRequest(req: number[]): Uint8Array {\n"
      "  const frame = new ArrayBuffer(req.length + 5);\n"
      "  new DataView(frame, 1, 4).setUint32(0, req.length, false);\n"
      "  new Uint8Array(frame, 5).set(req);\n"
      "  return new Uint8Array(frame);\n"
      "}\n\n");
  printer->Print(
      "export interface RpcOptions {\n"
      "  abort: AbortSignal;\n"
      "  headers?: Headers;\n"
      "  host: string;\n"
      "}\n\n");
}

void Generator::ServiceClass(io::Printer* printer, const ServiceDescriptor* des,
                             std::map<std::string, std::string>* vars) const {
  vars->insert_or_assign("service_name", des->name());
  vars->insert_or_assign("full_name", des->full_name());

  printer->Print("export class ");
  printer->Print(*vars, "~service_name~Client {\n");
  printer->Indent();
  printer->Print(*vars,
                 "abort: AbortSignal;\n"
                 "headers?: Headers;\n"
                 "host: string;\n"
                 "serviceName = '~full_name~';\n"
                 "constructor(options: RpcOptions) {\n");
  printer->Indent();
  printer->Print("this.abort = options.abort;\n");
  printer->Print("if (options.headers) {\n");
  printer->Indent();
  printer->Print(
      "this.headers = options.headers;\n"
      "this.headers.set('content-type', 'application/grpc-web-ts');\n");
  printer->Outdent();
  printer->Print("} else {\n");
  printer->Indent();
  printer->Print(
      "this.headers = new Headers();\n"
      "this.headers.set('content-type', 'application/grpc-web-ts');\n");
  printer->Outdent();
  printer->Print(
      "}\n"
      "this.host = options.host;\n");
  printer->Outdent();
  printer->Print("}\n\n");

  for (int i = 0; i < des->method_count(); i++) {
    const MethodDescriptor* method = des->method(i);

    vars->insert_or_assign("method_name", method->name());
    vars->insert_or_assign("input_type", method->input_type()->name());
    vars->insert_or_assign("output_type", method->output_type()->name());

    if (!method->client_streaming() && !method->server_streaming()) {
      printer->Print(
          *vars,
          "async ~method_name~(msg: ~input_type~): Promise<~output_type~> {\n");
      printer->Indent();
      printer->Print(
          *vars,
          "const framed = frameRequest(msg.serializeBinary());\n"
          "const url = `${this.host}/${this.serviceName}/~method_name~`;\n"
          "const res: Promise<~output_type~> = new Promise((resolve, reject) "
          "=> {\n");
      printer->Indent();
      printer->Print("fetch(url, {\n");
      printer->Indent();
      printer->Print(
          "method: 'POST',\n"
          "body: framed,\n"
          "credentials: 'include',\n"
          "headers: this.headers,\n"
          "signal: this.abort\n");
      printer->Outdent();
      printer->Print("})\n");
      printer->Indent();
      printer->Print(".then((res) => {\n");
      printer->Indent();
      printer->Print("if (res.ok) {\n");
      printer->Indent();
      printer->Print("if (res.body) {\n");
      printer->Indent();
      printer->Print(*vars,
                     "const reader = res.body.getReader();\n"
                     "const out = new ~output_type~();\n"
                     "reader.read().then(function noms(result): any {\n");
      printer->Indent();
      printer->Print(
          "if (result.done) {\n"
          "  return;\n"
          "} else if (result.value) {\n"
          "  try {\n"
          "    out.Unary(result.value);\n"
          "  } catch (err) {\n"
          "    reject(err);\n"
          "  }\n"
          "  return reader.read().then(noms);\n"
          "}\n");
      printer->Outdent();
      printer->Print(
          "});\n"
          "resolve(out);\n");
      printer->Outdent();
      printer->Print("}\n");
      printer->Outdent();
      printer->Print(
          "} else {\n"
          "  reject(res.statusText);\n"
          "}\n");
      printer->Outdent();
      printer->Print(
          "})\n"
          ".catch((err) => {\n"
          "  reject(err);\n"
          "});\n");
      printer->Outdent();
      printer->Outdent();
      printer->Print(
          "});\n"
          "return await res;\n");
      printer->Outdent();
      printer->Print("}\n\n");
    }
    if (!method->client_streaming() && method->server_streaming()) {
      printer->Print(*vars,
                     "async ~method_name~(msg: ~input_type~, arr: "
                     "Array<~output_type~>) {\n");
      printer->Indent();
      printer->Print(
          *vars,
          "const framed = frameRequest(msg.serializeBinary());\n"
          "const url = `${this.host}/${this.serviceName}/~method_name~`;\n");
      printer->Print("await fetch(url, {\n");
      printer->Indent();
      printer->Print(
          "method: 'POST',\n"
          "body: framed,\n"
          "credentials: 'include',\n"
          "headers: this.headers,\n"
          "signal: this.abort\n");
      printer->Outdent();
      printer->Print("}).then((res) => {\n");
      printer->Indent();
      printer->Print("if (res.ok) {\n");
      printer->Indent();
      printer->Print("if (res.body) {\n");
      printer->Indent();
      printer->Print(
          "const reader = res.body.getReader();\n"
          "reader.read().then(function noms(result): any {\n");
      printer->Indent();
      printer->Print(
          *vars,
          "if (result.done) {\n"
          "  return;\n"
          "} else if (result.value) {\n"
          "  try {\n"
          "    ~output_type~.Stream(result.value, ~output_type~, arr);\n"
          "  } catch (err) {\n"
          "    throw err;\n"
          "  }\n"
          "  return reader.read().then(noms);\n"
          "}\n");
      printer->Outdent();
      printer->Print("});\n");
      printer->Outdent();
      printer->Print("}\n");
      printer->Outdent();
      printer->Print(
          "} else {\n"
          "  throw res.statusText;\n"
          "}\n");
      printer->Outdent();
      printer->Print("});\n");
      printer->Outdent();
      printer->Print("}\n");
    }
  }

  printer->Outdent();
  printer->Print("}\n\n");
}

bool GeneratorOptions::ParseFromOptions(
    const std::vector<std::pair<std::string, std::string>>& options,
    std::string* error) {
  for (int i = 0; i < options.size(); i++) {
    if (options[i].first == "output_dir") {
      output_dir = options[i].second;
    } else if (options[i].first == "name") {
      name = options[i].second;
    } else if (options[i].first == "library") {
      library = options[i].second;
    } else if (options[i].first == "ratio") {
      if (options[i].second == "A~1") {
        ratio = options[i].second;
      } else if (options[i].second == "S~1") {
        ratio = options[i].second;
      } else {
        *error = "Unexpected option value for ratio";
        return false;
      }
    } else if (options[i].first == "deps") {
      if (options[i].second == "print") {
        deps = options[i].second;
      } else if (options[i].second == "import") {
        deps = options[i].second;
      } else if (options[i].second == "include") {
        deps = options[i].second;
      } else if (options[i].second == "") {
        *error = "Unexpected option value for deps";
        return false;
      }
    } else if (options[i].first == "services") {
      if (options[i].second != "") {
        *error = "Unexpected option value for services";
        return false;
      }
      services = true;
    } else {
      // Assume any other option is an output directory, as long as it is a bare
      // `key` rather than a `key=value` option.
      if (options[i].second != "") {
        *error = "Unknown option: " + options[i].first;
        return false;
      }
      output_dir = options[i].first;
    }
  }

  return true;
}

GeneratorOptions::OutputMode GeneratorOptions::output_mode() const {
  if (ratio == "A~1") {
    return kEverythingInOneFile;
  } else if (ratio == "S~1") {
    return kOneOutputFilePerService;
  }

  return kOneOutputFilePerInputFile;
}

bool Generator::File(const FileDescriptor* file,
                     const GeneratorOptions& options,
                     GeneratorContext* context) const {
  const std::string filename = options.output_dir + '/' +
                               StripProto(file->name()) +
                               options.GetFileNameExtension();
  std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
  GOOGLE_CHECK(output);
  io::Printer printer(output.get(), '~');

  std::map<std::string, std::string> vars;

  if (options.deps == "print") {
    for (int i = 0; i < file->dependency_count(); i++) {
      const std::string filename = options.output_dir + '/' +
                                   StripProto(file->dependency(i)->name()) +
                                   options.GetFileNameExtension();
      std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
      GOOGLE_CHECK(output);
      io::Printer depsprinter(output.get(), '~');

      vars["name"] = file->dependency(i)->name();
      Header(&vars, &depsprinter);

      for (int m = 0; m < file->dependency(i)->message_type_count(); m++) {
        vars["name"] = file->dependency(i)->message_type(m)->name();
        ClassConstructor(&depsprinter, &vars);
        depsprinter.Indent();
        Class(&depsprinter, file->dependency(i)->message_type(m), &vars);
        depsprinter.Print("\n");
      }

      for (int e = 0; e < file->dependency(i)->enum_type_count(); e++) {
        Enum(&depsprinter, file->dependency(i)->enum_type(e), &vars);
      }

      if (depsprinter.failed()) {
        return false;
      }
    }
  }

  File(&printer, file, options, &vars);

  if (options.services) {
    ServiceUtil(&printer);
    for (int i = 0; i < file->service_count(); i++) {
      ServiceClass(&printer, file->service(i), &vars);
    }
  }

  if (printer.failed()) {
    return false;
  }

  return true;
}

void Generator::File(io::Printer* printer, const FileDescriptor* file,
                     const GeneratorOptions& options,
                     std::map<std::string, std::string>* vars) const {
  vars->insert_or_assign("name", file->name());
  Header(vars, printer);

  if (options.deps == "import" || options.deps == "print") {
    ES6Imports(file, printer, vars);
  } else if (options.deps == "include") {
    for (int i = 0; i < file->dependency_count(); i++) {
      for (int m = 0; m < file->dependency(i)->message_type_count(); m++) {
        vars->insert_or_assign("name",
                               file->dependency(i)->message_type(m)->name());
        ClassConstructor(printer, vars);
        printer->Indent();
        Class(printer, file->dependency(i)->message_type(m), vars);
        printer->Print("\n");
      }

      for (int e = 0; e < file->dependency(i)->enum_type_count(); e++) {
        Enum(printer, file->dependency(i)->enum_type(e), vars);
      }
    }
  }

  ClassesAndEnums(printer, file, vars);
}

struct DepsGenerator {
  std::vector<const Descriptor*> operator()(const Descriptor* desc) const {
    std::vector<const Descriptor*> deps;

    auto maybe_add = [&](const Descriptor* d) {
      if (d) deps.push_back(d);
    };

    for (int i = 0; i < desc->field_count(); i++) {
      if (!desc->field(i)->is_extension()) {
        maybe_add(desc->field(i)->message_type());
      }
    }

    for (int i = 0; i < desc->nested_type_count(); i++) {
      if (desc->nested_type(i)->options().map_entry())
        maybe_add(desc->nested_type(i)
                      ->FindFieldByNumber(kMapValueField)
                      ->message_type());
    }

    return deps;
  }
};

bool Generator::GenerateAll(const std::vector<const FileDescriptor*>& files,
                            const std::string& parameter,
                            GeneratorContext* context,
                            std::string* error) const {
  std::vector<std::pair<std::string, std::string>> option_pairs;
  ParseGeneratorParameter(parameter, &option_pairs);
  GeneratorOptions options;
  if (!options.ParseFromOptions(option_pairs, error)) {
    return false;
  }

  if (options.output_mode() == GeneratorOptions::kEverythingInOneFile) {
    const std::string filename = options.output_dir + '/' + options.name +
                                 "_pb" + options.GetFileNameExtension();
    std::unique_ptr<io::ZeroCopyOutputStream> output(context->Open(filename));
    GOOGLE_CHECK(output.get());
    io::Printer printer(output.get(), '~');

    std::map<std::string, std::string> vars;
    vars["name"] = options.name;

    Header(&vars, &printer);

    std::set<const Descriptor*> messages;
    std::set<const EnumDescriptor*> enums;

    for (int f = 0; f < files.size(); f++) {
      for (int d = 0; d < files[f]->dependency_count(); d++) {
        for (int i = 0; i < files[f]->dependency(d)->message_type_count();
             i++) {
          messages.insert(files[f]->dependency(d)->message_type(i));
        }
        for (int e = 0; e < files[f]->dependency(d)->enum_type_count(); e++) {
          enums.insert(files[f]->dependency(d)->enum_type(e));
        }
      }
    }

    for (auto edes : enums) {
      Enum(&printer, edes, &vars);
    }

    for (auto des : messages) {
      vars["name"] = des->name();
      ClassConstructor(&printer, &vars);
      printer.Indent();
      Class(&printer, des, &vars);
      printer.Print("\n\n");
    }

    for (int f = 0; f < files.size(); f++) {
      ClassesAndEnums(&printer, files[f], &vars);

      if (options.services) {
        ServiceUtil(&printer);
        for (int i = 0; i < files[f]->service_count(); i++) {
          ServiceClass(&printer, files[f]->service(i), &vars);
        }
      }
    }

    if (printer.failed()) {
      return false;
    }
  } else if (options.output_mode() ==
             GeneratorOptions::kOneOutputFilePerService) {
    std::map<std::string, std::string> vars;
    std::set<const Descriptor*> have_printed;
    SCCAnalyzer<DepsGenerator> analyzer;

    for (int i = 0; i < files.size(); i++) {
      for (int s = 0; s < files[i]->service_count(); s++) {
        const std::string filename = options.output_dir + '/' +
                                     files[i]->service(s)->name() + "_pb" +
                                     options.GetFileNameExtension();
        std::unique_ptr<io::ZeroCopyOutputStream> output(
            context->Open(filename));
        GOOGLE_CHECK(output.get());
        io::Printer printer(output.get(), '~');

        vars["name"] = files[i]->name();

        Header(&vars, &printer);

        for (int m = 0; m < files[i]->service(s)->method_count(); m++) {
          const Descriptor* in = files[i]->service(s)->method(m)->input_type();
          const Descriptor* out =
              files[i]->service(s)->method(m)->output_type();

          if (have_printed.count(in) == 0) {
            vars["name"] = in->name();
            ClassConstructor(&printer, &vars);
            printer.Indent();
            Class(&printer, in, &vars);
            printer.Print("\n\n");
            have_printed.insert(in);
          }

          if (have_printed.count(out) == 0) {
            vars["name"] = out->name();
            ClassConstructor(&printer, &vars);
            printer.Indent();
            Class(&printer, out, &vars);
            printer.Print("\n\n");
            have_printed.insert(out);
          }
        }

        std::set<const Descriptor*> messages;
        std::set<const EnumDescriptor*> enums;

        for (auto desc : have_printed) {
          const SCC* scc = analyzer.GetSCC(desc);

          for (auto one_desc : scc->descriptors) {
            if (one_desc == desc) continue;
            if (one_desc->containing_type() == nullptr)
              messages.insert(one_desc);
            for (int i = 0; i < one_desc->field_count(); i++) {
              if (one_desc->field(i)->type() == FieldDescriptor::TYPE_ENUM) {
                if (one_desc->field(i)->enum_type()->containing_type() ==
                    nullptr) {
                  enums.insert(one_desc->field(i)->enum_type());
                }
              }
            }
          }
          for (int i = 0; i < desc->field_count(); i++) {
            if (desc->field(i)->type() == FieldDescriptor::TYPE_ENUM) {
              if (desc->field(i)->enum_type()->containing_type() == nullptr) {
                enums.insert(desc->field(i)->enum_type());
              }
            }
          }

          SaveTheChildren(scc->children, desc, &messages, &enums);
        }

        for (auto des : messages) {
          vars["name"] = des->name();
          ClassConstructor(&printer, &vars);
          printer.Indent();
          Class(&printer, des, &vars);
          printer.Print("\n\n");
        }

        for (auto edes : enums) {
          Enum(&printer, edes, &vars);
        }

        if (printer.failed()) {
          return false;
        }

        ServiceUtil(&printer);
        ServiceClass(&printer, files[i]->service(s), &vars);
      }
    }
  } else { /* options.output_mode() == kOneOutputFilePerInputFile */
    for (int i = 0; i < files.size(); i++) {
      if (!File(files[i], options, context)) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace ts
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
