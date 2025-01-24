// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/objectivec/message_field.h"

#include <string>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/compiler/objectivec/field.h"
#include "google/protobuf/compiler/objectivec/helpers.h"
#include "google/protobuf/compiler/objectivec/names.h"
#include "google/protobuf/compiler/objectivec/options.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

namespace {

void SetMessageVariables(const FieldDescriptor* descriptor,
                         SubstitutionMap& variables) {
  const std::string& message_type = ClassName(descriptor->message_type());
  const std::string& containing_class =
      ClassName(descriptor->containing_type());
  variables.Set("msg_type", message_type);
  variables.Set("containing_class", containing_class);
  variables.Set("dataTypeSpecific_value", ObjCClass(message_type));
}

}  // namespace

MessageFieldGenerator::MessageFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : ObjCObjFieldGenerator(descriptor, generation_options) {
  SetMessageVariables(descriptor, variables_);
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
    fwd_decls->insert(absl::StrCat("@class ", variable("msg_type"), ";"));
  }
}

void MessageFieldGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  fwd_decls->insert(ObjCClassDeclaration(variable("msg_type")));
}

void MessageFieldGenerator::DetermineNeededFiles(
    absl::flat_hash_set<const FileDescriptor*>* deps) const {
  if (descriptor_->file() != descriptor_->message_type()->file()) {
    deps->insert(descriptor_->message_type()->file());
  }
}

RepeatedMessageFieldGenerator::RepeatedMessageFieldGenerator(
    const FieldDescriptor* descriptor,
    const GenerationOptions& generation_options)
    : RepeatedFieldGenerator(descriptor, generation_options) {
  SetMessageVariables(descriptor, variables_);
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
    fwd_decls->insert(absl::StrCat("@class ", variable("msg_type"), ";"));
  }
}

void RepeatedMessageFieldGenerator::DetermineObjectiveCClassDefinitions(
    absl::btree_set<std::string>* fwd_decls) const {
  fwd_decls->insert(ObjCClassDeclaration(variable("msg_type")));
}

void RepeatedMessageFieldGenerator::DetermineNeededFiles(
    absl::flat_hash_set<const FileDescriptor*>* deps) const {
  if (descriptor_->file() != descriptor_->message_type()->file()) {
    deps->insert(descriptor_->message_type()->file());
  }
}

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
