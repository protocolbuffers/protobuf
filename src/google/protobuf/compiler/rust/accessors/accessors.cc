// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/accessors/accessors.h"

#include <memory>

#include "absl/log/absl_log.h"
#include "google/protobuf/compiler/rust/accessors/accessor_case.h"
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
    Context& ctx, const FieldDescriptor& field) {
  // TODO: We do not support [ctype=FOO] (used to set the field
  // type in C++ to cord or string_piece) in V0.6 API.
  if (field.options().has_ctype()) {
    return std::make_unique<UnsupportedField>(
        "fields with ctype not supported");
  }

  if (field.is_map()) {
    auto value_type = field.message_type()->map_value()->type();
    switch (value_type) {
      case FieldDescriptor::TYPE_ENUM:
      case FieldDescriptor::TYPE_MESSAGE:
        return std::make_unique<UnsupportedField>(
            "Maps with values of type enum and message are not "
            "supported");
      default:
        return std::make_unique<Map>();
    }
  }

  switch (field.type()) {
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
      if (field.is_repeated()) {
        return std::make_unique<RepeatedField>();
      }
      return std::make_unique<SingularScalar>();
    case FieldDescriptor::TYPE_ENUM:
      // TODO: support enums which are defined in other crates.
      if (!IsInCurrentlyGeneratingCrate(ctx, *field.enum_type())) {
        return std::make_unique<UnsupportedField>(
            "enum fields that are imported from another proto_library"
            " (defined in a separate Rust crate) are not supported");
      }
      if (field.is_repeated()) {
        return std::make_unique<RepeatedField>();
      }
      return std::make_unique<SingularScalar>();
    case FieldDescriptor::TYPE_BYTES:
    case FieldDescriptor::TYPE_STRING:
      if (field.is_repeated()) {
        return std::make_unique<RepeatedField>();
      }
      return std::make_unique<SingularString>();
    case FieldDescriptor::TYPE_MESSAGE:
      // TODO: support messages which are defined in other crates.
      if (!IsInCurrentlyGeneratingCrate(ctx, *field.message_type())) {
        return std::make_unique<UnsupportedField>(
            "message fields that are imported from another proto_library"
            " (defined in a separate Rust crate) are not supported");
      }
      if (field.is_repeated()) {
        return std::make_unique<RepeatedField>();
      }
      return std::make_unique<SingularMessage>();

    case FieldDescriptor::TYPE_GROUP:
      return std::make_unique<UnsupportedField>("group not supported");
  }

  ABSL_LOG(FATAL) << "Unexpected field type: " << field.type();
}

}  // namespace

void GenerateAccessorMsgImpl(Context& ctx, const FieldDescriptor& field,
                             AccessorCase accessor_case) {
  AccessorGeneratorFor(ctx, field)->GenerateMsgImpl(ctx, field, accessor_case);
}

void GenerateAccessorExternC(Context& ctx, const FieldDescriptor& field) {
  AccessorGeneratorFor(ctx, field)->GenerateExternC(ctx, field);
}

void GenerateAccessorThunkCc(Context& ctx, const FieldDescriptor& field) {
  AccessorGeneratorFor(ctx, field)->GenerateThunkCc(ctx, field);
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
