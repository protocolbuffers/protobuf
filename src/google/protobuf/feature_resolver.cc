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

#define RETURN_IF_ERROR(expr)                              \
  do {                                                     \
    const absl::Status _status = (expr);                   \
    if (ABSL_PREDICT_FALSE(!_status.ok())) return _status; \
  } while (0)

namespace google {
namespace protobuf {
namespace {

template <typename... Args>
absl::Status Error(Args... args) {
  return absl::FailedPreconditionError(absl::StrCat(args...));
}

absl::Status ValidateFeatureSupport(const FieldOptions::FeatureSupport& support,
                                    absl::string_view full_name) {
  if (support.has_edition_deprecated()) {
    if (support.edition_deprecated() < support.edition_introduced()) {
      return Error("Feature ", full_name,
                   " was deprecated before it was introduced.");
    }
    if (!support.has_deprecation_warning()) {
      return Error(
          "Feature ", full_name,
          " is deprecated but does not specify a deprecation warning.");
    }
  }
  if (!support.has_edition_deprecated() && support.has_deprecation_warning()) {
    return Error("Feature ", full_name,
                 " specifies a deprecation warning but is not marked "
                 "deprecated in any edition.");
  }
  if (support.has_edition_removed()) {
    if (support.edition_deprecated() >= support.edition_removed()) {
      return Error("Feature ", full_name,
                   " was deprecated after it was removed.");
    }
    if (support.edition_removed() < support.edition_introduced()) {
      return Error("Feature ", full_name,
                   " was removed before it was introduced.");
    }
  }

  return absl::OkStatus();
}

absl::Status ValidateFieldFeatureSupport(const FieldDescriptor& field) {
  if (!field.options().has_feature_support()) {
    return Error("Feature field ", field.full_name(),
                 " has no feature support specified.");
  }

  const FieldOptions::FeatureSupport& support =
      field.options().feature_support();
  if (!support.has_edition_introduced()) {
    return Error("Feature field ", field.full_name(),
                 " does not specify the edition it was introduced in.");
  }
  RETURN_IF_ERROR(ValidateFeatureSupport(support, field.full_name()));

  // Validate edition defaults specification wrt support windows.
  for (const auto& d : field.options().edition_defaults()) {
    if (d.edition() < Edition::EDITION_2023) {
      // Allow defaults to be specified in proto2/proto3, predating
      // editions.
      continue;
    }
    if (d.edition() < support.edition_introduced()) {
      return Error("Feature field ", field.full_name(),
                   " has a default specified for edition ", d.edition(),
                   ", before it was introduced.");
    }
    if (support.has_edition_removed() &&
        d.edition() > support.edition_removed()) {
      return Error("Feature field ", field.full_name(),
                   " has a default specified for edition ", d.edition(),
                   ", after it was removed.");
    }
  }

  return absl::OkStatus();
}

absl::Status ValidateValueFeatureSupport(
    const FieldOptions::FeatureSupport& parent,
    const EnumValueDescriptor& value, absl::string_view field_name) {
  if (!value.options().has_feature_support()) {
    // We allow missing support windows on feature values, and they'll inherit
    // from the feature spec.
    return absl::OkStatus();
  }

  FieldOptions::FeatureSupport support = parent;
  support.MergeFrom(value.options().feature_support());
  RETURN_IF_ERROR(ValidateFeatureSupport(support, value.full_name()));

  // Make sure the value doesn't expand any bounds.
  if (support.edition_introduced() < parent.edition_introduced()) {
    return Error("Feature value ", value.full_name(),
                 " was introduced before feature ", field_name, " was.");
  }
  if (parent.has_edition_removed() &&
      support.edition_removed() > parent.edition_removed()) {
    return Error("Feature value ", value.full_name(),
                 " was removed after feature ", field_name, " was.");
  }
  if (parent.has_edition_deprecated() &&
      support.edition_deprecated() > parent.edition_deprecated()) {
    return Error("Feature value ", value.full_name(),
                 " was deprecated after feature ", field_name, " was.");
  }

  return absl::OkStatus();
}

absl::Status ValidateValuesFeatureSupport(const FieldDescriptor& field) {
  // This only applies to enum features.
  ABSL_CHECK(field.enum_type() != nullptr);

  const FieldOptions::FeatureSupport& parent =
      field.options().feature_support();

  for (int i = 0; i < field.enum_type()->value_count(); ++i) {
    const EnumValueDescriptor& value = *field.enum_type()->value(i);
    RETURN_IF_ERROR(
        ValidateValueFeatureSupport(parent, value, field.full_name()));
  }

  return absl::OkStatus();
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
    if (field.type() != FieldDescriptor::TYPE_ENUM &&
        field.type() != FieldDescriptor::TYPE_BOOL) {
      return Error("Feature field ", field.full_name(),
                   " is not an enum or boolean.");
    }
    if (field.options().targets().empty()) {
      return Error("Feature field ", field.full_name(),
                   " has no target specified.");
    }

    bool has_legacy_default = false;
    for (const auto& d : field.options().edition_defaults()) {
      if (d.edition() == Edition::EDITION_LEGACY) {
        has_legacy_default = true;
        continue;
      }
    }
    if (!has_legacy_default) {
      return Error("Feature field ", field.full_name(),
                   " has no default specified for EDITION_LEGACY, before it "
                   "was introduced.");
    }

    RETURN_IF_ERROR(ValidateFieldFeatureSupport(field));
    if (field.enum_type() != nullptr) {
      RETURN_IF_ERROR(ValidateValuesFeatureSupport(field));
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

void MaybeInsertEdition(Edition edition, Edition maximum_edition,
                        absl::btree_set<Edition>& editions) {
  if (edition <= maximum_edition) {
    editions.insert(edition);
  }
}

// This collects all of the editions that are relevant to any features defined
// in a message descriptor.  We only need to consider editions where something
// has changed.
void CollectEditions(const Descriptor& descriptor, Edition maximum_edition,
                     absl::btree_set<Edition>& editions) {
  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldOptions& options = descriptor.field(i)->options();
    // Editions where a new feature is introduced should be captured.
    MaybeInsertEdition(options.feature_support().edition_introduced(),
                       maximum_edition, editions);

    // Editions where a feature is removed should be captured.
    if (options.feature_support().has_edition_removed()) {
      MaybeInsertEdition(options.feature_support().edition_removed(),
                         maximum_edition, editions);
    }

    // Any edition where a default value changes should be captured.
    for (const auto& def : options.edition_defaults()) {
      MaybeInsertEdition(def.edition(), maximum_edition, editions);
    }
  }
}

absl::Status FillDefaults(Edition edition, Message& fixed,
                          Message& overridable) {
  const Descriptor& descriptor = *fixed.GetDescriptor();
  ABSL_CHECK(&descriptor == overridable.GetDescriptor());

  auto comparator = [](const FieldOptions::EditionDefault& a,
                       const FieldOptions::EditionDefault& b) {
    return a.edition() < b.edition();
  };
  FieldOptions::EditionDefault edition_lookup;
  edition_lookup.set_edition(edition);

  for (int i = 0; i < descriptor.field_count(); ++i) {
    const FieldDescriptor& field = *descriptor.field(i);
    Message* msg = &overridable;
    if (field.options().has_feature_support()) {
      if ((field.options().feature_support().has_edition_introduced() &&
           edition < field.options().feature_support().edition_introduced()) ||
          (field.options().feature_support().has_edition_removed() &&
           edition >= field.options().feature_support().edition_removed())) {
        msg = &fixed;
      }
    }

    msg->GetReflection()->ClearField(msg, &field);
    ABSL_CHECK(!field.is_repeated());
    ABSL_CHECK(field.cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE);

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

    const std::string& def = std::prev(first_nonmatch)->value();
    if (!TextFormat::ParseFieldValueFromString(def, &field, msg)) {
      return Error("Parsing error in edition_defaults for feature field ",
                   field.full_name(), ". Could not parse: ", def);
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
  CHECK_ENUM_FEATURE(enforce_naming_style, EnforceNamingStyle,
                     ENFORCE_NAMING_STYLE)
  CHECK_ENUM_FEATURE(default_symbol_visibility,
                     VisibilityFeature::DefaultSymbolVisibility,
                     VisibilityFeature::DEFAULT_SYMBOL_VISIBILITY)

#undef CHECK_ENUM_FEATURE

  return absl::OkStatus();
}

void ValidateSingleFeatureLifetimes(
    Edition edition, absl::string_view full_name,
    const FieldOptions::FeatureSupport& support,
    FeatureResolver::ValidationResults& results) {
  // Skip fields that don't have feature support specified.
  if (&support == &FieldOptions::FeatureSupport::default_instance()) return;

  if (edition < support.edition_introduced()) {
    results.errors.emplace_back(
        absl::StrCat("Feature ", full_name, " wasn't introduced until edition ",
                     support.edition_introduced(),
                     " and can't be used in edition ", edition));
  }
  if (support.has_edition_removed() && edition >= support.edition_removed()) {
    results.errors.emplace_back(absl::StrCat(
        "Feature ", full_name, " has been removed in edition ",
        support.edition_removed(), " and can't be used in edition ", edition));
  } else if (support.has_edition_deprecated() &&
             edition >= support.edition_deprecated()) {
    results.warnings.emplace_back(absl::StrCat(
        "Feature ", full_name, " has been deprecated in edition ",
        support.edition_deprecated(), ": ", support.deprecation_warning()));
  }
}

void ValidateFeatureLifetimesImpl(Edition edition, const Message& message,
                                  FeatureResolver::ValidationResults& results) {
  std::vector<const FieldDescriptor*> fields;
  message.GetReflection()->ListFields(message, &fields);
  for (const FieldDescriptor* field : fields) {
    // Recurse into message extension.
    if (field->is_extension() &&
        field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      ValidateFeatureLifetimesImpl(
          edition, message.GetReflection()->GetMessage(message, field),
          results);
      continue;
    }

    if (field->enum_type() != nullptr) {
      int number = message.GetReflection()->GetEnumValue(message, field);
      auto value = field->enum_type()->FindValueByNumber(number);
      if (value == nullptr) {
        results.errors.emplace_back(absl::StrCat(
            "Feature ", field->full_name(), " has no known value ", number));
        continue;
      }
      ValidateSingleFeatureLifetimes(edition, value->full_name(),
                                     value->options().feature_support(),
                                     results);
    }

    ValidateSingleFeatureLifetimes(edition, field->full_name(),
                                   field->options().feature_support(), results);
  }
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
  // Sanity check validation conditions above.
  ABSL_CHECK(!editions.empty());
  if (*editions.begin() != EDITION_LEGACY) {
    return Error("Minimum edition ", *editions.begin(),
                 " is not EDITION_LEGACY");
  }

  if (*editions.begin() > minimum_edition) {
    return Error("Minimum edition ", minimum_edition,
                 " is earlier than the oldest valid edition ",
                 *editions.begin());
  }

  // Fill the default spec.
  FeatureSetDefaults defaults;
  defaults.set_minimum_edition(minimum_edition);
  defaults.set_maximum_edition(maximum_edition);
  auto message_factory = absl::make_unique<DynamicMessageFactory>();
  for (const auto& edition : editions) {
    auto fixed_defaults_dynamic =
        absl::WrapUnique(message_factory->GetPrototype(feature_set)->New());
    auto overridable_defaults_dynamic =
        absl::WrapUnique(message_factory->GetPrototype(feature_set)->New());
    RETURN_IF_ERROR(FillDefaults(edition, *fixed_defaults_dynamic,
                                 *overridable_defaults_dynamic));
    for (const auto* extension : extensions) {
      RETURN_IF_ERROR(FillDefaults(
          edition,
          *fixed_defaults_dynamic->GetReflection()->MutableMessage(
              fixed_defaults_dynamic.get(), extension),
          *overridable_defaults_dynamic->GetReflection()->MutableMessage(
              overridable_defaults_dynamic.get(), extension)));
    }
    auto* edition_defaults = defaults.mutable_defaults()->Add();
    edition_defaults->set_edition(edition);
    edition_defaults->mutable_fixed_features()->MergeFromString(
        fixed_defaults_dynamic->SerializeAsString());
    edition_defaults->mutable_overridable_features()->MergeFromString(
        overridable_defaults_dynamic->SerializeAsString());
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
    FeatureSet features = edition_default.fixed_features();
    features.MergeFrom(edition_default.overridable_features());
    RETURN_IF_ERROR(ValidateMergedFeatures(features));

    prev_edition = edition_default.edition();
  }

  auto features =
      internal::GetEditionFeatureSetDefaults(edition, compiled_defaults);
  RETURN_IF_ERROR(features.status());
  return FeatureResolver(std::move(features.value()));
}

absl::StatusOr<FeatureSet> FeatureResolver::MergeFeatures(
    const FeatureSet& merged_parent, const FeatureSet& unmerged_child) const {
  FeatureSet merged = defaults_;
  merged.MergeFrom(merged_parent);
  merged.MergeFrom(unmerged_child);

  RETURN_IF_ERROR(ValidateMergedFeatures(merged));

  return merged;
}

FeatureResolver::ValidationResults FeatureResolver::ValidateFeatureLifetimes(
    Edition edition, const FeatureSet& features,
    const Descriptor* pool_descriptor) {
  const Message* pool_features = nullptr;
  DynamicMessageFactory factory;
  std::unique_ptr<Message> features_storage;
  if (pool_descriptor == nullptr) {
    // The FeatureSet descriptor can be null if no custom extensions are defined
    // in any transitive dependency.  In this case, we can just use the
    // generated pool for validation, since there wouldn't be any feature
    // extensions defined anyway.
    pool_features = &features;
  } else {
    // Move the features back to the current pool so that we can reflect on any
    // extensions.
    features_storage =
        absl::WrapUnique(factory.GetPrototype(pool_descriptor)->New());
    features_storage->ParseFromString(features.SerializeAsString());
    pool_features = features_storage.get();
  }
  ABSL_CHECK(pool_features != nullptr);

  ValidationResults results;
  ValidateFeatureLifetimesImpl(edition, *pool_features, results);
  return results;
}

namespace internal {
absl::StatusOr<FeatureSet> GetEditionFeatureSetDefaults(
    Edition edition, const FeatureSetDefaults& defaults) {
  // Select the matching edition defaults.
  auto comparator = [](const auto& a, const auto& b) {
    return a.edition() < b.edition();
  };
  FeatureSetDefaults::FeatureSetEditionDefault search;
  search.set_edition(edition);
  auto first_nonmatch =
      absl::c_upper_bound(defaults.defaults(), search, comparator);
  if (first_nonmatch == defaults.defaults().begin()) {
    return Error("No valid default found for edition ", edition);
  }
  FeatureSet features = std::prev(first_nonmatch)->fixed_features();
  features.MergeFrom(std::prev(first_nonmatch)->overridable_features());
  return features;
}
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
