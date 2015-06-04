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
#include <google/protobuf/compiler/csharp/csharp_extension.h>
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
  // Because descriptor.proto (Google.ProtocolBuffers.DescriptorProtos) is
  // used in the construction of descriptors, we have a tricky bootstrapping
  // problem.  To help control static initialization order, we make sure all
  // descriptors and other static data that depends on them are members of
  // the proto-descriptor class.  This way, they will be initialized in
  // a deterministic order.

  std::string identifier = GetUniqueFileScopeIdentifier(descriptor_);

  if (!use_lite_runtime()) {
    // The descriptor for this type.
    printer->Print(
        "internal static pbd::MessageDescriptor internal__$identifier$__Descriptor;\n"
        "internal static pb::FieldAccess.FieldAccessorTable<$full_class_name$, $full_class_name$.Builder> internal__$identifier$__FieldAccessorTable;\n",
	"identifier", GetUniqueFileScopeIdentifier(descriptor_),
	"full_class_name", full_class_name());
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(descriptor_->nested_type(i));
    messageGenerator.GenerateStaticVariables(printer);
  }
}

void MessageGenerator::GenerateStaticVariableInitializers(io::Printer* printer) {
  map<string, string> vars;
  vars["identifier"] = GetUniqueFileScopeIdentifier(descriptor_);
  vars["index"] = SimpleItoa(descriptor_->index());
  vars["full_class_name"] = full_class_name();
  if (descriptor_->containing_type() != NULL) {
    vars["parent"] = GetUniqueFileScopeIdentifier(
	descriptor_->containing_type());
  }
  if (!use_lite_runtime()) {
    printer->Print(vars, "internal__$identifier$__Descriptor = ");

    if (!descriptor_->containing_type()) {
      printer->Print(vars, "Descriptor.MessageTypes[$index$];\n");
    } else {
      printer->Print(vars, "internal__$parent$__Descriptor.NestedTypes[$index$];\n");
    }

    printer->Print(
      vars,
      "internal__$identifier$__FieldAccessorTable = \n"
      "    new pb::FieldAccess.FieldAccessorTable<$full_class_name$, $full_class_name$.Builder>(internal__$identifier$__Descriptor,\n");
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
  }

  // Generate static member initializers for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(descriptor_->nested_type(i));
    messageGenerator.GenerateStaticVariableInitializers(printer);
  }

  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ExtensionGenerator extensionGenerator(descriptor_->extension(i));
    extensionGenerator.GenerateStaticVariableInitializers(printer);
  }
}

