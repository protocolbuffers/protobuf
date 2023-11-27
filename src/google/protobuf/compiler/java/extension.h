// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#ifndef GOOGLE_PROTOBUF_COMPILER_JAVA_EXTENSION_H__
#define GOOGLE_PROTOBUF_COMPILER_JAVA_EXTENSION_H__

#include <string>

#include "absl/container/flat_hash_map.h"
#include "google/protobuf/port.h"

namespace google {
namespace protobuf {
class FieldDescriptor;  // descriptor.h
namespace compiler {
namespace java {
class Context;            // context.h
class ClassNameResolver;  // name_resolver.h
}  // namespace java
}  // namespace compiler
namespace io {
class Printer;  // printer.h
}
}  // namespace protobuf
}  // namespace google

namespace google {
namespace protobuf {
namespace compiler {
namespace java {

// Generates code for an extension, which may be within the scope of some
// message or may be at file scope.  This is much simpler than FieldGenerator
// since extensions are just simple identifiers with interesting types.
class ExtensionGenerator {
 public:
  explicit ExtensionGenerator() {}
  ExtensionGenerator(const ExtensionGenerator&) = delete;
  ExtensionGenerator& operator=(const ExtensionGenerator&) = delete;
  virtual ~ExtensionGenerator() {}

  virtual void Generate(io::Printer* printer) = 0;

  // Returns an estimate of the number of bytes the printed code will compile
  // to
  virtual int GenerateNonNestedInitializationCode(io::Printer* printer) = 0;

  // Returns an estimate of the number of bytes the printed code will compile
  // to
  virtual int GenerateRegistrationCode(io::Printer* printer) = 0;

 protected:
  static void InitTemplateVars(
      const FieldDescriptor* descriptor, const std::string& scope,
      bool immutable, ClassNameResolver* name_resolver,
      absl::flat_hash_map<absl::string_view, std::string>* vars_pointer,
      Context* context);
};

class ImmutableExtensionGenerator : public ExtensionGenerator {
 public:
  explicit ImmutableExtensionGenerator(const FieldDescriptor* descriptor,
                                       Context* context);
  ImmutableExtensionGenerator(const ImmutableExtensionGenerator&) = delete;
  ImmutableExtensionGenerator& operator=(const ImmutableExtensionGenerator&) =
      delete;
  ~ImmutableExtensionGenerator() override;

  void Generate(io::Printer* printer) override;
  int GenerateNonNestedInitializationCode(io::Printer* printer) override;
  int GenerateRegistrationCode(io::Printer* printer) override;

 protected:
  const FieldDescriptor* descriptor_;
  ClassNameResolver* name_resolver_;
  std::string scope_;
  Context* context_;
};

}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_JAVA_EXTENSION_H__
