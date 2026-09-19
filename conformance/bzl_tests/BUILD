# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Analysis tests for //conformance:conformance.bzl.  They
# live in this leaf package, which nothing depends on, because they load
# rules_testing: a dev-only dependency of the open-source build that the
# exported conformance package itself must not require.

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("//conformance:conformance.bzl", "failure_lists")
load(":conformance_bzl_test.bzl", "conformance_bzl_test_suite")

package(default_applicable_licenses = ["//:license"])

# The analysis tests and their subjects (manual conformance_tests that are
# never run).  The subjects' failure lists are the fixtures in
# bzl_test_failure_lists/, and their testee is the stub below.
conformance_bzl_test_suite(name = "conformance_bzl_test")

failure_lists(name = "bzl_test_failure_lists")

cc_binary(
    name = "stub_testee",
    testonly = 1,
    srcs = ["stub_testee.cc"],
)

bzl_library(
    name = "conformance_bzl_test_bzl",
    srcs = ["conformance_bzl_test.bzl"],
    visibility = ["//visibility:private"],
    deps = [
        "//conformance:conformance_bzl",
        "@rules_testing//lib:analysis_test_bzl",
        "@rules_testing//lib:truth_bzl",
        "@rules_testing//lib:util_bzl",
    ],
)
