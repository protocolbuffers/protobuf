// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <algorithm>
#include <google/protobuf/stubs/hash.h>
#include <google/protobuf/compiler/java/java_message.h>
#include <google/protobuf/compiler/java/java_enum.h>
#include <google/protobuf/compiler/java/java_extension.h>
#include <google/protobuf/compiler/java/java_helpers.h>
#include <google/protobuf/stubs/strutil.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_inl.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

using internal::WireFormat;

namespace {

void PrintFieldComment(io::Printer* printer, const FieldDescriptor* field) {
  // Print the field's proto-syntax definition as a comment.  We don't want to
  // print group bodies so we cut off after the first line.
  string def = field->DebugString();
  printer->Print("// $def$\n",
    "def", def.substr(0, def.find_first_of('\n')));
}

struct FieldOrderingByNumber {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) const {
    return a->number() < b->number();
  }
};

struct ExtensionRangeOrdering {
  bool operator()(const Descriptor::ExtensionRange* a,
                  const Descriptor::ExtensionRange* b) const {
    return a->start < b->start;
  }
};

// Sort the fields of the given Descriptor by number into a new[]'d array
// and return it.
const FieldDescriptor** SortFieldsByNumber(const Descriptor* descriptor) {
  const FieldDescriptor** fields =
    new const FieldDescriptor*[descriptor->field_count()];
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields[i] = descriptor->field(i);
  }
  sort(fields, fields + descriptor->field_count(),
       FieldOrderingByNumber());
  return fields;
}

// Get an identifier that uniquely identifies this type within the file.
// This is used to declare static variables related to this type at the
// outermost file scope.
string UniqueFileScopeIdentifier(const Descriptor* descriptor) {
  return "static_" + StringReplace(descriptor->full_name(), ".", "_", true);
}

// Returns true if the message type has any required fields.  If it doesn't,
// we can optimize out calls to its isInitialized() method.
//
// already_seen is used to avoid checking the same type multiple times
// (and also to protect against recursion).
static bool HasRequiredFields(
    const Descriptor* type,
    hash_set<const Descriptor*>* already_seen) {
  if (already_seen->count(type) > 0) {
    // The type is already in cache.  This means that either:
    // a. The type has no required fields.
    // b. We are in the midst of checking if the type has required fields,
    //    somewhere up the stack.  In this case, we know that if the type
    //    has any required fields, they'll be found when we return to it,
    //    and the whole call to HasRequiredFields() will return true.
    //    Therefore, we don't have to check if this type has required fields
    //    here.
    return false;
  }
  already_seen->insert(type);

  // If the type has extensions, an extension with message type could contain
  // required fields, so we have to be conservative and assume such an
  // extension exists.
  if (type->extension_range_count() > 0) return true;

  for (int i = 0; i < type->field_count(); i++) {
    const FieldDescriptor* field = type->field(i);
    if (field->is_required()) {
      return true;
    }
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (HasRequiredFields(field->message_type(), already_seen)) {
        return true;
      }
    }
  }

  return false;
}

static bool HasRequiredFields(const Descriptor* type) {
  hash_set<const Descriptor*> already_seen;
  return HasRequiredFields(type, &already_seen);
}

}  // namespace

// ===================================================================

MessageGenerator::MessageGenerator(const Descriptor* descriptor)
  : descriptor_(descriptor),
    field_generators_(descriptor) {
}

MessageGenerator::~MessageGenerator() {}

