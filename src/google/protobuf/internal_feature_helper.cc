// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/internal_feature_helper.h"

#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/escaping.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/feature_resolver.h"

namespace google {
namespace protobuf {
namespace internal {
FeatureSet InternalFeatureHelper::ParseAndGetEditionResolvedFeatureSet(
    absl::string_view data, Edition edition) {
  FeatureSetDefaults defaults;
  std::string unescaped_data;
  absl::Base64Unescape(data, &unescaped_data);
  ABSL_CHECK(defaults.ParseFromString(unescaped_data));
  auto edition_feature_set = GetEditionFeatureSetDefaults(edition, defaults);
  ABSL_CHECK_OK(edition_feature_set);
  return *edition_feature_set;
}
}  // namespace internal
}  // namespace protobuf
}  // namespace google
