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
#include <google/protobuf/compiler/csharp/csharp_writer.h>

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

void MessageGenerator::GenerateStaticVariables(Writer* writer) {
  // Because descriptor.proto (Google.ProtocolBuffers.DescriptorProtos) is
  // used in the construction of descriptors, we have a tricky bootstrapping
  // problem.  To help control static initialization order, we make sure all
  // descriptors and other static data that depends on them are members of
  // the proto-descriptor class.  This way, they will be initialized in
  // a deterministic order.

  std::string identifier = GetUniqueFileScopeIdentifier(descriptor_);

  if (!use_lite_runtime()) {
    // The descriptor for this type.
    std::string access = "internal";
    writer->WriteLine(
        "$0$ static pbd::MessageDescriptor internal__$1$__Descriptor;", access,
        identifier);
    writer->WriteLine(
        "$0$ static pb::FieldAccess.FieldAccessorTable<$1$, $1$.Builder> internal__$2$__FieldAccessorTable;",
        access, full_class_name(), identifier);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(descriptor_->nested_type(i));
    messageGenerator.GenerateStaticVariables(writer);
  }
}

void MessageGenerator::GenerateStaticVariableInitializers(Writer* writer) {
  std::string identifier = GetUniqueFileScopeIdentifier(descriptor_);

  if (!use_lite_runtime()) {
    writer->Write("internal__$0$__Descriptor = ", identifier);

    if (!descriptor_->containing_type()) {
      writer->WriteLine("Descriptor.MessageTypes[$0$];",
                        SimpleItoa(descriptor_->index()));
    } else {
      writer->WriteLine(
          "internal__$0$__Descriptor.NestedTypes[$1$];",
          GetUniqueFileScopeIdentifier(descriptor_->containing_type()),
          SimpleItoa(descriptor_->index()));
    }

    writer->WriteLine("internal__$0$__FieldAccessorTable = ", identifier);
    writer->WriteLine(
        "    new pb::FieldAccess.FieldAccessorTable<$1$, $1$.Builder>(internal__$0$__Descriptor,",
        identifier, full_class_name());
    writer->Write("        new string[] { ");
    for (int i = 0; i < descriptor_->field_count(); i++) {
      writer->Write("\"$0$\", ", GetPropertyName(descriptor_->field(i)));
    }
    writer->WriteLine("});");
  }

  // Generate static member initializers for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(descriptor_->nested_type(i));
    messageGenerator.GenerateStaticVariableInitializers(writer);
  }

  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ExtensionGenerator extensionGenerator(descriptor_->extension(i));
    extensionGenerator.GenerateStaticVariableInitializers(writer);
  }
}

