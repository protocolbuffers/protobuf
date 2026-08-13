// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
// Author: jonp@google.com (Jon Perlow)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/full/string_field.h"

#include <cstdint>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/doc_comment.h"
#include "google/protobuf/compiler/java/field_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/compiler/java/internal_helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/io/printer.h"
#include "google/protobuf/wire_format.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

using internal::WireFormat;
using Semantic = ::google::protobuf::io::AnnotationCollector::Semantic;

namespace {

void SetPrimitiveVariables(
    const FieldDescriptor* descriptor, int bit_index,
    const FieldGeneratorInfo* info, ClassNameResolver* name_resolver,
    absl::flat_hash_map<absl::string_view, std::string>* variables,
    Context* context) {
  SetCommonFieldVariables(descriptor, info, variables);

  (*variables)["empty_list"] =
      "com.google.protobuf.LazyStringArrayList.emptyList()";

  (*variables)["default"] =
      ImmutableDefaultValue(descriptor, name_resolver, context->options());
  (*variables)["default_init"] = absl::StrCat(
      "= ",
      ImmutableDefaultValue(descriptor, name_resolver, context->options()));
  (*variables)["capitalized_type"] = "String";
  (*variables)["tag"] =
      absl::StrCat(static_cast<int32_t>(WireFormat::MakeTag(descriptor)));
  (*variables)["tag_size"] = absl::StrCat(
      WireFormat::TagSize(descriptor->number(), GetType(descriptor)));
  (*variables)["null_check"] = "java.util.Objects.requireNonNull(value);";
  (*variables)["isStringEmpty"] =
      "com.google.protobuf.GeneratedMessage.isStringEmpty";
  (*variables)["writeString"] =
      "com.google.protobuf.GeneratedMessage.writeString";
  (*variables)["computeStringSize"] =
      "com.google.protobuf.GeneratedMessage.computeStringSize";

  // TODO: Add @deprecated javadoc when generating javadoc is supported
  // by the proto compiler
  (*variables)["deprecation"] =
      descriptor->options().deprecated() ? "@java.lang.Deprecated " : "";
  (*variables)["on_changed"] = "onChanged();";

  if (HasHasbit(descriptor)) {
    // For singular messages and builders, one bit is used for the hasField bit.
    (*variables)["set_has_field_bit_to_local"] =
        GenerateSetBitToLocal(bit_index);

    (*variables)["is_field_present"] = GenerateGetBit(bit_index);
  } else {
    (*variables)["set_has_field_bit_to_local"] = "";

    variables->insert(
        {"is_field_present", absl::StrCat("!", (*variables)["isStringEmpty"],
                                          "(", (*variables)["name"], "_)")});
  }

  (*variables)["get_has_field_bit"] = GenerateGetBit(bit_index);
  (*variables)["get_has_field_bit_from_local"] =
      GenerateGetBitFromLocal(bit_index);
  (*variables)["set_has_field_bit"] =
      absl::StrCat(GenerateSetBit(bit_index), ";");
  (*variables)["clear_has_field_bit"] =
      absl::StrCat(GenerateClearBit(bit_index), ";");
}

}  // namespace

// ===================================================================

ImmutableStringFieldGenerator::ImmutableStringFieldGenerator(
    const FieldDescriptor* descriptor, int bit_index, Context* context)
    : ImmutableFieldGenerator(descriptor, bit_index, context) {
  SetPrimitiveVariables(descriptor, bit_index,
                        context->GetFieldGeneratorInfo(descriptor),
                        name_resolver_, &variables_, context);
}

ImmutableStringFieldGenerator::~ImmutableStringFieldGenerator() = default;

