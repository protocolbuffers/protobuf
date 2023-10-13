// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: dweis@google.com (Daniel Weis)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/message_lite.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/enum_lite.h"
#include "google/protobuf/compiler/java/extension_lite.h"
#include "google/protobuf/compiler/java/generator_factory.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/message_builder.h"
#include "google/protobuf/compiler/java/message_builder_lite.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_legacy.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

using internal::WireFormat;
using internal::WireFormatLite;

// ===================================================================
ImmutableMessageLiteGenerator::ImmutableMessageLiteGenerator(
    const Descriptor* descriptor, Context* context)
    : MessageGenerator(descriptor),
      context_(context),
      name_resolver_(context->GetNameResolver()),
      field_generators_(descriptor, context_) {
  ABSL_CHECK(!HasDescriptorMethods(descriptor->file(), context->EnforceLite()))
      << "Generator factory error: A lite message generator is used to "
         "generate non-lite messages.";
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (IsRealOneof(descriptor_->field(i))) {
      const OneofDescriptor* oneof = descriptor_->field(i)->containing_oneof();
      ABSL_CHECK(oneofs_.emplace(oneof->index(), oneof).first->second == oneof);
    }
  }
}

ImmutableMessageLiteGenerator::~ImmutableMessageLiteGenerator() {}

void ImmutableMessageLiteGenerator::GenerateStaticVariables(
    io::Printer* printer, int* bytecode_estimate) {
  // Generate static members for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO:  Reuse MessageGenerator objects?
    ImmutableMessageLiteGenerator(descriptor_->nested_type(i), context_)
        .GenerateStaticVariables(printer, bytecode_estimate);
  }
}

int ImmutableMessageLiteGenerator::GenerateStaticVariableInitializers(
    io::Printer* printer) {
  int bytecode_estimate = 0;
  // Generate static member initializers for all nested types.
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // TODO:  Reuse MessageGenerator objects?
    bytecode_estimate +=
        ImmutableMessageLiteGenerator(descriptor_->nested_type(i), context_)
            .GenerateStaticVariableInitializers(printer);
  }
  return bytecode_estimate;
}

// ===================================================================

void ImmutableMessageLiteGenerator::GenerateInterface(io::Printer* printer) {
  MaybePrintGeneratedAnnotation(context_, printer, descriptor_,
                                /* immutable = */ true, "OrBuilder");

  absl::flat_hash_map<absl::string_view, std::string> variables = {
      {"{", ""},
      {"}", ""},
      {"deprecation",
       descriptor_->options().deprecated() ? "@java.lang.Deprecated " : ""},
      {"extra_interfaces", ExtraMessageOrBuilderInterfaces(descriptor_)},
      {"classname", descriptor_->name()},
  };

  if (!context_->options().opensource_runtime) {
    printer->Print("@com.google.protobuf.Internal.ProtoNonnullApi\n");
  }
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
        variables,
        "$deprecation$public interface ${$$classname$OrBuilder$}$ extends \n"
        "    $extra_interfaces$\n"
        "     com.google.protobuf.GeneratedMessageLite.\n"
        "          ExtendableMessageOrBuilder<\n"
        "              $classname$, $classname$.Builder> {\n");
  } else {
    printer->Print(
        variables,
        "$deprecation$public interface ${$$classname$OrBuilder$}$ extends\n"
        "    $extra_interfaces$\n"
        "    com.google.protobuf.MessageLiteOrBuilder {\n");
  }
  printer->Annotate("{", "}", descriptor_);

  printer->Indent();
  for (int i = 0; i < descriptor_->field_count(); i++) {
    printer->Print("\n");
    field_generators_.get(descriptor_->field(i))
        .GenerateInterfaceMembers(printer);
  }
  for (auto& kv : oneofs_) {
    const OneofDescriptor* oneof = kv.second;
    variables["oneof_capitalized_name"] =
        context_->GetOneofGeneratorInfo(oneof)->capitalized_name;
    variables["classname"] =
        context_->GetNameResolver()->GetImmutableClassName(descriptor_);
    printer->Print(variables,
                   "\n"
                   "public ${$$classname$.$oneof_capitalized_name$Case$}$ "
                   "get$oneof_capitalized_name$Case();\n");
    printer->Annotate("{", "}", oneof);
  }
  printer->Outdent();

  printer->Print("}\n");
}