void MessageGenerator::Generate(io::Printer* printer) {
  map<string, string> vars;
  vars["class_name"] = class_name();
  vars["access_level"] = class_access_level();
  vars["extendable_or_generated"] = descriptor_->extension_range_count() > 0 ?
    "Extendable" : "Generated";
  vars["suffix"] = runtime_suffix();
  vars["umbrella_class_name"] = GetFullUmbrellaClassName(descriptor_->file());
  vars["identifier"] = GetUniqueFileScopeIdentifier(descriptor_);

  printer->Print(
      "[global::System.Diagnostics.DebuggerNonUserCodeAttribute()]\n");
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "$access_level$ sealed partial class $class_name$ : pb::$extendable_or_generated$Message$suffix$<$class_name$, $class_name$.Builder> {\n");
  printer->Indent();
  printer->Print(
    vars,
    "private $class_name$() { }\n"  // Private ctor.
    // Must call MakeReadOnly() to make sure all lists are made read-only
    "private static readonly $class_name$ defaultInstance = new $class_name$().MakeReadOnly();\n");

  if (optimize_speed()) {
    printer->Print(
      "private static readonly string[] _$name$FieldNames = "
      "new string[] { $slash$$field_names$$slash$ };\n",
      "name", UnderscoresToCamelCase(class_name(), false),
      "field_names", JoinStrings(field_names(), "\", \""),
      "slash", field_names().size() > 0 ? "\"" : "");
    std::vector<std::string> tags;
    for (int i = 0; i < field_names().size(); i++) {
      uint32 tag = internal::WireFormat::MakeTag(
          descriptor_->FindFieldByName(field_names()[i]));
      tags.push_back(SimpleItoa(tag));
    }
    printer->Print(
      "private static readonly uint[] _$name$FieldTags = new uint[] { $tags$ };\n",
      "name", UnderscoresToCamelCase(class_name(), false),
      "tags", JoinStrings(tags, ", "));
  }
  printer->Print(
    vars,
    "public static $class_name$ DefaultInstance {\n"
    "  get { return defaultInstance; }\n"
    "}\n"
    "\n"
    "public override $class_name$ DefaultInstanceForType {\n"
    "  get { return DefaultInstance; }\n"
    "}\n"
    "\n"
    "protected override $class_name$ ThisMessage {\n"
    "  get { return this; }\n"
    "}\n\n");

  if (!use_lite_runtime()) {
    printer->Print(
      vars,
      "public static pbd::MessageDescriptor Descriptor {\n"
      "  get { return $umbrella_class_name$.internal__$identifier$__Descriptor; }\n"
      "}\n"
      "\n"
      "protected override pb::FieldAccess.FieldAccessorTable<$class_name$, $class_name$.Builder> InternalFieldAccessors {\n"
      "  get { return $umbrella_class_name$.internal__$identifier$__FieldAccessorTable; }\n"
      "}\n"
      "\n");
  }

  // Extensions don't need to go in an extra nested type
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ExtensionGenerator extensionGenerator(descriptor_->extension(i));
    extensionGenerator.Generate(printer);
  }

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

  // oneof
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    vars["name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false);
    vars["property_name"] = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true);
    printer->Print(
      vars,
      "private object $name$_;\n"
      "public enum $property_name$OneofCase {\n");
    printer->Indent();
    for (int j = 0; j < descriptor_->oneof_decl(i)->field_count(); j++) {
      const FieldDescriptor* field = descriptor_->oneof_decl(i)->field(j);
      printer->Print("$field_property_name$ = $index$,\n",
                     "field_property_name", GetPropertyName(field),
                     "index", SimpleItoa(field->number()));
    }
    printer->Print("None = 0,\n");
    printer->Outdent();
    printer->Print("}\n");
    printer->Print(
      vars,
      "private $property_name$OneofCase $name$Case_ = $property_name$OneofCase.None;\n"
      "public $property_name$OneofCase $property_name$Case {\n"
      "  get { return $name$Case_; }\n"
      "}\n\n");
  }

  // Fields
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

  if (optimize_speed()) {
    if (SupportFieldPresence(descriptor_->file())) {
      GenerateIsInitialized(printer);
    }
    GenerateMessageSerializationMethods(printer);
  }
  if (use_lite_runtime()) {
    GenerateLiteRuntimeMethods(printer);
  }

  GenerateParseFromMethods(printer);
  GenerateBuilder(printer);

  // Force the static initialization code for the file to run, since it may
  // initialize static variables declared in this class.
  printer->Print(vars, "static $class_name$() {\n");
  // We call object.ReferenceEquals() just to make it a valid statement on its own.
  // Another option would be GetType(), but that causes problems in DescriptorProtoFile,
  // where the bootstrapping is somewhat recursive - type initializers call
  // each other, effectively. We temporarily see Descriptor as null.
  printer->Print(
    vars,
    "  object.ReferenceEquals($umbrella_class_name$.Descriptor, null);\n"
    "}\n");

  printer->Outdent();
  printer->Print("}\n");
  printer->Print("\n");

}