void MessageGenerator::GenerateStaticVariables(io::Printer* printer) {
  // Because descriptor.proto (com.google.protobuf.DescriptorProtos) is
  // used in the construction of descriptors, we have a tricky bootstrapping
  // problem.  To help control static initialization order, we make sure all
  // descriptors and other static data that depends on them are members of
  // the outermost class in the file.  This way, they will be initialized in
  // a deterministic order.

  map<string, string> vars;
  vars["identifier"] = UniqueFileScopeIdentifier(descriptor_);
  vars["index"] = SimpleItoa(descriptor_->index());
  vars["classname"] = ClassName(descriptor_);
  if (descriptor_->containing_type() != NULL) {
    vars["parent"] = UniqueFileScopeIdentifier(descriptor_->containing_type());
  }
  if (descriptor_->file()->options().java_multiple_files()) {
    // We can only make these package-private since the classes that use them
    // are in separate files.
    vars["private"] = "";
  } else {
    vars["private"] = "private ";
  }

  // The descriptor for this type.
  printer->Print(vars,
    "$private$static com.google.protobuf.Descriptors.Descriptor\n"
    "  internal_$identifier$_descriptor;\n");

  // And the FieldAccessorTable.
  printer->Print(vars,
    "$private$static\n"
    "  com.google.protobuf.GeneratedMessage.FieldAccessorTable\n"
    "    internal_$identifier$_fieldAccessorTable;\n");

  // Generate static members for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO(kenton):  Reuse MessageGenerator objects?
    MessageGenerator(descriptor_->nested_type(i))
      .GenerateStaticVariables(printer);
  }
}

void MessageGenerator::GenerateStaticVariableInitializers(
    io::Printer* printer) {
  map<string, string> vars;
  vars["identifier"] = UniqueFileScopeIdentifier(descriptor_);
  vars["index"] = SimpleItoa(descriptor_->index());
  vars["classname"] = ClassName(descriptor_);
  if (descriptor_->containing_type() != NULL) {
    vars["parent"] = UniqueFileScopeIdentifier(descriptor_->containing_type());
  }

  // The descriptor for this type.
  if (descriptor_->containing_type() == NULL) {
    printer->Print(vars,
      "internal_$identifier$_descriptor =\n"
      "  getDescriptor().getMessageTypes().get($index$);\n");
  } else {
    printer->Print(vars,
      "internal_$identifier$_descriptor =\n"
      "  internal_$parent$_descriptor.getNestedTypes().get($index$);\n");
  }

  // And the FieldAccessorTable.
  printer->Print(vars,
    "internal_$identifier$_fieldAccessorTable = new\n"
    "  com.google.protobuf.GeneratedMessage.FieldAccessorTable(\n"
    "    internal_$identifier$_descriptor,\n"
    "    new java.lang.String[] { ");
  for (int i = 0; i < descriptor_->field_count(); i++) {
    printer->Print(
      "\"$field_name$\", ",
      "field_name",
        UnderscoresToCapitalizedCamelCase(descriptor_->field(i)));
  }
  printer->Print("},\n"
    "    $classname$.class,\n"
    "    $classname$.Builder.class);\n",
    "classname", ClassName(descriptor_));

  // Generate static member initializers for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO(kenton):  Reuse MessageGenerator objects?
    MessageGenerator(descriptor_->nested_type(i))
      .GenerateStaticVariableInitializers(printer);
  }

  for (int i = 0; i < descriptor_->extension_count(); i++) {
    // TODO(kenton):  Reuse ExtensionGenerator objects?
    ExtensionGenerator(descriptor_->extension(i))
      .GenerateInitializationCode(printer);
  }
}

