// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/binary_test_util.h"

#include "absl/types/span.h"
#include "google/protobuf/descriptor.h"

namespace google {
namespace protobuf {
namespace conformance {

absl::Span<const FieldDescriptor::Type> AllFieldTypesExceptGroup() {
  static constexpr FieldDescriptor::Type kTypes[] = {
      FieldDescriptor::TYPE_DOUBLE,   FieldDescriptor::TYPE_FLOAT,
      FieldDescriptor::TYPE_INT64,    FieldDescriptor::TYPE_UINT64,
      FieldDescriptor::TYPE_INT32,    FieldDescriptor::TYPE_UINT32,
      FieldDescriptor::TYPE_FIXED64,  FieldDescriptor::TYPE_FIXED32,
      FieldDescriptor::TYPE_SFIXED64, FieldDescriptor::TYPE_SFIXED32,
      FieldDescriptor::TYPE_BOOL,     FieldDescriptor::TYPE_SINT32,
      FieldDescriptor::TYPE_SINT64,   FieldDescriptor::TYPE_STRING,
      FieldDescriptor::TYPE_BYTES,    FieldDescriptor::TYPE_ENUM,
      FieldDescriptor::TYPE_MESSAGE,
  };
  return kTypes;
}

absl::Span<const FieldDescriptor::Type> PackableFieldTypes() {
  static constexpr FieldDescriptor::Type kTypes[] = {
      FieldDescriptor::TYPE_DOUBLE,   FieldDescriptor::TYPE_FLOAT,
      FieldDescriptor::TYPE_INT64,    FieldDescriptor::TYPE_UINT64,
      FieldDescriptor::TYPE_INT32,    FieldDescriptor::TYPE_UINT32,
      FieldDescriptor::TYPE_FIXED64,  FieldDescriptor::TYPE_FIXED32,
      FieldDescriptor::TYPE_SFIXED64, FieldDescriptor::TYPE_SFIXED32,
      FieldDescriptor::TYPE_BOOL,     FieldDescriptor::TYPE_SINT32,
      FieldDescriptor::TYPE_SINT64,   FieldDescriptor::TYPE_ENUM,
  };
  return kTypes;
}

absl::Span<const FieldDescriptor::Type> LengthDelimitedFieldTypes() {
  static constexpr FieldDescriptor::Type kTypes[] = {
      FieldDescriptor::TYPE_STRING,
      FieldDescriptor::TYPE_BYTES,
      FieldDescriptor::TYPE_MESSAGE,
  };
  return kTypes;
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
