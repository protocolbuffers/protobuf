// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/full/message.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/field_common.h"
#include "google/protobuf/compiler/java/generator_common.h"
#include "google/protobuf/compiler/java/generator_factory.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/full/enum.h"
#include "google/protobuf/compiler/java/full/extension.h"
#include "google/protobuf/compiler/java/full/make_field_gens.h"
#include "google/protobuf/compiler/java/full/message_builder.h"
#include "google/protobuf/compiler/java/message_serialization.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {
std::string MapValueImmutableClassdName(const Descriptor* descriptor,
                                        ClassNameResolver* name_resolver) {
  const FieldDescriptor* value_field = descriptor->map_value();
  ABSL_CHECK_EQ(FieldDescriptor::TYPE_MESSAGE, value_field->type());
  return name_resolver->GetImmutableClassName(value_field->message_type());
}
}  // namespace

// ===================================================================

MessageGenerator::MessageGenerator(const Descriptor* descriptor)
    : descriptor_(descriptor) {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (IsRealOneof(descriptor_->field(i))) {
      const OneofDescriptor* oneof = descriptor_->field(i)->containing_oneof();
      ABSL_CHECK(oneofs_.emplace(oneof->index(), oneof).first->second == oneof);
    }
  }
}

MessageGenerator::~MessageGenerator() = default;

// ===================================================================
ImmutableMessageGenerator::ImmutableMessageGenerator(
    const Descriptor* descriptor, Context* context)
    : MessageGenerator(descriptor),
      context_(context),
      name_resolver_(context->GetNameResolver()),
      field_generators_(MakeImmutableFieldGenerators(descriptor, context_)) {
  ABSL_CHECK(HasDescriptorMethods(descriptor->file(), context->EnforceLite()))
      << "Generator factory error: A non-lite message generator is used to "
         "generate lite messages.";
}

ImmutableMessageGenerator::~ImmutableMessageGenerator() = default;

void ImmutableMessageGenerator::GenerateStaticVariables(
    io::Printer* printer, int* bytecode_estimate) {
  // Because descriptor.proto (com.google.protobuf.DescriptorProtos) is
  // used in the construction of descriptors, we have a tricky bootstrapping
  // problem.  To help control static initialization order, we make sure all
  // descriptors and other static data that depends on them are members of
  // the outermost class in the file.  This way, they will be initialized in
  // a deterministic order.

  absl::flat_hash_map<absl::string_view, std::string> vars;
  vars["identifier"] = UniqueFileScopeIdentifier(descriptor_);
  vars["index"] = absl::StrCat(descriptor_->index());
  vars["classname"] = name_resolver_->GetImmutableClassName(descriptor_);
  if (descriptor_->containing_type() != nullptr) {
    vars["parent"] = UniqueFileScopeIdentifier(descriptor_->containing_type());
  }
  if (NestedInFileClass(*descriptor_, /* immutable = */ true)) {
    vars["private"] = "private ";
  } else {
    // We can only make these package-private since the classes that use them
    // are in separate files.
    vars["private"] = "";
  }
  if (*bytecode_estimate <= kMaxStaticSize) {
    vars["final"] = "final ";
  } else {
    vars["final"] = "";
  }

  // The descriptor for this type.
  printer->Print(
      vars,
      // TODO: final needs to be added back. The way to fix it is to
      // generate methods that can construct the types, and then still declare
      // the types, and then init them in clinit with the new method calls.
      "$private$static $final$com.google.protobuf.Descriptors.Descriptor\n"
      "  internal_$identifier$_descriptor;\n");
  *bytecode_estimate += 30;

  // And the FieldAccessorTable.
  GenerateFieldAccessorTable(printer, bytecode_estimate);

  // Generate static members for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO:  Reuse MessageGenerator objects?
    ImmutableMessageGenerator(descriptor_->nested_type(i), context_)
        .GenerateStaticVariables(printer, bytecode_estimate);
  }
}

int ImmutableMessageGenerator::GenerateStaticVariableInitializers(
    io::Printer* printer) {
  int bytecode_estimate = 0;
  absl::flat_hash_map<absl::string_view, std::string> vars;
  vars["identifier"] = UniqueFileScopeIdentifier(descriptor_);
  vars["index"] = absl::StrCat(descriptor_->index());
  vars["classname"] = name_resolver_->GetImmutableClassName(descriptor_);
  if (descriptor_->containing_type() != nullptr) {
    vars["parent"] = UniqueFileScopeIdentifier(descriptor_->containing_type());
  }

  // The descriptor for this type.
  if (descriptor_->containing_type() == nullptr) {
    printer->Print(vars,
                   "internal_$identifier$_descriptor =\n"
                   "  getDescriptor().getMessageTypes().get($index$);\n");
    bytecode_estimate += 30;
  } else {
    printer->Print(
        vars,
        "internal_$identifier$_descriptor =\n"
        "  internal_$parent$_descriptor.getNestedTypes().get($index$);\n");
    bytecode_estimate += 30;
  }

  // And the FieldAccessorTable.
  bytecode_estimate += GenerateFieldAccessorTableInitializer(printer);

  // Generate static member initializers for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO:  Reuse MessageGenerator objects?
    bytecode_estimate +=
        ImmutableMessageGenerator(descriptor_->nested_type(i), context_)
            .GenerateStaticVariableInitializers(printer);
  }
  return bytecode_estimate;
}

