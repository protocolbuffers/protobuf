// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/primitive_field.h"

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "google/protobuf/compiler/objectivec/field.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

PrimitiveFieldGenerator::PrimitiveFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : SingleFieldGenerator(descriptor, generation_options) {}

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
    variables_.Set("storage_offset_value", absl::StrCat(index_base));
    variables_.Set("storage_offset_comment",
                   "  // Stored in _has_storage_ to save space.");
  }
}

PrimitiveObjFieldGenerator::PrimitiveObjFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : ObjCObjFieldGenerator(descriptor, generation_options) {
  variables_.Set("property_storage_attribute", "copy");
}

RepeatedPrimitiveFieldGenerator::RepeatedPrimitiveFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : RepeatedFieldGenerator(descriptor, generation_options) {}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
