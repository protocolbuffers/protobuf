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

#include <sstream>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <google/protobuf/compiler/nodejs/nodejs_generator.h>

using google::protobuf::internal::scoped_ptr;

namespace google {
namespace protobuf {
namespace compiler {
namespace nodejs {

// Forward decls.
struct CommaGenerator;
std::string IntToString(uint32 value);
std::string StripDotProto(const std::string& proto_file);
std::string LabelForField(google::protobuf::FieldDescriptor* field);
std::string TypeName(google::protobuf::FieldDescriptor* field);
void GenerateField(const google::protobuf::FieldDescriptor* field,
                   google::protobuf::io::Printer* printer,
                   CommaGenerator* comma);
void GenerateOneof(const google::protobuf::OneofDescriptor* oneof,
                   google::protobuf::io::Printer* printer,
                   CommaGenerator* comma);
void GenerateMessage(const google::protobuf::Descriptor* message,
                     google::protobuf::io::Printer* printer,
                     CommaGenerator* comma);
void GenerateEnum(const google::protobuf::EnumDescriptor* en,
                  google::protobuf::io::Printer* printer,
                  CommaGenerator* comma);
void GenerateMessageAssignment(
    const std::string& prefix,
    const google::protobuf::Descriptor* message,
    google::protobuf::io::Printer* printer);
void GenerateEnumAssignment(
    const std::string& prefix,
    const google::protobuf::EnumDescriptor* en,
    google::protobuf::io::Printer* printer);

std::string IntToString(uint32 value) {
  std::ostringstream os;
  os << value;
  return os.str();
}

std::string StripDotProto(const std::string& proto_file) {
  int lastindex = proto_file.find_last_of(".");
  return proto_file.substr(0, lastindex);
}

std::string LabelForField(const google::protobuf::FieldDescriptor* field) {
  switch (field->label()) {
    case FieldDescriptor::LABEL_OPTIONAL: return "OPTIONAL";
    case FieldDescriptor::LABEL_REQUIRED: return "REQUIRED";
    case FieldDescriptor::LABEL_REPEATED: return "REPEATED";
    default: assert(false); return "";
  }
}

std::string TypeName(const google::protobuf::FieldDescriptor* field) {
  switch (field->cpp_type()) {
    case FieldDescriptor::CPPTYPE_INT32: return "INT32";
    case FieldDescriptor::CPPTYPE_INT64: return "INT64";
    case FieldDescriptor::CPPTYPE_UINT32: return "UINT32";
    case FieldDescriptor::CPPTYPE_UINT64: return "UINT64";
    case FieldDescriptor::CPPTYPE_DOUBLE: return "DOUBLE";
    case FieldDescriptor::CPPTYPE_FLOAT: return "FLOAT";
    case FieldDescriptor::CPPTYPE_BOOL: return "BOOL";
    case FieldDescriptor::CPPTYPE_ENUM: return "ENUM";
    case FieldDescriptor::CPPTYPE_STRING: return "STRING";
    case FieldDescriptor::CPPTYPE_MESSAGE: return "MESSAGE";
    default: assert(false); return "";
  }
}

// Abstract this out because some Generate*() functions may generate more than
// one item in a list (e.g., in the case of nested messages).
struct CommaGenerator {
  CommaGenerator(google::protobuf::io::Printer* printer)
      : printer_(printer), first_(true)  {}

  void BeforeItem() {
    if (!first_) {
      printer_->Print("\n,");
    } else {
      first_ = false;
    }
  }

  google::protobuf::io::Printer* printer_;
  bool first_;
};

void GenerateField(const google::protobuf::FieldDescriptor* field,
                   google::protobuf::io::Printer* printer,
                   CommaGenerator* comma) {

  comma->BeforeItem();
  printer->Print(
    "new protobuf.FieldDescriptor({\n"
    "  name: \"$name$\",\n"
    "  number: $number$,\n"
    "  label: protobuf.FieldDescriptor.LABEL_$label$,\n"
    "  type: protobuf.FieldDescriptor.TYPE_$type$",
    "label", LabelForField(field),
    "name", field->name(),
    "type", TypeName(field),
    "number", IntToString(field->number()));
  if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
    printer->Print(
      ",\n  subtype_name: \"$subtype$\"",
     "subtype", field->message_type()->full_name());
  } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
    printer->Print(
      ",\n  subtype_name: \"$subtype$\"",
      "subtype", field->enum_type()->full_name());
  }
  printer->Print("\n})");
}

void GenerateOneof(const google::protobuf::OneofDescriptor* oneof,
                   google::protobuf::io::Printer* printer,
                   CommaGenerator* comma) {

  comma->BeforeItem();
  printer->Print(
      "new protobuf.OneofDescriptor(\"$name$\", [\n",
      "name", oneof->name());
  printer->Indent();

  CommaGenerator subcomma(printer);
  for (int i = 0; i < oneof->field_count(); i++) {
    const FieldDescriptor* field = oneof->field(i);
    GenerateField(field, printer, &subcomma);
  }

  printer->Outdent();
  printer->Print("\n])");
}

void GenerateMessage(const google::protobuf::Descriptor* message,
                     google::protobuf::io::Printer* printer,
                     CommaGenerator* comma) {

  comma->BeforeItem();
  printer->Print(
    "new protobuf.Descriptor(\"$name$\", [\n",
    "name", message->full_name());
  printer->Indent();

  CommaGenerator field_comma(printer);
  for (int i = 0; i < message->field_count(); i++) {
    const FieldDescriptor* field = message->field(i);
    if (!field->containing_oneof()) {
      GenerateField(field, printer, &field_comma);
    }
  }

  printer->Outdent();
  printer->Print("\n], [\n");
  printer->Indent();

  CommaGenerator oneof_comma(printer);
  for (int i = 0; i < message->oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = message->oneof_decl(i);
    GenerateOneof(oneof, printer, &oneof_comma);
  }

  printer->Outdent();
  printer->Print(
    "], $mapentry$)",
    "mapentry", message->options().map_entry() ? "true" : "false");

  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateMessage(message->nested_type(i), printer, comma);
  }
  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnum(message->enum_type(i), printer, comma);
  }
}

