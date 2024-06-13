// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/internal_helpers.h"

#include "absl/log/absl_log.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/name_resolver.h"
#include "google/protobuf/descriptor.pb.h"

// Must be last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

int GetExperimentalJavaFieldTypeForSingular(const FieldDescriptor* field) {
  // j/c/g/protobuf/FieldType.java lists field types in a slightly different
  // order from FieldDescriptor::Type so we can't do a simple cast.
  //
  // TODO: Make j/c/g/protobuf/FieldType.java follow the same order.
  int result = field->type();
  if (result == FieldDescriptor::TYPE_GROUP) {
    return 17;
  } else if (result < FieldDescriptor::TYPE_GROUP) {
    return result - 1;
  } else {
    return result - 2;
  }
}

int GetExperimentalJavaFieldTypeForRepeated(const FieldDescriptor* field) {
  if (field->type() == FieldDescriptor::TYPE_GROUP) {
    return 49;
  } else {
    return GetExperimentalJavaFieldTypeForSingular(field) + 18;
  }
}

int GetExperimentalJavaFieldTypeForPacked(const FieldDescriptor* field) {
  int result = field->type();
  if (result < FieldDescriptor::TYPE_STRING) {
    return result + 34;
  } else if (result > FieldDescriptor::TYPE_BYTES) {
    return result + 30;
  } else {
    ABSL_LOG(FATAL) << field->full_name() << " can't be packed.";
    return 0;
  }
}
}  // namespace

int GetExperimentalJavaFieldType(const FieldDescriptor* field) {
  static const int kMapFieldType = 50;
  static const int kOneofFieldTypeOffset = 51;

  static const int kRequiredBit = 0x100;
  static const int kUtf8CheckBit = 0x200;
  static const int kCheckInitialized = 0x400;
  static const int kLegacyEnumIsClosedBit = 0x800;
  static const int kHasHasBit = 0x1000;
  int extra_bits = field->is_required() ? kRequiredBit : 0;
  if (field->type() == FieldDescriptor::TYPE_STRING && CheckUtf8(field)) {
    extra_bits |= kUtf8CheckBit;
  }
  if (field->is_required() || (GetJavaType(field) == JAVATYPE_MESSAGE &&
                               HasRequiredFields(field->message_type()))) {
    extra_bits |= kCheckInitialized;
  }
  if (HasHasbit(field)) {
    extra_bits |= kHasHasBit;
  }
  if (GetJavaType(field) == JAVATYPE_ENUM && !SupportUnknownEnumValue(field)) {
    extra_bits |= kLegacyEnumIsClosedBit;
  }

  if (field->is_map()) {
    if (!SupportUnknownEnumValue(MapValueField(field))) {
      const FieldDescriptor* value = field->message_type()->map_value();
      if (GetJavaType(value) == JAVATYPE_ENUM) {
        extra_bits |= kLegacyEnumIsClosedBit;
      }
    }
    return kMapFieldType | extra_bits;
  } else if (field->is_packed()) {
    return GetExperimentalJavaFieldTypeForPacked(field) | extra_bits;
  } else if (field->is_repeated()) {
    return GetExperimentalJavaFieldTypeForRepeated(field) | extra_bits;
  } else if (IsRealOneof(field)) {
    return (GetExperimentalJavaFieldTypeForSingular(field) +
            kOneofFieldTypeOffset) |
           extra_bits;
  } else {
    return GetExperimentalJavaFieldTypeForSingular(field) | extra_bits;
  }
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