void MessageGenerator::GenerateLiteRuntimeMethods(io::Printer* printer) {
  map<string, string> vars;
  vars["class_name"] = class_name();

  bool callbase = descriptor_->extension_range_count() > 0;
  printer->Print("#region Lite runtime methods\n"
                 "public override int GetHashCode() {\n");
  printer->Indent();
  printer->Print("int hash = GetType().GetHashCode();\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->containing_oneof() == NULL) {
      scoped_ptr<FieldGeneratorBase> generator(
          CreateFieldGeneratorInternal(field));
      generator->WriteHash(printer);
    }
  }
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    string name = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false);
    string property_name = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true);
    printer->Print(
      "if ($name$Case_ != $property_name$OneofCase.None) {\n"
      "  hash ^= $name$_.GetHashCode();\n"
      "}\n",
      "name", UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false),
      "property_name",
      UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true));
  }
  if (callbase) {
    printer->Print("hash ^= base.GetHashCode();\n");
  }
  printer->Print("return hash;\n");
  printer->Outdent();
  printer->Print("}\n");
  printer->Print("\n");

  printer->Print("public override bool Equals(object obj) {\n");
  printer->Indent();
  printer->Print(
    vars,
    "$class_name$ other = obj as $class_name$;\n"
    "if (other == null) return false;\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    generator->WriteEquals(printer);
  }
  if (callbase) {
    printer->Print("if (!base.Equals(other)) return false;\n");
  }
  printer->Print("return true;\n");
  printer->Outdent();
  printer->Print("}\n");
  printer->Print("\n");

  printer->Print(
    "public override void PrintTo(global::System.IO.TextWriter writer) {\n");
  printer->Indent();

  for (int i = 0; i < fields_by_number().size(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(fields_by_number()[i]));
    generator->WriteToString(printer);
  }

  if (callbase) {
    printer->Print("base.PrintTo(writer);\n");
  }
  printer->Outdent();
  printer->Print("}\n");
  printer->Print("#endregion\n");
  printer->Print("\n");
}

bool CompareExtensionRangesStart(const Descriptor::ExtensionRange* r1,
                                 const Descriptor::ExtensionRange* r2) {
  return r1->start < r2->start;
}

void MessageGenerator::GenerateMessageSerializationMethods(io::Printer* printer) {
  std::vector<const Descriptor::ExtensionRange*> extension_ranges_sorted;
  for (int i = 0; i < descriptor_->extension_range_count(); i++) {
    extension_ranges_sorted.push_back(descriptor_->extension_range(i));
  }
  std::sort(extension_ranges_sorted.begin(), extension_ranges_sorted.end(),
            CompareExtensionRangesStart);

  printer->Print(
      "public override void WriteTo(pb::ICodedOutputStream output) {\n");
  printer->Indent();
  // Make sure we've computed the serialized length, so that packed fields are generated correctly.
  printer->Print("CalcSerializedSize();\n"
		 "string[] field_names = _$class_name$FieldNames;\n",
                 "class_name", UnderscoresToCamelCase(class_name(), false));
  if (descriptor_->extension_range_count()) {
    printer->Print(
      "pb::ExtendableMessage$runtime_suffix$<$class_name$, $class_name$.Builder>.ExtensionWriter extensionWriter = CreateExtensionWriter(this);\n",
      "class_name", class_name(),
      "runtime_suffix", runtime_suffix());
  }

  // Merge the fields and the extension ranges, both sorted by field number.
  for (int i = 0, j = 0;
      i < fields_by_number().size() || j < extension_ranges_sorted.size();) {
    if (i == fields_by_number().size()) {
      GenerateSerializeOneExtensionRange(printer, extension_ranges_sorted[j++]);
    } else if (j == extension_ranges_sorted.size()) {
      GenerateSerializeOneField(printer, fields_by_number()[i++]);
    } else if (fields_by_number()[i]->number()
        < extension_ranges_sorted[j]->start) {
      GenerateSerializeOneField(printer, fields_by_number()[i++]);
    } else {
      GenerateSerializeOneExtensionRange(printer, extension_ranges_sorted[j++]);
    }
  }

  if (!use_lite_runtime()) {
    if (descriptor_->options().message_set_wire_format())
    {
      printer->Print("UnknownFields.WriteAsMessageSetTo(output);\n");
    } else {
      printer->Print("UnknownFields.WriteTo(output);\n");
    }
  }

  printer->Outdent();
  printer->Print(
    "}\n"
    "\n"
    "private int memoizedSerializedSize = -1;\n"
    "public override int SerializedSize {\n");
  printer->Indent();
  printer->Print("get {\n");
  printer->Indent();
  printer->Print(
    "int size = memoizedSerializedSize;\n"
    "if (size != -1) return size;\n"
    "return CalcSerializedSize();\n");
  printer->Outdent();
  printer->Print("}\n");
  printer->Outdent();
  printer->Print("}\n");
  printer->Print("\n");

  printer->Print("private int CalcSerializedSize() {\n");
  printer->Indent();
  printer->Print(
    "int size = memoizedSerializedSize;\n"
    "if (size != -1) return size;\n"
    "\n"
    "size = 0;\n");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    generator->GenerateSerializedSizeCode(printer);
  }
  if (descriptor_->extension_range_count() > 0) {
    printer->Print("size += ExtensionsSerializedSize;\n");
  }

  if (!use_lite_runtime()) {
    if (descriptor_->options().message_set_wire_format()) {
      printer->Print("size += UnknownFields.SerializedSizeAsMessageSet;\n");
    } else {
      printer->Print("size += UnknownFields.SerializedSize;\n");
    }
  }
  printer->Print(
    "memoizedSerializedSize = size;\n"
    "return size;\n");
  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::GenerateSerializeOneField(
    io::Printer* printer, const FieldDescriptor* fieldDescriptor) {
  scoped_ptr<FieldGeneratorBase> generator(
      CreateFieldGeneratorInternal(fieldDescriptor));
  generator->GenerateSerializationCode(printer);
}

