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
#include <algorithm>
#include <map>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/wire_format_lite.h>

#include <google/protobuf/compiler/fsharp/fsharp_doc_comment.h>
#include <google/protobuf/compiler/fsharp/fsharp_enum.h>
#include <google/protobuf/compiler/fsharp/fsharp_field_base.h>
#include <google/protobuf/compiler/fsharp/fsharp_helpers.h>
#include <google/protobuf/compiler/fsharp/fsharp_message.h>
#include <google/protobuf/compiler/fsharp/fsharp_names.h>
#include <google/protobuf/compiler/fsharp/fsharp_oneof.h>

using google::protobuf::internal::scoped_ptr;

namespace google {
namespace protobuf {
namespace compiler {
namespace fsharp {

bool CompareFieldNumbers(const FieldDescriptor* d1, const FieldDescriptor* d2) {
  return d1->number() < d2->number();
}

MessageGenerator::MessageGenerator(const Descriptor* descriptor,
  const Options* options)
  : SourceGeneratorBase(descriptor->file(), options),
  descriptor_(descriptor),
  generators_(descriptor, options) {

  // sorted field names
  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_names_.push_back(descriptor_->field(i)->name());
  }
  std::sort(field_names_.begin(), field_names_.end());

  // fields by number
  for (int i = 0; i < descriptor_->field_count(); i++) {
    fields_by_number_.push_back(descriptor_->field(i));
  }
  std::sort(fields_by_number_.begin(), fields_by_number_.end(),
    CompareFieldNumbers);
}

MessageGenerator::~MessageGenerator() {
}

std::string MessageGenerator::class_name() {
  return GetTypeName(descriptor_);
}

const std::vector<std::string>& MessageGenerator::field_names() {
  return field_names_;
}

const std::vector<const FieldDescriptor*>& MessageGenerator::fields_by_number() {
  return fields_by_number_;
}

void MessageGenerator::AddDeprecatedFlag(io::Printer* printer) {
  if (descriptor_->options().deprecated()) {
    printer->Print("[<System.ObsoleteAttribute>]\n");
  }
}

void MessageGenerator::Generate(io::Printer* printer) {

  std::map<string, string> vars;
  vars["class_name"] = class_name();
  vars["access_level"] = class_access_level();

  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    scoped_ptr<OneofGenerator> generator(new OneofGenerator(descriptor_->oneof_decl(i), &generators_));
    generator->GenerateTypeDefinition(printer);
  }

  WriteMessageDocComment(printer, descriptor_);
  AddDeprecatedFlag(printer);

  printer->Print(
    vars,
    "and [<AllowNullLiteral>] $access_level$ $class_name$ =\n"
  );
  printer->Indent();

  // Fields/properties
  for (int i = 0; i < descriptor_->field_count(); i++) {
    generators_.Get(i)->GenerateValDeclaration(printer);
  }

  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    scoped_ptr<OneofGenerator> generator(new OneofGenerator(descriptor_->oneof_decl(i), &generators_));
    generator->GenerateValDeclaration(printer);
  }

  printer->Print("\n");