void ImmutableMessageGenerator::GenerateFieldAccessorTable(
    io::Printer* printer, int* bytecode_estimate) {
  absl::flat_hash_map<absl::string_view, std::string> vars;
  vars["identifier"] = UniqueFileScopeIdentifier(descriptor_);
  if (NestedInFileClass(*descriptor_, /* immutable = */ true)) {
    vars["private"] = "private ";
  } else {
    // We can only make these package-private since the classes that use them
    // are in separate files.
    vars["private"] = "";
  }
  if (*bytecode_estimate <= kMaxStaticSize) {
    vars["final"] = "final ";
  } else {
    vars["final"] = "";
  }
  printer->Print(vars,
                 "$private$static $final$\n"
                 "  com.google.protobuf.GeneratedMessage.FieldAccessorTable\n"
                 "    internal_$identifier$_fieldAccessorTable;\n");

  // The following bytecode_estimate calculation logic must stay in sync with
  // the similar logic in the GenerateFieldAccessorTableInitializer method below
  // to make sure that the generated static final fields are initialized  in the
  // static initialization block directly.
  //
  // 6 bytes per field and oneof
  *bytecode_estimate +=
      10 + 6 * descriptor_->field_count() + 6 * descriptor_->oneof_decl_count();
}

int ImmutableMessageGenerator::GenerateFieldAccessorTableInitializer(
    io::Printer* printer) {
  int bytecode_estimate = 10;
  printer->Print(
      "internal_$identifier$_fieldAccessorTable = new\n"
      "  com.google.protobuf.GeneratedMessage.FieldAccessorTable(\n"
      "    internal_$identifier$_descriptor,\n"
      "    new java.lang.String[] { ",
      "identifier", UniqueFileScopeIdentifier(descriptor_));
  // All the bytecode_estimate calculation logic in this method must stay in
  // sync with the similar logic in the GenerateFieldAccessorTable method
  // above. See the corresponding comment in GenerateFieldAccessorTable for
  // details.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    const FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);
    bytecode_estimate += 6;
    printer->Print("\"$field_name$\", ", "field_name", info->capitalized_name);
  }

  for (int i = 0; i < descriptor_->real_oneof_decl_count(); i++) {
    const OneofDescriptor* oneof = descriptor_->real_oneof_decl(i);
    const OneofGeneratorInfo* info = context_->GetOneofGeneratorInfo(oneof);
    bytecode_estimate += 6;
    printer->Print("\"$oneof_name$\", ", "oneof_name", info->capitalized_name);
  }
  printer->Print("});\n");
  return bytecode_estimate;
}

// ===================================================================

void ImmutableMessageGenerator::GenerateInterface(io::Printer* printer) {
  MaybePrintGeneratedAnnotation(context_, printer, descriptor_,
                                /* immutable = */ true, "OrBuilder");
  if (!context_->options().opensource_runtime) {
    printer->Print("@com.google.protobuf.Internal.ProtoNonnullApi\n");
  }
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
        "$deprecation$public interface ${$$classname$OrBuilder$}$ extends\n"
        "    $extra_interfaces$\n"
        "    com.google.protobuf.GeneratedMessage.\n"
        "        ExtendableMessageOrBuilder<$classname$> {\n",
        "deprecation",
        descriptor_->options().deprecated() ? "@java.lang.Deprecated " : "",
        "extra_interfaces", ExtraMessageOrBuilderInterfaces(descriptor_),
        "classname", descriptor_->name(), "{", "", "}", "");
  } else {
    printer->Print(
        "$deprecation$public interface ${$$classname$OrBuilder$}$ extends\n"
        "    $extra_interfaces$\n"
        "    com.google.protobuf.MessageOrBuilder {\n",
        "deprecation",
        descriptor_->options().deprecated() ? "@java.lang.Deprecated " : "",
        "extra_interfaces", ExtraMessageOrBuilderInterfaces(descriptor_),
        "classname", descriptor_->name(), "{", "", "}", "");
  }
  printer->Annotate("{", "}", descriptor_);

  printer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    printer->Print("\n");
    field_generators_.get(descriptor_->field(i))
        .GenerateInterfaceMembers(printer);
  }
  for (const auto& kv : oneofs_) {
    printer->Print(
        "\n"
        "$classname$.$oneof_capitalized_name$Case "
        "get$oneof_capitalized_name$Case();\n",
        "oneof_capitalized_name",
        context_->GetOneofGeneratorInfo(kv.second)->capitalized_name,
        "classname",
        context_->GetNameResolver()->GetImmutableClassName(descriptor_));
  }
  printer->Outdent();

  printer->Print("}\n");
}

// ===================================================================

