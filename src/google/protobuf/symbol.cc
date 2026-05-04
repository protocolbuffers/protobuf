// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/symbol.h"

#include <string>
#include <utility>

#include "absl/log/absl_check.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/port.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

const FileDescriptor* Symbol::GetFile() const {
  switch (type()) {
    case MESSAGE:
      return descriptor()->file();
    case FIELD:
      return field_descriptor()->file();
    case ONEOF:
      return oneof_descriptor()->containing_type()->file();
    case ENUM:
      return enum_descriptor()->file();
    case ENUM_VALUE:
      return enum_value_descriptor()->type()->file();
    case SERVICE:
      return service_descriptor()->file();
    case METHOD:
      return method_descriptor()->service()->file();
    case FULL_PACKAGE:
      return file_descriptor();
    case SUB_PACKAGE:
      return sub_package_file_descriptor()->file;
    default:
      return nullptr;
  }
}

absl::string_view Symbol::full_name() const {
  switch (type()) {
    case MESSAGE:
      return descriptor()->full_name();
    case FIELD:
      return field_descriptor()->full_name();
    case ONEOF:
      return oneof_descriptor()->full_name();
    case ENUM:
      return enum_descriptor()->full_name();
    case ENUM_VALUE:
      return enum_value_descriptor()->full_name();
    case SERVICE:
      return service_descriptor()->full_name();
    case METHOD:
      return method_descriptor()->full_name();
    case FULL_PACKAGE:
      return file_descriptor()->package();
    case SUB_PACKAGE:
      return sub_package_file_descriptor()->file->package().substr(
          0, sub_package_file_descriptor()->name_size);
    default:
      ABSL_CHECK(false);
  }
  return "";
}

std::pair<const void*, absl::string_view> Symbol::parent_name_key() const {
  const auto or_file = [&](const void* p) { return p ? p : GetFile(); };
  switch (type()) {
    case MESSAGE:
      return {or_file(descriptor()->containing_type()), descriptor()->name()};
    case FIELD: {
      auto* field = field_descriptor();
      return {or_file(field->is_extension() ? field->extension_scope()
                                            : field->containing_type()),
              field->name()};
    }
    case ONEOF:
      return {oneof_descriptor()->containing_type(),
              oneof_descriptor()->name()};
    case ENUM:
      return {or_file(enum_descriptor()->containing_type()),
              enum_descriptor()->name()};
    case ENUM_VALUE:
      return {or_file(enum_value_descriptor()->type()->containing_type()),
              enum_value_descriptor()->name()};
    case ENUM_VALUE_OTHER_PARENT:
      return {enum_value_descriptor()->type(), enum_value_descriptor()->name()};
    case SERVICE:
      return {GetFile(), service_descriptor()->name()};
    case METHOD:
      return {method_descriptor()->service(), method_descriptor()->name()};
    default:
      ABSL_CHECK(false);
  }
  return {};
}

const FeatureSet& Symbol::features() const {
  switch (type()) {
    case MESSAGE:
      return descriptor()->features();
    case FIELD:
      return field_descriptor()->features();
    case ONEOF:
      return oneof_descriptor()->features();
    case ENUM:
      return enum_descriptor()->features();
    case ENUM_VALUE:
      return enum_value_descriptor()->features();
    case SERVICE:
      return service_descriptor()->features();
    case METHOD:
      return method_descriptor()->features();
    case FULL_PACKAGE:
      return file_descriptor()->features();
    case SUB_PACKAGE:
    default:
      internal::Unreachable();
  }
}

bool Symbol::is_placeholder() const {
  switch (type()) {
    case MESSAGE:
      return descriptor()->is_placeholder();
    case ENUM:
      return enum_descriptor()->is_placeholder();
    case FULL_PACKAGE:
      return file_descriptor()->is_placeholder();
    default:
      return false;
  }
}

SymbolVisibility Symbol::visibility_keyword() const {
  switch (type()) {
    case MESSAGE:
      return descriptor()->visibility_keyword();
    case ENUM:
      return enum_descriptor()->visibility_keyword();
    default:
      return SymbolVisibility::VISIBILITY_UNSET;
  }
}

