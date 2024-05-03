// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: liujisi@google.com (Pherl Liu)

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_GENERATOR_FACTORY_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_GENERATOR_FACTORY_H__

#include "google/protobuf/port.h"

namespace google {
namespace protobuf {
class FieldDescriptor;    // descriptor.h
class Descriptor;         // descriptor.h
class ServiceDescriptor;  // descriptor.h
namespace compiler {
namespace java {
class MessageGenerator;    // message.h
class ExtensionGenerator;  // extension.h
class ServiceGenerator;    // service.h
class Context;             // context.h
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

class GeneratorFactory {
 public:
  GeneratorFactory();
  GeneratorFactory(const GeneratorFactory&) = delete;
  GeneratorFactory& operator=(const GeneratorFactory&) = delete;
  virtual ~GeneratorFactory();

  virtual MessageGenerator* NewMessageGenerator(
      const Descriptor* descriptor) const = 0;

  virtual ExtensionGenerator* NewExtensionGenerator(
      const FieldDescriptor* descriptor) const = 0;

  virtual ServiceGenerator* NewServiceGenerator(
      const ServiceDescriptor* descriptor) const = 0;
};

// Factory that creates generators for immutable-default messages.
class ImmutableGeneratorFactory : public GeneratorFactory {
 public:
  ImmutableGeneratorFactory(Context* context);
  ImmutableGeneratorFactory(const ImmutableGeneratorFactory&) = delete;
  ImmutableGeneratorFactory& operator=(const ImmutableGeneratorFactory&) =
      delete;
  ~ImmutableGeneratorFactory() override;

  MessageGenerator* NewMessageGenerator(
      const Descriptor* descriptor) const override;

  ExtensionGenerator* NewExtensionGenerator(
      const FieldDescriptor* descriptor) const override;

  ServiceGenerator* NewServiceGenerator(
      const ServiceDescriptor* descriptor) const override;

 private:
  Context* context_;
};


}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_GENERATOR_FACTORY_H__
