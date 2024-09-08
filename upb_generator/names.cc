// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb_generator/names.h"

#include <array>
#include <cstdint>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace upb {
namespace generator {

namespace {

struct Prefix {
  absl::string_view name;
  uint32_t conflict_set;
};

constexpr uint32_t kAnyField = UINT32_MAX;

// Prefixes used by C code generator for field access.
static constexpr std::array<Prefix, 6> kPrefixes{
    Prefix{"clear_", kContainerField | kStringField},
    Prefix{"delete_", kContainerField},
    Prefix{"add_", kContainerField},
    Prefix{"resize_", kContainerField},
    Prefix{"set_", kAnyField},
    Prefix{"has_", kAnyField},
};

bool HasConflict(absl::string_view name,
                 const absl::flat_hash_map<std::string, FieldClass>& fields) {
  for (const auto& prefix : kPrefixes) {
    if (!absl::StartsWith(name, prefix.name)) continue;
    auto match = fields.find(name.substr(prefix.name.size()));
    if (match == fields.end()) continue;
    if (prefix.conflict_set & match->second) return true;
  }
  return false;
}

}  // namespace

NameMangler::NameMangler(
    const absl::flat_hash_map<std::string, FieldClass>& fields) {
  for (const auto& pair : fields) {
    const std::string& field_name = pair.first;
    if (HasConflict(field_name, fields)) {
      names_.emplace(field_name, absl::StrCat(field_name, "_"));
    }
  }
}

}  // namespace generator
}  // namespace upb
