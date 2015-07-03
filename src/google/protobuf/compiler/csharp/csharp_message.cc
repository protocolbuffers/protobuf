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

#include <google/protobuf/compiler/csharp/csharp_enum.h>
#include <google/protobuf/compiler/csharp/csharp_message.h>
#include <google/protobuf/compiler/csharp/csharp_helpers.h>
#include <google/protobuf/compiler/csharp/csharp_field_base.h>

using google::protobuf::internal::scoped_ptr;

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

bool CompareFieldNumbers(const FieldDescriptor* d1, const FieldDescriptor* d2) {
  return d1->number() < d2->number();
}

MessageGenerator::MessageGenerator(const Descriptor* descriptor)
    : SourceGeneratorBase(descriptor->file()),
      descriptor_(descriptor) {

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
  return descriptor_->name();
}

std::string MessageGenerator::full_class_name() {
  return GetClassName(descriptor_);
}

const std::vector<std::string>& MessageGenerator::field_names() {
  return field_names_;
}

const std::vector<const FieldDescriptor*>& MessageGenerator::fields_by_number() {
  return fields_by_number_;
}

/// Get an identifier that uniquely identifies this type within the file.
/// This is used to declare static variables related to this type at the
/// outermost file scope.
std::string GetUniqueFileScopeIdentifier(const Descriptor* descriptor) {
  std::string result = descriptor->full_name();
  std::replace(result.begin(), result.end(), '.', '_');
  return "static_" + result;
}

void MessageGenerator::GenerateStaticVariables(io::Printer* printer) {
  // Because descriptor.proto (Google.Protobuf.DescriptorProtos) is
  // used in the construction of descriptors, we have a tricky bootstrapping
  // problem.  To help control static initialization order, we make sure all
  // descriptors and other static data that depends on them are members of
  // the proto-descriptor class.  This way, they will be initialized in
  // a deterministic order.

  std::string identifier = GetUniqueFileScopeIdentifier(descriptor_);

  // The descriptor for this type.
  printer->Print(
      "internal static pb::FieldAccess.FieldAccessorTable internal__$identifier$__FieldAccessorTable;\n",
      "identifier", GetUniqueFileScopeIdentifier(descriptor_),
      "full_class_name", full_class_name());

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(descriptor_->nested_type(i));
    messageGenerator.GenerateStaticVariables(printer);
  }
}

void MessageGenerator::GenerateStaticVariableInitializers(io::Printer* printer) {
  map<string, string> vars;
  vars["identifier"] = GetUniqueFileScopeIdentifier(descriptor_);
  vars["full_class_name"] = full_class_name();

  // Work out how to get to the message descriptor (which may be multiply nested) from the file
  // descriptor.
  string descriptor_chain;
  const Descriptor* current_descriptor = descriptor_;
  while (current_descriptor->containing_type()) {
    descriptor_chain = ".NestedTypes[" + SimpleItoa(current_descriptor->index()) + "]" + descriptor_chain;
    current_descriptor = current_descriptor->containing_type();
  }
  descriptor_chain = "descriptor.MessageTypes[" + SimpleItoa(current_descriptor->index()) + "]" + descriptor_chain;
  vars["descriptor_chain"] = descriptor_chain;

  printer->Print(
    vars,
    "internal__$identifier$__FieldAccessorTable = \n"
    "    new pb::FieldAccess.FieldAccessorTable(typeof($full_class_name$), $descriptor_chain$,\n");
  printer->Print("        new string[] { ");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    printer->Print("\"$property_name$\", ",
                   "property_name", GetPropertyName(descriptor_->field(i)));
  }
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print("\"$oneof_name$\", ",
                   "oneof_name",
                   UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true));
  }
  printer->Print("});\n");

  // Generate static member initializers for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(descriptor_->nested_type(i));
    messageGenerator.GenerateStaticVariableInitializers(printer);
  }
}

