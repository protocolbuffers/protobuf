#ifndef GOOGLE_PROTOBUF_EDITIONS_EDITION_DEFAULTS_TEST_UTILS_H__
#define GOOGLE_PROTOBUF_EDITIONS_EDITION_DEFAULTS_TEST_UTILS_H__

// This file contains test-only helper methods for dealing with
// edition defaults.  Only helpers that are specific to
// edition defaults tests should be added here.

#include "google/protobuf/descriptor.pb.h"
#include "absl/types/optional.h"

namespace google {
namespace protobuf {
namespace compiler {

// Returns the FeatureSetEditionDefault for the given edition, or absl::nullopt
// if edition is not found in defaults.
absl::optional<FeatureSetDefaults::FeatureSetEditionDefault> FindEditionDefault(
    const FeatureSetDefaults& defaults, Edition edition);

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EDITIONS_EDITION_DEFAULTS_TEST_UTILS_H__