void MessageGenerator::Generate(io::Printer* printer) {
  bool is_own_file =
    descriptor_->containing_type() == NULL &&
    descriptor_->file()->options().java_multiple_files();

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "public $static$ final class $classname$ extends\n"
      "    com.google.protobuf.GeneratedMessage.ExtendableMessage<\n"
      "      $classname$> {\n",
      "static", is_own_file ? "" : "static",
      "classname", descriptor_->name());
  } else {
    printer->Print(
      "public $static$ final class $classname$ extends\n"
      "    com.google.protobuf.GeneratedMessage {\n",
      "static", is_own_file ? "" : "static",
      "classname", descriptor_->name());
  }
  printer->Indent();
  printer->Print(
    "// Use $classname$.newBuilder() to construct.\n"
    "private $classname$() {}\n"
    "\n"
    "private static final $classname$ defaultInstance = new $classname$();\n"
    "public static $classname$ getDefaultInstance() {\n"
    "  return defaultInstance;\n"
    "}\n"
    "\n"
    "public $classname$ getDefaultInstanceForType() {\n"
    "  return defaultInstance;\n"
    "}\n"
    "\n",
    "classname", descriptor_->name());
  printer->Print(
    "public static final com.google.protobuf.Descriptors.Descriptor\n"
    "    getDescriptor() {\n"
    "  return $fileclass$.internal_$identifier$_descriptor;\n"
    "}\n"
    "\n"
    "@Override\n"
    "protected com.google.protobuf.GeneratedMessage.FieldAccessorTable\n"
    "    internalGetFieldAccessorTable() {\n"
    "  return $fileclass$.internal_$identifier$_fieldAccessorTable;\n"
    "}\n"
    "\n",
    "fileclass", ClassName(descriptor_->file()),
    "identifier", UniqueFileScopeIdentifier(descriptor_));

  // Nested types and extensions
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    EnumGenerator(descriptor_->enum_type(i)).Generate(printer);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator(descriptor_->nested_type(i)).Generate(printer);
  }

  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ExtensionGenerator(descriptor_->extension(i)).Generate(printer);
  }

  // Fields
  for (int i = 0; i < descriptor_->field_count(); i++) {
    PrintFieldComment(printer, descriptor_->field(i));
    field_generators_.get(descriptor_->field(i)).GenerateMembers(printer);
    printer->Print("\n");
  }

  if (descriptor_->file()->options().optimize_for() == FileOptions::SPEED) {
    GenerateIsInitialized(printer);
    GenerateMessageSerializationMethods(printer);
  }

  GenerateParseFromMethods(printer);
  GenerateBuilder(printer);

  // Force the static initialization code for the file to run, since it may
  // initialize static variables declared in this class.
  printer->Print(
    "\n"
    "static {\n"
    "  $file$.getDescriptor();\n"
    "}\n",
    "file", ClassName(descriptor_->file()));

  printer->Outdent();
  printer->Print("}\n\n");
}

// ===================================================================

void MessageGenerator::
GenerateMessageSerializationMethods(io::Printer* printer) {
  scoped_array<const FieldDescriptor*> sorted_fields(
    SortFieldsByNumber(descriptor_));

  vector<const Descriptor::ExtensionRange*> sorted_extensions;
  for (int i = 0; i < descriptor_->extension_range_count(); ++i) {
    sorted_extensions.push_back(descriptor_->extension_range(i));
  }
  sort(sorted_extensions.begin(), sorted_extensions.end(),
       ExtensionRangeOrdering());

  printer->Print(
    "@Override\n"
    "public void writeTo(com.google.protobuf.CodedOutputStream output)\n"
    "                    throws java.io.IOException {\n");
  printer->Indent();

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "com.google.protobuf.GeneratedMessage.ExtendableMessage\n"
      "  .ExtensionWriter extensionWriter = newExtensionWriter();\n");
  }

  // Merge the fields and the extension ranges, both sorted by field number.
  for (int i = 0, j = 0;
       i < descriptor_->field_count() || j < sorted_extensions.size();
       ) {
    if (i == descriptor_->field_count()) {
      GenerateSerializeOneExtensionRange(printer, sorted_extensions[j++]);
    } else if (j == sorted_extensions.size()) {
      GenerateSerializeOneField(printer, sorted_fields[i++]);
    } else if (sorted_fields[i]->number() < sorted_extensions[j]->start) {
      GenerateSerializeOneField(printer, sorted_fields[i++]);
    } else {
      GenerateSerializeOneExtensionRange(printer, sorted_extensions[j++]);
    }
  }

  if (descriptor_->options().message_set_wire_format()) {
    printer->Print(
      "getUnknownFields().writeAsMessageSetTo(output);\n");
  } else {
    printer->Print(
      "getUnknownFields().writeTo(output);\n");
  }

  printer->Outdent();
  printer->Print(
    "}\n"
    "\n"
    "private int memoizedSerializedSize = -1;\n"
    "@Override\n"
    "public int getSerializedSize() {\n"
    "  int size = memoizedSerializedSize;\n"
    "  if (size != -1) return size;\n"
    "\n"
    "  size = 0;\n");
  printer->Indent();

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(sorted_fields[i]).GenerateSerializedSizeCode(printer);
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "size += extensionsSerializedSize();\n");
  }

  if (descriptor_->options().message_set_wire_format()) {
    printer->Print(
      "size += getUnknownFields().getSerializedSizeAsMessageSet();\n");
  } else {
    printer->Print(
      "size += getUnknownFields().getSerializedSize();\n");
  }

  printer->Outdent();
  printer->Print(
    "  memoizedSerializedSize = size;\n"
    "  return size;\n"
    "}\n"
    "\n");
}