void MessageGenerator::Generate(io::Printer* printer) {
  map<string, string> vars;
  vars["class_name"] = class_name();
  vars["access_level"] = class_access_level();
  vars["umbrella_class_name"] = GetFullUmbrellaClassName(descriptor_->file());
  vars["identifier"] = GetUniqueFileScopeIdentifier(descriptor_);

  printer->Print(
    "[global::System.Diagnostics.DebuggerNonUserCodeAttribute()]\n");
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "$access_level$ sealed partial class $class_name$ : pb::IMessage<$class_name$> {\n");
  printer->Indent();

  // All static fields and properties
  printer->Print(
      vars,
      "private static readonly pb::MessageParser<$class_name$> _parser = new pb::MessageParser<$class_name$>(() => new $class_name$());\n"
      "public static pb::MessageParser<$class_name$> Parser { get { return _parser; } }\n\n");
  printer->Print(
    "private static readonly string[] _fieldNames = "
    "new string[] { $slash$$field_names$$slash$ };\n",
    "field_names", JoinStrings(field_names(), "\", \""),
      "slash", field_names().size() > 0 ? "\"" : "");
  std::vector<std::string> tags;
  for (int i = 0; i < field_names().size(); i++) {
    uint32 tag = FixedMakeTag(descriptor_->FindFieldByName(field_names()[i]));
    tags.push_back(SimpleItoa(tag));
  }
  printer->Print(
    "private static readonly uint[] _fieldTags = new uint[] { $tags$ };\n",
    "tags", JoinStrings(tags, ", "));

  // Access the message descriptor via the relevant file descriptor or containing message descriptor.
  if (!descriptor_->containing_type()) {
    vars["descriptor_accessor"] = GetFullUmbrellaClassName(descriptor_->file())
        + ".Descriptor.MessageTypes[" + SimpleItoa(descriptor_->index()) + "]";
  } else {
    vars["descriptor_accessor"] = GetClassName(descriptor_->containing_type())
        + ".Descriptor.NestedTypes[" + SimpleItoa(descriptor_->index()) + "]";
  }

  printer->Print(
    vars,
    "public static pbd::MessageDescriptor Descriptor {\n"
    "  get { return $descriptor_accessor$; }\n"
    "}\n"
    "\n"
    "public pb::FieldAccess.FieldAccessorTable Fields {\n"
    "  get { return $umbrella_class_name$.internal__$identifier$__FieldAccessorTable; }\n"
    "}\n"
    "\n"
    "private bool _frozen = false;\n"
    "public bool IsFrozen { get { return _frozen; } }\n\n");

  // Parameterless constructor and partial OnConstruction method.
  printer->Print(
    vars,
    "public $class_name$() {\n"
    "  OnConstruction();\n"
    "}\n\n"
    "partial void OnConstruction();\n\n");

  GenerateCloningCode(printer);
  GenerateFreezingCode(printer);

  // Fields/properties
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* fieldDescriptor = descriptor_->field(i);

    // Rats: we lose the debug comment here :(
    printer->Print(
      "public const int $field_constant_name$ = $index$;\n",
      "field_constant_name", GetFieldConstantName(fieldDescriptor),
      "index", SimpleItoa(fieldDescriptor->number()));
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(fieldDescriptor));
    generator->GenerateMembers(printer);
    printer->Print("\n");
  }

  // oneof properties
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    vars["name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false);
    vars["property_name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true);
    printer->Print(
      vars,
      "private object $name$_;\n"
      "public enum $property_name$OneofCase {\n");
    printer->Indent();
    printer->Print("None = 0,\n");
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = descriptor_->oneof_decl(i)->field(j);
      printer->Print("$field_property_name$ = $index$,\n",
                     "field_property_name", GetPropertyName(field),
                     "index", SimpleItoa(field->number()));
    }
    printer->Outdent();
    printer->Print("}\n");
    printer->Print(
      vars,
      "private $property_name$OneofCase $name$Case_ = $property_name$OneofCase.None;\n"
      "public $property_name$OneofCase $property_name$Case {\n"
      "  get { return $name$Case_; }\n"
      "}\n\n"
      "public void Clear$property_name$() {\n"
      "  pb::Freezable.CheckMutable(this);\n"
      "  $name$Case_ = $property_name$OneofCase.None;\n"
      "  $name$_ = null;\n"
      "}\n\n");
  }

  // Standard methods
  GenerateFrameworkMethods(printer);
  GenerateMessageSerializationMethods(printer);
  GenerateMergingMethods(printer);

  // Nested messages and enums
  if (descriptor_->enum_type_count() + descriptor_->nested_type_count() > 0) {
    printer->Print("#region Nested types\n"
		   "[global::System.Diagnostics.DebuggerNonUserCodeAttribute()]\n");
    WriteGeneratedCodeAttributes(printer);
    printer->Print("public static partial class Types {\n");
    printer->Indent();
    for (int i = 0; i < descriptor_->enum_type_count(); i++) {
      EnumGenerator enumGenerator(descriptor_->enum_type(i));
      enumGenerator.Generate(printer);
    }
    for (int i = 0; i < descriptor_->nested_type_count(); i++) {
      MessageGenerator messageGenerator(descriptor_->nested_type(i));
      messageGenerator.Generate(printer);
    }
    printer->Outdent();
    printer->Print("}\n"
                   "#endregion\n"
                   "\n");
  }

  printer->Outdent();
  printer->Print("}\n");
  printer->Print("\n");
}