// A note about how strings are handled. This code used to just store a String
// in the Message. This had two issues:
//
//  1. It wouldn't roundtrip byte arrays that were not valid UTF-8 encoded
//     strings, but rather fields that were raw bytes incorrectly marked
//     as strings in the proto file. This is common because in the proto1
//     syntax, string was the way to indicate bytes and C++ engineers can
//     easily make this mistake without affecting the C++ API. By converting to
//     strings immediately, some java code might corrupt these byte arrays as
//     it passes through a java server even if the field was never accessed by
//     application code.
//
//  2. There's a performance hit to converting between bytes and strings and
//     it many cases, the field is never even read by the application code. This
//     avoids unnecessary conversions in the common use cases.
//
// So now, the field for String is maintained as an Object reference which can
// either store a String or a ByteString. The code uses an instanceof check
// to see which one it has and converts to the other one if needed. It remembers
// the last value requested (in a thread safe manner) as this is most likely
// the one needed next. The thread safety is such that if two threads both
// convert the field because the changes made by each thread were not visible to
// the other, they may cause a conversion to happen more times than would
// otherwise be necessary. This was deemed better than adding synchronization
// overhead. It will not cause any corruption issues or affect the behavior of
// the API. The instanceof check is also highly optimized in the JVM and we
// decided it was better to reduce the memory overhead by not having two
// separate fields but rather use dynamic type checking.
//
// For single fields, the logic for this is done inside the generated code. For
// repeated fields, the logic is done in LazyStringArrayList.
void ImmutableStringFieldGenerator::GenerateInterfaceHasMethod(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(variables_,
                   "$deprecation$boolean has$capitalized_name$();\n");
  }
}

void ImmutableStringFieldGenerator::GenerateInterfaceGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$java.lang.String get$capitalized_name$();\n");
}

void ImmutableStringFieldGenerator::GenerateInterfaceGetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(variables_,
                 "$deprecation$com.google.protobuf.ByteString\n"
                 "    get$capitalized_name$Bytes();\n");
}

void ImmutableStringFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  GenerateInterfaceHasMethod(printer);
  GenerateInterfaceGetMethod(printer);
  GenerateInterfaceGetBytesMethod(printer);
}

void ImmutableStringFieldGenerator::GenerateHasMethod(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(
        variables_,
        "@java.lang.Override\n"
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $is_field_present$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
}

void ImmutableStringFieldGenerator::GenerateGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  java.lang.Object ref = $name$_;\n"
      "  if (ref instanceof java.lang.String) {\n"
      "    return (java.lang.String) ref;\n"
      "  } else {\n"
      "    com.google.protobuf.ByteString bs = \n"
      "        (com.google.protobuf.ByteString) ref;\n"
      "    java.lang.String s = bs.toStringUtf8();\n");
  printer->Annotate("{", "}", descriptor_);
  if (CheckUtf8(descriptor_)) {

    printer->Print(variables_, "    $name$_ = s;\n");

  } else {
    printer->Print(variables_,
                   "    if (bs.isValidUtf8()) {\n"
                   "      $name$_ = s;\n"
                   "    }\n");
  }
  printer->Print(variables_,
                 "    return s;\n"
                 "  }\n"
                 "}\n");
}

void ImmutableStringFieldGenerator::GenerateGetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$() {\n"
                 "  java.lang.Object ref = $name$_;\n"
                 "  if (ref instanceof java.lang.String) {\n"
                 "    com.google.protobuf.ByteString b = \n"
                 "        com.google.protobuf.ByteString.copyFromUtf8(\n"
                 "            (java.lang.String) ref);\n"
                 "    $name$_ = b;\n"
                 "    return b;\n"
                 "  } else {\n"
                 "    return (com.google.protobuf.ByteString) ref;\n"
                 "  }\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableStringFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "@SuppressWarnings(\"serial\")\n"
                 "private volatile java.lang.Object $name$_ = $default$;\n");
  PrintExtraFieldInfo(variables_, printer);
  GenerateHasMethod(printer);
  GenerateGetMethod(printer);
  GenerateGetBytesMethod(printer);
}

void ImmutableStringFieldGenerator::GenerateBuilderHasMethod(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                                 context_->options());
    printer->Print(
        variables_,
        "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
        "  return $get_has_field_bit$;\n"
        "}\n");
    printer->Annotate("{", "}", descriptor_);
  }
}

void ImmutableStringFieldGenerator::GenerateBuilderGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  java.lang.Object ref = $name$_;\n"
      "  if (!(ref instanceof java.lang.String)) {\n"
      "    com.google.protobuf.ByteString bs =\n"
      "        (com.google.protobuf.ByteString) ref;\n"
      "    java.lang.String s = bs.toStringUtf8();\n");
  printer->Annotate("{", "}", descriptor_);
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "    $name$_ = s;\n");
  } else {
    printer->Print(variables_,
                   "    if (bs.isValidUtf8()) {\n"
                   "      $name$_ = s;\n"
                   "    }\n");
  }
  printer->Print(variables_,
                 "    return s;\n"
                 "  } else {\n"
                 "    return (java.lang.String) ref;\n"
                 "  }\n"
                 "}\n");
}