void GenerateEnum(const google::protobuf::EnumDescriptor* en,
                  google::protobuf::io::Printer* printer,
                  CommaGenerator* comma) {

  comma->BeforeItem();
  printer->Print(
    "new protobuf.EnumDescriptor(\"$name$\",\n",
    "name", en->full_name());
  printer->Indent();

  CommaGenerator subcomma(printer);
  for (int i = 0; i < en->value_count(); i++) {
    subcomma.BeforeItem();
    const EnumValueDescriptor* value = en->value(i);
    printer->Print(
      "\"$name$\", $number$",
      "name", value->name(),
      "number", IntToString(value->number()));
  }

  printer->Outdent();
  printer->Print(
    ")");
}

void GenerateEnumAssignment(const std::string& prefix,
                            const google::protobuf::EnumDescriptor* en,
                            google::protobuf::io::Printer* printer) {
  printer->Print(
    "exports.$prefix$$name$ = "
    "protobuf.DescriptorPool.generatedPool.lookup('$fullname$').enumobject;\n",
    "prefix", prefix,
    "name", en->name(),
    "fullname", en->full_name());
}

void GenerateMessageAssignment(const std::string& prefix,
                               const google::protobuf::Descriptor* message,
                               google::protobuf::io::Printer* printer) {
  // Don't generate exports/names for MapEntry messages. They're internal-only.
  if (!message->options().map_entry()) {
    printer->Print(
      "exports.$prefix$$name$ = "
      "protobuf.DescriptorPool.generatedPool.lookup('$fullname$').msgclass;\n",
      "prefix", prefix,
      "name", message->name(),
      "fullname", message->full_name());
  }

  for (int i = 0; i < message->nested_type_count(); i++) {
    GenerateMessageAssignment(prefix + message->name() + ".",
                              message->nested_type(i), printer);
  }

  for (int i = 0; i < message->enum_type_count(); i++) {
    GenerateEnumAssignment(prefix + message->name() + ".",
                           message->enum_type(i), printer);
  }
}

void GenerateFile(const google::protobuf::FileDescriptor* file,
                  google::protobuf::io::Printer* printer) {
  printer->Print(
    "// Generated by the protocol buffer compiler.  DO NOT EDIT!\n"
    "// source: $filename$\n"
    "\n",
    "filename", file->name());

  printer->Print(
    "var protobuf = require('google_protobuf');\n");

  printer->Print(
    "protobuf.DescriptorPool.generatedPool.add([\n");
  printer->Indent();
  CommaGenerator pool_comma(printer);
  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateMessage(file->message_type(i), printer, &pool_comma);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    GenerateEnum(file->enum_type(i), printer, &pool_comma);
  }
  printer->Outdent();
  printer->Print(
    "\n]);\n\n");

  for (int i = 0; i < file->message_type_count(); i++) {
    GenerateMessageAssignment("", file->message_type(i), printer);
  }
  for (int i = 0; i < file->enum_type_count(); i++) {
    GenerateEnumAssignment("", file->enum_type(i), printer);
  }
}

bool Generator::Generate(
    const FileDescriptor* file,
    const string& parameter,
    GeneratorContext* generator_context,
    string* error) const {

  if (file->syntax() != FileDescriptor::SYNTAX_PROTO3) {
    *error =
        "Can only generate Node.js code for proto3 .proto files.\n"
        "Please add 'syntax = \"proto3\";' to the top of your .proto file.\n";
    return false;
  }

  std::string filename =
      StripDotProto(file->name()) + ".js";
  scoped_ptr<io::ZeroCopyOutputStream> output(generator_context->Open(filename));
  io::Printer printer(output.get(), '$');

  GenerateFile(file, &printer);

  return true;
}

}  // namespace nodejs
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
