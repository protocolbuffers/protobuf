// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: dweis@google.com (Daniel Weis)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/full/message_builder.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/field_common.h"
#include "google/protobuf/compiler/java/generator_factory.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/full/enum.h"
#include "google/protobuf/compiler/java/full/extension.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/compiler/java/full/make_field_gens.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/descriptor.pb.h"
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

namespace {
std::string MapValueImmutableClassdName(const Descriptor* descriptor,
                                        ClassNameResolver* name_resolver) {
  const FieldDescriptor* value_field = descriptor->map_value();
  ABSL_CHECK_EQ(FieldDescriptor::TYPE_MESSAGE, value_field->type());
  return name_resolver->GetImmutableClassName(value_field->message_type());
}

bool BitfieldTracksMutability(const FieldDescriptor* const descriptor) {
  if (!descriptor->is_repeated() || IsMapField(descriptor)) {
    return false;
  }
  // TODO: update this to migrate repeated fields to use
  // ProtobufList (which tracks immutability internally). That allows us to use
  // the presence bit to skip work on the repeated field if it is not populated.
  // Once all repeated fields are held in ProtobufLists, this method shouldn't
  // be needed.
  switch (descriptor->type()) {
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_ENUM:
      return true;
    default:
      return false;
  }
}
}  // namespace

MessageBuilderGenerator::MessageBuilderGenerator(const Descriptor* descriptor,
                                                 Context* context)
    : descriptor_(descriptor),
      context_(context),
      name_resolver_(context->GetNameResolver()),
      field_generators_(MakeImmutableFieldGenerators(descriptor, context_)) {
  ABSL_CHECK(HasDescriptorMethods(descriptor->file(), context->EnforceLite()))
      << "Generator factory error: A non-lite message generator is used to "
         "generate lite messages.";
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (IsRealOneof(descriptor_->field(i))) {
      const OneofDescriptor* oneof = descriptor_->field(i)->containing_oneof();
      ABSL_CHECK(oneofs_.emplace(oneof->index(), oneof).first->second == oneof);
    }
  }
}

MessageBuilderGenerator::~MessageBuilderGenerator() {}

void MessageBuilderGenerator::Generate(io::Printer* printer) {
  WriteMessageDocComment(printer, descriptor_, context_->options());
  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
        "public static final class Builder extends\n"
        "    com.google.protobuf.GeneratedMessage.ExtendableBuilder<\n"
        "      $classname$, Builder> implements\n"
        "    $extra_interfaces$\n"
        "    $classname$OrBuilder {\n",
        "classname", name_resolver_->GetImmutableClassName(descriptor_),
        "extra_interfaces", ExtraBuilderInterfaces(descriptor_));
  } else {
    printer->Print(
        "public static final class Builder extends\n"
        "    com.google.protobuf.GeneratedMessage.Builder<Builder> "
        "implements\n"
        "    $extra_interfaces$\n"
        "    $classname$OrBuilder {\n",
        "classname", name_resolver_->GetImmutableClassName(descriptor_),
        "extra_interfaces", ExtraBuilderInterfaces(descriptor_));
  }
  printer->Indent();

  GenerateDescriptorMethods(printer);
  GenerateCommonBuilderMethods(printer);

  if (context_->HasGeneratedMethods(descriptor_)) {
    GenerateIsInitialized(printer);
    GenerateBuilderParsingMethods(printer);
  }

  // oneof
  absl::flat_hash_map<absl::string_view, std::string> vars;
  for (auto& kv : oneofs_) {
    const OneofDescriptor* oneof = kv.second;
    vars["oneof_name"] = context_->GetOneofGeneratorInfo(oneof)->name;
    vars["oneof_capitalized_name"] =
        context_->GetOneofGeneratorInfo(oneof)->capitalized_name;
    vars["oneof_index"] = absl::StrCat(oneof->index());
    // oneofCase_ and oneof_
    printer->Print(vars,
                   "private int $oneof_name$Case_ = 0;\n"
                   "private java.lang.Object $oneof_name$_;\n");
    // oneofCase() and clearOneof()
    printer->Print(vars,
                   "public $oneof_capitalized_name$Case\n"
                   "    get$oneof_capitalized_name$Case() {\n"
                   "  return $oneof_capitalized_name$Case.forNumber(\n"
                   "      $oneof_name$Case_);\n"
                   "}\n"
                   "\n"
                   "public Builder clear$oneof_capitalized_name$() {\n"
                   "  $oneof_name$Case_ = 0;\n"
                   "  $oneof_name$_ = null;\n"
                   "  onChanged();\n"
                   "  return this;\n"
                   "}\n"
                   "\n");
  }

  // Integers for bit fields.
  int totalBits = 0;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    totalBits +=
        field_generators_.get(descriptor_->field(i)).GetNumBitsForBuilder();
  }
  int totalInts = (totalBits + 31) / 32;
  for (int i = 0; i < totalInts; i++) {
    printer->Print("private int $bit_field_name$;\n", "bit_field_name",
                   GetBitFieldName(i));
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    printer->Print("\n");
    field_generators_.get(descriptor_->field(i))
        .GenerateBuilderMembers(printer);
  }

  printer->Print(
      "\n"
      "// @@protoc_insertion_point(builder_scope:$full_name$)\n",
      "full_name", descriptor_->full_name());

  printer->Outdent();
  printer->Print("}\n");
}

