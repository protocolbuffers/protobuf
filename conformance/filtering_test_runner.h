// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_FILTERING_TEST_RUNNER_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_FILTERING_TEST_RUNNER_H__

#include <string>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "conformance/test_runner.h"

namespace google {
namespace protobuf {
namespace conformance {

// Decorates a ConformanceTestRunner so that only the tests named in a set are
// forwarded to the delegate.  Any other test gets a ConformanceResponse with
// `skipped` set to kTestNotSelectedSkipReason (see test_runner.h), without
// contacting the delegate.  The one exception is the discovery handshake's
// probe (kProbeName in test_runner.h), which is always forwarded and never
// counted as run: the Testee needs the testee's protocol version before the
// first selected test, and a filtered run must discover the same version as a
// full one.  (So the testee process is started even if none of the tests is
// selected.)
//
// This implements the legacy `--test <name>` isolation for gtest-based
// conformance suites.  The shared reason string is what lets the reporting
// side (Yields(), see matchers.h) tell such a test apart from one the testee
// itself opted out of.  NamesRun() tells which of the requested names came up,
// so that the caller can tell which ones are still to be looked for elsewhere.
//
// The delegate is not owned and must outlive this object.
class FilteringTestRunner : public ConformanceTestRunner {
 public:
  FilteringTestRunner(ConformanceTestRunner* absl_nonnull delegate,
                      absl::flat_hash_set<std::string> names_to_run);
  ~FilteringTestRunner() override;

  FilteringTestRunner(const FilteringTestRunner&) = delete;
  FilteringTestRunner& operator=(const FilteringTestRunner&) = delete;

  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override;

  // The names forwarded to the delegate so far, in sorted order.
  std::vector<std::string> NamesRun() const;

 private:
  ConformanceTestRunner* absl_nonnull delegate_;
  absl::flat_hash_set<std::string> names_to_run_;
  absl::flat_hash_set<std::string> names_run_;
};

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_FILTERING_TEST_RUNNER_H__
