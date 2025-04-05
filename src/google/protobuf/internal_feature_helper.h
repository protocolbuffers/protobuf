#ifndef GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__
#define GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__

#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/extension_set.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
// This class is for internal use only and provides access to the resolved
// runtime FeatureSets of any descriptor.  These features are not designed
// to be stable, and depending directly on them (vs the public descriptor APIs)
// is not safe.
class PROTOBUF_EXPORT InternalFeatureHelper {
 public:
  template <typename DescriptorT>
  static const FeatureSet& GetFeatures(const DescriptorT& desc) {
    return desc.features();
  }

  template <typename DescriptorT, typename TypeTraitsT, uint8_t field_type,
            bool is_packed>
  static auto GetFeatureExtension(
      const DescriptorT& descriptor,
      const google::protobuf::internal::ExtensionIdentifier<
          FeatureSet, TypeTraitsT, field_type, is_packed>& extension) {
    // Gets and parses the language feature set defaults data from the gencode.
    auto features = GetFeatures(descriptor);
    auto lang_features = features.GetExtension(extension);
    FeatureSetDefaults lang_feature_set_defaults;
    std::string serialized_data(lang_features.kFeatureDefaults,
                                sizeof(lang_features.kFeatureDefaults) - 1);
    ABSL_CHECK(lang_feature_set_defaults.ParseFromString(serialized_data));

    // Finds the language feature set defaults for the edition of the descriptor
    // and merges them with original feature set extension.
    auto comparator = [](const auto& a, const auto& b) {
      return a.edition() < b.edition();
    };
    FeatureSetDefaults::FeatureSetEditionDefault edition_lookup;
    edition_lookup.set_edition(GetEdition(descriptor));
    auto first_nonmatch = absl::c_upper_bound(
        lang_feature_set_defaults.defaults(), edition_lookup, comparator);

    // If no edition is found, return the original feature set extension.
    if (first_nonmatch == lang_feature_set_defaults.defaults().begin()) {
      return lang_features;
    }

    decltype(lang_features) lang_features_ret =
        std::prev(first_nonmatch)->fixed_features().GetExtension(extension);
    lang_features_ret.MergeFrom(std::prev(first_nonmatch)
                                    ->overridable_features()
                                    .GetExtension(extension));
    lang_features_ret.MergeFrom(lang_features);
    return lang_features_ret;
  }

 private:
  friend class ::google::protobuf::compiler::CodeGenerator;
  friend class ::google::protobuf::compiler::CommandLineInterface;

  // Provides a restricted view exclusively to code generators to query their
  // own unresolved features.  Unresolved features are virtually meaningless to
  // everyone else. Code generators will need them to validate their own
  // features, and runtimes may need them internally to be able to properly
  // represent the original proto files from generated code.
  template <typename DescriptorT, typename TypeTraitsT, uint8_t field_type,
            bool is_packed>
  static typename TypeTraitsT::ConstType GetUnresolvedFeatures(
      const DescriptorT& descriptor,
      const google::protobuf::internal::ExtensionIdentifier<
          FeatureSet, TypeTraitsT, field_type, is_packed>& extension) {
    return descriptor.proto_features_->GetExtension(extension);
  }

  // Provides a restricted view exclusively to code generators to query the
  // edition of files being processed.  While most people should never write
  // edition-dependent code, generators frequently will need to.
  static Edition GetEdition(const FileDescriptor& desc) {
    return desc.edition();
  }

  template <typename DescriptorT>
  static Edition GetEdition(const DescriptorT& desc) {
    return GetEdition(*desc.file());
  }
};
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__

#include "google/protobuf/port_undef.inc"
