// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/feature_resolver.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_set.h"
#include "absl/log/absl_check.h"
#include "absl/memory/memory.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "google/protobuf/cpp_features.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/dynamic_message.h"
#include "google/protobuf/message.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/text_format.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

#define RETURN_IF_ERROR(expr)                                  \
  do {                                                         \
    const absl::Status _status = (expr);                       \
    if (PROTOBUF_PREDICT_FALSE(!_status.ok())) return _status; \
  } while (0)

namespace google {
namespace protobuf {
namespace {

template <typename... Args>
absl::Status Error(Args... args) {
  return absl::FailedPreconditionError(absl::StrCat(args...));
}

absl::Status ValidateDescriptor(const Descriptor& descriptor) {
  if (descriptor.oneof_decl_count() > 0) {
    return Error("Type ", descriptor.full_name(),
                 " contains unsupported oneof feature fields.");
  }
  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);

    if (field.is_required()) {
      return Error("Feature field ", field.full_name(),
                   " is an unsupported required field.");
    }
    if (field.is_repeated()) {
      return Error("Feature field ", field.full_name(),
                   " is an unsupported repeated field.");
    }
    if (field.options().targets().empty()) {
      return Error("Feature field ", field.full_name(),
                   " has no target specified.");
    }
  }

  return absl::OkStatus();
}

absl::Status ValidateExtension(const Descriptor& feature_set,
                               const FieldDescriptor* extension) {
  if (extension == nullptr) {
    return Error("Unknown extension of ", feature_set.full_name(), ".");
  }

  if (extension->containing_type() != &feature_set) {
    return Error("Extension ", extension->full_name(),
                 " is not an extension of ", feature_set.full_name(), ".");
  }

  if (extension->message_type() == nullptr) {
    return Error("FeatureSet extension ", extension->full_name(),
                 " is not of message type.  Feature extensions should "
                 "always use messages to allow for evolution.");
  }

  if (extension->is_repeated()) {
    return Error(
        "Only singular features extensions are supported.  Found "
        "repeated extension ",
        extension->full_name());
  }

  if (extension->message_type()->extension_count() > 0 ||
      extension->message_type()->extension_range_count() > 0) {
    return Error("Nested extensions in feature extension ",
                 extension->full_name(), " are not supported.");
  }

  return absl::OkStatus();
}

void CollectEditions(const Descriptor& descriptor, Edition maximum_edition,
                     absl::btree_set<Edition>& editions) {
  for (int i = 0; i < descriptor.field_count(); ++i) {
    for (const auto& def : descriptor.field(i)->options().edition_defaults()) {
      if (maximum_edition < def.edition()) continue;
      editions.insert(def.edition());
    }
  }
}

absl::Status FillDefaults(Edition edition, Message& msg) {
  const Descriptor& descriptor = *msg.GetDescriptor();

  auto comparator = [](const FieldOptions::EditionDefault& a,
                       const FieldOptions::EditionDefault& b) {
    return a.edition() < b.edition();
  };
  FieldOptions::EditionDefault edition_lookup;
  edition_lookup.set_edition(edition);

  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);

    msg.GetReflection()->ClearField(&msg, &field);
    ABSL_CHECK(!field.is_repeated());

    std::vector<FieldOptions::EditionDefault> defaults{
        field.options().edition_defaults().begin(),
        field.options().edition_defaults().end()};
    absl::c_sort(defaults, comparator);
    auto first_nonmatch =
        absl::c_upper_bound(defaults, edition_lookup, comparator);
    if (first_nonmatch == defaults.begin()) {
      return Error("No valid default found for edition ", edition,
                   " in feature field ", field.full_name());
    }

    if (field.cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      for (auto it = defaults.begin(); it != first_nonmatch; ++it) {
        if (!TextFormat::MergeFromString(
                it->value(),
                msg.GetReflection()->MutableMessage(&msg, &field))) {
          return Error("Parsing error in edition_defaults for feature field ",
                       field.full_name(), ". Could not parse: ", it->value());
        }
      }
    } else {
      const std::string& def = std::prev(first_nonmatch)->value();
      if (!TextFormat::ParseFieldValueFromString(def, &field, &msg)) {
        return Error("Parsing error in edition_defaults for feature field ",
                     field.full_name(), ". Could not parse: ", def);
      }
    }
  }

  return absl::OkStatus();
}

absl::Status ValidateMergedFeatures(const FeatureSet& features) {
// Avoid using reflection here because this is called early in the descriptor
// builds.  Instead, a reflection-based test will be used to keep this in sync
// with descriptor.proto.  These checks should be run on every global feature
// in FeatureSet.
#define CHECK_ENUM_FEATURE(FIELD, CAMELCASE, UPPERCASE)               \
  if (!FeatureSet::CAMELCASE##_IsValid(features.FIELD()) ||           \
      features.FIELD() == FeatureSet::UPPERCASE##_UNKNOWN) {          \
    return Error("Feature field `" #FIELD                             \
                 "` must resolve to a known value, found " #UPPERCASE \
                 "_UNKNOWN");                                         \
  }

  CHECK_ENUM_FEATURE(field_presence, FieldPresence, FIELD_PRESENCE)
  CHECK_ENUM_FEATURE(enum_type, EnumType, ENUM_TYPE)
  CHECK_ENUM_FEATURE(repeated_field_encoding, RepeatedFieldEncoding,
                     REPEATED_FIELD_ENCODING)
  CHECK_ENUM_FEATURE(utf8_validation, Utf8Validation, UTF8_VALIDATION)
  CHECK_ENUM_FEATURE(message_encoding, MessageEncoding, MESSAGE_ENCODING)
  CHECK_ENUM_FEATURE(json_format, JsonFormat, JSON_FORMAT)

#undef CHECK_ENUM_FEATURE

  return absl::OkStatus();
}

}  // namespace