void ImmutableMessageGenerator::Generate(io::Printer* printer) {
  bool is_own_file = IsOwnFile(descriptor_, /* immutable = */ true);

  absl::flat_hash_map<absl::string_view, std::string> variables;
  variables["static"] = is_own_file ? "" : "static ";
  variables["classname"] = std::string(descriptor_->name());
  variables["extra_interfaces"] = ExtraMessageInterfaces(descriptor_);
  variables["deprecation"] =
      descriptor_->options().deprecated() ? "@java.lang.Deprecated " : "";

  WriteMessageDocComment(printer, descriptor_, context_->options());
  MaybePrintGeneratedAnnotation(context_, printer, descriptor_,
                                /* immutable = */ true);
  if (!context_->options().opensource_runtime) {
    printer->Print("@com.google.protobuf.Internal.ProtoNonnullApi\n");
  }

  // The builder_type stores the super type name of the nested Builder class.
  std::string builder_type;
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
        variables,
        "$deprecation$public $static$final class $classname$ extends\n");
    printer->Annotate("classname", descriptor_);
    printer->Print(
        variables,
        "    com.google.protobuf.GeneratedMessage.ExtendableMessage<\n"
        "      $classname$> implements\n"
        "    $extra_interfaces$\n"
        "    $classname$OrBuilder {\n");
    builder_type = absl::Substitute(
        "com.google.protobuf.GeneratedMessage.ExtendableBuilder<$0, ?>",
        name_resolver_->GetImmutableClassName(descriptor_));
  } else {
    printer->Print(
        variables,
        "$deprecation$public $static$final class $classname$ extends\n");
    printer->Annotate("classname", descriptor_);
    printer->Print(variables,
                   "    com.google.protobuf.GeneratedMessage implements\n"
                   "    $extra_interfaces$\n"
                   "    $classname$OrBuilder {\n");
    builder_type = "com.google.protobuf.GeneratedMessage.Builder<?>";
  }
  printer->Print("private static final long serialVersionUID = 0L;\n");

  printer->Indent();

  printer->Print("static {\n");
  printer->Indent();
  PrintGencodeVersionValidator(printer, context_->options().opensource_runtime,
                               descriptor_->name());
  printer->Outdent();
  printer->Print("}\n");

  // Using builder_type, instead of Builder, prevents the Builder class from
  // being loaded into PermGen space when the default instance is created.
  // This optimizes the PermGen space usage for clients that do not modify
  // messages.
  printer->Print(
      "// Use $classname$.newBuilder() to construct.\n"
      "private $classname$($buildertype$ builder) {\n"
      "  super(builder);\n"
      "}\n",
      "classname", descriptor_->name(), "buildertype", builder_type);
  printer->Print("private $classname$() {\n", "classname", descriptor_->name());
  printer->Indent();
  GenerateInitializers(printer);
  printer->Outdent();
  printer->Print(
      "}\n"
      "\n");

  GenerateDescriptorMethods(printer);

  // Nested types
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    EnumNonLiteGenerator(descriptor_->enum_type(i), true, context_)
        .Generate(printer);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // Don't generate Java classes for map entry messages.
    if (IsMapEntry(descriptor_->nested_type(i))) continue;
    ImmutableMessageGenerator messageGenerator(descriptor_->nested_type(i),
                                               context_);
    messageGenerator.GenerateInterface(printer);
    messageGenerator.Generate(printer);
  }

  // Integers for bit fields.
  int totalBits = 0;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    totalBits +=
        field_generators_.get(descriptor_->field(i)).GetNumBitsForMessage();
  }
  int totalInts = (totalBits + 31) / 32;
  for (int i = 0; i < totalInts; i++) {
    printer->Print("private int $bit_field_name$;\n", "bit_field_name",
                   GetBitFieldName(i));
  }

  // oneof
  absl::flat_hash_map<absl::string_view, std::string> vars;
  for (const auto& kv : oneofs_) {
    const OneofDescriptor* oneof = kv.second;
    vars["oneof_name"] = context_->GetOneofGeneratorInfo(oneof)->name;
    vars["oneof_capitalized_name"] =
        context_->GetOneofGeneratorInfo(oneof)->capitalized_name;
    vars["oneof_index"] = absl::StrCat((oneof)->index());
    vars["{"] = "";
    vars["}"] = "";
    // oneofCase_ and oneof_
    printer->Print(vars,
                   "private int $oneof_name$Case_ = 0;\n"
                   "@SuppressWarnings(\"serial\")\n"
                   "private java.lang.Object $oneof_name$_;\n");
    // OneofCase enum
    printer->Print(
        vars,
        "public enum ${$$oneof_capitalized_name$Case$}$\n"
        // TODO: Remove EnumLite when we want to break compatibility with
        // 3.x users
        "    implements com.google.protobuf.Internal.EnumLite,\n"
        "        com.google.protobuf.AbstractMessage.InternalOneOfEnum {\n");
    printer->Annotate("{", "}", oneof);
    printer->Indent();
    for (int j = 0; j < (oneof)->field_count(); j++) {
      const FieldDescriptor* field = (oneof)->field(j);
      printer->Print(
          "$deprecation$$field_name$($field_number$),\n", "deprecation",
          field->options().deprecated() ? "@java.lang.Deprecated " : "",
          "field_name", absl::AsciiStrToUpper(field->name()), "field_number",
          absl::StrCat(field->number()));
      printer->Annotate("field_name", field);
    }
    printer->Print("$cap_oneof_name$_NOT_SET(0);\n", "cap_oneof_name",
                   absl::AsciiStrToUpper(vars["oneof_name"]));
    printer->Print(vars,
                   "private final int value;\n"
                   "private $oneof_capitalized_name$Case(int value) {\n"
                   "  this.value = value;\n"
                   "}\n");
    if (context_->options().opensource_runtime) {
      printer->Print(
          vars,
          "/**\n"
          " * @param value The number of the enum to look for.\n"
          " * @return The enum associated with the given number.\n"
          " * @deprecated Use {@link #forNumber(int)} instead.\n"
          " */\n"
          "@java.lang.Deprecated\n"
          "public static $oneof_capitalized_name$Case valueOf(int value) {\n"
          "  return forNumber(value);\n"
          "}\n"
          "\n");
    }
    if (!context_->options().opensource_runtime) {
      printer->Print(
          "@com.google.protobuf.Internal.ProtoMethodMayReturnNull\n");
    }
    printer->Print(
        vars,
        "public static $oneof_capitalized_name$Case forNumber(int value) {\n"
        "  switch (value) {\n");
    for (int j = 0; j < (oneof)->field_count(); j++) {
      const FieldDescriptor* field = (oneof)->field(j);
      printer->Print("    case $field_number$: return $field_name$;\n",
                     "field_number", absl::StrCat(field->number()),
                     "field_name", absl::AsciiStrToUpper(field->name()));
    }
    printer->Print(
        "    case 0: return $cap_oneof_name$_NOT_SET;\n"
        "    default: return null;\n"
        "  }\n"
        "}\n"
        "public int getNumber() {\n"
        "  return this.value;\n"
        "}\n",
        "cap_oneof_name", absl::AsciiStrToUpper(vars["oneof_name"]));
    printer->Outdent();
    printer->Print("};\n\n");
    // oneofCase()
    printer->Print(vars,
                   "public $oneof_capitalized_name$Case\n"
                   "${$get$oneof_capitalized_name$Case$}$() {\n"
                   "  return $oneof_capitalized_name$Case.forNumber(\n"
                   "      $oneof_name$Case_);\n"
                   "}\n"
                   "\n");
    printer->Annotate("{", "}", oneof);
  }

  if (IsAnyMessage(descriptor_)) {
    GenerateAnyMethods(printer);
  }

  // Fields
  for (int i = 0; i < descriptor_->field_count(); i++) {
    printer->Print("public static final int $constant_name$ = $number$;\n",
                   "constant_name", FieldConstantName(descriptor_->field(i)),
                   "number", absl::StrCat(descriptor_->field(i)->number()));
    printer->Annotate("constant_name", descriptor_->field(i));
    field_generators_.get(descriptor_->field(i)).GenerateMembers(printer);
    printer->Print("\n");
  }

  if (context_->HasGeneratedMethods(descriptor_)) {
    GenerateIsInitialized(printer);
    GenerateMessageSerializationMethods(printer);
    GenerateEqualsAndHashCode(printer);
  }


  GenerateParseFromMethods(printer);
  GenerateBuilder(printer);

  printer->Print(
      "\n"
      "// @@protoc_insertion_point(class_scope:$full_name$)\n",
      "full_name", descriptor_->full_name());

  // Carefully initialize the default instance in such a way that it doesn't
  // conflict with other initialization.
  printer->Print("private static final $classname$ DEFAULT_INSTANCE;\n",
                 "classname",
                 name_resolver_->GetImmutableClassName(descriptor_));
  printer->Print(
      "static {\n"
      "  DEFAULT_INSTANCE = new $classname$();\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  printer->Print(
      "public static $classname$ getDefaultInstance() {\n"
      "  return DEFAULT_INSTANCE;\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  // 'of' method for Wrappers
  if (IsWrappersProtoFile(descriptor_->file())) {
    printer->Print(
        "public static $classname$ of($field_type$ value) {\n"
        "  return newBuilder().setValue(value).build();\n"
        "}\n"
        "\n",
        "classname", name_resolver_->GetImmutableClassName(descriptor_),
        "field_type", PrimitiveTypeName(GetJavaType(descriptor_->field(0))));
  }

  GenerateParser(printer);

  printer->Print(
      "@java.lang.Override\n"
      "public $classname$ getDefaultInstanceForType() {\n"
      "  return DEFAULT_INSTANCE;\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  // Extensions must be declared after the DEFAULT_INSTANCE is initialized
  // because the DEFAULT_INSTANCE is used by the extension to lazily retrieve
  // the outer class's FileDescriptor.
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ImmutableExtensionGenerator(descriptor_->extension(i), context_)
        .Generate(printer);
  }

  printer->Outdent();
  printer->Print("}\n\n");
}

