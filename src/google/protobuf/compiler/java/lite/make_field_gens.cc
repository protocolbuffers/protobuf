// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include "google/protobuf/compiler/java/lite/make_field_gens.h"

#include <memory>
#include <utility>

#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/generator_common.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/lite/enum_field.h"
#include "google/protobuf/compiler/java/lite/field_generator.h"
#include "google/protobuf/compiler/java/lite/map_field.h"
#include "google/protobuf/compiler/java/lite/message_field.h"
#include "google/protobuf/compiler/java/lite/primitive_field.h"
#include "google/protobuf/compiler/java/lite/string_field.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

namespace {

std::unique_ptr<ImmutableFieldLiteGenerator> CreateImmutableLiteGenerator(
    const FieldDescriptor* field, int messageBitIndex, Context* context) {
  if (field->is_repeated()) {
    switch (GetJavaType(field)) {
      case JAVATYPE_MESSAGE:
        if (IsMapEntry(field->message_type())) {
          return std::make_unique<ImmutableMapFieldLiteGenerator>(
              field, messageBitIndex, context);
        } else {
          return std::make_unique<RepeatedImmutableMessageFieldLiteGenerator>(
              field, messageBitIndex, context);
        }
      case JAVATYPE_ENUM:
        return std::make_unique<RepeatedImmutableEnumFieldLiteGenerator>(
            field, messageBitIndex, context);
      case JAVATYPE_STRING:
        return std::make_unique<RepeatedImmutableStringFieldLiteGenerator>(
            field, messageBitIndex, context);
      default:
        return std::make_unique<RepeatedImmutablePrimitiveFieldLiteGenerator>(
            field, messageBitIndex, context);
    }
  } else {
    if (IsRealOneof(field)) {
      switch (GetJavaType(field)) {
        case JAVATYPE_MESSAGE:
          return std::make_unique<ImmutableMessageOneofFieldLiteGenerator>(
              field, messageBitIndex, context);
        case JAVATYPE_ENUM:
          return std::make_unique<ImmutableEnumOneofFieldLiteGenerator>(
              field, messageBitIndex, context);
        case JAVATYPE_STRING:
          return std::make_unique<ImmutableStringOneofFieldLiteGenerator>(
              field, messageBitIndex, context);
        default:
          return std::make_unique<ImmutablePrimitiveOneofFieldLiteGenerator>(
              field, messageBitIndex, context);
      }
    } else {
      switch (GetJavaType(field)) {
        case JAVATYPE_MESSAGE:
          return std::make_unique<ImmutableMessageFieldLiteGenerator>(
              field, messageBitIndex, context);
        case JAVATYPE_ENUM:
          return std::make_unique<ImmutableEnumFieldLiteGenerator>(
              field, messageBitIndex, context);
        case JAVATYPE_STRING:
          return std::make_unique<ImmutableStringFieldLiteGenerator>(
              field, messageBitIndex, context);
        default:
          return std::make_unique<ImmutablePrimitiveFieldLiteGenerator>(
              field, messageBitIndex, context);
      }
    }
  }
}

}  // namespace

FieldGeneratorMap<ImmutableFieldLiteGenerator> MakeImmutableFieldLiteGenerators(
    const Descriptor* descriptor, Context* context) {
  int messageBitIndex = 0;
  FieldGeneratorMap<ImmutableFieldLiteGenerator> field_generators(descriptor);
  for (int i = 0; i < descriptor->field_count(); i++) {
    const FieldDescriptor* field = descriptor->field(i);
    auto generator =
        CreateImmutableLiteGenerator(field, messageBitIndex, context);
    messageBitIndex += generator->GetNumBitsForMessage();
    field_generators.Add(field, std::move(generator));
  }
  return field_generators;
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