void MessageGenerator::GenerateSerializeOneExtensionRange(
    io::Printer* printer, const Descriptor::ExtensionRange* extensionRange) {
  printer->Print("extensionWriter.WriteUntil($range_end$, output);\n",
		 "range_end", SimpleItoa(extensionRange->end));
}

void MessageGenerator::GenerateParseFromMethods(io::Printer* printer) {
  // Note:  These are separate from GenerateMessageSerializationMethods()
  //   because they need to be generated even for messages that are optimized
  //   for code size.
  map<string, string> vars;
  vars["class_name"] = class_name();

  printer->Print(
    vars,
    "public static $class_name$ ParseFrom(pb::ByteString data) {\n"
    "  return ((Builder) CreateBuilder().MergeFrom(data)).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseFrom(pb::ByteString data, pb::ExtensionRegistry extensionRegistry) {\n"
    "  return ((Builder) CreateBuilder().MergeFrom(data, extensionRegistry)).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseFrom(byte[] data) {\n"
    "  return ((Builder) CreateBuilder().MergeFrom(data)).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseFrom(byte[] data, pb::ExtensionRegistry extensionRegistry) {\n"
    "  return ((Builder) CreateBuilder().MergeFrom(data, extensionRegistry)).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseFrom(global::System.IO.Stream input) {\n"
    "  return ((Builder) CreateBuilder().MergeFrom(input)).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseFrom(global::System.IO.Stream input, pb::ExtensionRegistry extensionRegistry) {\n"
    "  return ((Builder) CreateBuilder().MergeFrom(input, extensionRegistry)).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseDelimitedFrom(global::System.IO.Stream input) {\n"
    "  return CreateBuilder().MergeDelimitedFrom(input).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseDelimitedFrom(global::System.IO.Stream input, pb::ExtensionRegistry extensionRegistry) {\n"
    "  return CreateBuilder().MergeDelimitedFrom(input, extensionRegistry).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseFrom(pb::ICodedInputStream input) {\n"
    "  return ((Builder) CreateBuilder().MergeFrom(input)).BuildParsed();\n"
    "}\n"
    "public static $class_name$ ParseFrom(pb::ICodedInputStream input, pb::ExtensionRegistry extensionRegistry) {\n"
    "  return ((Builder) CreateBuilder().MergeFrom(input, extensionRegistry)).BuildParsed();\n"
    "}\n");
}

