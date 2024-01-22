// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef UPB_PROTOS_GENERATOR_NAMES_H
#define UPB_PROTOS_GENERATOR_NAMES_H

#include <string>

#include "absl/base/attributes.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "upb/reflection/def.hpp"

namespace upb {
namespace generator {

using NameToFieldDescriptorMap =
    absl::flat_hash_map<absl::string_view, const google::protobuf::FieldDescriptor*>;

// Returns field name by resolving naming conflicts across
// proto field names (such as clear_ prefixes).
std::string ResolveFieldName(const google::protobuf::FieldDescriptor* field,
                             const NameToFieldDescriptorMap& field_names);

// Returns field map by name to use for conflict checks.
NameToFieldDescriptorMap CreateFieldNameMap(const google::protobuf::Descriptor* message);

using NameToFieldDefMap =
    absl::flat_hash_map<absl::string_view, upb::FieldDefPtr>;

// Returns field name by resolving naming conflicts across
// proto field names (such as clear_ prefixes).
std::string ResolveFieldName(upb::FieldDefPtr field,
                             const NameToFieldDefMap& field_names);

// Returns field map by name to use for conflict checks.
NameToFieldDefMap CreateFieldNameMap(upb::MessageDefPtr message);

// Private array getter name postfix for repeated fields.
ABSL_CONST_INIT extern const absl::string_view kRepeatedFieldArrayGetterPostfix;
ABSL_CONST_INIT extern const absl::string_view
    kRepeatedFieldMutableArrayGetterPostfix;

// Private getter name postfix for map fields.
ABSL_CONST_INIT extern const absl::string_view kMapGetterPostfix;
ABSL_CONST_INIT extern const absl::string_view kMutableMapGetterPostfix;

}  // namespace generator
}  // namespace upb

#endif  // UPB_PROTOS_GENERATOR_NAMES_H