absl::StatusOr<FeatureSetDefaults> FeatureResolver::CompileDefaults(
    const Descriptor* feature_set,
    absl::Span<const FieldDescriptor* const> extensions,
    Edition minimum_edition, Edition maximum_edition) {
  if (minimum_edition > maximum_edition) {
    return Error("Invalid edition range, edition ", minimum_edition,
                 " is newer than edition ", maximum_edition);
  }
  // Find and validate the FeatureSet in the pool.
  if (feature_set == nullptr) {
    return Error(
        "Unable to find definition of google.protobuf.FeatureSet in descriptor pool.");
  }
  RETURN_IF_ERROR(ValidateDescriptor(*feature_set));

  // Collect and validate all the FeatureSet extensions.
  for (const auto* extension : extensions) {
    RETURN_IF_ERROR(ValidateExtension(*feature_set, extension));
    RETURN_IF_ERROR(ValidateDescriptor(*extension->message_type()));
  }

  // Collect all the editions with unique defaults.
  absl::btree_set<Edition> editions;
  CollectEditions(*feature_set, maximum_edition, editions);
  for (const auto* extension : extensions) {
    CollectEditions(*extension->message_type(), maximum_edition, editions);
  }
  if (editions.empty() || *editions.begin() > minimum_edition) {
    // Always insert the minimum edition to make sure the full range is covered
    // in valid defaults.
    editions.insert(minimum_edition);
  }

  // Fill the default spec.
  FeatureSetDefaults defaults;
  defaults.set_minimum_edition(minimum_edition);
  defaults.set_maximum_edition(maximum_edition);
  auto message_factory = absl::make_unique<DynamicMessageFactory>();
  for (const auto& edition : editions) {
    auto defaults_dynamic =
        absl::WrapUnique(message_factory->GetPrototype(feature_set)->New());
    RETURN_IF_ERROR(FillDefaults(edition, *defaults_dynamic));
    for (const auto* extension : extensions) {
      RETURN_IF_ERROR(FillDefaults(
          edition, *defaults_dynamic->GetReflection()->MutableMessage(
                       defaults_dynamic.get(), extension)));
    }
    auto* edition_defaults = defaults.mutable_defaults()->Add();
    edition_defaults->set_edition(edition);
    edition_defaults->mutable_features()->MergeFromString(
        defaults_dynamic->SerializeAsString());
  }
  return defaults;
}

absl::StatusOr<FeatureResolver> FeatureResolver::Create(
    Edition edition, const FeatureSetDefaults& compiled_defaults) {
  if (edition < compiled_defaults.minimum_edition()) {
    return Error("Edition ", edition,
                 " is earlier than the minimum supported edition ",
                 compiled_defaults.minimum_edition());
  }
  if (compiled_defaults.maximum_edition() < edition) {
    return Error("Edition ", edition,
                 " is later than the maximum supported edition ",
                 compiled_defaults.maximum_edition());
  }

  // Validate compiled defaults.
  Edition prev_edition = EDITION_UNKNOWN;
  for (const auto& edition_default : compiled_defaults.defaults()) {
    if (edition_default.edition() == EDITION_UNKNOWN) {
      return Error("Invalid edition ", edition_default.edition(),
                   " specified.");
    }
    if (prev_edition != EDITION_UNKNOWN) {
      if (edition_default.edition() <= prev_edition) {
        return Error(
            "Feature set defaults are not strictly increasing.  Edition ",
            prev_edition, " is greater than or equal to edition ",
            edition_default.edition(), ".");
      }
    }
    RETURN_IF_ERROR(ValidateMergedFeatures(edition_default.features()));

    prev_edition = edition_default.edition();
  }

  // Select the matching edition defaults.
  auto comparator = [](const auto& a, const auto& b) {
    return a.edition() < b.edition();
  };
  FeatureSetDefaults::FeatureSetEditionDefault search;
  search.set_edition(edition);
  auto first_nonmatch =
      absl::c_upper_bound(compiled_defaults.defaults(), search, comparator);
  if (first_nonmatch == compiled_defaults.defaults().begin()) {
    return Error("No valid default found for edition ", edition);
  }

  return FeatureResolver(std::prev(first_nonmatch)->features());
}

absl::StatusOr<FeatureSet> FeatureResolver::MergeFeatures(
    const FeatureSet& merged_parent, const FeatureSet& unmerged_child) const {
  FeatureSet merged = defaults_;
  merged.MergeFrom(merged_parent);
  merged.MergeFrom(unmerged_child);

  RETURN_IF_ERROR(ValidateMergedFeatures(merged));

  return merged;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