void MessageGenerator::Generate(Writer* writer) {
  writer->WriteLine(
      "[global::System.Diagnostics.DebuggerNonUserCodeAttribute()]");
  WriteGeneratedCodeAttributes(writer);
  writer->WriteLine(
      "$0$ sealed partial class $1$ : pb::$2$Message$3$<$1$, $1$.Builder> {",
      class_access_level(), class_name(),
      descriptor_->extension_range_count() > 0 ? "Extendable" : "Generated",
      runtime_suffix());
  writer->Indent();
  writer->WriteLine("private $0$() { }", class_name());  // Private ctor.
  // Must call MakeReadOnly() to make sure all lists are made read-only
  writer->WriteLine(
      "private static readonly $0$ defaultInstance = new $0$().MakeReadOnly();",
      class_name());

  if (optimize_speed()) {
    writer->WriteLine(
        "private static readonly string[] _$0$FieldNames = new string[] { $2$$1$$2$ };",
        UnderscoresToCamelCase(class_name(), false),
        JoinStrings(field_names(), "\", \""),
        field_names().size() > 0 ? "\"" : "");
    std::vector<std::string> tags;
    for (int i = 0; i < field_names().size(); i++) {
      uint32 tag = internal::WireFormat::MakeTag(
          descriptor_->FindFieldByName(field_names()[i]));
      tags.push_back(SimpleItoa(tag));
    }
    writer->WriteLine(
        "private static readonly uint[] _$0$FieldTags = new uint[] { $1$ };",
        UnderscoresToCamelCase(class_name(), false), JoinStrings(tags, ", "));
  }
  writer->WriteLine("public static $0$ DefaultInstance {", class_name());
  writer->WriteLine("  get { return defaultInstance; }");
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine("public override $0$ DefaultInstanceForType {",
                    class_name());
  writer->WriteLine("  get { return DefaultInstance; }");
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine("protected override $0$ ThisMessage {", class_name());
  writer->WriteLine("  get { return this; }");
  writer->WriteLine("}");
  writer->WriteLine();
  if (!use_lite_runtime()) {
    writer->WriteLine("public static pbd::MessageDescriptor Descriptor {");
    writer->WriteLine("  get { return $0$.internal__$1$__Descriptor; }",
                      GetFullUmbrellaClassName(descriptor_->file()),
                      GetUniqueFileScopeIdentifier(descriptor_));
    writer->WriteLine("}");
    writer->WriteLine();
    writer->WriteLine(
        "protected override pb::FieldAccess.FieldAccessorTable<$0$, $0$.Builder> InternalFieldAccessors {",
        class_name());
    writer->WriteLine("  get { return $0$.internal__$1$__FieldAccessorTable; }",
                      GetFullUmbrellaClassName(descriptor_->file()),
                      GetUniqueFileScopeIdentifier(descriptor_));
    writer->WriteLine("}");
    writer->WriteLine();
  }

  // Extensions don't need to go in an extra nested type
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ExtensionGenerator extensionGenerator(descriptor_->extension(i));
    extensionGenerator.Generate(writer);
  }

  if (descriptor_->enum_type_count() + descriptor_->nested_type_count() > 0) {
    writer->WriteLine("#region Nested types");
    writer->WriteLine(
        "[global::System.Diagnostics.DebuggerNonUserCodeAttribute()]");
    WriteGeneratedCodeAttributes(writer);
    writer->WriteLine("public static partial class Types {");
    writer->Indent();
    for (int i = 0; i < descriptor_->enum_type_count(); i++) {
      EnumGenerator enumGenerator(descriptor_->enum_type(i));
      enumGenerator.Generate(writer);
    }
    for (int i = 0; i < descriptor_->nested_type_count(); i++) {
      MessageGenerator messageGenerator(descriptor_->nested_type(i));
      messageGenerator.Generate(writer);
    }
    writer->Outdent();
    writer->WriteLine("}");
    writer->WriteLine("#endregion");
    writer->WriteLine();
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* fieldDescriptor = descriptor_->field(i);

    // Rats: we lose the debug comment here :(
    writer->WriteLine("public const int $0$ = $1$;",
                      GetFieldConstantName(fieldDescriptor),
                      SimpleItoa(fieldDescriptor->number()));
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(fieldDescriptor));
    generator->GenerateMembers(writer);
    writer->WriteLine();
  }

  if (optimize_speed()) {
    if (SupportFieldPresence(descriptor_->file())) {
      GenerateIsInitialized(writer);
    }
    GenerateMessageSerializationMethods(writer);
  }
  if (use_lite_runtime()) {
    GenerateLiteRuntimeMethods(writer);
  }

  GenerateParseFromMethods(writer);
  GenerateBuilder(writer);

  // Force the static initialization code for the file to run, since it may
  // initialize static variables declared in this class.
  writer->WriteLine("static $0$() {", class_name());
  // We call object.ReferenceEquals() just to make it a valid statement on its own.
  // Another option would be GetType(), but that causes problems in DescriptorProtoFile,
  // where the bootstrapping is somewhat recursive - type initializers call
  // each other, effectively. We temporarily see Descriptor as null.
  writer->WriteLine("  object.ReferenceEquals($0$.Descriptor, null);",
                    GetFullUmbrellaClassName(descriptor_->file()));
  writer->WriteLine("}");

  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();

}