void MessageGenerator::
GenerateParseFromMethods(io::Printer* printer) {
  // Note:  These are separate from GenerateMessageSerializationMethods()
  //   because they need to be generated even for messages that are optimized
  //   for code size.
  printer->Print(
    "public static $classname$ parseFrom(\n"
    "    com.google.protobuf.ByteString data)\n"
    "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
    "  return newBuilder().mergeFrom(data).buildParsed();\n"
    "}\n"
    "public static $classname$ parseFrom(\n"
    "    com.google.protobuf.ByteString data,\n"
    "    com.google.protobuf.ExtensionRegistry extensionRegistry)\n"
    "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
    "  return newBuilder().mergeFrom(data, extensionRegistry)\n"
    "           .buildParsed();\n"
    "}\n"
    "public static $classname$ parseFrom(byte[] data)\n"
    "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
    "  return newBuilder().mergeFrom(data).buildParsed();\n"
    "}\n"
    "public static $classname$ parseFrom(\n"
    "    byte[] data,\n"
    "    com.google.protobuf.ExtensionRegistry extensionRegistry)\n"
    "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
    "  return newBuilder().mergeFrom(data, extensionRegistry)\n"
    "           .buildParsed();\n"
    "}\n"
    "public static $classname$ parseFrom(java.io.InputStream input)\n"
    "    throws java.io.IOException {\n"
    "  return newBuilder().mergeFrom(input).buildParsed();\n"
    "}\n"
    "public static $classname$ parseFrom(\n"
    "    java.io.InputStream input,\n"
    "    com.google.protobuf.ExtensionRegistry extensionRegistry)\n"
    "    throws java.io.IOException {\n"
    "  return newBuilder().mergeFrom(input, extensionRegistry)\n"
    "           .buildParsed();\n"
    "}\n"
    "public static $classname$ parseFrom(\n"
    "    com.google.protobuf.CodedInputStream input)\n"
    "    throws java.io.IOException {\n"
    "  return newBuilder().mergeFrom(input).buildParsed();\n"
    "}\n"
    "public static $classname$ parseFrom(\n"
    "    com.google.protobuf.CodedInputStream input,\n"
    "    com.google.protobuf.ExtensionRegistry extensionRegistry)\n"
    "    throws java.io.IOException {\n"
    "  return newBuilder().mergeFrom(input, extensionRegistry)\n"
    "           .buildParsed();\n"
    "}\n"
    "\n",
    "classname", ClassName(descriptor_));
}

void MessageGenerator::GenerateSerializeOneField(
    io::Printer* printer, const FieldDescriptor* field) {
  field_generators_.get(field).GenerateSerializationCode(printer);
}

void MessageGenerator::GenerateSerializeOneExtensionRange(
    io::Printer* printer, const Descriptor::ExtensionRange* range) {
  printer->Print(
    "extensionWriter.writeUntil($end$, output);\n",
    "end", SimpleItoa(range->end));
}

// ===================================================================

void MessageGenerator::GenerateBuilder(io::Printer* printer) {
  printer->Print(
    "public static Builder newBuilder() { return new Builder(); }\n"
    "public Builder newBuilderForType() { return new Builder(); }\n"
    "public static Builder newBuilder($classname$ prototype) {\n"
    "  return new Builder().mergeFrom(prototype);\n"
    "}\n"
    "public Builder toBuilder() { return newBuilder(this); }\n"
    "\n",
    "classname", ClassName(descriptor_));

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "public static final class Builder extends\n"
      "    com.google.protobuf.GeneratedMessage.ExtendableBuilder<\n"
      "      $classname$, Builder> {\n",
      "classname", ClassName(descriptor_));
  } else {
    printer->Print(
      "public static final class Builder extends\n"
      "    com.google.protobuf.GeneratedMessage.Builder<Builder> {\n",
      "classname", ClassName(descriptor_));
  }
  printer->Indent();

  GenerateCommonBuilderMethods(printer);

  if (descriptor_->file()->options().optimize_for() == FileOptions::SPEED) {
    GenerateBuilderParsingMethods(printer);
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    printer->Print("\n");
    PrintFieldComment(printer, descriptor_->field(i));
    field_generators_.get(descriptor_->field(i))
                     .GenerateBuilderMembers(printer);
  }

  printer->Outdent();
  printer->Print("}\n");
}