void ImmutableStringFieldGenerator::GenerateBuilderGetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(variables_,
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$() {\n"
                 "  java.lang.Object ref = $name$_;\n"
                 "  if (ref instanceof String) {\n"
                 "    com.google.protobuf.ByteString b = \n"
                 "        com.google.protobuf.ByteString.copyFromUtf8(\n"
                 "            (java.lang.String) ref);\n"
                 "    $name$_ = b;\n"
                 "    return b;\n"
                 "  } else {\n"
                 "    return (com.google.protobuf.ByteString) ref;\n"
                 "  }\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableStringFieldGenerator::GenerateBuilderSetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
                 "  $null_check$\n"
                 "  $name$_ = value;\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableStringFieldGenerator::GenerateBuilderClearMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  printer->Print(variables_,
                 "  $name$_ = getDefaultInstance().get$capitalized_name$();\n");
  printer->Print(variables_,
                 "  $clear_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
}

void ImmutableStringFieldGenerator::GenerateBuilderSetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options(),
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$set$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  $null_check$\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "  checkByteStringIsUtf8(value);\n");
  }
  printer->Print(variables_,
                 "  $name$_ = value;\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
}

void ImmutableStringFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "private java.lang.Object $name$_ $default_init$;\n");
  GenerateBuilderHasMethod(printer);
  GenerateBuilderGetMethod(printer);
  GenerateBuilderGetBytesMethod(printer);
  GenerateBuilderSetMethod(printer);
  GenerateBuilderClearMethod(printer);
  GenerateBuilderSetBytesMethod(printer);
}

void ImmutableStringFieldGenerator::GenerateFieldBuilderInitializationCode(
    io::Printer* printer) const {
  // noop for primitives
}

void ImmutableStringFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default$;\n");
}

void ImmutableStringFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  printer->Print(variables_, "$name$_ = $default$;\n");
}

void ImmutableStringFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  if (descriptor_->has_presence()) {
    // Allow a slight breach of abstraction here in order to avoid forcing
    // all string fields to Strings when copying fields from a Message.
    printer->Print(variables_,
                   "if (other.has$capitalized_name$()) {\n"
                   "  $name$_ = other.$name$_;\n"
                   "  $set_has_field_bit$\n"
                   "  $on_changed$\n"
                   "}\n");
  } else {
    printer->Print(variables_,
                   "if (!other.get$capitalized_name$().isEmpty()) {\n"
                   "  $name$_ = other.$name$_;\n"
                   "  $set_has_field_bit$\n"
                   "  $on_changed$\n"
                   "}\n");
  }
}

void ImmutableStringFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($get_has_field_bit_from_local$) {\n"
                 "  result.$name$_ = $name$_;\n");
  if (GetNumBits() > 0) {
    printer->Print(variables_, "  $set_has_field_bit_to_local$;\n");
  }
  printer->Print("}\n");
}

void ImmutableStringFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_,
                   "$name$_ = "
                   "input.readStringRequireUtf8();\n"
                   "$set_has_field_bit$\n");
  } else {
    printer->Print(variables_,
                   "$name$_ = input.readBytes();\n"
                   "$set_has_field_bit$\n");
  }
}

void ImmutableStringFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($is_field_present$) {\n"
                 "  $writeString$(output, $number$, $name$_);\n"
                 "}\n");
}

void ImmutableStringFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($is_field_present$) {\n"
                 "  size += $computeStringSize$($number$, $name$_);\n"
                 "}\n");
}

void ImmutableStringFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if (!get$capitalized_name$()\n"
                 "    .equals(other.get$capitalized_name$())) return false;\n");
}

void ImmutableStringFieldGenerator::GenerateHashCode(
    io::Printer* printer) const {
  printer->Print(variables_, "hash = (37 * hash) + $constant_name$;\n");
  printer->Print(variables_,
                 "hash = (53 * hash) + get$capitalized_name$().hashCode();\n");
}

