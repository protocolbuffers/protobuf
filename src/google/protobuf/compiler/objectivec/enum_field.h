// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_ENUM_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_ENUM_FIELD_H__

#include <string>

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "google/protobuf/compiler/objectivec/field.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

class EnumFieldGenerator : public SingleFieldGenerator {
  friend FieldGenerator* FieldGenerator::Make(const FieldDescriptor* field);

  EnumFieldGenerator(const EnumFieldGenerator&) = delete;
  EnumFieldGenerator& operator=(const EnumFieldGenerator&) = delete;

 public:
  void GenerateCFunctionDeclarations(io::Printer* printer) const override;
  void GenerateCFunctionImplementations(io::Printer* printer) const override;
  void DetermineForwardDeclarations(absl::btree_set<std::string>* fwd_decls,
                                    bool include_external_types) const override;
  void DetermineNeededFiles(
      absl::flat_hash_set<const FileDescriptor*>* deps) const override;

 protected:
  explicit EnumFieldGenerator(const FieldDescriptor* descriptor);
  ~EnumFieldGenerator() override = default;
};

class RepeatedEnumFieldGenerator : public RepeatedFieldGenerator {
  friend FieldGenerator* FieldGenerator::Make(const FieldDescriptor* field);

 public:
  void EmitArrayComment(io::Printer* printer) const override;
  void DetermineNeededFiles(
      absl::flat_hash_set<const FileDescriptor*>* deps) const override;

 protected:
  explicit RepeatedEnumFieldGenerator(const FieldDescriptor* descriptor);
  ~RepeatedEnumFieldGenerator() override = default;
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_ENUM_FIELD_H__
