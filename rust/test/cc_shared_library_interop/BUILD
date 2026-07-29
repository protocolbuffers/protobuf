# Dummy comment to force re-analysis on Linux 2.
# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_shared_library.bzl", "cc_shared_library")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_rust//rust:defs.bzl", "rust_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//rust:defs.bzl", "rust_proto_library")
load("//tools/build_defs/proto/cpp:cc_proto_library.bzl", "cc_proto_library")
load("//tools/build_rules:build_test.bzl", "build_test")
load(":cc_shared_library_link_order_test.bzl", "cc_shared_library_link_order_test")

package(
    default_testonly = True,
    default_visibility = ["//visibility:public"],
)

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
)

cc_proto_library(
    name = "foo_cc_proto",
    deps = [":foo_proto"],
)

rust_proto_library(
    name = "foo_rust_proto",
    deps = [":foo_proto"],
)

rust_library(
    name = "bar",
    srcs = ["bar.rs"],
    deps = [":foo_rust_proto"],
)

cc_library(
    name = "baz",
    srcs = ["baz.cc"],
    hdrs = ["baz.h"],
    deps = [
        ":bar",
        ":foo_cc_proto",
    ],
)

cc_binary(
    name = "bin",
    srcs = ["main.cc"],
    deps = [
        ":baz",
        "//devtools/rust:rust_okay_here",
    ],
)

cc_library(
    name = "qux_lib",
    srcs = ["qux.cc"],
    hdrs = ["qux.h"],
    deps = [":baz"],
)

cc_shared_library(
    name = "shared_lib",
    deps = [
        ":bar",
        ":qux_lib",
        "//devtools/rust:rust_okay_here",
        "//rust:protobuf",
    ],
)

cc_binary(
    name = "dynamic_bin",
    srcs = ["dynamic_main.cc"],
    dynamic_deps = [":shared_lib"],
    deps = [
        "//devtools/rust:rust_okay_here",
    ],
)

build_test(
    name = "cc_shared_library_interop_build_test",
    targets = [
        ":shared_lib",
        ":dynamic_bin",
    ],
)

cc_shared_library_link_order_test(
    name = "link_order_test",
    target_under_test = ":shared_lib",
)