void MessageGenerator::GenerateLiteRuntimeMethods(Writer* writer) {
  bool callbase = descriptor_->extension_range_count() > 0;
  writer->WriteLine("#region Lite runtime methods");
  writer->WriteLine("public override int GetHashCode() {");
  writer->Indent();
  writer->WriteLine("int hash = GetType().GetHashCode();");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    generator->WriteHash(writer);
  }
  if (callbase) {
    writer->WriteLine("hash ^= base.GetHashCode();");
  }
  writer->WriteLine("return hash;");
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();

  writer->WriteLine("public override bool Equals(object obj) {");
  writer->Indent();
  writer->WriteLine("$0$ other = obj as $0$;", class_name());
  writer->WriteLine("if (other == null) return false;");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    generator->WriteEquals(writer);
  }
  if (callbase) {
    writer->WriteLine("if (!base.Equals(other)) return false;");
  }
  writer->WriteLine("return true;");
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();

  writer->WriteLine(
      "public override void PrintTo(global::System.IO.TextWriter writer) {");
  writer->Indent();

  for (int i = 0; i < fields_by_number().size(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(fields_by_number()[i]));
    generator->WriteToString(writer);
  }

  if (callbase) {
    writer->WriteLine("base.PrintTo(writer);");
  }
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine("#endregion");
  writer->WriteLine();
}

bool CompareExtensionRangesStart(const Descriptor::ExtensionRange* r1,
                                 const Descriptor::ExtensionRange* r2) {
  return r1->start < r2->start;
}

void MessageGenerator::GenerateMessageSerializationMethods(Writer* writer) {
  std::vector<const Descriptor::ExtensionRange*> extension_ranges_sorted;
  for (int i = 0; i < descriptor_->extension_range_count(); i++) {
    extension_ranges_sorted.push_back(descriptor_->extension_range(i));
  }
  std::sort(extension_ranges_sorted.begin(), extension_ranges_sorted.end(),
            CompareExtensionRangesStart);

  writer->WriteLine(
      "public override void WriteTo(pb::ICodedOutputStream output) {");
  writer->Indent();
  // Make sure we've computed the serialized length, so that packed fields are generated correctly.
  writer->WriteLine("CalcSerializedSize();");
  writer->WriteLine("string[] field_names = _$0$FieldNames;",
                    UnderscoresToCamelCase(class_name(), false));
  if (descriptor_->extension_range_count()) {
    writer->WriteLine(
        "pb::ExtendableMessage$1$<$0$, $0$.Builder>.ExtensionWriter extensionWriter = CreateExtensionWriter(this);",
        class_name(), runtime_suffix());
  }

  // Merge the fields and the extension ranges, both sorted by field number.
  for (int i = 0, j = 0;
      i < fields_by_number().size() || j < extension_ranges_sorted.size();) {
    if (i == fields_by_number().size()) {
      GenerateSerializeOneExtensionRange(writer, extension_ranges_sorted[j++]);
    } else if (j == extension_ranges_sorted.size()) {
      GenerateSerializeOneField(writer, fields_by_number()[i++]);
    } else if (fields_by_number()[i]->number()
        < extension_ranges_sorted[j]->start) {
      GenerateSerializeOneField(writer, fields_by_number()[i++]);
    } else {
      GenerateSerializeOneExtensionRange(writer, extension_ranges_sorted[j++]);
    }
  }

  if (!use_lite_runtime()) {
    if (descriptor_->options().message_set_wire_format())
    {
      writer->WriteLine("UnknownFields.WriteAsMessageSetTo(output);");
    } else {
      writer->WriteLine("UnknownFields.WriteTo(output);");
    }
  }

  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine("private int memoizedSerializedSize = -1;");
  writer->WriteLine("public override int SerializedSize {");
  writer->Indent();
  writer->WriteLine("get {");
  writer->Indent();
  writer->WriteLine("int size = memoizedSerializedSize;");
  writer->WriteLine("if (size != -1) return size;");
  writer->WriteLine("return CalcSerializedSize();");
  writer->Outdent();
  writer->WriteLine("}");
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();

  writer->WriteLine("private int CalcSerializedSize() {");
  writer->Indent();
  writer->WriteLine("int size = memoizedSerializedSize;");
  writer->WriteLine("if (size != -1) return size;");
  writer->WriteLine();
  writer->WriteLine("size = 0;");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    generator->GenerateSerializedSizeCode(writer);
  }
  if (descriptor_->extension_range_count() > 0) {
    writer->WriteLine("size += ExtensionsSerializedSize;");
  }

  if (!use_lite_runtime()) {
    if (descriptor_->options().message_set_wire_format()) {
      writer->WriteLine("size += UnknownFields.SerializedSizeAsMessageSet;");
    } else {
      writer->WriteLine("size += UnknownFields.SerializedSize;");
    }
  }
  writer->WriteLine("memoizedSerializedSize = size;");
  writer->WriteLine("return size;");
  writer->Outdent();
  writer->WriteLine("}");
}