// ===================================================================

void ImmutableMessageLiteGenerator::Generate(io::Printer* printer) {
  bool is_own_file = IsOwnFile(descriptor_, /* immutable = */ true);

  absl::flat_hash_map<absl::string_view, std::string> variables = {{"{", ""},
                                                                   {"}", ""}};
  variables["static"] = is_own_file ? " " : " static ";
  variables["classname"] = descriptor_->name();
  variables["extra_interfaces"] = ExtraMessageInterfaces(descriptor_);
  variables["deprecation"] =
      descriptor_->options().deprecated() ? "@java.lang.Deprecated " : "";

  WriteMessageDocComment(printer, descriptor_);
  MaybePrintGeneratedAnnotation(context_, printer, descriptor_,
                                /* immutable = */ true);


  // The builder_type stores the super type name of the nested Builder class.
  std::string builder_type;
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
        variables,
        "$deprecation$public $static$final class ${$$classname$$}$ extends\n"
        "    com.google.protobuf.GeneratedMessageLite.ExtendableMessage<\n"
        "      $classname$, $classname$.Builder> implements\n"
        "    $extra_interfaces$\n"
        "    $classname$OrBuilder {\n");
    builder_type = absl::Substitute(
        "com.google.protobuf.GeneratedMessageLite.ExtendableBuilder<$0, ?>",
        name_resolver_->GetImmutableClassName(descriptor_));
  } else {
    printer->Print(
        variables,
        "$deprecation$public $static$final class ${$$classname$$}$ extends\n"
        "    com.google.protobuf.GeneratedMessageLite<\n"
        "        $classname$, $classname$.Builder> implements\n"
        "    $extra_interfaces$\n"
        "    $classname$OrBuilder {\n");

    builder_type = "com.google.protobuf.GeneratedMessageLite.Builder";
  }
  printer->Annotate("{", "}", descriptor_);
  printer->Indent();

  GenerateConstructor(printer);

  // Nested types
  for (int i = 0; i < descriptor_->enum_type_count(); i++) {
    EnumLiteGenerator(descriptor_->enum_type(i), true, context_)
        .Generate(printer);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    // Don't generate Java classes for map entry messages.
    if (IsMapEntry(descriptor_->nested_type(i))) continue;
    ImmutableMessageLiteGenerator messageGenerator(descriptor_->nested_type(i),
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
  absl::flat_hash_map<absl::string_view, std::string> vars = {{"{", ""},
                                                              {"}", ""}};
  for (auto& kv : oneofs_) {
    const OneofDescriptor* oneof = kv.second;
    vars["oneof_name"] = context_->GetOneofGeneratorInfo(oneof)->name;
    vars["oneof_capitalized_name"] =
        context_->GetOneofGeneratorInfo(oneof)->capitalized_name;
    vars["oneof_index"] = absl::StrCat((oneof)->index());
    if (context_->options().opensource_runtime) {
      // oneofCase_ and oneof_
      printer->Print(vars,
                     "private int $oneof_name$Case_ = 0;\n"
                     "private java.lang.Object $oneof_name$_;\n");
    }
    // OneofCase enum
    printer->Print(vars, "public enum ${$$oneof_capitalized_name$Case$}$ {\n");
    printer->Annotate("{", "}", oneof);
    printer->Indent();
    for (int j = 0; j < (oneof)->field_count(); j++) {
      const FieldDescriptor* field = (oneof)->field(j);
      printer->Print("$field_name$($field_number$),\n", "field_name",
                     absl::AsciiStrToUpper(field->name()), "field_number",
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
          " * @deprecated Use {@link #forNumber(int)} instead.\n"
          " */\n"
          "@java.lang.Deprecated\n"
          "public static $oneof_capitalized_name$Case valueOf(int value) {\n"
          "  return forNumber(value);\n"
          "}\n"
          "\n");
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
        // TODO: Rename this to "getFieldNumber" or something to
        // disambiguate it from actual proto enums.
        "public int getNumber() {\n"
        "  return this.value;\n"
        "}\n",
        "cap_oneof_name", absl::AsciiStrToUpper(vars["oneof_name"]));
    printer->Outdent();
    printer->Print("};\n\n");
    // oneofCase()
    printer->Print(vars,
                   "@java.lang.Override\n"
                   "public $oneof_capitalized_name$Case\n"
                   "${$get$oneof_capitalized_name$Case$}$() {\n"
                   "  return $oneof_capitalized_name$Case.forNumber(\n"
                   "      $oneof_name$Case_);\n"
                   "}\n");
    printer->Annotate("{", "}", oneof);
    printer->Print(vars,
                   "\n"
                   "private void ${$clear$oneof_capitalized_name$$}$() {\n"
                   "  $oneof_name$Case_ = 0;\n"
                   "  $oneof_name$_ = null;\n"
                   "}\n"
                   "\n");
    printer->Annotate("{", "}", oneof);
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

  GenerateParseFromMethods(printer);
  GenerateBuilder(printer);

  if (HasRequiredFields(descriptor_)) {
    // Memoizes whether the protocol buffer is fully initialized (has all
    // required fields). 0 means false, 1 means true, and all other values
    // mean not yet computed.
    printer->Print("private byte memoizedIsInitialized = 2;\n");
  }

  printer->Print(
      "@java.lang.Override\n"
      "@java.lang.SuppressWarnings({\"unchecked\", \"fallthrough\"})\n"
      "protected final java.lang.Object dynamicMethod(\n"
      "    com.google.protobuf.GeneratedMessageLite.MethodToInvoke method,\n"
      "    java.lang.Object arg0, java.lang.Object arg1) {\n"
      "  switch (method) {\n"
      "    case NEW_MUTABLE_INSTANCE: {\n"
      "      return new $classname$();\n"
      "    }\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  printer->Indent();
  printer->Indent();

  printer->Print("case NEW_BUILDER: {\n");

  printer->Indent();
  GenerateDynamicMethodNewBuilder(printer);
  printer->Outdent();

  printer->Print(
      "}\n"
      "case BUILD_MESSAGE_INFO: {\n");

  printer->Indent();
  GenerateDynamicMethodNewBuildMessageInfo(printer);
  printer->Outdent();

  printer->Print(
      "}\n"
      "// fall through\n"
      "case GET_DEFAULT_INSTANCE: {\n"
      "  return DEFAULT_INSTANCE;\n"
      "}\n"
      "case GET_PARSER: {\n"
      // Generally one would use the lazy initialization holder pattern for
      // manipulating static fields but that has exceptional cost on Android as
      // it will generate an extra class for every message. Instead, use the
      // double-check locking pattern which works just as well.
      //
      // The "parser" temporary mirrors the "PARSER" field to eliminate a read
      // at the final return statement.
      "  com.google.protobuf.Parser<$classname$> parser = PARSER;\n"
      "  if (parser == null) {\n"
      "    synchronized ($classname$.class) {\n"
      "      parser = PARSER;\n"
      "      if (parser == null) {\n"
      "        parser =\n"
      "            new DefaultInstanceBasedParser<$classname$>(\n"
      "                DEFAULT_INSTANCE);\n"
      "        PARSER = parser;\n"
      "      }\n"
      "    }\n"
      "  }\n"
      "  return parser;\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  printer->Outdent();

  if (HasRequiredFields(descriptor_)) {
    printer->Print(
        "}\n"
        "case GET_MEMOIZED_IS_INITIALIZED: {\n"
        "  return memoizedIsInitialized;\n"
        "}\n"
        "case SET_MEMOIZED_IS_INITIALIZED: {\n"
        "  memoizedIsInitialized = (byte) (arg0 == null ? 0 : 1);\n"
        "  return null;\n"
        "}\n");
  } else {
    printer->Print(
        "}\n"
        "case GET_MEMOIZED_IS_INITIALIZED: {\n"
        "  return (byte) 1;\n"
        "}\n"
        "case SET_MEMOIZED_IS_INITIALIZED: {\n"
        "  return null;\n"
        "}\n");
  }

  printer->Outdent();
  printer->Print(
      "  }\n"
      "  throw new UnsupportedOperationException();\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

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
      "  $classname$ defaultInstance = new $classname$();\n"
      "  // New instances are implicitly immutable so no need to make\n"
      "  // immutable.\n"
      "  DEFAULT_INSTANCE = defaultInstance;\n"
      // Register the default instance in a map. This map will be used by
      // experimental runtime to lookup default instance given a class instance
      // without using Java reflection.
      "  com.google.protobuf.GeneratedMessageLite.registerDefaultInstance(\n"
      "    $classname$.class, defaultInstance);\n"
      "}\n"
      "\n",
      "classname", descriptor_->name());

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

  // Extensions must be declared after the DEFAULT_INSTANCE is initialized
  // because the DEFAULT_INSTANCE is used by the extension to lazily retrieve
  // the outer class's FileDescriptor.
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ImmutableExtensionLiteGenerator(descriptor_->extension(i), context_)
        .Generate(printer);
  }

  printer->Outdent();
  printer->Print("}\n\n");
}

void ImmutableMessageLiteGenerator::GenerateDynamicMethodNewBuildMessageInfo(
    io::Printer* printer) {
  printer->Indent();

  // Collect field info into a sequence of UTF-16 chars. It will be embedded
  // as a Java string in the generated code.
  std::vector<uint16_t> chars;

  int flags = 0;
  if (FileDescriptorLegacy(descriptor_->file()).syntax() ==
      FileDescriptorLegacy::Syntax::SYNTAX_PROTO2) {
    flags |= 0x1;
  }
  if (descriptor_->options().message_set_wire_format()) {
    flags |= 0x2;
  }
  if (FileDescriptorLegacy(descriptor_->file()).syntax() ==
      FileDescriptorLegacy::Syntax::SYNTAX_EDITIONS) {
    flags |= 0x4;
  }

  WriteIntToUtf16CharSequence(flags, &chars);
  WriteIntToUtf16CharSequence(descriptor_->field_count(), &chars);

  if (descriptor_->field_count() == 0) {
    printer->Print("java.lang.Object[] objects = null;");
  } else {
    // A single array of all fields (including oneof, oneofCase, hasBits).
    printer->Print("java.lang.Object[] objects = new java.lang.Object[] {\n");
    printer->Indent();

    // Record the number of oneofs.
    WriteIntToUtf16CharSequence(oneofs_.size(), &chars);
    for (auto& kv : oneofs_) {
      printer->Print(
          "\"$oneof_name$_\",\n"
          "\"$oneof_name$Case_\",\n",
          "oneof_name", context_->GetOneofGeneratorInfo(kv.second)->name);
    }

    // Integers for bit fields.
    int total_bits = 0;
    for (int i = 0; i < descriptor_->field_count(); i++) {
      total_bits +=
          field_generators_.get(descriptor_->field(i)).GetNumBitsForMessage();
    }
    int total_ints = (total_bits + 31) / 32;
    for (int i = 0; i < total_ints; i++) {
      printer->Print("\"$bit_field_name$\",\n", "bit_field_name",
                     GetBitFieldName(i));
    }
    WriteIntToUtf16CharSequence(total_ints, &chars);

    int map_count = 0;
    int repeated_count = 0;
    std::unique_ptr<const FieldDescriptor*[]> sorted_fields(
        SortFieldsByNumber(descriptor_));
    for (int i = 0; i < descriptor_->field_count(); i++) {
      const FieldDescriptor* field = sorted_fields[i];
      if (field->is_map()) {
        map_count++;
      } else if (field->is_repeated()) {
        repeated_count++;
      }
    }

    WriteIntToUtf16CharSequence(sorted_fields[0]->number(), &chars);
    WriteIntToUtf16CharSequence(
        sorted_fields[descriptor_->field_count() - 1]->number(), &chars);
    WriteIntToUtf16CharSequence(descriptor_->field_count(), &chars);
    WriteIntToUtf16CharSequence(map_count, &chars);
    WriteIntToUtf16CharSequence(repeated_count, &chars);

    std::vector<const FieldDescriptor*> fields_for_is_initialized_check;
    for (int i = 0; i < descriptor_->field_count(); i++) {
      if (descriptor_->field(i)->is_required() ||
          (GetJavaType(descriptor_->field(i)) == JAVATYPE_MESSAGE &&
           HasRequiredFields(descriptor_->field(i)->message_type()))) {
        fields_for_is_initialized_check.push_back(descriptor_->field(i));
      }
    }
    WriteIntToUtf16CharSequence(fields_for_is_initialized_check.size(), &chars);

    for (int i = 0; i < descriptor_->field_count(); i++) {
      const FieldDescriptor* field = sorted_fields[i];
      field_generators_.get(field).GenerateFieldInfo(printer, &chars);
    }
    printer->Outdent();
    printer->Print("};\n");
  }

  printer->Print("java.lang.String info =\n");
  std::string line;
  for (size_t i = 0; i < chars.size(); i++) {
    uint16_t code = chars[i];
    EscapeUtf16ToString(code, &line);
    if (line.size() >= 80) {
      printer->Print("    \"$string$\" +\n", "string", line);
      line.clear();
    }
  }
  printer->Print("    \"$string$\";\n", "string", line);

  printer->Print("return newMessageInfo(DEFAULT_INSTANCE, info, objects);\n");
  printer->Outdent();
}

// ===================================================================

void ImmutableMessageLiteGenerator::GenerateParseFromMethods(
    io::Printer* printer) {
  printer->Print(
      "public static $classname$ parseFrom(\n"
      "    java.nio.ByteBuffer data)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, data);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    java.nio.ByteBuffer data,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, data, extensionRegistry);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    com.google.protobuf.ByteString data)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, data);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    com.google.protobuf.ByteString data,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, data, extensionRegistry);\n"
      "}\n"
      "public static $classname$ parseFrom(byte[] data)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, data);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    byte[] data,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws com.google.protobuf.InvalidProtocolBufferException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, data, extensionRegistry);\n"
      "}\n"
      "public static $classname$ parseFrom(java.io.InputStream input)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, input);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    java.io.InputStream input,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, input, extensionRegistry);\n"
      "}\n"
      "$parsedelimitedreturnannotation$\n"
      "public static $classname$ parseDelimitedFrom(java.io.InputStream "
      "input)\n"
      "    throws java.io.IOException {\n"
      "  return parseDelimitedFrom(DEFAULT_INSTANCE, input);\n"
      "}\n"
      "$parsedelimitedreturnannotation$\n"
      "public static $classname$ parseDelimitedFrom(\n"
      "    java.io.InputStream input,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws java.io.IOException {\n"
      "  return parseDelimitedFrom(DEFAULT_INSTANCE, input, "
      "extensionRegistry);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    com.google.protobuf.CodedInputStream input)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, input);\n"
      "}\n"
      "public static $classname$ parseFrom(\n"
      "    com.google.protobuf.CodedInputStream input,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws java.io.IOException {\n"
      "  return com.google.protobuf.GeneratedMessageLite.parseFrom(\n"
      "      DEFAULT_INSTANCE, input, extensionRegistry);\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_),
      "parsedelimitedreturnannotation",
      context_->options().opensource_runtime
          ? ""
          : "@com.google.protobuf.Internal.ProtoMethodMayReturnNull");
}

// ===================================================================

void ImmutableMessageLiteGenerator::GenerateBuilder(io::Printer* printer) {
  printer->Print(
      "public static Builder newBuilder() {\n"
      "  return (Builder) DEFAULT_INSTANCE.createBuilder();\n"
      "}\n"
      "public static Builder newBuilder($classname$ prototype) {\n"
      "  return DEFAULT_INSTANCE.createBuilder(prototype);\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  MessageBuilderLiteGenerator builderGenerator(descriptor_, context_);
  builderGenerator.Generate(printer);
}

// ===================================================================

void ImmutableMessageLiteGenerator::GenerateDynamicMethodNewBuilder(
    io::Printer* printer) {
  printer->Print("return new Builder();\n");
}

// ===================================================================

void ImmutableMessageLiteGenerator::GenerateExtensionRegistrationCode(
    io::Printer* printer) {
  for (int i = 0; i < descriptor_->extension_count(); i++) {
    ImmutableExtensionLiteGenerator(descriptor_->extension(i), context_)
        .GenerateRegistrationCode(printer);
  }

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    ImmutableMessageLiteGenerator(descriptor_->nested_type(i), context_)
        .GenerateExtensionRegistrationCode(printer);
  }
}

// ===================================================================
void ImmutableMessageLiteGenerator::GenerateConstructor(io::Printer* printer) {
  printer->Print("private $classname$() {\n", "classname", descriptor_->name());
  printer->Indent();

  // Initialize all fields to default.
  GenerateInitializers(printer);

  printer->Outdent();
  printer->Print("}\n");
}

// ===================================================================
void ImmutableMessageLiteGenerator::GenerateParser(io::Printer* printer) {
  printer->Print(
      "private static volatile com.google.protobuf.Parser<$classname$> "
      "PARSER;\n"
      "\n"
      "public static com.google.protobuf.Parser<$classname$> parser() {\n"
      "  return DEFAULT_INSTANCE.getParserForType();\n"
      "}\n",
      "classname", descriptor_->name());
}

// ===================================================================
void ImmutableMessageLiteGenerator::GenerateInitializers(io::Printer* printer) {
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (!IsRealOneof(descriptor_->field(i))) {
      field_generators_.get(descriptor_->field(i))
          .GenerateInitializationCode(printer);
    }
  }
}

void ImmutableMessageLiteGenerator::GenerateKotlinDsl(
    io::Printer* printer) const {
  printer->Print(
      "@kotlin.OptIn"
      "(com.google.protobuf.kotlin.OnlyForUseByGeneratedProtoCode::class)\n"
      "@com.google.protobuf.kotlin.ProtoDslMarker\n");
  printer->Print(
      "public class Dsl private constructor(\n"
      "  private val _builder: $message$.Builder\n"
      ") {\n"
      "  public companion object {\n"
      "    @kotlin.jvm.JvmSynthetic\n"
      "    @kotlin.PublishedApi\n"
      "    internal fun _create(builder: $message$.Builder): Dsl = "
      "Dsl(builder)\n"
      "  }\n"
      "\n"
      "  @kotlin.jvm.JvmSynthetic\n"
      "  @kotlin.PublishedApi\n"
      "  internal fun _build(): $message$ = _builder.build()\n",
      "message",
      EscapeKotlinKeywords(name_resolver_->GetClassName(descriptor_, true)));

  printer->Indent();

  for (int i = 0; i < descriptor_->field_count(); i++) {
    printer->Print("\n");
    field_generators_.get(descriptor_->field(i))
        .GenerateKotlinDslMembers(printer);
  }

  for (auto& kv : oneofs_) {
    const OneofDescriptor* oneof = kv.second;
    printer->Print(
        "public val $oneof_name$Case: $message$.$oneof_capitalized_name$Case\n"
        "  @JvmName(\"get$oneof_capitalized_name$Case\")\n"
        "  get() = _builder.get$oneof_capitalized_name$Case()\n\n"
        "public fun clear$oneof_capitalized_name$() {\n"
        "  _builder.clear$oneof_capitalized_name$()\n"
        "}\n",
        "oneof_name", context_->GetOneofGeneratorInfo(oneof)->name,
        "oneof_capitalized_name",
        context_->GetOneofGeneratorInfo(oneof)->capitalized_name, "message",
        EscapeKotlinKeywords(name_resolver_->GetClassName(descriptor_, true)));
  }

  if (descriptor_->extension_range_count() > 0) {
    GenerateKotlinExtensions(printer);
  }

  printer->Outdent();
  printer->Print("}\n");
}

void ImmutableMessageLiteGenerator::GenerateKotlinMembers(
    io::Printer* printer) const {
  printer->Print("@kotlin.jvm.JvmName(\"-initialize$camelcase_name$\")\n",
                 "camelcase_name",
                 name_resolver_->GetKotlinFactoryName(descriptor_));

  printer->Print(
      "public inline fun $camelcase_name$(block: $message_kt$.Dsl.() -> "
      "kotlin.Unit): $message$ =\n"
      "  $message_kt$.Dsl._create($message$.newBuilder()).apply { block() "
      "}._build()\n",
      "camelcase_name", name_resolver_->GetKotlinFactoryName(descriptor_),
      "message_kt",
      EscapeKotlinKeywords(
          name_resolver_->GetKotlinExtensionsClassName(descriptor_)),
      "message",
      EscapeKotlinKeywords(name_resolver_->GetClassName(descriptor_, true)));

  WriteMessageDocComment(printer, descriptor_, /* kdoc */ true);
  printer->Print("public object $name$Kt {\n", "name", descriptor_->name());
  printer->Indent();
  GenerateKotlinDsl(printer);
  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    if (IsMapEntry(descriptor_->nested_type(i))) continue;
    ImmutableMessageLiteGenerator(descriptor_->nested_type(i), context_)
        .GenerateKotlinMembers(printer);
  }
  printer->Outdent();
  printer->Print("}\n");
}

void ImmutableMessageLiteGenerator::GenerateTopLevelKotlinMembers(
    io::Printer* printer) const {
  printer->Print(
      "public inline fun $message$.copy(block: $message_kt$.Dsl.() -> "
      "kotlin.Unit): $message$ =\n"
      "  $message_kt$.Dsl._create(this.toBuilder()).apply { block() "
      "}._build()\n\n",
      "message",
      EscapeKotlinKeywords(name_resolver_->GetClassName(descriptor_, true)),
      "message_kt",
      name_resolver_->GetKotlinExtensionsClassNameEscaped(descriptor_));

  for (int i = 0; i < descriptor_->nested_type_count(); i++) {
    if (IsMapEntry(descriptor_->nested_type(i))) continue;
    ImmutableMessageLiteGenerator(descriptor_->nested_type(i), context_)
        .GenerateTopLevelKotlinMembers(printer);
  }

  GenerateKotlinOrNull(printer);
}

void ImmutableMessageLiteGenerator::GenerateKotlinOrNull(
    io::Printer* printer) const {
  // Generate getFieldOrNull getters for all optional message fields.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    if (field->has_presence() && GetJavaType(field) == JAVATYPE_MESSAGE) {
      printer->Print(
          "public val $full_classname$OrBuilder.$camelcase_name$OrNull: "
          "$full_name$?\n"
          "  get() = if (has$name$()) get$name$() else null\n\n",
          "full_classname",
          EscapeKotlinKeywords(name_resolver_->GetClassName(descriptor_, true)),
          "camelcase_name", context_->GetFieldGeneratorInfo(field)->name,
          "full_name",
          EscapeKotlinKeywords(
              name_resolver_->GetImmutableClassName(field->message_type())),
          "name", context_->GetFieldGeneratorInfo(field)->capitalized_name);
    }
  }
}