bool Symbol::IsNestedDefinition() const {
  switch (type()) {
    case MESSAGE:
      return descriptor()->containing_type() != nullptr;
    case ENUM:
      return enum_descriptor()->containing_type() != nullptr;
    case FIELD:  // For extension fields
      return field_descriptor()->containing_type() != nullptr;
    default:
      return false;
  }
}

SymbolVisibility Symbol::GetEffectiveVisibility() const {
  // Only Types have visibility
  if (!IsType()) {
    return SymbolVisibility::VISIBILITY_UNSET;
  }

  SymbolVisibility effective_visibility = visibility_keyword();

  // If our visibility is specifically set we can return that.  We'll validate
  // whether it's reasonable or not later.
  if (effective_visibility == SymbolVisibility::VISIBILITY_UNSET) {
    switch (features().default_symbol_visibility()) {
      case FeatureSet::VisibilityFeature::EXPORT_ALL:
        return SymbolVisibility::VISIBILITY_EXPORT;
      case FeatureSet::VisibilityFeature::EXPORT_TOP_LEVEL:
        return IsNestedDefinition() ? SymbolVisibility::VISIBILITY_LOCAL
                                    : SymbolVisibility::VISIBILITY_EXPORT;
      case FeatureSet::VisibilityFeature::LOCAL_ALL:
      case FeatureSet::VisibilityFeature::STRICT:
        return SymbolVisibility::VISIBILITY_LOCAL;

      // Unset shouldn't be possible from the compiler without there being an
      // error (recursive import, for example), but happens in unit
      // tests so assume it represents pre-edition 2024 defaults. In either
      // case we want to fail open.  We have a DCHECK here to make sure it can
      // fail in tests, but not released code.
      case FeatureSet::VisibilityFeature::DEFAULT_SYMBOL_VISIBILITY_UNKNOWN:
      default:
        ABSL_DCHECK(false);
        return SymbolVisibility::VISIBILITY_EXPORT;
    }
  }

  return effective_visibility;
}

/*
 * Calculate whether this symbol can be accessed from the given
 * FileDescriptor*.
 *
 * Returns true if the symbol is in the same file OR the symbol is `export`
 */
bool Symbol::IsVisibleFrom(FileDescriptor* other) const {
  if (other == nullptr || GetFile() == nullptr) {
    return false;
  }

  // Only Types (message/enum) have visibility.
  if (!IsType()) {
    return true;
  }

  // If we're dealing with a placeholder then just stop now, visibility can't
  // be determined and we have to rely on the proto-compiler previously having
  // checked the validity.
  if (is_placeholder()) {
    return true;
  }

  if (GetFile() == other) {
    return true;
  }

  SymbolVisibility effective_visibility = GetEffectiveVisibility();

  return effective_visibility == SymbolVisibility::VISIBILITY_EXPORT;
}

std::string Symbol::GetVisibilityError(FileDescriptor* other,
                                       absl::string_view usage) const {
  const absl::string_view file_path =
      GetFile() != nullptr ? GetFile()->name() : "unknown_file";
  const absl::string_view symbol_name = full_name();

  if (!IsType()) {
    return absl::StrCat(
        "Attempt to get a visibility error for a non-message/enum symbol \"",
        symbol_name, "\", defined in \"", file_path, "\"");
  }

  SymbolVisibility explicit_visibility = visibility_keyword();

  std::string reason =
      explicit_visibility == SymbolVisibility::VISIBILITY_LOCAL
          ? "It is explicitly marked 'local'"
          : absl::StrCat(
                "It defaulted to local from file-level 'option "
                "features.default_symbol_visibility = '",
                FeatureSet_VisibilityFeature_DefaultSymbolVisibility_Name(
                    features().default_symbol_visibility()),
                "';");

  return absl::StrCat("Symbol \"", symbol_name, "\", defined in \"", file_path,
                      "\" ", usage, " is ", "not visible from \"",
                      other->name(), "\". ", reason,
                      " and cannot be accessed outside its own file");
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