// ===================================================================

void ImmutableMessageGenerator::GenerateMessageSerializationMethods(
    io::Printer* printer) {
  std::unique_ptr<const FieldDescriptor*[]> sorted_fields(
      SortFieldsByNumber(descriptor_));

  printer->Print(
      "@java.lang.Override\n"
      "public void writeTo(com.google.protobuf.CodedOutputStream output)\n"
      "                    throws java.io.IOException {\n");
  printer->Indent();

  if (HasPackedFields(descriptor_)) {
    // writeTo(CodedOutputStream output) might be invoked without
    // getSerializedSize() ever being called, but we need the memoized
    // sizes in case this message has packed fields. Rather than emit checks
    // for each packed field, just call getSerializedSize() up front. In most
    // cases, getSerializedSize() will have already been called anyway by one
    // of the wrapper writeTo() methods, making this call cheap.
    printer->Print("getSerializedSize();\n");
  }

  if (descriptor_->extension_range_count() > 0) {
    if (descriptor_->options().message_set_wire_format()) {
      printer->Print(
          "com.google.protobuf.GeneratedMessage\n"
          "  .ExtendableMessage.ExtensionSerializer\n"
          "    extensionWriter = newMessageSetExtensionSerializer();\n");
    } else {
      printer->Print(
          "com.google.protobuf.GeneratedMessage\n"
          "  .ExtendableMessage.ExtensionSerializer\n"
          "    extensionWriter = newExtensionSerializer();\n");
    }
  }

  GenerateSerializeFieldsAndExtensions(printer,
                                       field_generators_.field_generators(),
                                       descriptor_, sorted_fields.get());

  if (descriptor_->options().message_set_wire_format()) {
    printer->Print("getUnknownFields().writeAsMessageSetTo(output);\n");
  } else {
    printer->Print("getUnknownFields().writeTo(output);\n");
  }

  printer->Outdent();
  printer->Print(
      "}\n"
      "\n"
      "@java.lang.Override\n"
      "public int getSerializedSize() {\n"
      "  int size = memoizedSize;\n"
      "  if (size != -1) return size;\n"
      "\n");
  printer->Indent();

  printer->Print("size = 0;\n");

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(sorted_fields[i]).GenerateSerializedSizeCode(printer);
  }

  if (descriptor_->extension_range_count() > 0) {
    if (descriptor_->options().message_set_wire_format()) {
      printer->Print("size += extensionsSerializedSizeAsMessageSet();\n");
    } else {
      printer->Print("size += extensionsSerializedSize();\n");
    }
  }

  if (descriptor_->options().message_set_wire_format()) {
    printer->Print(
        "size += getUnknownFields().getSerializedSizeAsMessageSet();\n");
  } else {
    printer->Print("size += getUnknownFields().getSerializedSize();\n");
  }

  printer->Print(
      "memoizedSize = size;\n"
      "return size;\n");

  printer->Outdent();
  printer->Print(
      "}\n"
      "\n");
}