// ===================================================================

void MessageGenerator::GenerateCommonBuilderMethods(io::Printer* printer) {
  printer->Print(
    "// Construct using $classname$.newBuilder()\n"
    "private Builder() {}\n"
    "\n"
    "$classname$ result = new $classname$();\n"
    "\n"
    "@Override\n"
    "protected $classname$ internalGetResult() {\n"
    "  return result;\n"
    "}\n"
    "\n"
    "@Override\n"
    "public Builder clear() {\n"
    "  result = new $classname$();\n"
    "  return this;\n"
    "}\n"
    "\n"
    "@Override\n"
    "public Builder clone() {\n"
    "  return new Builder().mergeFrom(result);\n"
    "}\n"
    "\n"
    "@Override\n"
    "public com.google.protobuf.Descriptors.Descriptor\n"
    "    getDescriptorForType() {\n"
    "  return $classname$.getDescriptor();\n"
    "}\n"
    "\n"
    "public $classname$ getDefaultInstanceForType() {\n"
    "  return $classname$.getDefaultInstance();\n"
    "}\n"
    "\n",
    "classname", ClassName(descriptor_));

  // -----------------------------------------------------------------

  printer->Print(
    "public $classname$ build() {\n"
    "  if (!isInitialized()) {\n"
    "    throw new com.google.protobuf.UninitializedMessageException(\n"
    "      result);\n"
    "  }\n"
    "  return buildPartial();\n"
    "}\n"
    "\n"
    "private $classname$ buildParsed()\n"
    "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
    "  if (!isInitialized()) {\n"
    "    throw new com.google.protobuf.UninitializedMessageException(\n"
    "      result).asInvalidProtocolBufferException();\n"
    "  }\n"
    "  return buildPartial();\n"
    "}\n"
    "\n"
    "public $classname$ buildPartial() {\n",
    "classname", ClassName(descriptor_));
  printer->Indent();

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i)).GenerateBuildingCode(printer);
  }

  printer->Outdent();
  printer->Print(
    "  $classname$ returnMe = result;\n"
    "  result = null;\n"
    "  return returnMe;\n"
    "}\n"
    "\n",
    "classname", ClassName(descriptor_));

  // -----------------------------------------------------------------

  if (descriptor_->file()->options().optimize_for() == FileOptions::SPEED) {
    printer->Print(
      "@Override\n"
      "public Builder mergeFrom(com.google.protobuf.Message other) {\n"
      "  if (other instanceof $classname$) {\n"
      "    return mergeFrom(($classname$)other);\n"
      "  } else {\n"
      "    super.mergeFrom(other);\n"
      "    return this;\n"
      "  }\n"
      "}\n"
      "\n"
      "public Builder mergeFrom($classname$ other) {\n"
      // Optimization:  If other is the default instance, we know none of its
      //   fields are set so we can skip the merge.
      "  if (other == $classname$.getDefaultInstance()) return this;\n",
      "classname", ClassName(descriptor_));
    printer->Indent();

    for (int i = 0; i < descriptor_->field_count(); i++) {
      field_generators_.get(descriptor_->field(i)).GenerateMergingCode(printer);
    }

    printer->Outdent();

    // if message type has extensions
    if (descriptor_->extension_range_count() > 0) {
      printer->Print(
        "  this.mergeExtensionFields(other);\n");
    }

    printer->Print(
      "  this.mergeUnknownFields(other.getUnknownFields());\n"
      "  return this;\n"
      "}\n"
      "\n");
  }
}

// ===================================================================

