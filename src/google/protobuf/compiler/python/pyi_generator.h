// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: jieluo@google.com (Jie Luo)
//
// Generates Python stub (.pyi) for a given .proto file.

#ifndef GOOGLE_PROTOBUF_COMPILER_PYTHON_PYI_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_PYTHON_PYI_GENERATOR_H__

#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "google/protobuf/compiler/code_generator.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
class Descriptor;
class EnumDescriptor;
class FieldDescriptor;
class MethodDescriptor;
class ServiceDescriptor;

namespace io {
class Printer;
}

namespace compiler {
namespace python {

class PROTOC_EXPORT PyiGenerator : public google::protobuf::compiler::CodeGenerator {
 public:
  PyiGenerator();
  PyiGenerator(const PyiGenerator&) = delete;
  PyiGenerator& operator=(const PyiGenerator&) = delete;
  ~PyiGenerator() override;

  // CodeGenerator methods.
  uint64_t GetSupportedFeatures() const override {
    // Code generators must explicitly support proto3 optional.
    return CodeGenerator::FEATURE_PROTO3_OPTIONAL;
  }
  bool Generate(const FileDescriptor* file, const std::string& parameter,
                GeneratorContext* generator_context,
                std::string* error) const override;

 private:
  void PrintImportForDescriptor(
      const FileDescriptor& desc,
      absl::flat_hash_set<std::string>* seen_aliases) const;
  template <typename DescriptorT>
  void Annotate(const std::string& label, const DescriptorT* descriptor) const;
  void PrintImports() const;
  void PrintTopLevelEnums() const;
  void PrintEnum(const EnumDescriptor& enum_descriptor) const;
  void PrintEnumValues(const EnumDescriptor& enum_descriptor,
                       bool is_classvar = false) const;
  template <typename DescriptorT>
  void PrintExtensions(const DescriptorT& descriptor) const;
  void PrintMessages() const;
  void PrintMessage(const Descriptor& message_descriptor, bool is_nested) const;
  void PrintServices() const;
  std::string GetFieldType(
      const FieldDescriptor& field_des, const Descriptor& containing_des) const;
  template <typename DescriptorT>
  std::string ModuleLevelName(const DescriptorT& descriptor) const;
  std::string PublicPackage() const;
  std::string InternalPackage() const;

  bool opensource_runtime_ = true;

  // Very coarse-grained lock to ensure that Generate() is reentrant.
  // Guards file_, printer_, and import_map_.
  mutable absl::Mutex mutex_;
  mutable const FileDescriptor* file_;  // Set in Generate().  Under mutex_.
  mutable io::Printer* printer_;        // Set in Generate().  Under mutex_.
  // import_map will be a mapping from filename to module alias, e.g.
  // "google3/foo/bar.py" -> "_bar"
  mutable absl::flat_hash_map<std::string, std::string> import_map_;
};

}  // namespace python
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_COMPILER_PYTHON_PYI_GENERATOR_H__
