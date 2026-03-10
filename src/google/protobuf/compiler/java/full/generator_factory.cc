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
#include "google/protobuf/compiler/java/full/enum.h"
#include "google/protobuf/compiler/java/full/extension.h"
#include "google/protobuf/compiler/java/full/message.h"
#include "google/protobuf/compiler/java/full/service.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// Factory that creates generators for immutable-default messages.
class ImmutableGeneratorFactory : public GeneratorFactory {
 public:
  explicit ImmutableGeneratorFactory(Context* context) : context_(context) {}
  ImmutableGeneratorFactory(const ImmutableGeneratorFactory&) = delete;
  ImmutableGeneratorFactory& operator=(const ImmutableGeneratorFactory&) =
      delete;
  ~ImmutableGeneratorFactory() override = default;

  std::unique_ptr<MessageGenerator> NewMessageGenerator(
      const Descriptor* descriptor) const override {
    return std::make_unique<ImmutableMessageGenerator>(descriptor, context_);
  }

  std::unique_ptr<EnumGenerator> NewEnumGenerator(
      const EnumDescriptor* descriptor) const override {
    return std::make_unique<EnumNonLiteGenerator>(descriptor, true, context_);
  }

  std::unique_ptr<ExtensionGenerator> NewExtensionGenerator(
      const FieldDescriptor* descriptor) const override {
    return std::make_unique<ImmutableExtensionGenerator>(descriptor, context_);
  }

  std::unique_ptr<ServiceGenerator> NewServiceGenerator(
      const ServiceDescriptor* descriptor) const override {
    return std::make_unique<ImmutableServiceGenerator>(descriptor, context_);
  }

 private:
  Context* context_;
};

std::unique_ptr<GeneratorFactory> MakeImmutableGeneratorFactory(
    Context* context) {
  return std::make_unique<ImmutableGeneratorFactory>(context);
}

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