void MessageGenerator::GenerateBuilder(io::Printer* printer) {
  map<string, string> vars;
  vars["class_name"] = class_name();
  vars["access_level"] = class_access_level();
  vars["extendable_or_generated"] = descriptor_->extension_range_count() > 0 ?
    "Extendable" : "Generated";
  vars["suffix"] = runtime_suffix();

  printer->Print(vars, "private $class_name$ MakeReadOnly() {\n");
  printer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    generator->GenerateBuildingCode(printer);
  }
  printer->Print("return this;\n");
  printer->Outdent();
  printer->Print("}\n\n");

  printer->Print(
    vars,
    "public static Builder CreateBuilder() { return new Builder(); }\n"
    "public override Builder ToBuilder() { return CreateBuilder(this); }\n"
    "public override Builder CreateBuilderForType() { return new Builder(); }\n"
    "public static Builder CreateBuilder($class_name$ prototype) {\n"
    "  return new Builder(prototype);\n"
    "}\n"
    "\n"
    "[global::System.Diagnostics.DebuggerNonUserCodeAttribute()]\n");
  WriteGeneratedCodeAttributes(printer);
  printer->Print(
    vars,
    "$access_level$ sealed partial class Builder : pb::$extendable_or_generated$Builder$suffix$<$class_name$, Builder> {\n");
  printer->Indent();
  printer->Print(
    "protected override Builder ThisBuilder {\n"
    "  get { return this; }\n"
    "}\n");
  GenerateCommonBuilderMethods(printer);
  if (optimize_speed()) {
    GenerateBuilderParsingMethods(printer);
  }
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    printer->Print("\n");
    // No field comment :(
    generator->GenerateBuilderMembers(printer);
  }

  // oneof
  for (int i = 0; i < descriptor_->oneof_decl_count(); i++) {
    printer->Print("\n");
    string name = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false);
    string property_name = UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true);
    printer->Print(
      "public $property_name$OneofCase $property_name$Case {\n"
      "  get { return result.$name$Case_; }\n"
      "}\n"
      "public Builder Clear$property_name$() {\n"
      "  PrepareBuilder();\n"
      "  result.$name$_ = null;\n"
      "  result.$name$Case_ = $property_name$OneofCase.None;\n"
      "  return this;\n"
      "}\n",
      "name", UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), false),
      "property_name",
      UnderscoresToCamelCase(descriptor_->oneof_decl(i)->name(), true));
  }

  printer->Outdent();
  printer->Print("}\n");
}

void MessageGenerator::GenerateCommonBuilderMethods(io::Printer* printer) {
  map<string, string> vars;
  vars["class_name"] = class_name();
  vars["full_class_name"] = full_class_name();
  vars["suffix"] = runtime_suffix();

  printer->Print(
    vars,
    //default constructor
    "public Builder() {\n"
    //Durring static initialization of message, DefaultInstance is expected to return null.
    "  result = DefaultInstance;\n"
    "  resultIsReadOnly = true;\n"
    "}\n"
    //clone constructor
    "internal Builder($class_name$ cloneFrom) {\n"
    "  result = cloneFrom;\n"
    "  resultIsReadOnly = true;\n"
    "}\n"
    "\n"
    "private bool resultIsReadOnly;\n"
    "private $class_name$ result;\n"
    "\n"
    "private $class_name$ PrepareBuilder() {\n"
    "  if (resultIsReadOnly) {\n"
    "    $class_name$ original = result;\n"
    "    result = new $class_name$();\n"
    "    resultIsReadOnly = false;\n"
    "    MergeFrom(original);\n"
    "  }\n"
    "  return result;\n"
    "}\n"
    "\n"
    "public override bool IsInitialized {\n"
    "  get { return result.IsInitialized; }\n"
    "}\n"
    "\n"
    "protected override $class_name$ MessageBeingBuilt {\n"
    "  get { return PrepareBuilder(); }\n"
    "}\n"
    "\n");
  //Not actually expecting that DefaultInstance would ever be null here; however, we will ensure it does not break
  printer->Print(
    "public override Builder Clear() {\n"
    "  result = DefaultInstance;\n"
    "  resultIsReadOnly = true;\n"
    "  return this;\n"
    "}\n"
    "\n"
    "public override Builder Clone() {\n"
    "  if (resultIsReadOnly) {\n"
    "    return new Builder(result);\n"
    "  } else {\n"
    "    return new Builder().MergeFrom(result);\n"
    "  }\n"
    "}\n"
    "\n");
  if (!use_lite_runtime()) {
    printer->Print(
      vars,
      "public override pbd::MessageDescriptor DescriptorForType {\n"
      "  get { return $full_class_name$.Descriptor; }\n"
      "}\n\n");
  }
  printer->Print(
    vars,
    "public override $class_name$ DefaultInstanceForType {\n"
    "  get { return $full_class_name$.DefaultInstance; }\n"
    "}\n\n");

  printer->Print(
    vars,
    "public override $class_name$ BuildPartial() {\n"
    "  if (resultIsReadOnly) {\n"
    "    return result;\n"
    "  }\n"
    "  resultIsReadOnly = true;\n"
    "  return result.MakeReadOnly();\n"
    "}\n\n");

  if (optimize_speed()) {
    printer->Print(
      vars,
      "public override Builder MergeFrom(pb::IMessage$suffix$ other) {\n"
      "  if (other is $class_name$) {\n"
      "    return MergeFrom(($class_name$) other);\n"
      "  } else {\n"
      "    base.MergeFrom(other);\n"
      "    return this;\n"
      "  }\n"
      "}\n\n");

    printer->Print(vars,"public override Builder MergeFrom($class_name$ other) {\n");
    // Optimization:  If other is the default instance, we know none of its
    // fields are set so we can skip the merge.
    printer->Indent();
    printer->Print(
      vars,
      "if (other == $full_class_name$.DefaultInstance) return this;\n"
      "PrepareBuilder();\n");
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
          "case $property_name$OneofCase.$field_property_name$: {\n");
	if (field->type() == FieldDescriptor::TYPE_GROUP ||
	    field->type() == FieldDescriptor::TYPE_MESSAGE) {
	  printer->Print(
            vars,
            "  Merge$field_property_name$(other.$field_property_name$);\n");
	} else {
	  printer->Print(
            vars,
            "  Set$field_property_name$(other.$field_property_name$);\n");
	}
        printer->Print("  break;\n");
	printer->Print("}\n");
      }
      printer->Print(vars, "case $property_name$OneofCase.None: { break; }\n");
      printer->Outdent();
      printer->Print("}\n");
    }

    // if message type has extensions
    if (descriptor_->extension_range_count() > 0) {
      printer->Print("  this.MergeExtensionFields(other);\n");
    }
    if (!use_lite_runtime()) {
      printer->Print("this.MergeUnknownFields(other.UnknownFields);\n");
    }
    printer->Print("return this;\n");
    printer->Outdent();
    printer->Print("}\n\n");
  }

}

