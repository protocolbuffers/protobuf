// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// upb's in-process conformance testee: defines MakeTesteeRunner() (see the
// conformance framework's testee_runner.h) to host
// upb_ConformanceHarness in the test process, so that
// `conformance_test(in_process = True)` runs the upb suites without forking
// a conformance_upb binary.
//
// This file is compiled into two libraries (see BUILD), one per def pool
// setup of the harness, selected by UPB_CONFORMANCE_REBUILD_MINITABLES:
//
//  * 0: :upb_in_process_testee, the generated mini tables (what the
//    conformance_upb binary runs).
//  * 1: :upb_dynamic_minitable_in_process_testee, mini tables rebuilt at
//    runtime from the descriptors (conformance_upb_dynamic_minitable).
//
// They are separate libraries rather than one library with a flag because a
// conformance_test() links exactly one testee and passes it no arguments; the
// define also names the variant in the messages below.

#include <memory>
#include <string>

#include "absl/log/absl_check.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/test_runner.h"
#include "conformance/testee_runner.h"
#include "upb/base/string_view.h"
#include "upb/conformance/conformance_upb_harness.h"
#include "upb/mem/arena.hpp"

#ifndef UPB_CONFORMANCE_REBUILD_MINITABLES
#error "Define UPB_CONFORMANCE_REBUILD_MINITABLES to 0 or 1 (see BUILD)."
#endif

namespace google {
namespace protobuf {
namespace conformance {
namespace {

#if UPB_CONFORMANCE_REBUILD_MINITABLES
constexpr bool kRebuildMinitables = true;
constexpr absl::string_view kTesteeName =
    "upb_dynamic_minitable_in_process_testee";
#else
constexpr bool kRebuildMinitables = false;
constexpr absl::string_view kTesteeName = "upb_in_process_testee";
#endif

// Answers each request with upb_ConformanceHarness_Run().  The harness (and
// its def pool) lives as long as the runner; each request gets its own arena,
// freed once the response bytes have been copied out.
class UpbInProcessTestRunner : public ConformanceTestRunner {
 public:
  explicit UpbInProcessTestRunner(bool rebuild_minitables)
      : harness_(upb_ConformanceHarness_New(rebuild_minitables)) {}
  UpbInProcessTestRunner(const UpbInProcessTestRunner&) = delete;
  UpbInProcessTestRunner& operator=(const UpbInProcessTestRunner&) = delete;
  ~UpbInProcessTestRunner() override { upb_ConformanceHarness_Free(harness_); }

  std::string RunTest(absl::string_view test_name,
                      absl::string_view input) override {
    upb::Arena arena;
    upb_StringView response = upb_ConformanceHarness_Run(
        harness_, upb_StringView_FromDataAndSize(input.data(), input.size()),
        arena.ptr());
    return std::string(response.data, response.size);
  }

 private:
  upb_ConformanceHarness* const harness_;
};

}  // namespace

std::unique_ptr<ConformanceTestRunner> MakeTesteeRunner(
    absl::string_view testee_binary,
    absl::Span<const std::string> testee_args) {
  // The flags of a forked testee have no meaning here; refusing them keeps a
  // stale command line from silently testing the linked testee instead.
  ABSL_QCHECK(testee_binary.empty())
      << "--testee_binary=" << testee_binary
      << " was given, but this test links the upb testee in-process ("
      << kTesteeName << ") and can't run another binary; drop --testee_binary.";
  ABSL_QCHECK(testee_args.empty())
      << "--testee_args / positional arguments ["
      << absl::StrJoin(testee_args, " ")
      << "] were given, but this test links the upb testee in-process ("
      << kTesteeName << "), which takes no arguments; drop them.";
  return std::make_unique<UpbInProcessTestRunner>(kRebuildMinitables);
}

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
