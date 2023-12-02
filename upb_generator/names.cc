// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/names.h"

#include <string>

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "upb/reflection/def.hpp"

namespace upb {
namespace generator {

namespace protobuf = ::google::protobuf;

// Prefixes used by C code generator for field access.
static constexpr absl::string_view kClearMethodPrefix = "clear_";
static constexpr absl::string_view kSetMethodPrefix = "set_";
static constexpr absl::string_view kHasMethodPrefix = "has_";
static constexpr absl::string_view kDeleteMethodPrefix = "delete_";
static constexpr absl::string_view kAddToRepeatedMethodPrefix = "add_";
static constexpr absl::string_view kResizeArrayMethodPrefix = "resize_";

ABSL_CONST_INIT const absl::string_view kRepeatedFieldArrayGetterPostfix =
    "upb_array";
ABSL_CONST_INIT const absl::string_view
    kRepeatedFieldMutableArrayGetterPostfix = "mutable_upb_array";

ABSL_CONST_INIT const absl::string_view kMapGetterPostfix = "upb_map";
ABSL_CONST_INIT const absl::string_view kMutableMapGetterPostfix =
    "mutable_upb_map";

// List of generated accessor prefixes to check against.
// Example:
//     optional repeated string phase = 236;
//     optional bool clear_phase = 237;
static constexpr absl::string_view kAccessorPrefixes[] = {
    kClearMethodPrefix,       kDeleteMethodPrefix, kAddToRepeatedMethodPrefix,
    kResizeArrayMethodPrefix, kSetMethodPrefix,    kHasMethodPrefix};

std::string ResolveFieldName(const protobuf::FieldDescriptor* field,
                             const NameToFieldDescriptorMap& field_names) {
  absl::string_view field_name = field->name();
  for (const auto prefix : kAccessorPrefixes) {
    // If field name starts with a prefix such as clear_ and the proto
    // contains a field name with trailing end, depending on type of field
    // (repeated, map, message) we have a conflict to resolve.
    if (absl::StartsWith(field_name, prefix)) {
      auto match = field_names.find(field_name.substr(prefix.size()));
      if (match != field_names.end()) {
        const auto* candidate = match->second;
        if (candidate->is_repeated() || candidate->is_map() ||
            (candidate->cpp_type() ==
                 protobuf::FieldDescriptor::CPPTYPE_STRING &&
             prefix == kClearMethodPrefix) ||
            prefix == kSetMethodPrefix || prefix == kHasMethodPrefix) {
          return absl::StrCat(field_name, "_");
        }
      }
    }
  }
  return std::string(field_name);
}

// Returns field map by name to use for conflict checks.
NameToFieldDescriptorMap CreateFieldNameMap(
    const protobuf::Descriptor* message) {
  NameToFieldDescriptorMap field_names;
  for (int i = 0; i < message->field_count(); i++) {
    const protobuf::FieldDescriptor* field = message->field(i);
    field_names.emplace(field->name(), field);
  }
  return field_names;
}

NameToFieldDefMap CreateFieldNameMap(upb::MessageDefPtr message) {
  NameToFieldDefMap field_names;
  field_names.reserve(message.field_count());
  for (const auto& field : message.fields()) {
    field_names.emplace(field.name(), field);
  }
  return field_names;
}

std::string ResolveFieldName(upb::FieldDefPtr field,
                             const NameToFieldDefMap& field_names) {
  absl::string_view field_name(field.name());
  for (absl::string_view prefix : kAccessorPrefixes) {
    // If field name starts with a prefix such as clear_ and the proto
    // contains a field name with trailing end, depending on type of field
    // (repeated, map, message) we have a conflict to resolve.
    if (absl::StartsWith(field_name, prefix)) {
      auto match = field_names.find(field_name.substr(prefix.size()));
      if (match != field_names.end()) {
        const auto candidate = match->second;
        if (candidate.IsSequence() || candidate.IsMap() ||
            (candidate.ctype() == kUpb_CType_String &&
             prefix == kClearMethodPrefix) ||
            prefix == kSetMethodPrefix || prefix == kHasMethodPrefix) {
          return absl::StrCat(field_name, "_");
        }
      }
    }
  }
  return std::string(field_name);
}

}  // namespace generator
}  // namespace upb
