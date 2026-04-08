#include "editions/edition_defaults_test_utils.h"

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/test_textproto.h"

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

namespace {
// TODO: Remove this function once test util Partially is available
// in OSS.
//
// Removes any EDITION_UNSTABLE entries from the defaults repeated field.
FeatureSetDefaults ScrubUnstable(const FeatureSetDefaults& defaults) {
  FeatureSetDefaults result = defaults;
  auto* mutable_defaults = result.mutable_defaults();
  for (int i = 0; i < mutable_defaults->size(); ++i) {
    if (mutable_defaults->Get(i).edition() == EDITION_UNSTABLE) {
      mutable_defaults->DeleteSubrange(i, 1);
      --i;
    }
  }
  return result;
}
}  // namespace

// Partially matches FeatureSetDefaults against a textproto for the purpose of
// ignoring defaults for EDITION_UNSTABLE.
::testing::Matcher<FeatureSetDefaults> PartiallyMatchesEditionDefaults(
    absl::string_view expected_textproto) {
  return ::testing::ResultOf(ScrubUnstable,
                             ::google::protobuf::EqualsProto(expected_textproto));
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
