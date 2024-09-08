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

namespace upb {
namespace generator {

enum FieldClass {
  kStringField = 1 << 0,
  kContainerField = 1 << 1,
  kOtherField = 1 << 2,
};

class NameMangler {
 public:
  explicit NameMangler(
      const absl::flat_hash_map<std::string, FieldClass>& fields);

  std::string ResolveFieldName(absl::string_view name) const {
    auto it = names_.find(name);
    return it == names_.end() ? std::string(name) : it->second;
  }

 private:
  // Maps field_name -> mangled_name.  If a field name is not in the map, it
  // is not mangled.
  absl::flat_hash_map<std::string, std::string> names_;
};

// Here we provide functions for building field lists from both C++ and upb
// reflection.  They are templated so as to not actually introduce dependencies
// on either C++ or upb.

template <class T>
absl::flat_hash_map<std::string, FieldClass> GetCppFields(const T* descriptor) {
  absl::flat_hash_map<std::string, FieldClass> fields;
  for (int i = 0; i < descriptor->field_count(); ++i) {
    const auto* field = descriptor->field(i);
    if (field->is_repeated() || field->is_map()) {
      fields.emplace(field->name(), kContainerField);
    } else if (field->cpp_type() == field->CPPTYPE_STRING) {
      fields.emplace(field->name(), kStringField);
    } else {
      fields.emplace(field->name(), kOtherField);
    }
  }
  return fields;
}

template <class T>
absl::flat_hash_map<std::string, FieldClass> GetUpbFields(const T& msg_def) {
  absl::flat_hash_map<std::string, FieldClass> fields;
  for (const auto field : msg_def.fields()) {
    if (field.IsSequence() || field.IsMap()) {
      fields.emplace(field.name(), kContainerField);
    } else if (field.ctype() == decltype(field)::CType::kUpb_CType_String) {
      fields.emplace(field.name(), kStringField);
    } else {
      fields.emplace(field.name(), kOtherField);
    }
  }
  return fields;
}

ABSL_CONST_INIT const absl::string_view kRepeatedFieldArrayGetterPostfix =
    "upb_array";
ABSL_CONST_INIT const absl::string_view
    kRepeatedFieldMutableArrayGetterPostfix = "mutable_upb_array";

ABSL_CONST_INIT const absl::string_view kMapGetterPostfix = "upb_map";
ABSL_CONST_INIT const absl::string_view kMutableMapGetterPostfix =
    "mutable_upb_map";

}  // namespace generator
}  // namespace upb

#endif  // UPB_PROTOS_GENERATOR_NAMES_H