void ImmutableMessageGenerator::GenerateParseFromMethods(io::Printer* printer) {
  // Note:  These are separate from GenerateMessageSerializationMethods()
  //   because they need to be generated even for messages that are optimized
  //   for code size.
  printer->Print(
      "public static $classname$ parseFrom(\n"
      "    java.nio.ByteBuffer data)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return PARSER.parseFrom(data);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    java.nio.ByteBuffer data,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return PARSER.parseFrom(data, extensionRegistry);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    com.google.protobuf.ByteString data)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return PARSER.parseFrom(data);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    com.google.protobuf.ByteString data,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return PARSER.parseFrom(data, extensionRegistry);\n"
      "}\n"
      "public static $classname$ parseFrom(byte[] data)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return PARSER.parseFrom(data);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    byte[] data,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return PARSER.parseFrom(data, extensionRegistry);\n"
      "}\n"
      "public static $classname$ parseFrom(java.io.InputStream input)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessage\n"
      "      .parseWithIOException(PARSER, input);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    java.io.InputStream input,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessage\n"
      "      .parseWithIOException(PARSER, input, extensionRegistry);\n"
      "}\n"
      "$parsedelimitedreturnannotation$\n"
      "public static $classname$ parseDelimitedFrom(java.io.InputStream "
      "input)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessage\n"
      "      .parseDelimitedWithIOException(PARSER, input);\n"
      "}\n"
      "$parsedelimitedreturnannotation$\n"
      "public static $classname$ parseDelimitedFrom(\n"
      "    java.io.InputStream input,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessage\n"
      "      .parseDelimitedWithIOException(PARSER, input, "
      "extensionRegistry);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    com.google.protobuf.CodedInputStream input)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessage\n"
      "      .parseWithIOException(PARSER, input);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    com.google.protobuf.CodedInputStream input,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessage\n"
      "      .parseWithIOException(PARSER, input, extensionRegistry);\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_),
      "parsedelimitedreturnannotation",
      context_->options().opensource_runtime
          ? ""
          : "@com.google.protobuf.Internal.ProtoMethodMayReturnNull");
}

// ===================================================================

void ImmutableMessageGenerator::GenerateBuilder(io::Printer* printer) {
  // LITE_RUNTIME implements this at the GeneratedMessageLite level.
  printer->Print(
      "@java.lang.Override\n"
      "public Builder newBuilderForType() { return newBuilder(); }\n");

  printer->Print(
      "public static Builder newBuilder() {\n"
      "  return DEFAULT_INSTANCE.toBuilder();\n"
      "}\n"
      "public static Builder newBuilder($classname$ prototype) {\n"
      "  return DEFAULT_INSTANCE.toBuilder().mergeFrom(prototype);\n"
      "}\n"
      "@java.lang.Override\n"
      "public Builder toBuilder() {\n"
      "  return this == DEFAULT_INSTANCE\n"
      "      ? new Builder() : new Builder().mergeFrom(this);\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  printer->Print(
      "@java.lang.Override\n"
      "protected Builder newBuilderForType(\n"
      "    com.google.protobuf.GeneratedMessage.BuilderParent parent) {\n"
      "  Builder builder = new Builder(parent);\n"
      "  return builder;\n"
      "}\n");

  MessageBuilderGenerator builderGenerator(descriptor_, context_);
  builderGenerator.Generate(printer);
}

