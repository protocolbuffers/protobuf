// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/filtering_test_runner.h"

#include <string>
#include <utility>
#include <vector>

#include "absl/algorithm/container.h"
#include "absl/base/nullability.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

FilteringTestRunner::FilteringTestRunner(
    ConformanceTestRunner* absl_nonnull delegate,
    absl::flat_hash_set<std::string> names_to_run)
    : delegate_(delegate), names_to_run_(std::move(names_to_run)) {}

FilteringTestRunner::~FilteringTestRunner() = default;

std::string FilteringTestRunner::RunTest(absl::string_view test_name,
                                         absl::string_view input) {
  // The discovery handshake isn't a test and isn't subject to the filter:
  // the Testee needs its answer before the first (selected) test, whatever
  // was selected.  It isn't one of the names --test asked for either, so it
  // doesn't count as run.
  if (test_name == kProbeName) {
    return delegate_->RunTest(test_name, input);
  }
  if (!names_to_run_.contains(test_name)) {
    // Fully qualified: inside google::protobuf::conformance, a bare `conformance::`
    // would name this namespace rather than the protos' one.
    ::conformance::ConformanceResponse response;
    response.set_skipped(kTestNotSelectedSkipReason);
    return response.SerializeAsString();
  }
  names_run_.insert(std::string(test_name));
  return delegate_->RunTest(test_name, input);
}

std::vector<std::string> FilteringTestRunner::NamesRun() const {
  std::vector<std::string> names_run(names_run_.begin(), names_run_.end());
  absl::c_sort(names_run);
  return names_run;
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
