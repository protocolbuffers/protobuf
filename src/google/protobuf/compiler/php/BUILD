# Copyright (c) 2019, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Public targets for PHP code generator.

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("//build_defs:cpp_opts.bzl", "COPTS")

cc_library(
    name = "names",
    srcs = ["names.cc"],
    hdrs = ["names.h"],
    copts = COPTS,
    strip_include_prefix = "/src",
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//src/google/protobuf",
        "//src/google/protobuf:port",
        "//src/google/protobuf/compiler:code_generator",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "php_generator",
    srcs = [
        "php_generator.cc",
    ],
    hdrs = [
        "php_generator.h",
    ],
    copts = COPTS,
    strip_include_prefix = "/src",
    visibility = [
        "//pkg:__pkg__",
        "//src/google/protobuf/compiler:__pkg__",
    ],
    deps = [
        ":names",
        "//src/google/protobuf",
        "//src/google/protobuf:port",
        "//src/google/protobuf/compiler:code_generator",
        "//src/google/protobuf/compiler:retention",
        "//src/google/protobuf/io",
        "//src/google/protobuf/io:printer",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "generator_unittest",
    srcs = ["generator_unittest.cc"],
    deps = [
        ":php_generator",
        "//:protobuf",
        "//src/google/protobuf/compiler:command_line_interface_tester",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

################################################################################
# Distribution packaging
####

filegroup(
    name = "test_srcs",
    srcs = glob(
        [
            "*_test.cc",
            "*unittest.cc",
        ],
        allow_empty = True,
    ),
    visibility = ["//src/google/protobuf/compiler:__pkg__"],
)
