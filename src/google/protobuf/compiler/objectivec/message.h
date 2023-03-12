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

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_MESSAGE_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_MESSAGE_H__

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "absl/container/btree_set.h"
#include "google/protobuf/compiler/objectivec/field.h"
#include "google/protobuf/compiler/objectivec/oneof.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/printer.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

class ExtensionGenerator;

class MessageGenerator {
 public:
  MessageGenerator(const std::string& file_description_name,
                   const Descriptor* descriptor);
  ~MessageGenerator() = default;

  MessageGenerator(const MessageGenerator&) = delete;
  MessageGenerator& operator=(const MessageGenerator&) = delete;

  void AddExtensionGenerators(
      std::vector<std::unique_ptr<ExtensionGenerator>>* extension_generators);

  void GenerateMessageHeader(io::Printer* printer) const;
  void GenerateSource(io::Printer* printer) const;
  void DetermineObjectiveCClassDefinitions(
      absl::btree_set<std::string>* fwd_decls) const;
  void DetermineForwardDeclarations(absl::btree_set<std::string>* fwd_decls,
                                    bool include_external_types) const;

  // Checks if the message or a nested message includes a oneof definition.
  bool IncludesOneOfDefinition() const { return !oneof_generators_.empty(); }

 private:
  const std::string root_classname_;
  const std::string file_description_name_;
  const Descriptor* descriptor_;
  FieldGeneratorMap field_generators_;
  const std::string class_name_;
  const std::string deprecated_attribute_;
  std::vector<const ExtensionGenerator*> extension_generators_;
  std::vector<std::unique_ptr<OneofGenerator>> oneof_generators_;
  size_t sizeof_has_storage_;
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_MESSAGE_H__
