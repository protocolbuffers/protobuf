# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# These conformance tests are in a dedicated subdirectory to avoid including
# the dev-only dependency on `rules_ruby` in the main conformance directory,
# which is meant to be externally accessible.

load("@rules_ruby//ruby:defs.bzl", "rb_binary")
load("//python:internal.bzl", "internal_copy_files")
load("//ruby:defs.bzl", "internal_ruby_proto_library")

internal_ruby_proto_library(
    name = "conformance_ruby_proto",
    testonly = True,
    srcs = [":copied_conformance_proto"],
)

internal_ruby_proto_library(
    name = "test_messages_edition2023_ruby_proto",
    testonly = True,
    srcs = [":copied_test_messages_edition2023_proto"],
)

internal_ruby_proto_library(
    name = "test_messages_edition_unstable_ruby_proto",
    testonly = True,
    srcs = [":copied_test_messages_edition_unstable_proto"],
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
        ":test_messages_edition_unstable_ruby_proto",
        "//ruby:conformance_editions_test_ruby_proto",
        "//ruby:conformance_test_ruby_proto",
        "//ruby:protobuf",
    ],
)

# Source proto files need to be copied to the package directory
# because `internal_ruby_proto_library` expects them to be there.
internal_copy_files(
    name = "copied_conformance_proto",
    testonly = True,
    srcs = ["//conformance:conformance.proto"],
    strip_prefix = "conformance",
)

internal_copy_files(
    name = "copied_test_messages_edition2023_proto",
    testonly = True,
    srcs = ["//conformance/test_protos:test_messages_edition2023.proto"],
    strip_prefix = "conformance/test_protos",
)

internal_copy_files(
    name = "copied_test_messages_edition_unstable_proto",
    testonly = True,
    srcs = ["//conformance/test_protos:test_messages_edition_unstable.proto"],
    strip_prefix = "conformance/test_protos",
)