void MessageGenerator::GenerateBuilderParsingMethods(io::Printer* printer) {
  printer->Print(
    "public override Builder MergeFrom(pb::ICodedInputStream input) {\n"
    "  return MergeFrom(input, pb::ExtensionRegistry.Empty);\n"
    "}\n\n");

  printer->Print(
    "public override Builder MergeFrom(pb::ICodedInputStream input, pb::ExtensionRegistry extensionRegistry) {\n");
  printer->Indent();
  printer->Print("PrepareBuilder();\n");
  if (!use_lite_runtime()) {
    printer->Print("pb::UnknownFieldSet.Builder unknownFields = null;\n");
  }
  printer->Print(
    "uint tag;\n"
    "string field_name;\n"
    "while (input.ReadTag(out tag, out field_name)) {\n");
  printer->Indent();
  printer->Print("if(tag == 0 && field_name != null) {\n");
  printer->Indent();
  //if you change from StringComparer.Ordinal, the array sort in FieldNames { get; } must also change
  printer->Print(
    "int field_ordinal = global::System.Array.BinarySearch(_$camel_class_name$FieldNames, field_name, global::System.StringComparer.Ordinal);\n"
    "if(field_ordinal >= 0)\n"
    "  tag = _$camel_class_name$FieldTags[field_ordinal];\n"
    "else {\n",
    "camel_class_name", UnderscoresToCamelCase(class_name(), false));
  if (!use_lite_runtime()) {
    printer->Print(
      "  if (unknownFields == null) {\n"  // First unknown field - create builder now
      "    unknownFields = pb::UnknownFieldSet.CreateBuilder(this.UnknownFields);\n"
      "  }\n");
  }
  printer->Print(
    "  ParseUnknownField(input, $prefix$extensionRegistry, tag, field_name);\n",
    "prefix", use_lite_runtime() ? "" : "unknownFields, ");
  printer->Print("  continue;\n");
  printer->Print("}\n");
  printer->Outdent();
  printer->Print("}\n");

  printer->Print("switch (tag) {\n");
  printer->Indent();
  printer->Print(
    "case 0: {\n"  // 0 signals EOF / limit reached
    "  throw pb::InvalidProtocolBufferException.InvalidTag();\n"
    "}\n"
    "default: {\n"
    "  if (pb::WireFormat.IsEndGroupTag(tag)) {\n");
  if (!use_lite_runtime()) {
    printer->Print(
      "    if (unknownFields != null) {\n"
      "      this.UnknownFields = unknownFields.Build();\n"
      "    }\n");
  }
  printer->Print(
    "    return this;\n"  // it's an endgroup tag
    "  }\n");
  if (!use_lite_runtime()) {
    printer->Print(
      "  if (unknownFields == null) {\n"  // First unknown field - create builder now
      "    unknownFields = pb::UnknownFieldSet.CreateBuilder(this.UnknownFields);\n"
      "  }\n");
  }
  printer->Print(
      "  ParseUnknownField(input, $prefix$extensionRegistry, tag, field_name);\n",
      "prefix", use_lite_runtime() ? "" : "unknownFields, ");
  printer->Print("  break;\n");
  printer->Print("}\n");

  for (int i = 0; i < fields_by_number().size(); i++) {
    const FieldDescriptor* field = fields_by_number()[i];
    internal::WireFormatLite::WireType wt =
        internal::WireFormat::WireTypeForFieldType(field->type());
    uint32 tag = internal::WireFormatLite::MakeTag(field->number(), wt);
    if (field->is_repeated()
        && (wt == internal::WireFormatLite::WIRETYPE_VARINT
            || wt == internal::WireFormatLite::WIRETYPE_FIXED32
            || wt == internal::WireFormatLite::WIRETYPE_FIXED64)) {
      printer->Print(
        "case $number$:\n",
        "number",
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
  printer->Print("}\n");
  printer->Outdent();
  printer->Print("}\n");
  printer->Print("\n");
  if (!use_lite_runtime()) {
    printer->Print(
      "if (unknownFields != null) {\n"
      "  this.UnknownFields = unknownFields.Build();\n"
      "}\n");
  }
  printer->Print("return this;\n");
  printer->Outdent();
  printer->Print("}\n\n");
}

void MessageGenerator::GenerateIsInitialized(io::Printer* printer) {
  printer->Print("public override bool IsInitialized {\n");
  printer->Indent();
  printer->Print("get {\n");
  printer->Indent();

  // Check that all required fields in this message are set.
  // TODO(kenton):  We can optimize this when we switch to putting all the
  // "has" fields into a single bitfield.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (descriptor_->field(i)->is_required()) {
      printer->Print("if (!has$property_name$) return false;\n",
		     "property_name", GetPropertyName(descriptor_->field(i)));
    }
  }

  // Now check that all embedded messages are initialized.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

      if (field->type() != FieldDescriptor::TYPE_MESSAGE ||
          !HasRequiredFields(field->message_type()))
      {
          continue;
      }
      // TODO(jtattermusch): shouldn't we use GetPropertyName here?
      string propertyName = UnderscoresToPascalCase(GetFieldName(field));
      if (field->is_repeated())
      {
        printer->Print(
          "foreach ($class_name$ element in $property_name$List) {\n"
          "  if (!element.IsInitialized) return false;\n"
          "}\n",
          "class_name", GetClassName(field->message_type()),
          "property_name", propertyName);
      }
      else if (field->is_optional())
      {
        printer->Print(
          "if (Has$property_name$) {\n"
          "  if (!$property_name$.IsInitialized) return false;\n"
          "}\n",
          "property_name", propertyName);
      }
      else
      {
        printer->Print(
          "if (!$property_name$.IsInitialized) return false;\n",
          "property_name", propertyName);
      }
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print("if (!ExtensionsAreInitialized) return false;\n");
  }
  printer->Print("return true;\n");
  printer->Outdent();
  printer->Print("}\n");
  printer->Outdent();
  printer->Print("}\n");
  printer->Print("\n");
}

void MessageGenerator::GenerateExtensionRegistrationCode(io::Printer* printer) {
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ExtensionGenerator extensionGenerator(descriptor_->extension(i));
    extensionGenerator.GenerateExtensionRegistrationCode(printer);
  }
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(descriptor_->nested_type(i));
    messageGenerator.GenerateExtensionRegistrationCode(printer);
  }
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