std::string ImmutableStringFieldGenerator::GetBoxedType() const {
  return "java.lang.String";
}

// ===================================================================

ImmutableStringOneofFieldGenerator::ImmutableStringOneofFieldGenerator(
    const FieldDescriptor* descriptor, int bit_index, Context* context)
    : ImmutableStringFieldGenerator(descriptor, bit_index, context) {
  const OneofGeneratorInfo* info =
      context->GetOneofGeneratorInfo(descriptor->containing_oneof());
  SetCommonOneofVariables(descriptor, info, &variables_);
}

ImmutableStringOneofFieldGenerator::~ImmutableStringOneofFieldGenerator() =
    default;

void ImmutableStringOneofFieldGenerator::GenerateHasMethod(
    io::Printer* printer) const {
  ABSL_DCHECK(descriptor_->has_presence());
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableStringOneofFieldGenerator::GenerateGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  if ($negated_has_oneof_case_message$) {\n"
      "    return $default$;\n"
      "  }\n"
      "  java.lang.Object ref = $oneof_name$_;\n"
      "  if (ref instanceof java.lang.String) {\n"
      "    return (java.lang.String) ref;\n"
      "  } else {\n"
      "    com.google.protobuf.ByteString bs = \n"
      "        (com.google.protobuf.ByteString) ref;\n"
      "    java.lang.String s = bs.toStringUtf8();\n");
  printer->Annotate("{", "}", descriptor_);
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "    $oneof_name$_ = s;\n");
  } else {
    printer->Print(variables_,
                   "    if (bs.isValidUtf8()) {\n"
                   "      $oneof_name$_ = s;\n"
                   "    }\n");
  }
  printer->Print(variables_,
                 "    return s;\n"
                 "  }\n"
                 "}\n");
}

void ImmutableStringOneofFieldGenerator::GenerateGetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());

  printer->Print(
      variables_,
      "$deprecation$public com.google.protobuf.ByteString\n"
      "    ${$get$capitalized_name$Bytes$}$() {\n"
      "  if ($negated_has_oneof_case_message$) {\n"
      "    return com.google.protobuf.ByteString.copyFromUtf8($default$);\n"
      "  }\n"
      "  java.lang.Object ref = $oneof_name$_;\n"
      "  if (ref instanceof java.lang.String) {\n"
      "    com.google.protobuf.ByteString b = \n"
      "        com.google.protobuf.ByteString.copyFromUtf8(\n"
      "            (java.lang.String) ref);\n"
      "    $oneof_name$_ = b;\n"
      "    return b;\n"
      "  } else {\n"
      "    return (com.google.protobuf.ByteString) ref;\n"
      "  }\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableStringOneofFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  PrintExtraFieldInfo(variables_, printer);
  GenerateHasMethod(printer);
  GenerateGetMethod(printer);
  GenerateGetBytesMethod(printer);
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderHasMethod(
    io::Printer* printer) const {
  ABSL_DCHECK(descriptor_->has_presence());
  WriteFieldAccessorDocComment(printer, descriptor_, HAZZER,
                               context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public boolean ${$has$capitalized_name$$}$() {\n"
                 "  return $has_oneof_case_message$;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "@java.lang.Override\n"
      "$deprecation$public java.lang.String ${$get$capitalized_name$$}$() {\n"
      "  if ($negated_has_oneof_case_message$) {\n"
      "    return $default$;\n"
      "  }\n"
      "  java.lang.Object ref = $oneof_name$_;\n"
      "  if (!(ref instanceof java.lang.String)) {\n"
      "    com.google.protobuf.ByteString bs =\n"
      "        (com.google.protobuf.ByteString) ref;\n"
      "    java.lang.String s = bs.toStringUtf8();\n");
  printer->Annotate("{", "}", descriptor_);
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "      $oneof_name$_ = s;\n");
  } else {
    printer->Print(variables_,
                   "      if (bs.isValidUtf8()) {\n"
                   "        $oneof_name$_ = s;\n"
                   "      }\n");
  }
  printer->Print(variables_,
                 "    return s;\n"
                 "  } else {\n"
                 "    return (java.lang.String) ref;\n"
                 "  }\n"
                 "}\n");
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderGetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, GETTER,
                                          context_->options());
  printer->Print(variables_,
                 "@java.lang.Override\n"
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$() {\n"
                 "  if ($negated_has_oneof_case_message$) {\n"
                 "    return com.google.protobuf.ByteString.copyFromUtf8("
                 "        $default$);\n"
                 "  }\n"
                 "  java.lang.Object ref = $oneof_name$_;\n"
                 "  if (ref instanceof String) {\n"
                 "    com.google.protobuf.ByteString b = \n"
                 "        com.google.protobuf.ByteString.copyFromUtf8(\n"
                 "            (java.lang.String) ref);\n"
                 "    $oneof_name$_ = b;\n"
                 "    return b;\n"
                 "  } else {\n"
                 "    return (com.google.protobuf.ByteString) ref;\n"
                 "  }\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderSetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
                 "  $null_check$\n"
                 "  $set_oneof_case_message$;\n"
                 "  $oneof_name$_ = value;\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderClearMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  if ($has_oneof_case_message$) {\n"
      "    $clear_oneof_case_message$;\n"
      "    $oneof_name$_ = null;\n"
      "    $on_changed$\n"
      "  }\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderSetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, SETTER,
                                          context_->options(),
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$set$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  $null_check$\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "  checkByteStringIsUtf8(value);\n");
  }
  printer->Print(variables_,
                 "  $set_oneof_case_message$;\n"
                 "  $oneof_name$_ = value;\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  GenerateBuilderHasMethod(printer);
  GenerateBuilderGetMethod(printer);
  GenerateBuilderGetBytesMethod(printer);
  GenerateBuilderSetMethod(printer);
  GenerateBuilderClearMethod(printer);
  GenerateBuilderSetBytesMethod(printer);
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  // No-Op: String fields in oneofs are correctly cleared by clearing the oneof
}

void ImmutableStringOneofFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  // Allow a slight breach of abstraction here in order to avoid forcing
  // all string fields to Strings when copying fields from a Message.
  printer->Print(variables_,
                 "$set_oneof_case_message$;\n"
                 "$oneof_name$_ = other.$oneof_name$_;\n"
                 "$on_changed$\n");
}

void ImmutableStringOneofFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  // No-Op: Handled by single block statement in GenerateBuildPartialShard.
}

void ImmutableStringOneofFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_,
                   "$set_oneof_case_message$;\n"
                   "$oneof_name$_ = "
                   "input.readStringRequireUtf8();\n"
    );
  } else {
    printer->Print(variables_,
                   "com.google.protobuf.ByteString bs = input.readBytes();\n"
                   "$set_oneof_case_message$;\n"
                   "$oneof_name$_ = bs;\n");
  }
}

void ImmutableStringOneofFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($has_oneof_case_message$) {\n"
                 "  $writeString$(output, $number$, $oneof_name$_);\n"
                 "}\n");
}

void ImmutableStringOneofFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "if ($has_oneof_case_message$) {\n"
                 "  size += $computeStringSize$($number$, $oneof_name$_);\n"
                 "}\n");
}

// ===================================================================

RepeatedImmutableStringFieldGenerator::RepeatedImmutableStringFieldGenerator(
    const FieldDescriptor* descriptor, int bit_index, Context* context)
    : ImmutableStringFieldGenerator(descriptor, bit_index, context) {}

RepeatedImmutableStringFieldGenerator::
    ~RepeatedImmutableStringFieldGenerator() = default;

void RepeatedImmutableStringFieldGenerator::GenerateInterfaceGetListMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$java.util.List<java.lang.String>\n"
      "    get$capitalized_name$List();\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateInterfaceGetCountMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$int get$capitalized_name$Count();\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateInterfaceGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$java.lang.String get$capitalized_name$(int index);\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateInterfaceGetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(
      printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
  printer->Print(variables_,
                 "$deprecation$com.google.protobuf.ByteString\n"
                 "    get$capitalized_name$Bytes(int index);\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateInterfaceMembers(
    io::Printer* printer) const {
  GenerateInterfaceGetListMethod(printer);
  GenerateInterfaceGetCountMethod(printer);
  GenerateInterfaceGetMethod(printer);
  GenerateInterfaceGetBytesMethod(printer);
}