void MessageGenerator::GenerateSerializeOneField(
    Writer* writer, const FieldDescriptor* fieldDescriptor) {
  scoped_ptr<FieldGeneratorBase> generator(
      CreateFieldGeneratorInternal(fieldDescriptor));
  generator->GenerateSerializationCode(writer);
}

void MessageGenerator::GenerateSerializeOneExtensionRange(
    Writer* writer, const Descriptor::ExtensionRange* extensionRange) {
  writer->WriteLine("extensionWriter.WriteUntil($0$, output);",
                    SimpleItoa(extensionRange->end));
}

void MessageGenerator::GenerateParseFromMethods(Writer* writer) {
  // Note:  These are separate from GenerateMessageSerializationMethods()
  //   because they need to be generated even for messages that are optimized
  //   for code size.

  writer->WriteLine("public static $0$ ParseFrom(pb::ByteString data) {",
                    class_name());
  writer->WriteLine(
      "  return ((Builder) CreateBuilder().MergeFrom(data)).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine(
      "public static $0$ ParseFrom(pb::ByteString data, pb::ExtensionRegistry extensionRegistry) {",
      class_name());
  writer->WriteLine(
      "  return ((Builder) CreateBuilder().MergeFrom(data, extensionRegistry)).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine("public static $0$ ParseFrom(byte[] data) {", class_name());
  writer->WriteLine(
      "  return ((Builder) CreateBuilder().MergeFrom(data)).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine(
      "public static $0$ ParseFrom(byte[] data, pb::ExtensionRegistry extensionRegistry) {",
      class_name());
  writer->WriteLine(
      "  return ((Builder) CreateBuilder().MergeFrom(data, extensionRegistry)).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine(
      "public static $0$ ParseFrom(global::System.IO.Stream input) {",
      class_name());
  writer->WriteLine(
      "  return ((Builder) CreateBuilder().MergeFrom(input)).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine(
      "public static $0$ ParseFrom(global::System.IO.Stream input, pb::ExtensionRegistry extensionRegistry) {",
      class_name());
  writer->WriteLine(
      "  return ((Builder) CreateBuilder().MergeFrom(input, extensionRegistry)).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine(
      "public static $0$ ParseDelimitedFrom(global::System.IO.Stream input) {",
      class_name());
  writer->WriteLine(
      "  return CreateBuilder().MergeDelimitedFrom(input).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine(
      "public static $0$ ParseDelimitedFrom(global::System.IO.Stream input, pb::ExtensionRegistry extensionRegistry) {",
      class_name());
  writer->WriteLine(
      "  return CreateBuilder().MergeDelimitedFrom(input, extensionRegistry).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine(
      "public static $0$ ParseFrom(pb::ICodedInputStream input) {",
      class_name());
  writer->WriteLine(
      "  return ((Builder) CreateBuilder().MergeFrom(input)).BuildParsed();");
  writer->WriteLine("}");
  writer->WriteLine(
      "public static $0$ ParseFrom(pb::ICodedInputStream input, pb::ExtensionRegistry extensionRegistry) {",
      class_name());
  writer->WriteLine(
      "  return ((Builder) CreateBuilder().MergeFrom(input, extensionRegistry)).BuildParsed();");
  writer->WriteLine("}");
}

void MessageGenerator::GenerateBuilder(Writer* writer) {
  writer->WriteLine("private $0$ MakeReadOnly() {", class_name());
  writer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    generator->GenerateBuildingCode(writer);
  }
  writer->WriteLine("return this;");
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();

  writer->WriteLine(
      "public static Builder CreateBuilder() { return new Builder(); }");
  writer->WriteLine(
      "public override Builder ToBuilder() { return CreateBuilder(this); }");
  writer->WriteLine(
      "public override Builder CreateBuilderForType() { return new Builder(); }");
  writer->WriteLine("public static Builder CreateBuilder($0$ prototype) {",
                    class_name());
  writer->WriteLine("  return new Builder(prototype);");
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine(
      "[global::System.Diagnostics.DebuggerNonUserCodeAttribute()]");
  WriteGeneratedCodeAttributes(writer);
  writer->WriteLine(
      "$0$ sealed partial class Builder : pb::$2$Builder$3$<$1$, Builder> {",
      class_access_level(), class_name(),
      descriptor_->extension_range_count() > 0 ? "Extendable" : "Generated",
      runtime_suffix());
  writer->Indent();
  writer->WriteLine("protected override Builder ThisBuilder {");
  writer->WriteLine("  get { return this; }");
  writer->WriteLine("}");
  GenerateCommonBuilderMethods(writer);
  if (optimize_speed()) {
    GenerateBuilderParsingMethods(writer);
  }
  for (int i = 0; i < descriptor_->field_count(); i++) {
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(descriptor_->field(i)));
    writer->WriteLine();
    // No field comment :(
    generator->GenerateBuilderMembers(writer);
  }
  writer->Outdent();
  writer->WriteLine("}");
}

