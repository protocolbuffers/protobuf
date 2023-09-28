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

}  // namespace generator
}  // namespace upb

#endif  // UPB_PROTOS_GENERATOR_NAMES_H
