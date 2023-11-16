// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/rust/accessors/helpers.h"

#include <cmath>
#include <limits>
#include <string>

#include "absl/log/absl_log.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "google/protobuf/compiler/rust/context.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/strtod.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

std::string DefaultValue(Context<FieldDescriptor> field) {
  switch (field.desc().type()) {
    case FieldDescriptor::TYPE_DOUBLE:
      if (std::isfinite(field.desc().default_value_double())) {
        return absl::StrCat(io::SimpleDtoa(field.desc().default_value_double()),
                            "f64");
      } else if (std::isnan(field.desc().default_value_double())) {
        return std::string("f64::NAN");
      } else if (field.desc().default_value_double() ==
                 std::numeric_limits<double>::infinity()) {
        return std::string("f64::INFINITY");
      } else if (field.desc().default_value_double() ==
                 -std::numeric_limits<double>::infinity()) {
        return std::string("f64::NEG_INFINITY");
      } else {
        ABSL_LOG(FATAL) << "unreachable";
      }
    case FieldDescriptor::TYPE_FLOAT:
      if (std::isfinite(field.desc().default_value_float())) {
        return absl::StrCat(io::SimpleFtoa(field.desc().default_value_float()),
                            "f32");
      } else if (std::isnan(field.desc().default_value_float())) {
        return std::string("f32::NAN");
      } else if (field.desc().default_value_float() ==
                 std::numeric_limits<float>::infinity()) {
        return std::string("f32::INFINITY");
      } else if (field.desc().default_value_float() ==
                 -std::numeric_limits<float>::infinity()) {
        return std::string("f32::NEG_INFINITY");
      } else {
        ABSL_LOG(FATAL) << "unreachable";
      }
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
      return absl::StrFormat("%d", field.desc().default_value_int32());
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
      return absl::StrFormat("%d", field.desc().default_value_int64());
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64:
      return absl::StrFormat("%u", field.desc().default_value_uint64());
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
      return absl::StrFormat("%u", field.desc().default_value_uint32());
    case FieldDescriptor::TYPE_BOOL:
      return absl::StrFormat("%v", field.desc().default_value_bool());
    case FieldDescriptor::TYPE_STRING:
    case FieldDescriptor::TYPE_BYTES:
      return absl::StrFormat(
          "b\"%s\"", absl::CHexEscape(field.desc().default_value_string()));
    case FieldDescriptor::TYPE_GROUP:
    case FieldDescriptor::TYPE_MESSAGE:
    case FieldDescriptor::TYPE_ENUM:
      ABSL_LOG(FATAL) << "Unsupported field type: " << field.desc().type_name();
  }
  ABSL_LOG(FATAL) << "unreachable";
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