void MessageGenerator::GenerateCommonBuilderMethods(Writer* writer) {
  //default constructor
  writer->WriteLine("public Builder() {");
  //Durring static initialization of message, DefaultInstance is expected to return null.
  writer->WriteLine("  result = DefaultInstance;");
  writer->WriteLine("  resultIsReadOnly = true;");
  writer->WriteLine("}");
  //clone constructor
  writer->WriteLine("internal Builder($0$ cloneFrom) {", class_name());
  writer->WriteLine("  result = cloneFrom;");
  writer->WriteLine("  resultIsReadOnly = true;");
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine("private bool resultIsReadOnly;");
  writer->WriteLine("private $0$ result;", class_name());
  writer->WriteLine();
  writer->WriteLine("private $0$ PrepareBuilder() {", class_name());
  writer->WriteLine("  if (resultIsReadOnly) {");
  writer->WriteLine("    $0$ original = result;", class_name());
  writer->WriteLine("    result = new $0$();", class_name());
  writer->WriteLine("    resultIsReadOnly = false;");
  writer->WriteLine("    MergeFrom(original);");
  writer->WriteLine("  }");
  writer->WriteLine("  return result;");
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine("public override bool IsInitialized {");
  writer->WriteLine("  get { return result.IsInitialized; }");
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine("protected override $0$ MessageBeingBuilt {", class_name());
  writer->WriteLine("  get { return PrepareBuilder(); }");
  writer->WriteLine("}");
  writer->WriteLine();
  //Not actually expecting that DefaultInstance would ever be null here; however, we will ensure it does not break
  writer->WriteLine("public override Builder Clear() {");
  writer->WriteLine("  result = DefaultInstance;");
  writer->WriteLine("  resultIsReadOnly = true;");
  writer->WriteLine("  return this;");
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine("public override Builder Clone() {");
  writer->WriteLine("  if (resultIsReadOnly) {");
  writer->WriteLine("    return new Builder(result);");
  writer->WriteLine("  } else {");
  writer->WriteLine("    return new Builder().MergeFrom(result);");
  writer->WriteLine("  }");
  writer->WriteLine("}");
  writer->WriteLine();
  if (!use_lite_runtime()) {
    writer->WriteLine(
        "public override pbd::MessageDescriptor DescriptorForType {");
    writer->WriteLine("  get { return $0$.Descriptor; }", full_class_name());
    writer->WriteLine("}");
    writer->WriteLine();
  }
  writer->WriteLine("public override $0$ DefaultInstanceForType {",
                    class_name());
  writer->WriteLine("  get { return $0$.DefaultInstance; }", full_class_name());
  writer->WriteLine("}");
  writer->WriteLine();

  writer->WriteLine("public override $0$ BuildPartial() {", class_name());
  writer->Indent();
  writer->WriteLine("if (resultIsReadOnly) {");
  writer->WriteLine("  return result;");
  writer->WriteLine("}");
  writer->WriteLine("resultIsReadOnly = true;");
  writer->WriteLine("return result.MakeReadOnly();");
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();

  if (optimize_speed()) {
    writer->WriteLine(
        "public override Builder MergeFrom(pb::IMessage$0$ other) {",
        runtime_suffix());
    writer->WriteLine("  if (other is $0$) {", class_name());
    writer->WriteLine("    return MergeFrom(($0$) other);", class_name());
    writer->WriteLine("  } else {");
    writer->WriteLine("    base.MergeFrom(other);");
    writer->WriteLine("    return this;");
    writer->WriteLine("  }");
    writer->WriteLine("}");
    writer->WriteLine();
    writer->WriteLine("public override Builder MergeFrom($0$ other) {",
                      class_name());
    // Optimization:  If other is the default instance, we know none of its
    // fields are set so we can skip the merge.
    writer->Indent();
    writer->WriteLine("if (other == $0$.DefaultInstance) return this;",
                      full_class_name());
    writer->WriteLine("PrepareBuilder();");
    for (int i = 0; i < descriptor_->field_count(); i++) {
      scoped_ptr<FieldGeneratorBase> generator(
          CreateFieldGeneratorInternal(descriptor_->field(i)));
      generator->GenerateMergingCode(writer);
    }
    // if message type has extensions
    if (descriptor_->extension_range_count() > 0) {
      writer->WriteLine("  this.MergeExtensionFields(other);");
    }
    if (!use_lite_runtime()) {
      writer->WriteLine("this.MergeUnknownFields(other.UnknownFields);");
    }
    writer->WriteLine("return this;");
    writer->Outdent();
    writer->WriteLine("}");
    writer->WriteLine();
  }

}

