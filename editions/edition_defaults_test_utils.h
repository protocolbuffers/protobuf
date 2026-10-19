// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_EDITIONS_EDITION_DEFAULTS_TEST_UTILS_H__
#define GOOGLE_PROTOBUF_EDITIONS_EDITION_DEFAULTS_TEST_UTILS_H__

// This file contains test-only helper methods for dealing with
// edition defaults.  Only helpers that are specific to
// edition defaults tests should be added here.

#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace google {
namespace protobuf {
namespace compiler {

// Returns the FeatureSetEditionDefault for the given edition, or absl::nullopt
// if edition is not found in defaults.
absl::optional<FeatureSetDefaults::FeatureSetEditionDefault> FindEditionDefault(
    const FeatureSetDefaults& defaults, Edition edition);

// Compares feature set defaults, ignoring unstable editions.
::testing::Matcher<FeatureSetDefaults> PartiallyMatchesEditionDefaults(
    absl::string_view expected_textproto);

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_EDITIONS_EDITION_DEFAULTS_TEST_UTILS_H__
