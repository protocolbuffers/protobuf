// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: liujisi@google.com (Pherl Liu)

#include "google/protobuf/compiler/java/generator_factory.h"

#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/enum_field.h"
#include "google/protobuf/compiler/java/extension.h"
#include "google/protobuf/compiler/java/extension_lite.h"
#include "google/protobuf/compiler/java/field.h"
#include "google/protobuf/compiler/java/helpers.h"
#include "google/protobuf/compiler/java/message.h"
#include "google/protobuf/compiler/java/message_lite.h"
#include "google/protobuf/compiler/java/service.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

GeneratorFactory::GeneratorFactory() {}
GeneratorFactory::~GeneratorFactory() {}

// ===================================================================

ImmutableGeneratorFactory::ImmutableGeneratorFactory(Context* context)
    : context_(context) {}
ImmutableGeneratorFactory::~ImmutableGeneratorFactory() {}

MessageGenerator* ImmutableGeneratorFactory::NewMessageGenerator(
    const Descriptor* descriptor) const {
  if (HasDescriptorMethods(descriptor, context_->EnforceLite())) {
    return new ImmutableMessageGenerator(descriptor, context_);
  } else {
    return new ImmutableMessageLiteGenerator(descriptor, context_);
  }
}

ExtensionGenerator* ImmutableGeneratorFactory::NewExtensionGenerator(
    const FieldDescriptor* descriptor) const {
  if (HasDescriptorMethods(descriptor->file(), context_->EnforceLite())) {
    return new ImmutableExtensionGenerator(descriptor, context_);
  } else {
    return new ImmutableExtensionLiteGenerator(descriptor, context_);
  }
}

ServiceGenerator* ImmutableGeneratorFactory::NewServiceGenerator(
    const ServiceDescriptor* descriptor) const {
  return new ImmutableServiceGenerator(descriptor, context_);
}


}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