void ImmutableMessageGenerator::GenerateDescriptorMethods(
    io::Printer* printer) {
  if (!descriptor_->options().no_standard_descriptor_accessor()) {
    printer->Print(
        "public static final com.google.protobuf.Descriptors.Descriptor\n"
        "    getDescriptor() {\n"
        "  return $fileclass$.internal_$identifier$_descriptor;\n"
        "}\n"
        "\n",
        "fileclass", name_resolver_->GetImmutableClassName(descriptor_->file()),
        "identifier", UniqueFileScopeIdentifier(descriptor_));
  }
  std::vector<const FieldDescriptor*> map_fields;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (GetJavaType(field) == JAVATYPE_MESSAGE &&
        IsMapEntry(field->message_type())) {
      map_fields.push_back(field);
    }
  }
  if (!map_fields.empty()) {
    printer->Print(
        "@SuppressWarnings({\"rawtypes\"})\n"
        "@java.lang.Override\n"
        "protected com.google.protobuf.MapFieldReflectionAccessor "
        "internalGetMapFieldReflection(\n"
        "    int number) {\n"
        "  switch (number) {\n");
    printer->Indent();
    printer->Indent();
    for (const FieldDescriptor* field : map_fields) {
      const FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);
      printer->Print(
          "case $number$:\n"
          "  return internalGet$capitalized_name$();\n",
          "number", absl::StrCat(field->number()), "capitalized_name",
          info->capitalized_name);
    }
    printer->Print(
        "default:\n"
        "  throw new RuntimeException(\n"
        "      \"Invalid map field number: \" + number);\n");
    printer->Outdent();
    printer->Outdent();
    printer->Print(
        "  }\n"
        "}\n");
  }
  printer->Print(
      "@java.lang.Override\n"
      "protected com.google.protobuf.GeneratedMessage.FieldAccessorTable\n"
      "    internalGetFieldAccessorTable() {\n"
      "  return $fileclass$.internal_$identifier$_fieldAccessorTable\n"
      "      .ensureFieldAccessorsInitialized(\n"
      "          $classname$.class, $classname$.Builder.class);\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_),
      "fileclass", name_resolver_->GetImmutableClassName(descriptor_->file()),
      "identifier", UniqueFileScopeIdentifier(descriptor_));
}

// ===================================================================

void ImmutableMessageGenerator::GenerateIsInitialized(io::Printer* printer) {
  // Memoizes whether the protocol buffer is fully initialized (has all
  // required fields). -1 means not yet computed. 0 means false and 1 means
  // true.
  printer->Print("private byte memoizedIsInitialized = -1;\n");
  printer->Print(
      "@java.lang.Override\n"
      "public final boolean isInitialized() {\n");
  printer->Indent();

  // Don't directly compare to -1 to avoid an Android x86 JIT bug.
  printer->Print(
      "byte isInitialized = memoizedIsInitialized;\n"
      "if (isInitialized == 1) return true;\n"
      "if (isInitialized == 0) return false;\n"
      "\n");

  // Check that all required fields in this message are set.
  // TODO:  We can optimize this when we switch to putting all the
  //   "has" fields into a single bitfield.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    const FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);

    if (field->is_required()) {
      printer->Print(
          "if (!has$name$()) {\n"
          "  memoizedIsInitialized = 0;\n"
          "  return false;\n"
          "}\n",
          "name", info->capitalized_name);
    }
  }

  // Now check that all embedded messages are initialized.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    const FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);
    if (GetJavaType(field) == JAVATYPE_MESSAGE &&
        HasRequiredFields(field->message_type())) {
      if (field->is_required()) {
        printer->Print(
            "if (!get$name$().isInitialized()) {\n"
            "  memoizedIsInitialized = 0;\n"
            "  return false;\n"
            "}\n",
            "type",
            name_resolver_->GetImmutableClassName(field->message_type()),
            "name", info->capitalized_name);
      } else if (field->is_repeated()) {
        if (IsMapEntry(field->message_type())) {
          printer->Print(
              "for ($type$ item : get$name$Map().values()) {\n"
              "  if (!item.isInitialized()) {\n"
              "    memoizedIsInitialized = 0;\n"
              "    return false;\n"
              "  }\n"
              "}\n",
              "type",
              MapValueImmutableClassdName(field->message_type(),
                                          name_resolver_),
              "name", info->capitalized_name);
        } else {
          printer->Print(
              "for (int i = 0; i < get$name$Count(); i++) {\n"
              "  if (!get$name$(i).isInitialized()) {\n"
              "    memoizedIsInitialized = 0;\n"
              "    return false;\n"
              "  }\n"
              "}\n",
              "type",
              name_resolver_->GetImmutableClassName(field->message_type()),
              "name", info->capitalized_name);
        }
      } else {
        printer->Print(
            "if (has$name$()) {\n"
            "  if (!get$name$().isInitialized()) {\n"
            "    memoizedIsInitialized = 0;\n"
            "    return false;\n"
            "  }\n"
            "}\n",
            "name", info->capitalized_name);
      }
    }
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
        "if (!extensionsAreInitialized()) {\n"
        "  memoizedIsInitialized = 0;\n"
        "  return false;\n"
        "}\n");
  }

  printer->Outdent();

  printer->Print("  memoizedIsInitialized = 1;\n");

  printer->Print(
      "  return true;\n"
      "}\n"
      "\n");
}

// ===================================================================

