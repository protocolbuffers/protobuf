// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google LLC nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