void MessageGenerator::GenerateBuilderParsingMethods(io::Printer* printer) {
  scoped_array<const FieldDescriptor*> sorted_fields(
    SortFieldsByNumber(descriptor_));

  printer->Print(
    "@Override\n"
    "public Builder mergeFrom(\n"
    "    com.google.protobuf.CodedInputStream input)\n"
    "    throws java.io.IOException {\n"
    "  return mergeFrom(input,\n"
    "    com.google.protobuf.ExtensionRegistry.getEmptyRegistry());\n"
    "}\n"
    "\n"
    "@Override\n"
    "public Builder mergeFrom(\n"
    "    com.google.protobuf.CodedInputStream input,\n"
    "    com.google.protobuf.ExtensionRegistry extensionRegistry)\n"
    "    throws java.io.IOException {\n");
  printer->Indent();

  printer->Print(
    "com.google.protobuf.UnknownFieldSet.Builder unknownFields =\n"
    "  com.google.protobuf.UnknownFieldSet.newBuilder(\n"
    "    this.getUnknownFields());\n"
    "while (true) {\n");
  printer->Indent();

  printer->Print(
    "int tag = input.readTag();\n"
    "switch (tag) {\n");
  printer->Indent();

  printer->Print(
    "case 0:\n"          // zero signals EOF / limit reached
    "  this.setUnknownFields(unknownFields.build());\n"
    "  return this;\n"
    "default: {\n"
    "  if (!parseUnknownField(input, unknownFields,\n"
    "                         extensionRegistry, tag)) {\n"
    "    this.setUnknownFields(unknownFields.build());\n"
    "    return this;\n"   // it's an endgroup tag
    "  }\n"
    "  break;\n"
    "}\n");

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = sorted_fields[i];
    uint32 tag = WireFormat::MakeTag(field->number(),
      WireFormat::WireTypeForField(field));

    printer->Print(
      "case $tag$: {\n",
      "tag", SimpleItoa(tag));
    printer->Indent();

    field_generators_.get(field).GenerateParsingCode(printer);

    printer->Outdent();
    printer->Print(
      "  break;\n"
      "}\n");
  }

  printer->Outdent();
  printer->Outdent();
  printer->Outdent();
  printer->Print(
    "    }\n"     // switch (tag)
    "  }\n"       // while (true)
    "}\n"
    "\n");
}

// ===================================================================

void MessageGenerator::GenerateIsInitialized(io::Printer* printer) {
  printer->Print(
    "@Override\n"
    "public final boolean isInitialized() {\n");
  printer->Indent();

  // Check that all required fields in this message are set.
  // TODO(kenton):  We can optimize this when we switch to putting all the
  //   "has" fields into a single bitfield.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);

    if (field->is_required()) {
      printer->Print(
        "if (!has$name$) return false;\n",
        "name", UnderscoresToCapitalizedCamelCase(field));
    }
  }

  // Now check that all embedded messages are initialized.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE &&
        HasRequiredFields(field->message_type())) {
      switch (field->label()) {
        case FieldDescriptor::LABEL_REQUIRED:
          printer->Print(
            "if (!get$name$().isInitialized()) return false;\n",
            "type", ClassName(field->message_type()),
            "name", UnderscoresToCapitalizedCamelCase(field));
          break;
        case FieldDescriptor::LABEL_OPTIONAL:
          printer->Print(
            "if (has$name$()) {\n"
            "  if (!get$name$().isInitialized()) return false;\n"
            "}\n",
            "type", ClassName(field->message_type()),
            "name", UnderscoresToCapitalizedCamelCase(field));
          break;
        case FieldDescriptor::LABEL_REPEATED:
          printer->Print(
            "for ($type$ element : get$name$List()) {\n"
            "  if (!element.isInitialized()) return false;\n"
            "}\n",
            "type", ClassName(field->message_type()),
            "name", UnderscoresToCapitalizedCamelCase(field));
          break;
      }
    }
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
      "if (!extensionsAreInitialized()) return false;\n");
  }

  printer->Outdent();
  printer->Print(
    "  return true;\n"
    "}\n"
    "\n");
}

// ===================================================================

void MessageGenerator::GenerateExtensionRegistrationCode(io::Printer* printer) {
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ExtensionGenerator(descriptor_->extension(i))
      .GenerateRegistrationCode(printer);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    MessageGenerator(descriptor_->nested_type(i))
      .GenerateExtensionRegistrationCode(printer);
  }
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
