// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: liujisi@google.com (Pherl Liu)

#include "google/protobuf/compiler/java/generator_factory.h"

#include <memory>

#include "google/protobuf/compiler/java/context.h"
#include "google/protobuf/compiler/java/full/service.h"
#include "google/protobuf/compiler/java/lite/enum.h"
#include "google/protobuf/compiler/java/lite/extension.h"
#include "google/protobuf/compiler/java/lite/message.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// Factory that creates generators for immutable-default messages.
class ImmutableLiteGeneratorFactory : public GeneratorFactory {
 public:
  explicit ImmutableLiteGeneratorFactory(Context* context)
      : context_(context) {}
  ImmutableLiteGeneratorFactory(const ImmutableLiteGeneratorFactory&) = delete;
  ImmutableLiteGeneratorFactory& operator=(
      const ImmutableLiteGeneratorFactory&) = delete;
  ~ImmutableLiteGeneratorFactory() override = default;

  std::unique_ptr<MessageGenerator> NewMessageGenerator(
      const Descriptor* descriptor) const override {
    return std::make_unique<ImmutableMessageLiteGenerator>(descriptor,
                                                           context_);
  }

  std::unique_ptr<EnumGenerator> NewEnumGenerator(
      const EnumDescriptor* descriptor) const override {
    return std::make_unique<EnumLiteGenerator>(descriptor, true, context_);
  }

  std::unique_ptr<ExtensionGenerator> NewExtensionGenerator(
      const FieldDescriptor* descriptor) const override {
    return std::make_unique<ImmutableExtensionLiteGenerator>(descriptor,
                                                             context_);
  }

  std::unique_ptr<ServiceGenerator> NewServiceGenerator(
      const ServiceDescriptor* descriptor) const override {
    return std::make_unique<ImmutableServiceGenerator>(descriptor, context_);
  }

 private:
  Context* context_;
};

std::unique_ptr<GeneratorFactory> MakeImmutableLiteGeneratorFactory(
    Context* context) {
  return std::make_unique<ImmutableLiteGeneratorFactory>(context);
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