void MessageGenerator::GenerateCloningCode(io::Printer* printer) {
  map<string, string> vars;
  vars["class_name"] = class_name();
    printer->Print(
    vars,
    "public $class_name$($class_name$ other) : this() {\n");
  printer->Indent();
  // Clone non-oneof fields first
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->containing_oneof()) {
      scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
      generator->GenerateCloningCode(printer);
    }
  }
  // Clone just the right field for each oneof
  for (int i = 0; i < descriptor_->oneof_decl_count(); ++i) {
    vars["name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false);
    vars["property_name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true);
    printer->Print(vars, "switch (other.$property_name$Case) {\n");
    printer->Indent();
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = descriptor_->oneof_decl(i)->field(j);
      scoped_ptr<FieldGeneratorBase> generator(CreateFieldGeneratorInternal(field));
      vars["field_property_name"] = GetPropertyName(field);
      printer->Print(
          vars,
          "case $property_name$OneofCase.$field_property_name$:\n");
      printer->Indent();
      generator->GenerateCloningCode(printer);
      printer->Print("break;\n");
      printer->Outdent();
    }
    printer->Outdent();
    printer->Print("}\n\n");
  }

  printer->Outdent();
  printer->Print("}\n\n");

  printer->Print(
    vars,
    "public $class_name$ Clone() {\n"
    "  return new $class_name$(this);\n"
    "}\n\n");
}