void ImmutableMessageGenerator::GenerateEqualsAndHashCode(
    io::Printer* printer) {
  printer->Print(
      "@java.lang.Override\n"
      "public boolean equals(");
  if (!context_->options().opensource_runtime) {
    printer->Print(
        "@com.google.protobuf.Internal.ProtoMethodAcceptsNullParameter\n");
  }
  printer->Print("final java.lang.Object obj) {\n");
  printer->Indent();
  printer->Print(
      "if (obj == this) {\n"
      " return true;\n"
      "}\n"
      "if (!(obj instanceof $classname$)) {\n"
      // don't simply return false because mutable and immutable types
      // can be equal
      "  return super.equals(obj);\n"
      "}\n"
      "$classname$ other = ($classname$) obj;\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (!IsRealOneof(field)) {
      const FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);
      if (field->has_presence()) {
        printer->Print(
            "if (has$name$() != other.has$name$()) return false;\n"
            "if (has$name$()) {\n",
            "name", info->capitalized_name);
        printer->Indent();
      }
      field_generators_.get(field).GenerateEqualsCode(printer);
      if (field->has_presence()) {
        printer->Outdent();
        printer->Print("}\n");
      }
    }
  }

  // Compare oneofs.
  for (const auto& kv : oneofs_) {
    const OneofDescriptor* oneof = kv.second;
    printer->Print(
        "if (!get$oneof_capitalized_name$Case().equals("
        "other.get$oneof_capitalized_name$Case())) return false;\n",
        "oneof_capitalized_name",
        context_->GetOneofGeneratorInfo(oneof)->capitalized_name);
    printer->Print("switch ($oneof_name$Case_) {\n", "oneof_name",
                   context_->GetOneofGeneratorInfo(oneof)->name);
    printer->Indent();
    for (int j = 0; j < (oneof)->field_count(); j++) {
      const FieldDescriptor* field = (oneof)->field(j);
      printer->Print("case $field_number$:\n", "field_number",
                     absl::StrCat(field->number()));
      printer->Indent();
      field_generators_.get(field).GenerateEqualsCode(printer);
      printer->Print("break;\n");
      printer->Outdent();
    }
    printer->Print(
        "case 0:\n"
        "default:\n");
    printer->Outdent();
    printer->Print("}\n");
  }

  // Always consider unknown fields for equality. This will sometimes return
  // false for non-canonical ordering when running in LITE_RUNTIME but it's
  // the best we can do.
  printer->Print(
      "if (!getUnknownFields().equals(other.getUnknownFields())) return "
      "false;\n");
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
        "if (!getExtensionFields().equals(other.getExtensionFields()))\n"
        "  return false;\n");
  }
  printer->Print("return true;\n");
  printer->Outdent();
  printer->Print(
      "}\n"
      "\n");

  printer->Print(
      "@java.lang.Override\n"
      "public int hashCode() {\n");
  printer->Indent();
  printer->Print("if (memoizedHashCode != 0) {\n");
  printer->Indent();
  printer->Print("return memoizedHashCode;\n");
  printer->Outdent();
  printer->Print(
      "}\n"
      "int hash = 41;\n");

  // If we output a getDescriptor() method, use that as it is more efficient.
  if (descriptor_->options().no_standard_descriptor_accessor()) {
    printer->Print("hash = (19 * hash) + getDescriptorForType().hashCode();\n");
  } else {
    printer->Print("hash = (19 * hash) + getDescriptor().hashCode();\n");
  }

  // hashCode non-oneofs.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (!IsRealOneof(field)) {
      const FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);
      if (field->has_presence()) {
        printer->Print("if (has$name$()) {\n", "name", info->capitalized_name);
        printer->Indent();
      }
      field_generators_.get(field).GenerateHashCode(printer);
      if (field->has_presence()) {
        printer->Outdent();
        printer->Print("}\n");
      }
    }
  }

  // hashCode oneofs.
  for (const auto& kv : oneofs_) {
    const OneofDescriptor* oneof = kv.second;
    printer->Print("switch ($oneof_name$Case_) {\n", "oneof_name",
                   context_->GetOneofGeneratorInfo(oneof)->name);
    printer->Indent();
    for (int j = 0; j < (oneof)->field_count(); j++) {
      const FieldDescriptor* field = (oneof)->field(j);
      printer->Print("case $field_number$:\n", "field_number",
                     absl::StrCat(field->number()));
      printer->Indent();
      field_generators_.get(field).GenerateHashCode(printer);
      printer->Print("break;\n");
      printer->Outdent();
    }
    printer->Print(
        "case 0:\n"
        "default:\n");
    printer->Outdent();
    printer->Print("}\n");
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print("hash = hashFields(hash, getExtensionFields());\n");
  }

  printer->Print("hash = (29 * hash) + getUnknownFields().hashCode();\n");
  printer->Print(
      "memoizedHashCode = hash;\n"
      "return hash;\n");
  printer->Outdent();
  printer->Print(
      "}\n"
      "\n");
}

// ===================================================================

void ImmutableMessageGenerator::GenerateExtensionRegistrationCode(
    io::Printer* printer) {
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ImmutableExtensionGenerator(descriptor_->extension(i), context_)
        .GenerateRegistrationCode(printer);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    ImmutableMessageGenerator(descriptor_->nested_type(i), context_)
        .GenerateExtensionRegistrationCode(printer);
  }
}

// ===================================================================
void ImmutableMessageGenerator::GenerateParser(io::Printer* printer) {
  printer->Print(
      "private static final com.google.protobuf.Parser<$classname$>\n"
      "    PARSER = new com.google.protobuf.AbstractParser<$classname$>() {\n"
      "  @java.lang.Override\n"
      "  public $classname$ parsePartialFrom(\n"
      "      com.google.protobuf.CodedInputStream input,\n"
      "      com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "      throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "    Builder builder = newBuilder();\n"
      "    try {\n"
      "      builder.mergeFrom(input, extensionRegistry);\n"
      "    } catch (com.google.protobuf.InvalidProtocolBufferException e) {\n"
      "      throw e.setUnfinishedMessage(builder.buildPartial());\n"
      "    } catch (com.google.protobuf.UninitializedMessageException e) {\n"
      "      throw "
      "e.asInvalidProtocolBufferException().setUnfinishedMessage(builder."
      "buildPartial());\n"
      "    } catch (java.io.IOException e) {\n"
      "      throw new com.google.protobuf.InvalidProtocolBufferException(e)\n"
      "          .setUnfinishedMessage(builder.buildPartial());\n"
      "    }\n"
      "    return builder.buildPartial();\n"
      "  }\n"
      "};\n"
      "\n"
      "public static com.google.protobuf.Parser<$classname$> parser() {\n"
      "  return PARSER;\n"
      "}\n"
      "\n"
      "@java.lang.Override\n"
      "public com.google.protobuf.Parser<$classname$> getParserForType() {\n"
      "  return PARSER;\n"
      "}\n"
      "\n",
      "classname", descriptor_->name());
}

