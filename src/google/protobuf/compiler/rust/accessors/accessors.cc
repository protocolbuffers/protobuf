// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/accessors/accessors.h"

#include <memory>

#include "google/protobuf/compiler/rust/accessors/accessor_generator.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

namespace {

std::unique_ptr<AccessorGenerator> AccessorGeneratorFor(
    const FieldDescriptor& desc) {
  // We do not support [ctype=FOO] (used to set the field type in C++ to
  // cord or string_piece) in V0 API.
  if (desc.options().has_ctype()) {
    return std::make_unique<UnsupportedField>();
  }

  if (desc.is_repeated()) {
    return std::make_unique<UnsupportedField>();
  }

  switch (desc.type()) {
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT32:
    case FieldDescriptor::TYPE_SINT64:
    case FieldDescriptor::TYPE_UINT32:
    case FieldDescriptor::TYPE_UINT64:
    case FieldDescriptor::TYPE_FLOAT:
    case FieldDescriptor::TYPE_DOUBLE:
    case FieldDescriptor::TYPE_BOOL:
      return std::make_unique<SingularScalar>();
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING:
      return std::make_unique<SingularString>();
    case FieldDescriptor::TYPE_MESSAGE:
      return std::make_unique<SingularMessage>();

    default:
      return std::make_unique<UnsupportedField>();
  }
}

}  // namespace

void GenerateAccessorMsgImpl(Context<FieldDescriptor> field) {
  AccessorGeneratorFor(field.desc())->GenerateMsgImpl(field);
}

void GenerateAccessorExternC(Context<FieldDescriptor> field) {
  AccessorGeneratorFor(field.desc())->GenerateExternC(field);
}

void GenerateAccessorThunkCc(Context<FieldDescriptor> field) {
  AccessorGeneratorFor(field.desc())->GenerateThunkCc(field);
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
