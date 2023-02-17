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

#include "google/protobuf/compiler/objectivec/message_field.h"

#include <string>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

void SetMessageVariables(
    const FieldDescriptor* descriptor,
    absl::flat_hash_map<absl::string_view, std::string>* variables) {
  const std::string& message_type = ClassName(descriptor->message_type());
  const std::string& containing_class =
      ClassName(descriptor->containing_type());
  (*variables)["type"] = message_type;
  (*variables)["containing_class"] = containing_class;
  (*variables)["storage_type"] = message_type;
  (*variables)["group_or_message"] =
      (descriptor->type() == FieldDescriptor::TYPE_GROUP) ? "Group" : "Message";
  (*variables)["dataTypeSpecific_value"] = ObjCClass(message_type);
}

}  // namespace

MessageFieldGenerator::MessageFieldGenerator(const FieldDescriptor* descriptor)
    : ObjCObjFieldGenerator(descriptor) {
  SetMessageVariables(descriptor, &variables_);
}

void MessageFieldGenerator::DetermineForwardDeclarations(
    absl::btree_set<std::string>* fwd_decls,
    bool include_external_types) const {
  ObjCObjFieldGenerator::DetermineForwardDeclarations(fwd_decls,
                                                      include_external_types);
  // Within a file there is no requirement on the order of the messages, so
  // local references need a forward declaration. External files (not WKTs),
  // need one when requested.
  if ((include_external_types && !IsProtobufLibraryBundledProtoFile(
                                     descriptor_->message_type()->file())) ||
      descriptor_->file() == descriptor_->message_type()->file()) {
    // Class name is already in "storage_type".
    fwd_decls->insert(absl::StrCat("@class ", variable("storage_type"), ";"));
  }
}

void MessageFieldGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  fwd_decls->insert(ObjCClassDeclaration(variable("storage_type")));
}

RepeatedMessageFieldGenerator::RepeatedMessageFieldGenerator(
    const FieldDescriptor* descriptor)
    : RepeatedFieldGenerator(descriptor) {
  SetMessageVariables(descriptor, &variables_);
  std::string storage_type = variables_["storage_type"];
  variables_["array_storage_type"] = "NSMutableArray";
  variables_["array_property_type"] =
      absl::StrCat("NSMutableArray<", storage_type, "*>");
}

void RepeatedMessageFieldGenerator::DetermineForwardDeclarations(
    absl::btree_set<std::string>* fwd_decls,
    bool include_external_types) const {
  RepeatedFieldGenerator::DetermineForwardDeclarations(fwd_decls,
                                                       include_external_types);
  // Within a file there is no requirement on the order of the messages, so
  // local references need a forward declaration. External files (not WKTs),
  // need one when requested.
  if ((include_external_types && !IsProtobufLibraryBundledProtoFile(
                                     descriptor_->message_type()->file())) ||
      descriptor_->file() == descriptor_->message_type()->file()) {
    // Class name is already in "storage_type".
    fwd_decls->insert(absl::StrCat("@class ", variable("storage_type"), ";"));
  }
}

void RepeatedMessageFieldGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  fwd_decls->insert(ObjCClassDeclaration(variable("storage_type")));
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