  WriteGeneratedCodeAttributes(printer);
  printer->Print("new () =\n");
  printer->Indent();
  printer->Print("{\n");
  printer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    generators_.Get(i)->GenerateConstructorValue(printer);
  }

  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    scoped_ptr<OneofGenerator> generator(
      new OneofGenerator(descriptor_->oneof_decl(i), &generators_));
    generator->GenerateConstructorValue(printer);
  }

  printer->Outdent();
  printer->Print("}\n\n");
  printer->Outdent();

  GenerateCloningCode(printer);

  // All static fields and properties
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "static member Parser = new MessageParser<$class_name$>(fun () -> new $class_name$())\n\n");

  // Access the message descriptor via the relevant file descriptor or containing message descriptor.
  if (!descriptor_->containing_type()) {
    vars["descriptor_accessor"] = GetReflectionClassUnqualifiedName(descriptor_->file())
      + ".Descriptor.MessageTypes.[" + SimpleItoa(descriptor_->index()) + "]";
  } else {
    vars["descriptor_accessor"] = GetTypeName(descriptor_->containing_type())
      + ".Descriptor.NestedTypes.[" + SimpleItoa(descriptor_->index()) + "]";
  }

  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "static member Descriptor = $descriptor_accessor$\n\n");

  // CustomOptions property, only for options messages
  if (IsDescriptorOptionMessage(descriptor_)) {
    printer->Print(
      "internal CustomOptions CustomOptions{ get; private set; } = CustomOptions.Empty;\n"
      "\n");
  }

  GenerateFreezingCode(printer);

  // Fields/properties
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* fieldDescriptor = descriptor_->field(i);

    // Rats: we lose the debug comment here :(
    printer->Print(
      "/// <summary>Field number for the \"$field_name$\" field.</summary>\n"
      "static member public $field_constant_name$ = $index$\n",
      "field_name", fieldDescriptor->name(),
      "field_constant_name", GetFieldConstantName(fieldDescriptor),
      "index", SimpleItoa(fieldDescriptor->number()));
    generators_.Get(i)->GenerateMembers(printer);
    printer->Print("\n");
  }

  // oneof properties
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    scoped_ptr<OneofGenerator> generator(
      new OneofGenerator(descriptor_->oneof_decl(i), &generators_));
    generator->GenerateMembers(printer);
  }

  // Override Equality
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "override this.Equals(other: System.Object) : bool =\n"
    "  match other with\n"
    "    | :? $class_name$ as x -> (x :> System.IEquatable<$class_name$>).Equals(this)\n"
    "    | _ -> false\n\n");

  // Override GetHashCode
  // Start with a non-zero value to easily distinguish between null and "empty" messages.
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    "override this.GetHashCode() : int =\n"
    "  let mutable hash = 1\n");
  printer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (!(field->containing_oneof())) {
      generators_.Get(i)->WriteHash(printer);
    }
  }

  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    scoped_ptr<OneofGenerator> generator(
      new OneofGenerator(descriptor_->oneof_decl(i), &generators_));
    generator->WriteHash(printer);
  }

  printer->Print("hash\n\n");
  printer->Outdent();

  // ToString with reflection
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    "override this.ToString() =\n"
    "  JsonFormatter.ToDiagnosticString(this)\n\n");

  // Standard methods
  printer->Print(vars, "interface IMessage<$class_name$> with\n");
  printer->Indent();
  GenerateMessageSerializationMethods(printer);
  GenerateMergingMethods(printer);
  GenerateFrameworkMethods(printer);
  printer->Print(vars, "member this.Descriptor : Reflection.MessageDescriptor = $class_name$.Descriptor\n\n");
  printer->Outdent();
  printer->Outdent();

  // Nested messages and enums
  if (HasNestedGeneratedTypes()) {
    printer->Print(
      vars,
      "//#region Nested types\n"
      "/// <summary>Container for nested types declared in the $class_name$ message type.</summary>\n");

    for (int i = 0; i < descriptor_->enum_type_count(); i++) {
      EnumGenerator enumGenerator(descriptor_->enum_type(i), this->options());
      enumGenerator.Generate(printer);
    }
    for (int i = 0; i < descriptor_->nested_type_count(); i++) {
      // Don't generate nested types for maps...
      if (!IsMapEntryMessage(descriptor_->nested_type(i))) {
        MessageGenerator messageGenerator(
          descriptor_->nested_type(i), this->options());
        messageGenerator.Generate(printer);
      }
    }
    printer->Print(
      vars,
      "//#endregion nested for nested types in the $class_name$\n"
      "\n");
  }
}

// Helper to work out whether we need to generate a class to hold nested types/enums.
// Only tricky because we don't want to generate map entry types.
bool MessageGenerator::HasNestedGeneratedTypes() {
  if (descriptor_->enum_type_count() > 0) {
    return true;
  }
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    if (!IsMapEntryMessage(descriptor_->nested_type(i))) {
      return true;
    }
  }
  return false;
}

void MessageGenerator::GenerateCloningCode(io::Printer* printer) {
  std::map<string, string> vars;
  WriteGeneratedCodeAttributes(printer);
  vars["class_name"] = class_name();
  printer->Print(
    vars,
    "new (other: $class_name$) =\n");
  printer->Indent();
  printer->Print("{\n");
  printer->Indent();
  // Clone non-oneof fields first
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->containing_oneof()) {
      generators_.Get(i)->GenerateCloningCode(printer);
    }
  }
  // Clone just the right field for each oneof
  for (int i = 0; i < descriptor_->oneof_decl_count(); ++i) {
    scoped_ptr<OneofGenerator> generator(new OneofGenerator(descriptor_->oneof_decl(i), &generators_));
    generator->GenerateCloningCode(printer);
  }

  printer->Outdent();
  printer->Print("}\n\n");
  printer->Outdent();
}

void MessageGenerator::GenerateFreezingCode(io::Printer* printer) {
}

void MessageGenerator::GenerateFrameworkMethods(io::Printer* printer) {
  std::map<string, string> vars;
  vars["class_name"] = class_name();


  // Clone
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "member this.Clone() : $class_name$ =\n"
    "  new $class_name$(this)\n\n");

  // Equality
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "member this.Equals(other: $class_name$) : bool =\n"
    "  if System.Object.ReferenceEquals(other, null) then\n"
    "    false\n"
    "  else if System.Object.ReferenceEquals(other, this) then\n"
    "    true\n");
  printer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->containing_oneof()) {
      generators_.Get(i)->WriteEquals(printer);
    }
  }
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print("else if not (this.$property_name$ = other.$property_name$) then false\n",
      "property_name", UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true));
  }
  printer->Print(
    "else true\n\n");
  printer->Outdent();
}

