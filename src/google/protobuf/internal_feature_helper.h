#ifndef GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__
#define GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__

#include <cstdint>

#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/extension_set.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

namespace google {
namespace protobuf {
namespace internal {
class InternalFeatureHelperTest;
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
  friend class ::google::protobuf::internal::InternalFeatureHelperTest;

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

  // Parses the serialized FeatureSetDefaults and returns the resolved
  // FeatureSet for a given edition.
  static FeatureSet ParseAndGetEditionResolvedFeatureSet(absl::string_view data,
                                                         Edition edition);

  // Gets the resolved FeatureSet extension for a given descriptor.
  //
  // If the descriptor's pool has already provided the resolved feature default
  // for the edition and the language FeatureSet extension, then the default
  // will be returned directly. Otherwise, the function will parse the
  // serialized FeatureSetDefaults data provided by the language FeatureSet
  // extension, and merge it with the original FeatureSet extension so that the
  // resolved feature set defaults will always be present.
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

    auto lang_features_ret =
        ParseAndGetEditionResolvedFeatureSet(
            ::pb::internal::GetFeatureSetDefaultsData<ExtType>(),
            GetEdition(descriptor))
            .GetExtension(extension);
    lang_features_ret.MergeFrom(lang_features);
    return lang_features_ret;
  }
};
}  // namespace internal
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"

#endif  // GOOGLE_PROTOBUF_INTERNAL_FEATURE_HELPER_H__
