// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/csharp/csharp_primitive_field.h"

#include <sstream>
#include <string>
#include <utility>

#include "google/protobuf/compiler/code_generator.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/csharp/csharp_doc_comment.h"
#include "google/protobuf/compiler/csharp/csharp_helpers.h"
#include "google/protobuf/compiler/csharp/csharp_options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace csharp {

PrimitiveFieldGenerator::PrimitiveFieldGenerator(
    const FieldDescriptor* descriptor, int presenceIndex, const Options *options)
    : FieldGeneratorBase(descriptor, presenceIndex, options) {
  // TODO: Make this cleaner...
  is_value_type = descriptor->type() != FieldDescriptor::TYPE_STRING
      && descriptor->type() != FieldDescriptor::TYPE_BYTES;
  if (!is_value_type && !SupportsPresenceApi(descriptor_)) {
    std::string property_name = variables_["property_name"];
    variables_["has_property_check"] =
        absl::StrCat(property_name, ".Length != 0");
    variables_["other_has_property_check"] =
        absl::StrCat("other.", property_name, ".Length != 0");
  }
}

PrimitiveFieldGenerator::~PrimitiveFieldGenerator() {
}

void PrimitiveFieldGenerator::GenerateMembers(io::Printer* printer) {
  // Note: in multiple places, this code assumes that all fields
  // that support presence are either nullable, or use a presence field bit.
  // Fields which are oneof members are not generated here; they're generated in PrimitiveOneofFieldGenerator below.
  // Extensions are not generated here either.

  // Explicit presence allows different default values to be specified. These
  // are retained via static fields. They don't particularly need to be, but we
  // don't need to change that. Under implicit presence we don't use static
  // fields for default values and just use the literals instead.
  if (descriptor_->has_presence()) {
    // Note: "private readonly static" isn't as idiomatic as
    // "private static readonly", but changing this now would create a lot of
    // churn in generated code with near-to-zero benefit.
    printer->Print(
      variables_,
      "private readonly static $type_name$ $property_name$DefaultValue = $default_value$;\n\n");
    std::string property_name = variables_["property_name"];
    variables_["default_value_access"] =
        absl::StrCat(property_name, "DefaultValue");
  } else {
    std::string default_value = variables_["default_value"];
    variables_["default_value_access"] = std::move(default_value);
  }

  // Declare the field itself.
  if (options()->emit_unity_attribs) {
    printer->Print("[UnityEngine.SerializeField]\n");
  }
  
  if (!options()->use_properties) {
    printer->Print(
      variables_,
      "$access_level$ $type_name$ $cs_field_name$ = $default_value_access$;\n");
    return;
  }

  printer->Print(
    variables_,
    "private $type_name$ $name_def_message$;\n");
  WritePropertyDocComment(printer, options(), descriptor_);
  AddPublicMemberAttributes(printer);

  // Most of the work is done in the property:
  // Declare the property itself (the same for all options)
  printer->Print(variables_, "$access_level$ $type_name$ $property_name$ {\n");
  
  // Specify the "getter", which may need to check for a presence field.
  if (SupportsPresenceApi(descriptor_)) {
    if (IsNullable(descriptor_)) {
      printer->Print(
        variables_,
        "  get { return $cs_field_name$ ?? $default_value_access$; }\n");
    } else {
      printer->Print(
        variables_,
        // Note: it's possible that this could be rewritten as a
        // conditional ?: expression, but there's no significant benefit
        // to changing it.
        "  get { if ($has_field_check$) { return $cs_field_name$; } else { return $default_value_access$; } }\n");
    }
  } else {
    printer->Print(
      variables_,
      "  get { return $cs_field_name$; }\n");
  }

  // Specify the "setter", which may need to set a field bit as well as the
  // value.
  printer->Print("  set {");
  if (presenceIndex_ != -1) {
    printer->Print(
      variables_,
      " $set_has_field$; ");
  }
  if (is_value_type) {
    printer->Print(
      variables_,
      " $cs_field_name$ = value; ");
  } else {
    printer->Print(
      variables_,
      " $cs_field_name$ = pb::ProtoPreconditions.CheckNotNull(value, \"value\"); ");
  }
  printer->Print(
    "}\n"
    "}\n");

  // The "HasFoo" property, where required.
  if (SupportsPresenceApi(descriptor_)) {
    printer->Print(variables_,
      "/// <summary>Gets whether the \"$descriptor_name$\" field is set</summary>\n");
    AddPublicMemberAttributes(printer);
    printer->Print(
      variables_,
      "$access_level$ bool Has$property_name$ {\n"
      "  get { return ");
    if (IsNullable(descriptor_)) {
      printer->Print(
        variables_,
        "$cs_field_name$ != null; }\n}\n");
    } else {
      printer->Print(
        variables_,
        "$has_field_check$; }\n}\n");
    }
  }

  // The "ClearFoo" method, where required.
  if (SupportsPresenceApi(descriptor_)) {
    printer->Print(variables_,
      "/// <summary>Clears the value of the \"$descriptor_name$\" field</summary>\n");
    AddPublicMemberAttributes(printer);
    printer->Print(
      variables_,
      "$access_level$ void Clear$property_name$() {\n");
    if (IsNullable(descriptor_)) {
      printer->Print(variables_, "  $cs_field_name$ = null;\n");
    } else {
      printer->Print(variables_, "  $clear_has_field$;\n");
    }
    printer->Print("}\n");
  }
}

void PrimitiveFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($other_has_property_check$) {\n"
    "  $property_name$ = other.$property_name$;\n"
    "}\n");
}

void PrimitiveFieldGenerator::GenerateParsingCode(io::Printer* printer) {
  // Note: invoke the property setter rather than writing straight to the field,
  // so that we can normalize "null to empty" for strings and bytes.
  printer->Print(
    variables_,
    "$property_name$ = input.Read$capitalized_type_name$();\n");
}

void PrimitiveFieldGenerator::GenerateSerializationCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n"
    "  output.WriteRawTag($tag_bytes$);\n"
    "  output.Write$capitalized_type_name$($property_name$);\n"
    "}\n");
}

void PrimitiveFieldGenerator::GenerateSerializedSizeCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "if ($has_property_check$) {\n");
  printer->Indent();
  int fixedSize = GetFixedSize(descriptor_->type());
  if (fixedSize == -1) {
    printer->Print(
      variables_,
      "size += $tag_size$ + pb::CodedOutputStream.Compute$capitalized_type_name$Size($property_name$);\n");
  } else {
    printer->Print(
      "size += $tag_size$ + $fixed_size$;\n",
      "fixed_size", absl::StrCat(fixedSize),
      "tag_size", variables_["tag_size"]);
  }
  printer->Outdent();
  printer->Print("}\n");
}

void PrimitiveFieldGenerator::WriteHash(io::Printer* printer) {
  const char *text;

  if (descriptor_->containing_oneof() != nullptr) {
    text = "hash = 17 * hash + $property_name$.GetHashCode();\n";
    if (descriptor_->type() == FieldDescriptor::TYPE_FLOAT) {
      text = "hash = 17 * hash + pbc::ProtobufEqualityComparers.BitwiseSingleEqualityComparer.GetHashCode($property_name$);\n";
    } else if (descriptor_->type() == FieldDescriptor::TYPE_DOUBLE) {
      text = "hash = 17 * hash + pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode($property_name$);\n";
    }
  } else {
    text = "hash = 17 * hash + $cs_field_name$.GetHashCode();\n";
    if (descriptor_->type() == FieldDescriptor::TYPE_FLOAT) {
      text = "hash = 17 * hash + pbc::ProtobufEqualityComparers.BitwiseSingleEqualityComparer.GetHashCode($cs_field_name$);\n";
    } else if (descriptor_->type() == FieldDescriptor::TYPE_DOUBLE) {
      text = "hash = 17 * hash + pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.GetHashCode($cs_field_name$);\n";
    }
  }
	printer->Print(variables_, text);
}

void PrimitiveFieldGenerator::WriteEquals(io::Printer* printer) {
  const char *text;
  if (descriptor_->containing_oneof() != nullptr) {
    text = "if ($property_name$ != other.$property_name$) return false;\n";
    if (descriptor_->type() == FieldDescriptor::TYPE_FLOAT) {
      text = "if (!pbc::ProtobufEqualityComparers.BitwiseSingleEqualityComparer.Equals($property_name$, other.$property_name$)) return false;\n";
    } else if (descriptor_->type() == FieldDescriptor::TYPE_DOUBLE) {
      text = "if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals($property_name$, other.$property_name$)) return false;\n";
    }
  } else {
    text = "if ($cs_field_name$ != other.$cs_field_name$) return false;\n";
    if (descriptor_->type() == FieldDescriptor::TYPE_FLOAT) {
      text = "if (!pbc::ProtobufEqualityComparers.BitwiseSingleEqualityComparer.Equals($cs_field_name$, other.$cs_field_name$)) return false;\n";
    } else if (descriptor_->type() == FieldDescriptor::TYPE_DOUBLE) {
      text = "if (!pbc::ProtobufEqualityComparers.BitwiseDoubleEqualityComparer.Equals($cs_field_name$, other.$cs_field_name$)) return false;\n";
    }
  }
  printer->Print(variables_, text);
}

void PrimitiveFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(
    variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $cs_field_name$, writer);\n");
}

void PrimitiveFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  printer->Print(variables_, "$cs_field_name$ = other.$cs_field_name$;\n");
}

void PrimitiveFieldGenerator::GenerateCodecCode(io::Printer* printer) {
  printer->Print(
    variables_,
    "pb::FieldCodec.For$capitalized_type_name$($tag$, $default_value$)");
}

void PrimitiveFieldGenerator::GenerateExtensionCode(io::Printer* printer) {
  WritePropertyDocComment(printer, options(), descriptor_);
  AddDeprecatedFlag(printer);
  printer->Print(
    variables_,
    "$access_level$ static readonly pb::Extension<$extended_type$, $type_name$> $property_name$ =\n"
    "  new pb::Extension<$extended_type$, $type_name$>($number$, ");
  GenerateCodecCode(printer);
  printer->Print(");\n");
}

PrimitiveOneofFieldGenerator::PrimitiveOneofFieldGenerator(
    const FieldDescriptor* descriptor, int presenceIndex, const Options *options)
    : PrimitiveFieldGenerator(descriptor, presenceIndex, options) {
  SetCommonOneofFieldVariables(&variables_);
}

PrimitiveOneofFieldGenerator::~PrimitiveOneofFieldGenerator() {
}

void PrimitiveOneofFieldGenerator::GenerateMembers(io::Printer* printer) {
  WritePropertyDocComment(printer, options(), descriptor_);
  AddPublicMemberAttributes(printer);
  printer->Print(
    variables_,
    "$access_level$ $type_name$ $property_name$ {\n");

  if (options()->enable_nullable) {
    printer->Print(
      variables_,
      "  get { return $has_property_check$ ? ($type_name$) $oneof_name$_! : $default_value$; }\n");
  } else {
    printer->Print(
      variables_,
      "  get { return $has_property_check$ ? ($type_name$) $oneof_name$_ : $default_value$; }\n");
  }
  printer->Print(
    "  set {\n");
  if (is_value_type) {
    printer->Print(
      variables_,
      "    $oneof_name$_ = value;\n");
  } else {
    printer->Print(
      variables_,
      "    $oneof_name$_ = pb::ProtoPreconditions.CheckNotNull(value, \"value\");\n");
  }
  printer->Print(
    variables_,
    "    $oneof_name$Case_ = $oneof_property_name$OneofCase.$oneof_case_name$;\n"
    "  }\n"
    "}\n");
  if (SupportsPresenceApi(descriptor_)) {
    printer->Print(
      variables_,
      "/// <summary>Gets whether the \"$descriptor_name$\" field is set</summary>\n");
    AddPublicMemberAttributes(printer);
    printer->Print(
      variables_,
      "$access_level$ bool Has$property_name$ {\n"
      "  get { return $oneof_name$Case_ == $oneof_property_name$OneofCase.$oneof_case_name$; }\n"
      "}\n");
    printer->Print(
      variables_,
      "/// <summary> Clears the value of the oneof if it's currently set to \"$descriptor_name$\" </summary>\n");
    AddPublicMemberAttributes(printer);
    printer->Print(
      variables_,
      "$access_level$ void Clear$property_name$() {\n"
      "  if ($has_property_check$) {\n"
      "    Clear$oneof_property_name$();\n"
      "  }\n"
      "}\n");
  }
}

void PrimitiveOneofFieldGenerator::GenerateMergingCode(io::Printer* printer) {
  printer->Print(variables_, "$property_name$ = other.$property_name$;\n");
}

void PrimitiveOneofFieldGenerator::WriteToString(io::Printer* printer) {
  printer->Print(variables_,
    "PrintField(\"$descriptor_name$\", $has_property_check$, $oneof_name$_, writer);\n");
}

void PrimitiveOneofFieldGenerator::GenerateParsingCode(io::Printer* printer) {
    printer->Print(
      variables_,
      "$property_name$ = input.Read$capitalized_type_name$();\n");
}

void PrimitiveOneofFieldGenerator::GenerateCloningCode(io::Printer* printer) {
  if (options()->enable_nullable) {
    printer->Print(variables_,
      "$property_name$ = other!.$property_name$;\n");
  } else {
    printer->Print(variables_,
      "$property_name$ = other.$property_name$;\n");
  }
}

}  // namespace csharp
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
