#ifndef GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__
#define GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__

#include <cstdint>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/feature_resolver.h"

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

 private:
  friend class ::google::protobuf::compiler::CodeGenerator;
  friend class ::google::protobuf::compiler::CommandLineInterface;

  static const DescriptorPool& GetDescriptorPool(const FileDescriptor& file) {
    return *file.pool();
  }

  template <typename DescriptorT>
  static const DescriptorPool& GetDescriptorPool(const DescriptorT& desc) {
    return GetDescriptorPool(*desc.file());
  }

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

  // Gets the resolved FeatureSet extension for a given descriptor. FeatureSet
  // extension messages (e.g. pb.JavaFeatures) provide the serialized data for
  // the language feature set defaults in its gencode. This function parses the
  // serialized data and merges it with the original feature set extension.
  template <typename DescriptorT, typename ExtType, uint8_t field_type,
            bool is_packed>
  static auto GetResolvedFeatureExtension(
      const DescriptorT& descriptor,
      const google::protobuf::internal::ExtensionIdentifier<
          FeatureSet, MessageTypeTraits<ExtType>, field_type, is_packed>&
          extension) {
    auto lang_features = GetFeatures(descriptor).GetExtension(extension);
    if (GetDescriptorPool(descriptor).ResolvesFeaturesFor(extension)) {
      return lang_features;
    }

    // Gets and parses the language feature set defaults data from the gencode.
    FeatureSetDefaults lang_feature_set_defaults;
    std::string unescaped_data;
    absl::Base64Unescape(::pb::internal::GetFeatureSetDefaultsData<ExtType>(),
                         &unescaped_data);
    ABSL_CHECK(lang_feature_set_defaults.ParseFromString(unescaped_data));

    // Gets the resolved feature set for the given edition and merge it with the
    // defaults.
    auto edition_feature_set = FeatureResolver::GetEditionFeatureSetDefaults(
        GetEdition(descriptor), lang_feature_set_defaults);
    ABSL_CHECK_OK(edition_feature_set);
    auto lang_features_ret = edition_feature_set->GetExtension(extension);
    lang_features_ret.MergeFrom(lang_features);
    return lang_features_ret;
  }
};
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__
