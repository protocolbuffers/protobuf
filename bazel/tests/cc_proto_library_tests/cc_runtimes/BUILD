# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load(":tests.bzl", "RUNTIME_COPT", "RUNTIME_DEFINE", "RUNTIME_LINKOPT", "cc_runtimes_test_suites", "mock_cc_runtime_library", "mock_cc_runtimes_toolchain")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

cc_proto_library(
    name = "foo_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

# The proto runtime is normally a cc_library, which depends on the C++ runtimes itself. So to check
# that cc_proto_library depends on the C++ runtimes directly, these targets use a proto runtime
# that doesn't depend on them.
proto_library(
    name = "foo_no_proto_runtime_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

cc_proto_library(
    name = "foo_no_proto_runtime_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_no_proto_runtime_proto"],
)

mock_cc_runtime_library(
    name = "empty_proto_runtime",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

# A C++ proto toolchain without a runtime library.
proto_lang_toolchain(
    name = "no_runtime_cc_proto_toolchain",
    command_line = "--cpp_out=$(OUT)",
    progress_message = "Generating C++ proto_library %{label}",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    toolchain_type = "//bazel/private:cc_toolchain_type",
)

mock_cc_runtime_library(
    name = "mock_runtime",
    defines = [RUNTIME_DEFINE],
    linkopts = [RUNTIME_LINKOPT],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

mock_cc_runtimes_toolchain(
    name = "mock_cc_runtimes",
    copts = [RUNTIME_COPT],
    runtimes = [":mock_runtime"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

# The toolchain type only exists as of Bazel 9, so this is only built when a test registers it.
toolchain(
    name = "mock_cc_runtimes_toolchain",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    toolchain = ":mock_cc_runtimes",
    toolchain_type = "@bazel_tools//tools/cpp:cc_runtimes_toolchain_type",
)

cc_runtimes_test_suites(
    name = "tests",
    no_runtime_cc_proto_toolchain = ":no_runtime_cc_proto_toolchain",
    toolchain = ":mock_cc_runtimes_toolchain",
)
