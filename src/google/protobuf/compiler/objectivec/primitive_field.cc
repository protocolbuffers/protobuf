// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/primitive_field.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/log/absl_log.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

const char* PrimitiveTypeName(const FieldDescriptor* descriptor) {
  ObjectiveCType type = GetObjectiveCType(descriptor);
  switch (type) {
    case OBJECTIVECTYPE_INT32:
      return "int32_t";
    case OBJECTIVECTYPE_UINT32:
      return "uint32_t";
    case OBJECTIVECTYPE_INT64:
      return "int64_t";
    case OBJECTIVECTYPE_UINT64:
      return "uint64_t";
    case OBJECTIVECTYPE_FLOAT:
      return "float";
    case OBJECTIVECTYPE_DOUBLE:
      return "double";
    case OBJECTIVECTYPE_BOOLEAN:
      return "BOOL";
    case OBJECTIVECTYPE_STRING:
      return "NSString";
    case OBJECTIVECTYPE_DATA:
      return "NSData";
    case OBJECTIVECTYPE_ENUM:
      return "int32_t";
    case OBJECTIVECTYPE_MESSAGE:
      return nullptr;  // Messages go through message_field.cc|h.
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return nullptr;
}

const char* PrimitiveArrayTypeName(const FieldDescriptor* descriptor) {
  ObjectiveCType type = GetObjectiveCType(descriptor);
  switch (type) {
    case OBJECTIVECTYPE_INT32:
      return "Int32";
    case OBJECTIVECTYPE_UINT32:
      return "UInt32";
    case OBJECTIVECTYPE_INT64:
      return "Int64";
    case OBJECTIVECTYPE_UINT64:
      return "UInt64";
    case OBJECTIVECTYPE_FLOAT:
      return "Float";
    case OBJECTIVECTYPE_DOUBLE:
      return "Double";
    case OBJECTIVECTYPE_BOOLEAN:
      return "Bool";
    case OBJECTIVECTYPE_STRING:
      return "";  // Want NSArray
    case OBJECTIVECTYPE_DATA:
      return "";  // Want NSArray
    case OBJECTIVECTYPE_ENUM:
      return "Enum";
    case OBJECTIVECTYPE_MESSAGE:
      // Want NSArray (but goes through message_field.cc|h).
      return "";
  }

  // Some compilers report reaching end of function even though all cases of
  // the enum are handed in the switch.
  ABSL_LOG(FATAL) << "Can't get here.";
  return nullptr;
}

void SetPrimitiveVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  std::string primitive_name = PrimitiveTypeName(descriptor);
  (*variables)["type"] = primitive_name;
  (*variables)["storage_type"] = primitive_name;
}

}  // namespace

PrimitiveFieldGenerator::PrimitiveFieldGenerator(
    const FieldDescriptor* descriptor)
    : SingleFieldGenerator(descriptor) {
  SetPrimitiveVariables(descriptor, &variables_);
}

void PrimitiveFieldGenerator::GenerateFieldStorageDeclaration(
    io::Printer* printer) const {
  if (GetObjectiveCType(descriptor_) == OBJECTIVECTYPE_BOOLEAN) {
    // Nothing, BOOLs are stored in the has bits.
  } else {
    SingleFieldGenerator::GenerateFieldStorageDeclaration(printer);
  }
}

int PrimitiveFieldGenerator::ExtraRuntimeHasBitsNeeded() const {
  if (GetObjectiveCType(descriptor_) == OBJECTIVECTYPE_BOOLEAN) {
    // Reserve a bit for the storage of the boolean.
    return 1;
  }
  return 0;
}

void PrimitiveFieldGenerator::SetExtraRuntimeHasBitsBase(int index_base) {
  if (GetObjectiveCType(descriptor_) == OBJECTIVECTYPE_BOOLEAN) {
    // Set into the offset the has bit to use for the actual value.
    variables_["storage_offset_value"] = absl::StrCat(index_base);
    variables_["storage_offset_comment"] =
        "  // Stored in _has_storage_ to save space.";
  }
}

PrimitiveObjFieldGenerator::PrimitiveObjFieldGenerator(
    const FieldDescriptor* descriptor)
    : ObjCObjFieldGenerator(descriptor) {
  SetPrimitiveVariables(descriptor, &variables_);
  variables_["property_storage_attribute"] = "copy";
}

RepeatedPrimitiveFieldGenerator::RepeatedPrimitiveFieldGenerator(
    const FieldDescriptor* descriptor)
    : RepeatedFieldGenerator(descriptor) {
  SetPrimitiveVariables(descriptor, &variables_);

  std::string base_name = PrimitiveArrayTypeName(descriptor);
  if (base_name.length()) {
    variables_["array_storage_type"] = absl::StrCat("GPB", base_name, "Array");
  } else {
    std::string storage_type = variables_["storage_type"];
    variables_["array_storage_type"] = "NSMutableArray";
    variables_["array_property_type"] =
        absl::StrCat("NSMutableArray<", storage_type, "*>");
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