// ===================================================================

void MessageBuilderGenerator::GenerateDescriptorMethods(io::Printer* printer) {
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
        "protected com.google.protobuf.MapFieldReflectionAccessor "
        "internalGetMapFieldReflection(\n"
        "    int number) {\n"
        "  switch (number) {\n");
    printer->Indent();
    printer->Indent();
    for (int i = 0; i < map_fields.size(); ++i) {
      const FieldDescriptor* field = map_fields[i];
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
    printer->Print(
        "@SuppressWarnings({\"rawtypes\"})\n"
        "protected com.google.protobuf.MapFieldReflectionAccessor "
        "internalGetMutableMapFieldReflection(\n"
        "    int number) {\n"
        "  switch (number) {\n");
    printer->Indent();
    printer->Indent();
    for (int i = 0; i < map_fields.size(); ++i) {
      const FieldDescriptor* field = map_fields[i];
      const FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);
      printer->Print(
          "case $number$:\n"
          "  return internalGetMutable$capitalized_name$();\n",
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

void MessageBuilderGenerator::GenerateCommonBuilderMethods(
    io::Printer* printer) {
  // Decide if we really need to have the "maybeForceBuilderInitialization()"
  // method.
  // TODO: Remove the need for this entirely
  bool need_maybe_force_builder_init = false;
  for (int i = 0; i < descriptor_->field_count(); i++) {
    if (descriptor_->field(i)->message_type() != nullptr &&
        !IsRealOneof(descriptor_->field(i)) &&
        HasHasbit(descriptor_->field(i))) {
      need_maybe_force_builder_init = true;
      break;
    }
  }

  const char* force_builder_init = need_maybe_force_builder_init
                                       ? "  maybeForceBuilderInitialization();"
                                       : "";

  printer->Print(
      "// Construct using $classname$.newBuilder()\n"
      "private Builder() {\n"
      "$force_builder_init$\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_),
      "force_builder_init", force_builder_init);

  printer->Print(
      "private Builder(\n"
      "    com.google.protobuf.GeneratedMessage.BuilderParent parent) {\n"
      "  super(parent);\n"
      "$force_builder_init$\n"
      "}\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_),
      "force_builder_init", force_builder_init);

  if (need_maybe_force_builder_init) {
    printer->Print(
        "private void maybeForceBuilderInitialization() {\n"
        "  if (com.google.protobuf.GeneratedMessage\n"
        "          .alwaysUseFieldBuilders) {\n");

    printer->Indent();
    printer->Indent();
    for (int i = 0; i < descriptor_->field_count(); i++) {
      if (!IsRealOneof(descriptor_->field(i))) {
        field_generators_.get(descriptor_->field(i))
            .GenerateFieldBuilderInitializationCode(printer);
      }
    }
    printer->Outdent();
    printer->Outdent();

    printer->Print(
        "  }\n"
        "}\n");
  }

  printer->Print(
      "@java.lang.Override\n"
      "public Builder clear() {\n"
      "  super.clear();\n");

  printer->Indent();
  int totalBuilderInts = (descriptor_->field_count() + 31) / 32;
  for (int i = 0; i < totalBuilderInts; i++) {
    printer->Print("$bit_field_name$ = 0;\n", "bit_field_name",
                   GetBitFieldName(i));
  }

  for (int i = 0; i < descriptor_->field_count(); i++) {
    field_generators_.get(descriptor_->field(i))
        .GenerateBuilderClearCode(printer);
  }

  for (auto& kv : oneofs_) {
    printer->Print(
        "$oneof_name$Case_ = 0;\n"
        "$oneof_name$_ = null;\n",
        "oneof_name", context_->GetOneofGeneratorInfo(kv.second)->name);
  }

  printer->Outdent();

  printer->Print(
      "  return this;\n"
      "}\n"
      "\n");

  printer->Print(
      "@java.lang.Override\n"
      "public com.google.protobuf.Descriptors.Descriptor\n"
      "    getDescriptorForType() {\n"
      "  return $fileclass$.internal_$identifier$_descriptor;\n"
      "}\n"
      "\n",
      "fileclass", name_resolver_->GetImmutableClassName(descriptor_->file()),
      "identifier", UniqueFileScopeIdentifier(descriptor_));

  // LITE runtime implements this in GeneratedMessageLite.
  printer->Print(
      "@java.lang.Override\n"
      "public $classname$ getDefaultInstanceForType() {\n"
      "  return $classname$.getDefaultInstance();\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  printer->Print(
      "@java.lang.Override\n"
      "public $classname$ build() {\n"
      "  $classname$ result = buildPartial();\n"
      "  if (!result.isInitialized()) {\n"
      "    throw newUninitializedMessageException(result);\n"
      "  }\n"
      "  return result;\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  GenerateBuildPartial(printer);

  // -----------------------------------------------------------------

  if (context_->HasGeneratedMethods(descriptor_)) {
    printer->Print(
        "@java.lang.Override\n"
        "public Builder mergeFrom(com.google.protobuf.Message other) {\n"
        "  if (other instanceof $classname$) {\n"
        "    return mergeFrom(($classname$)other);\n"
        "  } else {\n"
        "    super.mergeFrom(other);\n"
        "    return this;\n"
        "  }\n"
        "}\n"
        "\n",
        "classname", name_resolver_->GetImmutableClassName(descriptor_));

    printer->Print(
        "public Builder mergeFrom($classname$ other) {\n"
        // Optimization:  If other is the default instance, we know none of its
        //   fields are set so we can skip the merge.
        "  if (other == $classname$.getDefaultInstance()) return this;\n",
        "classname", name_resolver_->GetImmutableClassName(descriptor_));
    printer->Indent();

    for (int i = 0; i < descriptor_->field_count(); i++) {
      if (!IsRealOneof(descriptor_->field(i))) {
        field_generators_.get(descriptor_->field(i))
            .GenerateMergingCode(printer);
      }
    }

    // Merge oneof fields.
    for (auto& kv : oneofs_) {
      const OneofDescriptor* oneof = kv.second;
      printer->Print("switch (other.get$oneof_capitalized_name$Case()) {\n",
                     "oneof_capitalized_name",
                     context_->GetOneofGeneratorInfo(oneof)->capitalized_name);
      printer->Indent();
      for (int j = 0; j < oneof->field_count(); j++) {
        const FieldDescriptor* field = oneof->field(j);
        printer->Print("case $field_name$: {\n", "field_name",
                       absl::AsciiStrToUpper(field->name()));
        printer->Indent();
        field_generators_.get(field).GenerateMergingCode(printer);
        printer->Print("break;\n");
        printer->Outdent();
        printer->Print("}\n");
      }
      printer->Print(
          "case $cap_oneof_name$_NOT_SET: {\n"
          "  break;\n"
          "}\n",
          "cap_oneof_name",
          absl::AsciiStrToUpper(context_->GetOneofGeneratorInfo(oneof)->name));
      printer->Outdent();
      printer->Print("}\n");
    }

    printer->Outdent();

    // if message type has extensions
    if (descriptor_->extension_range_count() > 0) {
      printer->Print("  this.mergeExtensionFields(other);\n");
    }

    printer->Print("  this.mergeUnknownFields(other.getUnknownFields());\n");

    printer->Print("  onChanged();\n");

    printer->Print(
        "  return this;\n"
        "}\n"
        "\n");
  }
}

void MessageBuilderGenerator::GenerateBuildPartial(io::Printer* printer) {
  printer->Print(
      "@java.lang.Override\n"
      "public $classname$ buildPartial() {\n"
      "  $classname$ result = new $classname$(this);\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  printer->Indent();

  // Handle the repeated fields first so that the "mutable bits" are cleared.
  bool has_repeated_fields = false;
  for (int i = 0; i < descriptor_->field_count(); ++i) {
    if (BitfieldTracksMutability(descriptor_->field(i))) {
      has_repeated_fields = true;
      printer->Print("buildPartialRepeatedFields(result);\n");
      break;
    }
  }

  // One buildPartial#() per from_bit_field
  int totalBuilderInts = (descriptor_->field_count() + 31) / 32;
  if (totalBuilderInts > 0) {
    for (int i = 0; i < totalBuilderInts; ++i) {
      printer->Print(
          "if ($bit_field_name$ != 0) { buildPartial$piece$(result); }\n",
          "bit_field_name", GetBitFieldName(i), "piece", absl::StrCat(i));
    }
  }

  if (!oneofs_.empty()) {
    printer->Print("buildPartialOneofs(result);\n");
  }

  printer->Outdent();
  printer->Print(
      "  onBuilt();\n"
      "  return result;\n"
      "}\n"
      "\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_));

  // Build Repeated Fields
  if (has_repeated_fields) {
    printer->Print(
        "private void buildPartialRepeatedFields($classname$ result) {\n",
        "classname", name_resolver_->GetImmutableClassName(descriptor_));
    printer->Indent();
    for (int i = 0; i < descriptor_->field_count(); ++i) {
      if (BitfieldTracksMutability(descriptor_->field(i))) {
        const ImmutableFieldGenerator& field =
            field_generators_.get(descriptor_->field(i));
        field.GenerateBuildingCode(printer);
      }
    }
    printer->Outdent();
    printer->Print("}\n\n");
  }

  // Build non-oneof fields
  int start_field = 0;
  for (int i = 0; i < totalBuilderInts; i++) {
    start_field = GenerateBuildPartialPiece(printer, i, start_field);
  }

  // Build Oneofs
  if (!oneofs_.empty()) {
    printer->Print("private void buildPartialOneofs($classname$ result) {\n",
                   "classname",
                   name_resolver_->GetImmutableClassName(descriptor_));
    printer->Indent();
    for (auto& kv : oneofs_) {
      const OneofDescriptor* oneof = kv.second;
      printer->Print(
          "result.$oneof_name$Case_ = $oneof_name$Case_;\n"
          "result.$oneof_name$_ = this.$oneof_name$_;\n",
          "oneof_name", context_->GetOneofGeneratorInfo(oneof)->name);
      for (int i = 0; i < oneof->field_count(); ++i) {
        if (oneof->field(i)->message_type() != nullptr) {
          const ImmutableFieldGenerator& field =
              field_generators_.get(oneof->field(i));
          field.GenerateBuildingCode(printer);
        }
      }
    }
    printer->Outdent();
    printer->Print("}\n\n");
  }
}

int MessageBuilderGenerator::GenerateBuildPartialPiece(io::Printer* printer,
                                                       int piece,
                                                       int first_field) {
  printer->Print(
      "private void buildPartial$piece$($classname$ result) {\n"
      "  int from_$bit_field_name$ = $bit_field_name$;\n",
      "classname", name_resolver_->GetImmutableClassName(descriptor_), "piece",
      absl::StrCat(piece), "bit_field_name", GetBitFieldName(piece));
  printer->Indent();
  absl::btree_set<int> declared_to_bitfields;

  int bit = 0;
  int next = first_field;
  for (; bit < 32 && next < descriptor_->field_count(); ++next) {
    const ImmutableFieldGenerator& field =
        field_generators_.get(descriptor_->field(next));
    bit += field.GetNumBitsForBuilder();

    // Skip oneof fields that are handled separately
    if (IsRealOneof(descriptor_->field(next))) {
      continue;
    }

    // Skip repeated fields because they are currently handled
    // in separate buildPartial sub-methods.
    if (BitfieldTracksMutability(descriptor_->field(next))) {
      continue;
    }
    // Skip fields without presence bits in the builder
    if (field.GetNumBitsForBuilder() == 0) {
      continue;
    }

    // Track message bits if necessary
    if (field.GetNumBitsForMessage() > 0) {
      int to_bitfield = field.GetMessageBitIndex() / 32;
      if (declared_to_bitfields.count(to_bitfield) == 0) {
        printer->Print("int to_$bit_field_name$ = 0;\n", "bit_field_name",
                       GetBitFieldName(to_bitfield));
        declared_to_bitfields.insert(to_bitfield);
      }
    }

    // Copy the field from the builder to the message
    field.GenerateBuildingCode(printer);
  }

  // Copy the bit field results to the generated message
  for (int to_bitfield : declared_to_bitfields) {
    printer->Print("result.$bit_field_name$ |= to_$bit_field_name$;\n",
                   "bit_field_name", GetBitFieldName(to_bitfield));
  }

  printer->Outdent();
  printer->Print("}\n\n");

  return next;
}

// ===================================================================

void MessageBuilderGenerator::GenerateBuilderParsingMethods(
    io::Printer* printer) {
  printer->Print(
      "@java.lang.Override\n"
      "public Builder mergeFrom(\n"
      "    com.google.protobuf.CodedInputStream input,\n"
      "    com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "    throws java.io.IOException {\n"
      "  if (extensionRegistry == null) {\n"
      "    throw new java.lang.NullPointerException();\n"
      "  }\n"
      "  try {\n"
      "    boolean done = false;\n"
      "    while (!done) {\n"
      "      int tag = input.readTag();\n"
      "      switch (tag) {\n"
      "        case 0:\n"  // zero signals EOF / limit reached
      "          done = true;\n"
      "          break;\n");
  printer->Indent();  // method
  printer->Indent();  // try
  printer->Indent();  // while
  printer->Indent();  // switch
  GenerateBuilderFieldParsingCases(printer);
  printer->Outdent();  // switch
  printer->Outdent();  // while
  printer->Outdent();  // try
  printer->Outdent();  // method
  printer->Print(
      "        default: {\n"
      "          if (!super.parseUnknownField(input, extensionRegistry, tag)) "
      "{\n"
      "            done = true; // was an endgroup tag\n"
      "          }\n"
      "          break;\n"
      "        } // default:\n"
      "      } // switch (tag)\n"
      "    } // while (!done)\n"
      "  } catch (com.google.protobuf.InvalidProtocolBufferException e) {\n"
      "    throw e.unwrapIOException();\n"
      "  } finally {\n"
      "    onChanged();\n"
      "  } // finally\n"
      "  return this;\n"
      "}\n");
}

void MessageBuilderGenerator::GenerateBuilderFieldParsingCases(
    io::Printer* printer) {
  std::unique_ptr<const FieldDescriptor*[]> sorted_fields(
      SortFieldsByNumber(descriptor_));
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = sorted_fields[i];
    GenerateBuilderFieldParsingCase(printer, field);
    if (field->is_packable()) {
      GenerateBuilderPackedFieldParsingCase(printer, field);
    }
  }
}

void MessageBuilderGenerator::GenerateBuilderFieldParsingCase(
    io::Printer* printer, const FieldDescriptor* field) {
  uint32_t tag = WireFormatLite::MakeTag(
      field->number(), WireFormat::WireTypeForFieldType(field->type()));
  std::string tagString = absl::StrCat(static_cast<int32_t>(tag));
  printer->Print("case $tag$: {\n", "tag", tagString);
  printer->Indent();

  field_generators_.get(field).GenerateBuilderParsingCode(printer);

  printer->Outdent();
  printer->Print(
      "  break;\n"
      "} // case $tag$\n",
      "tag", tagString);
}

void MessageBuilderGenerator::GenerateBuilderPackedFieldParsingCase(
    io::Printer* printer, const FieldDescriptor* field) {
  // To make packed = true wire compatible, we generate parsing code from a
  // packed version of this field regardless of field->options().packed().
  uint32_t tag = WireFormatLite::MakeTag(
      field->number(), WireFormatLite::WIRETYPE_LENGTH_DELIMITED);
  std::string tagString = absl::StrCat(static_cast<int32_t>(tag));
  printer->Print("case $tag$: {\n", "tag", tagString);
  printer->Indent();

  field_generators_.get(field).GenerateBuilderParsingCodeFromPacked(printer);

  printer->Outdent();
  printer->Print(
      "  break;\n"
      "} // case $tag$\n",
      "tag", tagString);
}

// ===================================================================

void MessageBuilderGenerator::GenerateIsInitialized(io::Printer* printer) {
  printer->Print(
      "@java.lang.Override\n"
      "public final boolean isInitialized() {\n");
  printer->Indent();

  // Check that all required fields in this message are set.
  // TODO:  We can optimize this when we switch to putting all the
  //   "has" fields into a single bitfield.
  for (int i = 0; i < descriptor_->field_count(); i++) {
    const FieldDescriptor* field = descriptor_->field(i);
    const FieldGeneratorInfo* info = context_->GetFieldGeneratorInfo(field);

    if (field->is_required()) {
      printer->Print(
          "if (!has$name$()) {\n"
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
      switch (field->label()) {
        case FieldDescriptor::LABEL_REQUIRED:
          printer->Print(
              "if (!get$name$().isInitialized()) {\n"
              "  return false;\n"
              "}\n",
              "type",
              name_resolver_->GetImmutableClassName(field->message_type()),
              "name", info->capitalized_name);
          break;
        case FieldDescriptor::LABEL_OPTIONAL:
          printer->Print(
              "if (has$name$()) {\n"
              "  if (!get$name$().isInitialized()) {\n"
              "    return false;\n"
              "  }\n"
              "}\n",
              "name", info->capitalized_name);
          break;
        case FieldDescriptor::LABEL_REPEATED:
          if (IsMapEntry(field->message_type())) {
            printer->Print(
                "for ($type$ item : get$name$Map().values()) {\n"
                "  if (!item.isInitialized()) {\n"
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
                "    return false;\n"
                "  }\n"
                "}\n",
                "type",
                name_resolver_->GetImmutableClassName(field->message_type()),
                "name", info->capitalized_name);
          }
          break;
      }
    }
  }

  if (descriptor_->extension_range_count() > 0) {
    printer->Print(
        "if (!extensionsAreInitialized()) {\n"
        "  return false;\n"
        "}\n");
  }

  printer->Outdent();

  printer->Print(
      "  return true;\n"
      "}\n"
      "\n");
}

// ===================================================================

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