void MessageGenerator::GenerateMessageSerializationMethods(io::Printer* printer) {
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    "member this.WriteTo(output: CodedOutputStream) : unit =\n");
  printer->Indent();

  if (descriptor_->field_count() == 0) {
    printer->Print("()");
  } else {
    // Serialize all the fields
    for (int i = 0; i < fields_by_number().size(); i++) {
      generators_.Get(i)->GenerateSerializationCode(printer);
    }
  }

  printer->Outdent();
  printer->Print("\n");

  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    "member this.CalculateSize() : int =\n");
  printer->Indent();
  printer->Print("let mutable size = 0\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!(descriptor_->field(i)->containing_oneof())) {
      generators_.Get(i)->GenerateSerializedSizeCode(printer);
    }
  }
  for (int i = 0; i < descriptor_->oneof_decl_count(); ++i) {
    scoped_ptr<OneofGenerator> generator(new OneofGenerator(descriptor_->oneof_decl(i), &generators_));
    generator->GenerateSerializedSizeCode(printer);
  }

  printer->Print("size\n");
  printer->Outdent();
  printer->Print("\n");
}

void MessageGenerator::GenerateMergingMethods(io::Printer* printer) {
  // Note:  These are separate from GenerateMessageSerializationMethods()
  //   because they need to be generated even for messages that are optimized
  //   for code size.
  std::map<string, string> vars;
  vars["class_name"] = class_name();

  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "member this.MergeFrom(other: $class_name$) : unit =\n");
  printer->Indent();
  printer->Print(
    "if not (System.Object.ReferenceEquals(other, null)) then\n");
  printer->Indent();
  if (descriptor_->field_count() == 0) {
    printer->Print("()");
  } else {
    // Merge non-oneof fields
    for (int i = 0; i < descriptor_->field_count(); i++) {
      if (!descriptor_->field(i)->containing_oneof()) {
        generators_.Get(i)->GenerateMergingCode(printer);
      }
    }
    // Merge oneof fields
    for (int i = 0; i < descriptor_->oneof_decl_count(); ++i) {
      scoped_ptr<OneofGenerator>
        generator(new OneofGenerator(descriptor_->oneof_decl(i), &generators_));
      generator->GenerateMergingCode(printer);
    }
  }

  printer->Outdent();
  printer->Outdent();
  printer->Print("\n");
  WriteGeneratedCodeAttributes(printer);
  printer->Print("member this.MergeFrom(input: CodedInputStream) : unit =\n");
  printer->Indent();
  printer->Print(
    "let mutable tag = input.ReadTag()\n"
    "while (tag <> 0u) do\n"
    "  match tag with\n");
  printer->Indent();
  printer->Indent();

  for (int i = 0; i < fields_by_number().size(); i++) {
    const FieldDescriptor* field = fields_by_number()[i];
    internal::WireFormatLite::WireType wt =
      internal::WireFormat::WireTypeForFieldType(field->type());
    uint32 tag = internal::WireFormatLite::MakeTag(field->number(), wt);
    // Handle both packed and unpacked repeated fields with the same Read*Array call;
    // the two generated cases are the packed and unpacked tags.
    // TODO(jonskeet): Check that is_packable is equivalent to
    // is_repeated && wt in { VARINT, FIXED32, FIXED64 }.
    // It looks like it is...
    if (field->is_packable()) {
      printer->Print(
        "| $packed_tag$u\n",
        "packed_tag",
        SimpleItoa(
          internal::WireFormatLite::MakeTag(
            field->number(),
            internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED)));
    }

    printer->Print("| $tag$u ->\n", "tag", SimpleItoa(tag));
    printer->Indent();
    generators_.Get(i)->GenerateParsingCode(printer);
    printer->Outdent();
  }
  // Option messages need to store unknown fields so that options can be parsed later.
  if (IsDescriptorOptionMessage(descriptor_)) {
    printer->Print(
      "| _ ->\n"
      "  this.CustomOptions <- CustomOptions.ReadOrSkipUnknownField(input)\n");
  } else {
    printer->Print(
      "| _ ->\n"
      "  input.SkipLastField()\n"); // We're not storing the data, but we still need to consume it.
  }

  printer->Outdent(); // match
  printer->Print("tag <- input.ReadTag()\n");
  printer->Outdent(); // while
  printer->Outdent(); // method
  printer->Print("\n"); // method
}

int MessageGenerator::GetFieldOrdinal(const FieldDescriptor* descriptor) {
  for (int i = 0; i < field_names().size(); i++) {
    if (field_names()[i] == descriptor->name()) {
      return i;
    }
  }
  GOOGLE_LOG(DFATAL) << "Could not find ordinal for field " << descriptor->name();
  return -1;
}

FieldGeneratorBase* MessageGenerator::CreateFieldGeneratorInternal(
  const FieldDescriptor* descriptor) {
  return CreateFieldGenerator(descriptor, GetFieldOrdinal(descriptor), this->options());
}

}  // namespace fsharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
