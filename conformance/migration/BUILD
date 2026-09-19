# Copyright (c) 2026, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# google3-only tooling for the conformance-test modernization
# (go/modernizing-conformance-tests).  This whole directory is excluded from
# the GitHub export in //src/google/protobuf/copybara/cpp.copy.bara.sky.

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")
load("@rules_shell//shell:sh_library.bzl", "sh_library")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

package(default_applicable_licenses = ["//:license"])

# New BSD
licenses(["notice"])

# A testee that answers every conformance request with `skipped`.  The
# request golden must depend only on which requests the runner emits, never on
# a real implementation's behaviour.  Deliberately dependency-free: the fixed
# serialized ConformanceResponse is embedded in the source.
cc_binary(
    name = "skipping_testee",
    testonly = 1,
    srcs = ["skipping_testee.cc"],
    deps = [
        "@abseil-cpp//absl/strings:string_view",
    ],
)

# Shared by the test and the regeneration script so that they can never
# disagree about how the requests are recorded.
sh_library(
    name = "record_requests_lib",
    testonly = 1,
    srcs = ["record_requests_lib.sh"],
    data = [
        ":skipping_testee",
        "//conformance:conformance_test_runner",
    ],
)

# Fails if the set of requests conformance_test_runner sends differs from
# conformance_requests.golden (--maximum_edition 2023) or
# conformance_requests_default_edition.golden (proto2/proto3 only).  See
# README.md.
sh_test(
    name = "request_golden_test",
    size = "medium",
    srcs = ["request_golden_test.sh"],
    data = [
        "conformance_requests.golden",
        "conformance_requests_default_edition.golden",
    ],
    deps = [":record_requests_lib"],
)

# Rewrites both goldens in the user's workspace:
#   blaze run //conformance/migration:regenerate_golden
sh_binary(
    name = "regenerate_golden",
    testonly = 1,
    srcs = ["regenerate_golden.sh"],
    deps = [":record_requests_lib"],
)
