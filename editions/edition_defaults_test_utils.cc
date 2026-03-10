#include "editions/edition_defaults_test_utils.h"

#include "google/protobuf/descriptor.pb.h"
#include "absl/types/optional.h"

namespace google {
namespace protobuf {
namespace compiler {

absl::optional<FeatureSetDefaults::FeatureSetEditionDefault> FindEditionDefault(
    const FeatureSetDefaults& defaults, Edition edition) {
  for (const auto& edition_default : defaults.defaults()) {
    if (edition_default.edition() == edition) {
      return edition_default;
    }
  }
  return absl::nullopt;
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
