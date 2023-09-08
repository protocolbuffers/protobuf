// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_PRIMITIVE_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_PRIMITIVE_FIELD_H__

#include "google/protobuf/compiler/objectivec/field.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace objectivec {

class PrimitiveFieldGenerator : public SingleFieldGenerator {
  friend FieldGenerator* FieldGenerator::Make(const FieldDescriptor* field);

 protected:
  explicit PrimitiveFieldGenerator(const FieldDescriptor* descriptor);
  ~PrimitiveFieldGenerator() override = default;

  PrimitiveFieldGenerator(const PrimitiveFieldGenerator&) = delete;
  PrimitiveFieldGenerator& operator=(const PrimitiveFieldGenerator&) = delete;

  void GenerateFieldStorageDeclaration(io::Printer* printer) const override;

  int ExtraRuntimeHasBitsNeeded() const override;
  void SetExtraRuntimeHasBitsBase(int index_base) override;
};

class PrimitiveObjFieldGenerator : public ObjCObjFieldGenerator {
  friend FieldGenerator* FieldGenerator::Make(const FieldDescriptor* field);

 protected:
  explicit PrimitiveObjFieldGenerator(const FieldDescriptor* descriptor);
  ~PrimitiveObjFieldGenerator() override = default;

  PrimitiveObjFieldGenerator(const PrimitiveObjFieldGenerator&) = delete;
  PrimitiveObjFieldGenerator& operator=(const PrimitiveObjFieldGenerator&) =
      delete;
};

class RepeatedPrimitiveFieldGenerator : public RepeatedFieldGenerator {
  friend FieldGenerator* FieldGenerator::Make(const FieldDescriptor* field);

 protected:
  explicit RepeatedPrimitiveFieldGenerator(const FieldDescriptor* descriptor);
  ~RepeatedPrimitiveFieldGenerator() override = default;

  RepeatedPrimitiveFieldGenerator(const RepeatedPrimitiveFieldGenerator&) =
      delete;
  RepeatedPrimitiveFieldGenerator& operator=(
      const RepeatedPrimitiveFieldGenerator&) = delete;
};

}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_PRIMITIVE_FIELD_H__
