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
#include "google/protobuf/compiler/rust/naming.h"
#include "google/protobuf/compiler/rust/rust_field_type.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/strtod.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace rust {

std::string DefaultValue(Context& ctx, const FieldDescriptor& field) {
  switch (GetRustFieldType(field)) {
    case RustFieldType::DOUBLE:
      if (std::isfinite(field.default_value_double())) {
        return absl::StrCat(io::SimpleDtoa(field.default_value_double()),
                            "f64");
      } else if (std::isnan(field.default_value_double())) {
        return std::string("f64::NAN");
      } else if (field.default_value_double() ==
                 std::numeric_limits<double>::infinity()) {
        return std::string("f64::INFINITY");
      } else if (field.default_value_double() ==
                 -std::numeric_limits<double>::infinity()) {
        return std::string("f64::NEG_INFINITY");
      } else {
        ABSL_LOG(FATAL) << "unreachable";
      }
    case RustFieldType::FLOAT:
      if (std::isfinite(field.default_value_float())) {
        return absl::StrCat(io::SimpleFtoa(field.default_value_float()), "f32");
      } else if (std::isnan(field.default_value_float())) {
        return std::string("f32::NAN");
      } else if (field.default_value_float() ==
                 std::numeric_limits<float>::infinity()) {
        return std::string("f32::INFINITY");
      } else if (field.default_value_float() ==
                 -std::numeric_limits<float>::infinity()) {
        return std::string("f32::NEG_INFINITY");
      } else {
        ABSL_LOG(FATAL) << "unreachable";
      }
    case RustFieldType::INT32:
      return absl::StrFormat("%d", field.default_value_int32());
    case RustFieldType::INT64:
      return absl::StrFormat("%d", field.default_value_int64());
    case RustFieldType::UINT64:
      return absl::StrFormat("%u", field.default_value_uint64());
    case RustFieldType::UINT32:
      return absl::StrFormat("%u", field.default_value_uint32());
    case RustFieldType::BOOL:
      return absl::StrFormat("%v", field.default_value_bool());
    case RustFieldType::STRING:
    case RustFieldType::BYTES:
      return absl::StrFormat("b\"%s\"",
                             absl::CHexEscape(field.default_value_string()));
    case RustFieldType::ENUM:
      // `$EnumName$::default()` might seem like the right choice here, but
      // it is not. The default value for the enum type isn't the same as the
      // field, since in `syntax = "proto2"`, an enum field can have a default
      // value other than the first listed in the enum.
      //
      // Even in cases where there is no custom field default, `default()` can't
      // be used. This is because the vtables for field mutators store the
      // default value. They are `static`s which are constructed with a `const`
      // expression. Trait methods in a `const` context aren't currently stable.
      return absl::StrCat(RsTypePath(ctx, field),
                          "::", EnumValueRsName(*field.default_value_enum()));
    case RustFieldType::MESSAGE:
      ABSL_LOG(FATAL) << "Messages can't have defaults: " << field.type_name();
  }
  ABSL_LOG(FATAL) << "unreachable";
}

}  // namespace rust
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
