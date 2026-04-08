# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:proto_library.bzl", "proto_library")
load("//rust:defs.bzl", "rust_proto_library")

package(
    default_testonly = 1,
    default_visibility = ["//rust:__subpackages__"],
)

proto_library(
    name = "parent_proto",
    srcs = ["parent.proto"],
)

proto_library(
    name = "child_proto",
    srcs = ["child.proto"],
    exports = [":parent_proto"],
    deps = [":parent_proto"],
)

proto_library(
    name = "srcsless_alias_proto",
    deps = [":child_proto"],
)

proto_library(
    name = "srcsless_double_alias_proto",
    deps = [":srcsless_alias_proto"],
)

proto_library(
    name = "grandchild_proto",
    srcs = ["grandchild.proto"],
    exports = [":child_proto"],
    deps = [":srcsless_double_alias_proto"],
)

rust_proto_library(
    name = "parent_rust_proto",
    deps = [":parent_proto"],
)

rust_proto_library(
    name = "child_rust_proto",
    deps = [":child_proto"],
)

rust_proto_library(
    name = "grandchild_rust_proto",
    deps = [":grandchild_proto"],
)