void MessageGenerator::GenerateFreezingCode(io::Printer* printer) {
    map<string, string> vars;
    vars["class_name"] = class_name();
    printer->Print(
      "public void Freeze() {\n"
      "  if (IsFrozen) {\n"
      "    return;\n"
      "  }\n"
      "  _frozen = true;\n");
    printer->Indent();
    // Freeze non-oneof fields first (only messages and repeated fields will actually generate any code)
    for (int i = 0; i < descriptor_->field_count(); i++) {
        if (!descriptor_->field(i)->containing_oneof()) {
            scoped_ptr<FieldGeneratorBase> generator(
                CreateFieldGeneratorInternal(descriptor_->field(i)));
            generator->GenerateFreezingCode(printer);
        }
    }

    // For each oneof, if the value is freezable, freeze it. We don't actually need to know which type it was.
    for (int i = 0; i < descriptor_->oneof_decl_count(); ++i) {
        vars["name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false);
        printer->Print(vars,
            "if ($name$_ is IFreezable) ((IFreezable) $name$_).Freeze();\n");
    }

    printer->Outdent();
    printer->Print("}\n\n");
}

void MessageGenerator::GenerateFrameworkMethods(io::Printer* printer) {
    map<string, string> vars;
    vars["class_name"] = class_name();

    // Equality
    printer->Print(
        vars,
        "public override bool Equals(object other) {\n"
        "  return Equals(other as $class_name$);\n"
        "}\n\n"
        "public bool Equals($class_name$ other) {\n"
        "  if (ReferenceEquals(other, null)) {\n"
        "    return false;\n"
        "  }\n"
        "  if (ReferenceEquals(other, this)) {\n"
        "    return true;\n"
        "  }\n");
    printer->Indent();
    for (int i = 0; i < descriptor_->field_count(); i++) {
        scoped_ptr<FieldGeneratorBase> generator(
            CreateFieldGeneratorInternal(descriptor_->field(i)));
        generator->WriteEquals(printer);
    }
    printer->Outdent();
    printer->Print(
        "  return true;\n"
        "}\n\n");

    // GetHashCode
    // Start with a non-zero value to easily distinguish between null and "empty" messages.
    printer->Print(
        "public override int GetHashCode() {\n"
        "  int hash = 1;\n");
    printer->Indent();
    for (int i = 0; i < descriptor_->field_count(); i++) {
        scoped_ptr<FieldGeneratorBase> generator(
            CreateFieldGeneratorInternal(descriptor_->field(i)));
        generator->WriteHash(printer);
    }
    printer->Print("return hash;\n");
    printer->Outdent();
    printer->Print("}\n\n");

    printer->Print(
        "public override string ToString() {\n"
        "  return pb::JsonFormatter.Default.Format(this);\n"
        "}\n\n");
}

void MessageGenerator::GenerateMessageSerializationMethods(io::Printer* printer) {
  printer->Print(
      "public void WriteTo(pb::CodedOutputStream output) {\n");
  printer->Indent();

  // Serialize all the fields
  for (int i = 0; i < fields_by_number().size(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
      CreateFieldGeneratorInternal(fields_by_number()[i]));
    generator->GenerateSerializationCode(printer);
  }

  // TODO(jonskeet): Memoize size of frozen messages?
  printer->Outdent();
  printer->Print(
    "}\n"
    "\n"
    "public int CalculateSize() {\n");
  printer->Indent();
  printer->Print("int size = 0;\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    generator->GenerateSerializedSizeCode(printer);
  }
  printer->Print("return size;\n");
  printer->Outdent();
  printer->Print("}\n\n");
}

void MessageGenerator::GenerateMergingMethods(io::Printer* printer) {
  // Note:  These are separate from GenerateMessageSerializationMethods()
  //   because they need to be generated even for messages that are optimized
  //   for code size.
  map<string, string> vars;
  vars["class_name"] = class_name();

  printer->Print(
    vars,
    "public void MergeFrom($class_name$ other) {\n");
  printer->Indent();
  printer->Print(
    "if (other == null) {\n"
    "  return;\n"
    "}\n");
  // Merge non-oneof fields
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!descriptor_->field(i)->containing_oneof()) {      
      scoped_ptr<FieldGeneratorBase> generator(
          CreateFieldGeneratorInternal(descriptor_->field(i)));
      generator->GenerateMergingCode(printer);
    }
  }
  // Merge oneof fields
  for (int i = 0; i < descriptor_->oneof_decl_count(); ++i) {
    vars["name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false);
    vars["property_name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true);
    printer->Print(vars, "switch (other.$property_name$Case) {\n");
    printer->Indent();
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = descriptor_->oneof_decl(i)->field(j);
      vars["field_property_name"] = GetPropertyName(field);
      printer->Print(
        vars,
        "case $property_name$OneofCase.$field_property_name$:\n"
        "  $field_property_name$ = other.$field_property_name$;\n"
        "  break;\n");
    }
    printer->Outdent();
    printer->Print("}\n\n");
  }
  printer->Outdent();
  printer->Print("}\n\n");
  printer->Print("public void MergeFrom(pb::CodedInputStream input) {\n");
  printer->Indent();
  printer->Print(
    "uint tag;\n"
    "while (input.ReadTag(out tag)) {\n"
    "  switch(tag) {\n");
  printer->Indent();
  printer->Indent();
  printer->Print(
    "case 0:\n"  // 0 signals EOF / limit reached
    "  throw pb::InvalidProtocolBufferException.InvalidTag();\n"
    "default:\n"
    "  if (pb::WireFormat.IsEndGroupTag(tag)) {\n"
    "    return;\n"
    "  }\n"
    "  break;\n"); // Note: we're ignoring unknown fields here.
  for (int i = 0; i < fields_by_number().size(); i++) {
    const FieldDescriptor* field = fields_by_number()[i];
    internal::WireFormatLite::WireType wt =
        internal::WireFormat::WireTypeForFieldType(field->type());
    uint32 tag = internal::WireFormatLite::MakeTag(field->number(), wt);
    // Handle both packed and unpacked repeated fields with the same Read*Array call;
    // the two generated cases are the packed and unpacked tags.
    // TODO(jonskeet): Check that is_packable is equivalent to is_repeated && wt in { VARINT, FIXED32, FIXED64 }.
    // It looks like it is...
    if (field->is_packable()) {
      printer->Print(
        "case $packed_tag$:\n",
        "packed_tag",
        SimpleItoa(
            internal::WireFormatLite::MakeTag(
                field->number(),
                internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED)));
    }

    printer->Print("case $tag$: {\n", "tag", SimpleItoa(tag));
    printer->Indent();
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(field));
    generator->GenerateParsingCode(printer);
    printer->Print("break;\n");
    printer->Outdent();
    printer->Print("}\n");
  }
  printer->Outdent();
  printer->Print("}\n"); // switch
  printer->Outdent();
  printer->Print("}\n"); // while
  printer->Outdent();
  printer->Print("}\n\n"); // method
}

int MessageGenerator::GetFieldOrdinal(const FieldDescriptor* descriptor) {
  for (int i = 0; i < field_names().size(); i++) {
    if (field_names()[i] == descriptor->name()) {
      return i;
    }
  }
  GOOGLE_LOG(DFATAL)<< "Could not find ordinal for field " << descriptor->name();
  return -1;
}

FieldGeneratorBase* MessageGenerator::CreateFieldGeneratorInternal(
    const FieldDescriptor* descriptor) {
  return CreateFieldGenerator(descriptor, GetFieldOrdinal(descriptor));
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
