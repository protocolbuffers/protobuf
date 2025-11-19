# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# A portion of our conformance tests which are split off from top level BUILD file only to let that
# BULID file be public while avoiding having any load() of a non-dev-dependencies in Bazel.

load("@rules_ruby//ruby:defs.bzl", "rb_binary")
load("//ruby:defs.bzl", "internal_ruby_proto_library")

internal_ruby_proto_library(
    name = "conformance_ruby_proto",
    srcs = ["//conformance:conformance.proto"],
    visibility = [
        "//conformance:__pkg__",
        "//ruby:__pkg__",
    ],
)

internal_ruby_proto_library(
    name = "test_messages_edition2023_ruby_proto",
    srcs = ["//conformance/test_protos:test_messages_edition2023.proto"],
)

rb_binary(
    name = "conformance_ruby",
    testonly = True,
    srcs = ["conformance_ruby.rb"],
    main = "conformance_ruby.rb",
    visibility = ["//ruby:__subpackages__"],
    deps = [
        ":conformance_ruby_proto",
        ":test_messages_edition2023_ruby_proto",
        "//ruby:conformance_editions_test_ruby_proto",
        "//ruby:conformance_test_ruby_proto",
        "//ruby:protobuf",
    ],
)