void ImmutableMessageLiteGenerator::GenerateKotlinExtensions(
    io::Printer* printer) const {
  std::string message_name =
      EscapeKotlinKeywords(name_resolver_->GetClassName(descriptor_, true));

  printer->Print(
      "@Suppress(\"UNCHECKED_CAST\")\n"
      "@kotlin.jvm.JvmSynthetic\n"
      "public operator fun <T : kotlin.Any> get(extension: "
      "com.google.protobuf.ExtensionLite<$message$, T>): T {\n"
      "  return if (extension.isRepeated) {\n"
      "    get(extension as com.google.protobuf.ExtensionLite<$message$, "
      "kotlin.collections.List<*>>) as T\n"
      "  } else {\n"
      "    _builder.getExtension(extension)\n"
      "  }\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "@kotlin.OptIn"
      "(com.google.protobuf.kotlin.OnlyForUseByGeneratedProtoCode::class)\n"
      "@kotlin.jvm.JvmName(\"-getRepeatedExtension\")\n"
      "public operator fun <E : kotlin.Any> get(\n"
      "  extension: com.google.protobuf.ExtensionLite<$message$, "
      "kotlin.collections.List<E>>\n"
      "): com.google.protobuf.kotlin.ExtensionList<E, $message$> {\n"
      "  return com.google.protobuf.kotlin.ExtensionList(extension, "
      "_builder.getExtension(extension))\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "public operator fun contains(extension: "
      "com.google.protobuf.ExtensionLite<$message$, *>): "
      "Boolean {\n"
      "  return _builder.hasExtension(extension)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "public fun clear(extension: "
      "com.google.protobuf.ExtensionLite<$message$, *>) "
      "{\n"
      "  _builder.clearExtension(extension)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "public fun <T : kotlin.Any> setExtension(extension: "
      "com.google.protobuf.ExtensionLite<$message$, T>, "
      "value: T) {\n"
      "  _builder.setExtension(extension, value)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun <T : Comparable<T>> set(\n"
      "  extension: com.google.protobuf.ExtensionLite<$message$, T>,\n"
      "  value: T\n"
      ") {\n"
      "  setExtension(extension, value)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun set(\n"
      "  extension: com.google.protobuf.ExtensionLite<$message$, "
      "com.google.protobuf.ByteString>,\n"
      "  value: com.google.protobuf.ByteString\n"
      ") {\n"
      "  setExtension(extension, value)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun <T : com.google.protobuf.MessageLite> set(\n"
      "  extension: com.google.protobuf.ExtensionLite<$message$, T>,\n"
      "  value: T\n"
      ") {\n"
      "  setExtension(extension, value)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "public fun<E : kotlin.Any> com.google.protobuf.kotlin.ExtensionList<E, "
      "$message$>.add(value: E) {\n"
      "  _builder.addExtension(this.extension, value)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun <E : kotlin.Any> "
      "com.google.protobuf.kotlin.ExtensionList<E, "
      "$message$>.plusAssign"
      "(value: E) {\n"
      "  add(value)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "public fun<E : kotlin.Any> com.google.protobuf.kotlin.ExtensionList<E, "
      "$message$>.addAll(values: Iterable<E>) {\n"
      "  for (value in values) {\n"
      "    add(value)\n"
      "  }\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline operator fun <E : kotlin.Any> "
      "com.google.protobuf.kotlin.ExtensionList<E, "
      "$message$>.plusAssign(values: "
      "Iterable<E>) {\n"
      "  addAll(values)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "public operator fun <E : kotlin.Any> "
      "com.google.protobuf.kotlin.ExtensionList<E, "
      "$message$>.set(index: Int, value: "
      "E) {\n"
      "  _builder.setExtension(this.extension, index, value)\n"
      "}\n\n",
      "message", message_name);

  printer->Print(
      "@kotlin.jvm.JvmSynthetic\n"
      "@Suppress(\"NOTHING_TO_INLINE\")\n"
      "public inline fun com.google.protobuf.kotlin.ExtensionList<*, "
      "$message$>.clear() {\n"
      "  clear(extension)\n"
      "}\n\n",
      "message", message_name);
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
