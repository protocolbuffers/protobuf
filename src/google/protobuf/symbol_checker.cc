#include "google/protobuf/symbol_checker.h"

#include <string>
#include <vector>

#include "absl/strings/str_cat.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/descriptor_visitor.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {

std::string SymbolCheckerError::message() const {
  switch (type_) {
    case NESTED_MESSAGE_STRICT_VIOLATION:
      return absl::StrCat(
          "\"", symbol_name_,
          "\" is a nested message and cannot be `export` with STRICT "
          "default_symbol_visibility. It must be moved to top-level, "
          "ideally "
          "in its own file in order to be `export`.");
    case NESTED_ENUM_STRICT_VIOLATION:
      return absl::StrCat(
          "\"", symbol_name_,
          "\" is a nested enum and cannot be marked `export` with "
          "STRICT "
          "default_symbol_visibility. It must be moved to "
          "top-level, ideally "
          "in its own file in order to be `export`. For C++ "
          "namespacing of enums in a messages use: `local "
          "message <OuterNamespace> { export enum ",
          symbol_name_, " {...} reserved 1 to max; }`");
    default:
      return "";
  }
}

namespace {

// Populate VisibilityCheckerState for messages.
void VisitType(const Descriptor& message, const DescriptorProto& proto,
               internal::SymbolCheckerState* state) {
  if (message.containing_type() != nullptr) {
    state->nested_messages.push_back(
        internal::MessageDescriptorAndProto{&message, &proto});
  }
}

void VisitType(const EnumDescriptor& enm, const EnumDescriptorProto& proto,
               internal::SymbolCheckerState* state) {
  if (enm.containing_type() != nullptr) {
    if (SymbolChecker::IsNamespacedEnum(enm)) {
      state->namespaced_enums.push_back(
          internal::EnumDescriptorAndProto{&enm, &proto});
    } else {
      state->nested_enums.push_back(
          internal::EnumDescriptorAndProto{&enm, &proto});
    }
  }
}
void VisitType(const FileDescriptor& file, const FileDescriptorProto& proto,
               internal::SymbolCheckerState* state) {}

void VisitType(const FieldDescriptor& field, const FieldDescriptorProto& proto,
               internal::SymbolCheckerState* state) {}

void VisitType(const EnumValueDescriptor& value,
               const EnumValueDescriptorProto& proto,
               internal::SymbolCheckerState* state) {}

void VisitType(const OneofDescriptor& one, const OneofDescriptorProto& proto,
               internal::SymbolCheckerState* state) {}

void VisitType(const Descriptor::ExtensionRange&,
               const DescriptorProto::ExtensionRange& proto,
               internal::SymbolCheckerState* state) {}

void VisitType(const MethodDescriptor& method,
               const MethodDescriptorProto& proto,
               internal::SymbolCheckerState* state) {}

void VisitType(const ServiceDescriptor& service,
               const ServiceDescriptorProto& proto,
               internal::SymbolCheckerState* state) {}

}  // namespace

SymbolChecker::SymbolChecker(const FileDescriptor* file,
                             const FileDescriptorProto& proto)
    : initialized_(false), descriptor_(file), proto_(proto) {}

bool SymbolChecker::IsEnumNamespaceMessage(const Descriptor& container) {
  const FeatureSet::VisibilityFeature::DefaultSymbolVisibility
      default_visibility =
          container.file()->features().default_symbol_visibility();

  // Only allowed for top-level messages
  if (container.containing_type() != nullptr) {
    return false;
  }

  bool default_to_local =
      default_visibility == FeatureSet::VisibilityFeature::STRICT ||
      default_visibility == FeatureSet::VisibilityFeature::LOCAL_ALL;

  bool is_container_local =
      container.visibility_keyword() == SymbolVisibility::VISIBILITY_LOCAL ||
      (container.visibility_keyword() == SymbolVisibility::VISIBILITY_UNSET &&
       default_to_local);

  // the containing message must either be marked local, or unset with file
  // default making it local
  if (!is_container_local) {
    return false;
  }

  // We also require a single reserved range of 1 to max.
  if (container.reserved_range_count() != 1) {
    return false;
  }

  // Always not-null due to reserved_range_count check above.
  const Descriptor::ReservedRange* range = container.reserved_range(0);

  // The reserved range must be 1 to max.  Max is exclusive so results in
  // kMaxNumber + 1
  return range->start == 1 && range->end == (FieldDescriptor::kMaxNumber + 1);
}

bool SymbolChecker::IsNamespacedEnum(const EnumDescriptor& enm) {
  // If the enum is top-level, it definitely isn't namespaced.
  if (enm.containing_type() == nullptr) {
    return false;
  }

  const FeatureSet::VisibilityFeature::DefaultSymbolVisibility
      default_visibility = enm.file()->features().default_symbol_visibility();

  bool default_to_local =
      default_visibility == FeatureSet::VisibilityFeature::STRICT ||
      default_visibility == FeatureSet::VisibilityFeature::LOCAL_ALL;

  // This check only cares if the enum is explicitly marked export, the
  // containing message can be local or unset with the file default making it
  // local.
  bool is_exported =
      !default_to_local ||
      enm.visibility_keyword() == SymbolVisibility::VISIBILITY_EXPORT;
  return is_exported && IsEnumNamespaceMessage(*enm.containing_type());
}

void SymbolChecker::Initialize() {
  if (initialized_) {
    return;
  }

  internal::VisitDescriptors(*descriptor_, proto_,
                             [&](const auto& descriptor, const auto& proto) {
                               VisitType(descriptor, proto, &state_);
                             });

  initialized_ = true;
}

// Enforce File-wide visibility and future co-location rules.
std::vector<SymbolCheckerError> SymbolChecker::CheckSymbolVisibilityRules() {
  std::vector<SymbolCheckerError> errors;

  Initialize();

  // Check DefaultSymbolVisibility first.
  //
  // For Edition 2024:
  // If DefaultSymbolVisibility is STRICT enforce it with caveats for:

  // Build our state object so we can apply rules based on type.

  // In edition 2024 we only enforce STRICT visibility rules. There are possibly
  // more rules to come in future editions, but for now just apply the rule for
  // enforcing nested symbol local visibility. There is a single caveat for,
  // allowing nested enums to have visibility set only when
  //
  // local msg { export enum {} reserved 1 to max; }
  for (auto& nested : state_.nested_messages) {
    if (nested.descriptor->visibility_keyword() ==
            SymbolVisibility::VISIBILITY_EXPORT &&
        nested.descriptor->features().default_symbol_visibility() ==
            FeatureSet::VisibilityFeature::STRICT) {
      errors.push_back(SymbolCheckerError(nested.descriptor->full_name(),
                                          *nested.proto,
                                          NESTED_MESSAGE_STRICT_VIOLATION));
    }
  }

  for (auto& nested : state_.nested_enums) {
    if (nested.descriptor->visibility_keyword() ==
            SymbolVisibility::VISIBILITY_EXPORT &&
        nested.descriptor->features().default_symbol_visibility() ==
            FeatureSet::VisibilityFeature::STRICT) {
      // This list contains only enums not considered 'namespaced' by
      // IsEnumNamespaceMessage
      errors.push_back(SymbolCheckerError(nested.descriptor->full_name(),
                                          *nested.proto,
                                          NESTED_ENUM_STRICT_VIOLATION));
    }
  }

  // Enforce Future rules here:
  return errors;
}

}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