void RepeatedImmutableStringFieldGenerator::GenerateGetListMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public com.google.protobuf.ProtocolStringList\n"
                 "    ${$get$capitalized_name$List$}$() {\n"
                 "  return $name$_;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldGenerator::GenerateGetCountMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldGenerator::GenerateGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public java.lang.String "
                 "${$get$capitalized_name$$}$(int index) {\n"
                 "  return $name$_.get(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldGenerator::GenerateGetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(
      printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
  printer->Print(variables_,
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$(int index) {\n"
                 "  return $name$_.getByteString(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldGenerator::GenerateMembers(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "@SuppressWarnings(\"serial\")\n"
                 "private com.google.protobuf.LazyStringArrayList $name$_ =\n"
                 "    $empty_list$;\n");
  PrintExtraFieldInfo(variables_, printer);
  GenerateGetListMethod(printer);
  GenerateGetCountMethod(printer);
  GenerateGetMethod(printer);
  GenerateGetBytesMethod(printer);
}

void RepeatedImmutableStringFieldGenerator::GenerateEnsureIsMutableMethod(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "private void ensure$capitalized_name$IsMutable() {\n"
      "  if (!$name$_.isModifiable()) {\n"
      "    $name$_ = new com.google.protobuf.LazyStringArrayList($name$_);\n"
      "  }\n"
      "  $set_has_field_bit$\n"
      "}\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderGetListMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public com.google.protobuf.ProtocolStringList\n"
                 "    ${$get$capitalized_name$List$}$() {\n"
                 "  $name$_.makeImmutable();\n"
                 "  return $name$_;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderGetCountMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_COUNT,
                               context_->options());
  printer->Print(
      variables_,
      "$deprecation$public int ${$get$capitalized_name$Count$}$() {\n"
      "  return $name$_.size();\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderGetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_GETTER,
                               context_->options());
  printer->Print(variables_,
                 "$deprecation$public java.lang.String "
                 "${$get$capitalized_name$$}$(int index) {\n"
                 "  return $name$_.get(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderGetBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(
      printer, descriptor_, LIST_INDEXED_GETTER, context_->options());
  printer->Print(variables_,
                 "$deprecation$public com.google.protobuf.ByteString\n"
                 "    ${$get$capitalized_name$Bytes$}$(int index) {\n"
                 "  return $name$_.getByteString(index);\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_);
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderSetMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_INDEXED_SETTER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$set$capitalized_name$$}$(\n"
                 "    int index, java.lang.String value) {\n"
                 "  $null_check$\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.set(index, value);\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderAddMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$add$capitalized_name$$}$(\n"
                 "    java.lang.String value) {\n"
                 "  $null_check$\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(value);\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderAddAllMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, LIST_MULTI_ADDER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(variables_,
                 "$deprecation$public Builder ${$addAll$capitalized_name$$}$(\n"
                 "    java.lang.Iterable<java.lang.String> values) {\n"
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  com.google.protobuf.AbstractMessageLite.Builder.addAll(\n"
                 "      values, $name$_);\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderClearMethod(
    io::Printer* printer) const {
  WriteFieldAccessorDocComment(printer, descriptor_, CLEARER,
                               context_->options(),
                               /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$clear$capitalized_name$$}$() {\n"
      "  $name$_ =\n"
      "    $empty_list$;\n"
      "  $clear_has_field_bit$;\n"
      "  $on_changed$\n"
      "  return this;\n"
      "}\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderAddBytesMethod(
    io::Printer* printer) const {
  WriteFieldStringBytesAccessorDocComment(printer, descriptor_, LIST_ADDER,
                                          context_->options(),
                                          /* builder */ true);
  printer->Print(
      variables_,
      "$deprecation$public Builder ${$add$capitalized_name$Bytes$}$(\n"
      "    com.google.protobuf.ByteString value) {\n"
      "  $null_check$\n");
  printer->Annotate("{", "}", descriptor_, Semantic::kSet);
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_, "  checkByteStringIsUtf8(value);\n");
  }
  printer->Print(variables_,
                 "  ensure$capitalized_name$IsMutable();\n"
                 "  $name$_.add(value);\n"
                 "  $set_has_field_bit$\n"
                 "  $on_changed$\n"
                 "  return this;\n"
                 "}\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderMembers(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "private com.google.protobuf.LazyStringArrayList $name$_ =\n"
                 "    $empty_list$;\n");
  GenerateEnsureIsMutableMethod(printer);
  GenerateBuilderGetListMethod(printer);
  GenerateBuilderGetCountMethod(printer);
  GenerateBuilderGetMethod(printer);
  GenerateBuilderGetBytesMethod(printer);
  GenerateBuilderSetMethod(printer);
  GenerateBuilderAddMethod(printer);
  GenerateBuilderAddAllMethod(printer);
  GenerateBuilderClearMethod(printer);
  GenerateBuilderAddBytesMethod(printer);
}

void RepeatedImmutableStringFieldGenerator::
    GenerateFieldBuilderInitializationCode(io::Printer* printer) const {
  // noop for primitives
}

void RepeatedImmutableStringFieldGenerator::GenerateInitializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "$name$_ =\n"
                 "    $empty_list$;\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderClearCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "$name$_ =\n"
                 "    $empty_list$;\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateMergingCode(
    io::Printer* printer) const {
  // The code below does two optimizations:
  //   1. If the other list is empty, there's nothing to do. This ensures we
  //      don't allocate a new array if we already have an immutable one.
  //   2. If the other list is non-empty and our current list is empty, we can
  //      reuse the other list which is guaranteed to be immutable.
  printer->Print(variables_,
                 "if (!other.$name$_.isEmpty()) {\n"
                 "  if ($name$_.isEmpty()) {\n"
                 "    $name$_ = other.$name$_;\n"
                 "    $set_has_field_bit$\n"
                 "  } else {\n"
                 "    ensure$capitalized_name$IsMutable();\n"
                 "    $name$_.addAll(other.$name$_);\n"
                 "  }\n"
                 "  $on_changed$\n"
                 "}\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateBuildingCode(
    io::Printer* printer) const {
  // The code below ensures that the result has an immutable list. If our
  // list is immutable, we can just reuse it. If not, we make it immutable.
  printer->Print(variables_,
                 "if ($get_has_field_bit_from_local$) {\n"
                 "  $name$_.makeImmutable();\n"
                 "  result.$name$_ = $name$_;\n"
                 "}\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateBuilderParsingCode(
    io::Printer* printer) const {
  if (CheckUtf8(descriptor_)) {
    printer->Print(variables_,
                   "ensure$capitalized_name$IsMutable();\n"
                   "$name$_.add(input.readStringRequireUtf8());\n"
    );
  } else {
    printer->Print(variables_,
                   "com.google.protobuf.ByteString bs = input.readBytes();\n"
                   "ensure$capitalized_name$IsMutable();\n"
                   "$name$_.add(bs);\n");
  }
}

void RepeatedImmutableStringFieldGenerator::GenerateSerializationCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "for (int i = 0; i < $name$_.size(); i++) {\n"
                 "  $writeString$(output, $number$, $name$_.getRaw(i));\n"
                 "}\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateSerializedSizeCode(
    io::Printer* printer) const {
  printer->Print(variables_,
                 "{\n"
                 "  int dataSize = 0;\n");
  printer->Indent();

  printer->Print(variables_,
                 "for (int i = 0; i < $name$_.size(); i++) {\n"
                 "  dataSize += computeStringSizeNoTag($name$_.getRaw(i));\n"
                 "}\n");

  printer->Print("size += dataSize;\n");

  printer->Print(variables_,
                 "size += $tag_size$ * get$capitalized_name$List().size();\n");

  printer->Outdent();
  printer->Print("}\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateEqualsCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if (!get$capitalized_name$List()\n"
      "    .equals(other.get$capitalized_name$List())) return false;\n");
}

void RepeatedImmutableStringFieldGenerator::GenerateHashCode(
    io::Printer* printer) const {
  printer->Print(
      variables_,
      "if (get$capitalized_name$Count() > 0) {\n"
      "  hash = (37 * hash) + $constant_name$;\n"
      "  hash = (53 * hash) + get$capitalized_name$List().hashCode();\n"
      "}\n");
}

std::string RepeatedImmutableStringFieldGenerator::GetBoxedType() const {
  return "String";
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