// ===================================================================
void ImmutableMessageGenerator::GenerateInitializers(io::Printer* printer) {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!IsRealOneof(descriptor_->field(i))) {
      field_generators_.get(descriptor_->field(i))
          .GenerateInitializationCode(printer);
    }
  }
}


void ImmutableMessageGenerator::GenerateAnyMethods(io::Printer* printer) {
  printer->Print(
      "private static String getTypeUrl(\n"
      "    java.lang.String typeUrlPrefix,\n"
      "    com.google.protobuf.Descriptors.Descriptor descriptor) {\n"
      "  return typeUrlPrefix.endsWith(\"/\")\n"
      "      ? typeUrlPrefix + descriptor.getFullName()\n"
      "      : typeUrlPrefix + \"/\" + descriptor.getFullName();\n"
      "}\n"
      "\n"
      "private static String getTypeNameFromTypeUrl(\n"
      "    java.lang.String typeUrl) {\n"
      "  int pos = typeUrl.lastIndexOf('/');\n"
      "  return pos == -1 ? \"\" : typeUrl.substring(pos + 1);\n"
      "}\n"
      "\n"
      "public static <T extends com.google.protobuf.Message> Any pack(\n"
      "    T message) {\n"
      "  return Any.newBuilder()\n"
      "      .setTypeUrl(getTypeUrl(\"type.googleapis.com\",\n"
      "                             message.getDescriptorForType()))\n"
      "      .setValue(message.toByteString())\n"
      "      .build();\n"
      "}\n"
      "\n"
      "/**\n"
      " * Packs a message using the given type URL prefix. The type URL will\n"
      " * be constructed by concatenating the message type's full name to the\n"
      " * prefix with an optional \"/\" separator if the prefix doesn't end\n"
      " * with \"/\" already.\n"
      " */\n"
      "public static <T extends com.google.protobuf.Message> Any pack(\n"
      "    T message, java.lang.String typeUrlPrefix) {\n"
      "  return Any.newBuilder()\n"
      "      .setTypeUrl(getTypeUrl(typeUrlPrefix,\n"
      "                             message.getDescriptorForType()))\n"
      "      .setValue(message.toByteString())\n"
      "      .build();\n"
      "}\n"
      "\n"
      "public <T extends com.google.protobuf.Message> boolean is(\n"
      "    java.lang.Class<T> clazz) {\n"
      "  T defaultInstance =\n"
      "      com.google.protobuf.Internal.getDefaultInstance(clazz);\n"
      "  return getTypeNameFromTypeUrl(getTypeUrl()).equals(\n"
      "      defaultInstance.getDescriptorForType().getFullName());\n"
      "}\n"
      "\n"
      "public boolean isSameTypeAs(com.google.protobuf.Message message) {\n"
      "  return getTypeNameFromTypeUrl(getTypeUrl()).equals(\n"
      "      message.getDescriptorForType().getFullName());\n"
      "}\n"
      "\n"
      "@SuppressWarnings(\"serial\")\n"
      "private volatile com.google.protobuf.Message cachedUnpackValue;\n"
      "\n"
      "@java.lang.SuppressWarnings(\"unchecked\")\n"
      "public <T extends com.google.protobuf.Message> T unpack(\n"
      "    java.lang.Class<T> clazz)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n");
  printer->Print(
      "  boolean invalidClazz = false;\n"
      "  if (cachedUnpackValue != null) {\n"
      "    if (cachedUnpackValue.getClass() == clazz) {\n"
      "      return (T) cachedUnpackValue;\n"
      "    }\n"
      "    invalidClazz = true;\n"
      "  }\n"
      "  if (invalidClazz || !is(clazz)) {\n"
      "    throw new com.google.protobuf.InvalidProtocolBufferException(\n"
      "        \"Type of the Any message does not match the given class.\");\n"
      "  }\n"
      "  T defaultInstance =\n"
      "      com.google.protobuf.Internal.getDefaultInstance(clazz);\n"
      "  T result = (T) defaultInstance.getParserForType()\n"
      "      .parseFrom(getValue());\n"
      "  cachedUnpackValue = result;\n"
      "  return result;\n"
      "}\n"
      "\n"
      "@java.lang.SuppressWarnings(\"unchecked\")\n"
      "public <T extends com.google.protobuf.Message> T unpackSameTypeAs("
      "T message)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n");
  printer->Print(
      "  boolean invalidValue = false;\n"
      "  if (cachedUnpackValue != null) {\n"
      "    if (cachedUnpackValue.getClass() == message.getClass()) {\n"
      "      return (T) cachedUnpackValue;\n"
      "    }\n"
      "    invalidValue = true;\n"
      "  }\n"
      "  if (invalidValue || !isSameTypeAs(message)) {\n"
      "    throw new com.google.protobuf.InvalidProtocolBufferException(\n"
      "        \"Type of the Any message does not match the given "
      "exemplar.\");\n"
      "  }\n"
      "  T result = (T) message.getParserForType().parseFrom(getValue());\n"
      "  cachedUnpackValue = result;\n"
      "  return result;\n"
      "}\n"
      "\n");
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