void MessageGenerator::GenerateBuilderParsingMethods(Writer* writer) {
  writer->WriteLine(
      "public override Builder MergeFrom(pb::ICodedInputStream input) {");
  writer->WriteLine("  return MergeFrom(input, pb::ExtensionRegistry.Empty);");
  writer->WriteLine("}");
  writer->WriteLine();
  writer->WriteLine(
      "public override Builder MergeFrom(pb::ICodedInputStream input, pb::ExtensionRegistry extensionRegistry) {");
  writer->Indent();
  writer->WriteLine("PrepareBuilder();");
  if (!use_lite_runtime()) {
    writer->WriteLine("pb::UnknownFieldSet.Builder unknownFields = null;");
  }
  writer->WriteLine("uint tag;");
  writer->WriteLine("string field_name;");
  writer->WriteLine("while (input.ReadTag(out tag, out field_name)) {");
  writer->Indent();
  writer->WriteLine("if(tag == 0 && field_name != null) {");
  writer->Indent();
  //if you change from StringComparer.Ordinal, the array sort in FieldNames { get; } must also change
  writer->WriteLine(
      "int field_ordinal = global::System.Array.BinarySearch(_$0$FieldNames, field_name, global::System.StringComparer.Ordinal);",
      UnderscoresToCamelCase(class_name(), false));
  writer->WriteLine("if(field_ordinal >= 0)");
  writer->WriteLine("  tag = _$0$FieldTags[field_ordinal];",
                    UnderscoresToCamelCase(class_name(), false));
  writer->WriteLine("else {");
  if (!use_lite_runtime()) {
    writer->WriteLine("  if (unknownFields == null) {");  // First unknown field - create builder now
    writer->WriteLine(
        "    unknownFields = pb::UnknownFieldSet.CreateBuilder(this.UnknownFields);");
    writer->WriteLine("  }");
  }
  writer->WriteLine(
      "  ParseUnknownField(input, $0$extensionRegistry, tag, field_name);",
      use_lite_runtime() ? "" : "unknownFields, ");
  writer->WriteLine("  continue;");
  writer->WriteLine("}");
  writer->Outdent();
  writer->WriteLine("}");

  writer->WriteLine("switch (tag) {");
  writer->Indent();
  writer->WriteLine("case 0: {");  // 0 signals EOF / limit reached
  writer->WriteLine("  throw pb::InvalidProtocolBufferException.InvalidTag();");
  writer->WriteLine("}");
  writer->WriteLine("default: {");
  writer->WriteLine("  if (pb::WireFormat.IsEndGroupTag(tag)) {");
  if (!use_lite_runtime()) {
    writer->WriteLine("    if (unknownFields != null) {");
    writer->WriteLine("      this.UnknownFields = unknownFields.Build();");
    writer->WriteLine("    }");
  }
  writer->WriteLine("    return this;");  // it's an endgroup tag
  writer->WriteLine("  }");
  if (!use_lite_runtime()) {
    writer->WriteLine("  if (unknownFields == null) {");  // First unknown field - create builder now
    writer->WriteLine(
        "    unknownFields = pb::UnknownFieldSet.CreateBuilder(this.UnknownFields);");
    writer->WriteLine("  }");
  }
  writer->WriteLine(
      "  ParseUnknownField(input, $0$extensionRegistry, tag, field_name);",
      use_lite_runtime() ? "" : "unknownFields, ");
  writer->WriteLine("  break;");
  writer->WriteLine("}");

  for (int i = 0; i < fields_by_number().size(); i++) {
    const FieldDescriptor* field = fields_by_number()[i];
    internal::WireFormatLite::WireType wt =
        internal::WireFormat::WireTypeForFieldType(field->type());
    uint32 tag = internal::WireFormatLite::MakeTag(field->number(), wt);
    if (field->is_repeated()
        && (wt == internal::WireFormatLite::WIRETYPE_VARINT
            || wt == internal::WireFormatLite::WIRETYPE_FIXED32
            || wt == internal::WireFormatLite::WIRETYPE_FIXED64)) {
      writer->WriteLine(
          "case $0$:",
          SimpleItoa(
              internal::WireFormatLite::MakeTag(
                  field->number(),
                  internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED)));
    }

    writer->WriteLine("case $0$: {", SimpleItoa(tag));
    writer->Indent();
    scoped_ptr<FieldGeneratorBase> generator(
        CreateFieldGeneratorInternal(field));
    generator->GenerateParsingCode(writer);
    writer->WriteLine("break;");
    writer->Outdent();
    writer->WriteLine("}");
  }

  writer->Outdent();
  writer->WriteLine("}");
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();
  if (!use_lite_runtime()) {
    writer->WriteLine("if (unknownFields != null) {");
    writer->WriteLine("  this.UnknownFields = unknownFields.Build();");
    writer->WriteLine("}");
  }
  writer->WriteLine("return this;");
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();
}

