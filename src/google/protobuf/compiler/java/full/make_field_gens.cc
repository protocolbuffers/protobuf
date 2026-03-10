// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <memory>
#include <utility>

#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/generator_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/full/enum_field.h"
#include "google/protobuf/compiler/java/full/field_generator.h"
#include "google/protobuf/compiler/java/full/map_field.h"
#include "google/protobuf/compiler/java/full/message_field.h"
#include "google/protobuf/compiler/java/full/primitive_field.h"
#include "google/protobuf/compiler/java/full/string_field.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

std::unique_ptr<ImmutableFieldGenerator> MakeImmutableGenerator(
    const FieldDescriptor* field, int messageBitIndex, int builderBitIndex,
    Context* context) {
  if (field->is_repeated()) {
    switch (GetJavaType(field)) {
      case JAVATYPE_MESSAGE:
        if (IsMapEntry(field->message_type())) {
          return std::make_unique<ImmutableMapFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
        } else {
          return std::make_unique<RepeatedImmutableMessageFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
        }
      case JAVATYPE_ENUM:
        return std::make_unique<RepeatedImmutableEnumFieldGenerator>(
            field, messageBitIndex, builderBitIndex, context);
      case JAVATYPE_STRING:
        return std::make_unique<RepeatedImmutableStringFieldGenerator>(
            field, messageBitIndex, builderBitIndex, context);
      default:
        return std::make_unique<RepeatedImmutablePrimitiveFieldGenerator>(
            field, messageBitIndex, builderBitIndex, context);
    }
  } else {
    if (IsRealOneof(field)) {
      switch (GetJavaType(field)) {
        case JAVATYPE_MESSAGE:
          return std::make_unique<ImmutableMessageOneofFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
        case JAVATYPE_ENUM:
          return std::make_unique<ImmutableEnumOneofFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
        case JAVATYPE_STRING:
          return std::make_unique<ImmutableStringOneofFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
        default:
          return std::make_unique<ImmutablePrimitiveOneofFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
      }
    } else {
      switch (GetJavaType(field)) {
        case JAVATYPE_MESSAGE:
          return std::make_unique<ImmutableMessageFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
        case JAVATYPE_ENUM:
          return std::make_unique<ImmutableEnumFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
        case JAVATYPE_STRING:
          return std::make_unique<ImmutableStringFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
        default:
          return std::make_unique<ImmutablePrimitiveFieldGenerator>(
              field, messageBitIndex, builderBitIndex, context);
      }
    }
  }
}

}  // namespace

FieldGeneratorMap<ImmutableFieldGenerator> MakeImmutableFieldGenerators(
    const Descriptor* descriptor, Context* context) {
  // Construct all the FieldGenerators and assign them bit indices for their
  // bit fields.
  int messageBitIndex = 0;
  int builderBitIndex = 0;
  FieldGeneratorMap<ImmutableFieldGenerator> ret(descriptor);
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    auto generator = MakeImmutableGenerator(field, messageBitIndex,
                                            builderBitIndex, context);
    messageBitIndex += generator->GetNumBitsForMessage();
    builderBitIndex += generator->GetNumBitsForBuilder();
    ret.Add(field, std::move(generator));
  }
  return ret;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