void MessageGenerator::GenerateIsInitialized(Writer* writer) {
  writer->WriteLine("public override bool IsInitialized {");
  writer->Indent();
  writer->WriteLine("get {");
  writer->Indent();

  // Check that all required fields in this message are set.
  // TODO(kenton):  We can optimize this when we switch to putting all the
  // "has" fields into a single bitfield.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (descriptor_->field(i)->is_required()) {
      writer->WriteLine("if (!has$0$) return false;",
                        GetPropertyName(descriptor_->field(i)));
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
          writer->WriteLine("foreach ($0$ element in $1$List) {",
                           GetClassName(field->message_type()),
                           propertyName);
          writer->WriteLine("  if (!element.IsInitialized) return false;");
          writer->WriteLine("}");
      }
      else if (field->is_optional())
      {
          writer->WriteLine("if (Has$0$) {", propertyName);
          writer->WriteLine("  if (!$0$.IsInitialized) return false;", propertyName);
          writer->WriteLine("}");
      }
      else
      {
          writer->WriteLine("if (!$0$.IsInitialized) return false;", propertyName);
      }
  }

  if (descriptor_->extension_range_count() > 0) {
    writer->WriteLine("if (!ExtensionsAreInitialized) return false;");
  }
  writer->WriteLine("return true;");
  writer->Outdent();
  writer->WriteLine("}");
  writer->Outdent();
  writer->WriteLine("}");
  writer->WriteLine();
}

void MessageGenerator::GenerateExtensionRegistrationCode(Writer* writer) {
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ExtensionGenerator extensionGenerator(descriptor_->extension(i));
    extensionGenerator.GenerateExtensionRegistrationCode(writer);
  }
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator messageGenerator(descriptor_->nested_type(i));
    messageGenerator.GenerateExtensionRegistrationCode(writer);
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
