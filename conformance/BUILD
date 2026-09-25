# Copyright (c) 2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Protobuf conformance tests.

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "objc_library")
load("@rules_java//java:defs.bzl", "java_binary")
load("@rules_python//python:py_binary.bzl", "py_binary")
load(
    "//:protobuf.bzl",
    "internal_csharp_proto_library",
    "internal_objc_proto_library",
    "internal_php_proto_library",
    "internal_py_proto_library",
)
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:java_lite_proto_library.bzl", "java_lite_proto_library")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//build_defs:internal_shell.bzl", "inline_sh_binary")

exports_files([
    "bazel_conformance_test_runner.sh",
    "conformance.proto",
    "failure_list_cpp.txt",
    "failure_list_csharp.txt",
    "failure_list_csharp_performance.txt",
    "failure_list_java.txt",
    "failure_list_java_lite.txt",
    "failure_list_objc.txt",
    "failure_list_php.txt",
    "failure_list_php_c.txt",
    "failure_list_python.txt",
    "failure_list_python_cpp.txt",
    "failure_list_python_upb.txt",
    "failure_list_ruby.txt",
    "failure_list_jruby.txt",
    "failure_list_jruby_ffi.txt",
    "text_format_failure_list_cpp.txt",
    "text_format_failure_list_java.txt",
    "text_format_failure_list_java_lite.txt",
    "text_format_failure_list_php.txt",
    "text_format_failure_list_php_c.txt",
    "text_format_failure_list_python.txt",
])

cc_library(
    name = "conformance_test_lib",
    testonly = 1,
    srcs = [
        "conformance_test.cc",
        "conformance_test_runner.cc",
    ],
    hdrs = [
        "conformance_test.h",
    ],
    deps = [
        ":conformance_cc_proto",
        ":failure_list_trie_node",
        ":fork_pipe_runner",
        ":recording_test_runner",
        ":test_runner",
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_legacy",
        "//src/google/protobuf:endian",
        "//src/google/protobuf/util:differencer",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:btree",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@abseil-cpp//absl/types:span",
    ],
)

# The transitional main of conformance_test_runner: runs the legacy suites
# above and any gtest-based conformance suites linked into the same binary
# (none yet) with one command line and one verdict.  See
# conformance_test_main.cc.
cc_library(
    name = "conformance_test_main",
    testonly = 1,
    srcs = ["conformance_test_main.cc"],
    deps = [
        ":binary_json_conformance_suite",
        ":conformance_test_lib",
        ":filtering_test_runner",
        ":fork_pipe_runner",
        ":recording_test_runner",
        ":test_environment",
        ":test_manager",
        ":test_runner",
        ":text_format_conformance_suite",
        "//src/google/protobuf",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@abseil-cpp//absl/types:span",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "conformance_test_runner_test",
    srcs = ["conformance_test_runner_test.cc"],
    deps = [
        ":conformance_cc_proto",
        ":conformance_test_lib",
        ":test_runner",
        "//src/google/protobuf",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "failure_list_trie_node",
    srcs = ["failure_list_trie_node.cc"],
    hdrs = ["failure_list_trie_node.h"],
    deps = [
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/types:optional",
    ],
)

cc_test(
    name = "failure_list_trie_node_test",
    srcs = ["failure_list_trie_node_test.cc"],
    deps = [
        ":failure_list_trie_node",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# Add more meta-testing here. This is not to be confused with a conformance test itself.
test_suite(
    name = "conformance_framework_tests",
    tests = [
        "binary_test_util_test",
        "binary_wireformat_test",
        "conformance_test_runner_test",
        "failure_list_trie_node_test",
        "filtering_test_runner_test",
        "fork_pipe_runner_test",
        "matchers_test",
        "naming_test",
        "recording_test_runner_test",
        "test_environment_test",
        "test_manager_test",
        "testee_test",
    ],
)

cc_library(
    name = "binary_json_conformance_suite",
    testonly = 1,
    srcs = ["binary_json_conformance_suite.cc"],
    hdrs = ["binary_json_conformance_suite.h"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":conformance_test_lib",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//conformance/test_protos:test_messages_edition_unstable_cc_proto",
        "//editions:test_messages_proto2_editions_cc_proto",
        "//editions:test_messages_proto3_editions_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:protobuf_lite",
        "//src/google/protobuf/json",
        "//src/google/protobuf/util:differencer",
        "//src/google/protobuf/util:type_resolver",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/log:die_if_null",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@jsoncpp",
    ],
)

# Helpers shared by the gtest-based binary conformance tests (the field-type
# tables the tests are parameterized over).
cc_library(
    name = "binary_test_util",
    testonly = 1,
    srcs = ["binary_test_util.cc"],
    hdrs = ["binary_test_util.h"],
    deps = [
        "//src/google/protobuf",
        "@abseil-cpp//absl/types:span",
    ],
)

cc_test(
    name = "binary_test_util_test",
    srcs = ["binary_test_util_test.cc"],
    deps = [
        ":binary_test_util",
        "//src/google/protobuf",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "text_format_conformance_suite",
    testonly = 1,
    srcs = ["text_format_conformance_suite.cc"],
    hdrs = ["text_format_conformance_suite.h"],
    deps = [
        ":conformance_test_lib",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//conformance/test_protos:test_messages_edition_unstable_cc_proto",
        "//editions:test_messages_proto2_editions_cc_proto",
        "//editions:test_messages_proto3_editions_cc_proto",
        "//src/google/protobuf",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/log:die_if_null",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_library(
    name = "test_runner",
    hdrs = ["test_runner.h"],
    deps = ["@abseil-cpp//absl/strings:string_view"],
)

cc_library(
    name = "fork_pipe_runner",
    srcs = [
        "fork_pipe_runner.cc",
        "fork_pipe_runner_posix.cc",
        "fork_pipe_runner_win32.cc",
    ],
    hdrs = ["fork_pipe_runner.h"],
    deps = [
        ":conformance_cc_proto",
        ":test_runner",
        "//src/google/protobuf:endian",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",  # fork_pipe_runner_win32.cc
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/time",
        "@abseil-cpp//absl/types:optional",
        "@abseil-cpp//absl/types:span",
    ],
)

cc_test(
    name = "fork_pipe_runner_test",
    srcs = ["fork_pipe_runner_test.cc"],
    deps = [
        ":conformance_cc_proto",
        ":fork_pipe_runner",
        "@abseil-cpp//absl/time",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "recording_test_runner",
    srcs = ["recording_test_runner.cc"],
    hdrs = ["recording_test_runner.h"],
    deps = [
        ":conformance_cc_proto",
        ":test_runner",
        "//src/google/protobuf",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_test(
    name = "recording_test_runner_test",
    srcs = ["recording_test_runner_test.cc"],
    deps = [
        ":conformance_cc_proto",
        ":recording_test_runner",
        ":test_runner",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "filtering_test_runner",
    srcs = ["filtering_test_runner.cc"],
    hdrs = ["filtering_test_runner.h"],
    deps = [
        ":conformance_cc_proto",
        ":test_runner",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_test(
    name = "filtering_test_runner_test",
    srcs = ["filtering_test_runner_test.cc"],
    deps = [
        ":conformance_cc_proto",
        ":filtering_test_runner",
        ":test_runner",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "binary_wireformat",
    testonly = 1,
    srcs = ["binary_wireformat.cc"],
    hdrs = ["binary_wireformat.h"],
    visibility = ["//upb/conformance:__pkg__"],
    deps = [
        "//src/google/protobuf:endian",
        "//src/google/protobuf:protobuf_lite",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "test_manager",
    testonly = 1,
    srcs = ["test_manager.cc"],
    hdrs = ["test_manager.h"],
    deps = [
        ":conformance_cc_proto",
        ":failure_list_trie_node",
        "@abseil-cpp//absl/algorithm:container",
        "@abseil-cpp//absl/container:btree",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
    ],
)

cc_library(
    name = "naming",
    hdrs = ["naming.h"],
    deps = [
        ":conformance_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_legacy",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "naming_test",
    srcs = ["naming_test.cc"],
    deps = [
        ":naming",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//conformance/test_protos:test_messages_edition_unstable_cc_proto",
        "//editions:test_messages_proto2_editions_cc_proto",
        "//editions:test_messages_proto3_editions_cc_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "testee",
    testonly = 1,
    srcs = ["testee.cc"],
    hdrs = ["testee.h"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":naming",
        ":test_runner",
        "//src/google/protobuf",
        "@abseil-cpp//absl/base:no_destructor",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "testee_test",
    srcs = ["testee_test.cc"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":test_runner",
        ":testee",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:test_textproto",
        "//src/google/protobuf/util:differencer",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_manager_test",
    srcs = ["test_manager_test.cc"],
    deps = [
        ":test_manager",
        "//src/google/protobuf/testing",
        "//src/google/protobuf/testing:file",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "global_test_environment",
    testonly = 1,
    hdrs = ["global_test_environment.h"],
    deps = [":test_manager"],
)

# gMock matchers for conformance TestResults.
#
# NOTE: Consumers must link a definition of GetGlobalTestManager(), declared in
# global_test_environment.h.  The conformance test environment provides it in
# the real test binary; matchers_test provides an in-memory stand-in.
cc_library(
    name = "matchers",
    testonly = 1,
    srcs = ["matchers.cc"],
    hdrs = ["matchers.h"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":global_test_environment",
        ":test_manager",
        ":test_runner",
        ":testee",
        "//src/google/protobuf",
        "//src/google/protobuf/util:differencer",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "matchers_test",
    srcs = ["matchers_test.cc"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":global_test_environment",
        ":matchers",
        ":test_manager",
        ":test_runner",
        ":testee",
        "//:test_messages_proto2_cc_proto",
        "//src/google/protobuf",
        "@abseil-cpp//absl/base:log_severity",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:scoped_mock_log",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "binary_wireformat_test",
    srcs = ["binary_wireformat_test.cc"],
    deps = [
        ":binary_wireformat",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "test_environment",
    testonly = 1,
    srcs = ["test_environment.cc"],
    hdrs = ["test_environment.h"],
    deps = [
        ":fork_pipe_runner",
        ":global_test_environment",
        ":test_manager",
        ":test_runner",
        ":testee",
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_legacy",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
    ],
)

# Defines the global --testee_binary, --failure_list, --fix, --performance,
# ... flags of a conformance test binary.  Only link it into binaries with a
# test_environment_main-style main (never into conformance_test_runner, whose
# legacy command line has flags of the same names).
cc_library(
    name = "test_environment_flags",
    testonly = 1,
    srcs = ["test_environment_flags.cc"],
    hdrs = ["test_environment_flags.h"],
    deps = [
        ":test_environment",
        "//src/google/protobuf",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
    ],
)

# The main() for gtest-based conformance suites.  Depend on this instead of a
# generic gtest main.
cc_library(
    name = "test_environment_main",
    testonly = 1,
    srcs = ["test_environment_main.cc"],
    deps = [
        ":test_environment",
        ":test_environment_flags",
        "@abseil-cpp//absl/flags:parse",  # buildcleaner: keep
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/types:span",
        "@googletest//:gtest",
    ],
    alwayslink = 1,
)

cc_test(
    name = "test_environment_test",
    srcs = ["test_environment_test.cc"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":global_test_environment",
        ":matchers",
        ":test_environment",
        ":test_environment_flags",
        ":test_manager",
        ":test_runner",
        ":testee",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//conformance/test_protos:test_messages_edition_unstable_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf/testing",
        "//src/google/protobuf/testing:file",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:reflection",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "conformance_test_runner",
    testonly = 1,
    visibility = ["//visibility:public"],
    deps = [":conformance_test_main"],
)

proto_library(
    name = "conformance_proto",
    srcs = ["conformance.proto"],
    visibility = ["//visibility:public"],
)

cc_proto_library(
    name = "conformance_cc_proto",
    deps = [":conformance_proto"],
)

java_proto_library(
    name = "conformance_java_proto",
    deps = [":conformance_proto"],
)

java_lite_proto_library(
    name = "conformance_java_proto_lite",
    deps = [":conformance_proto"],
)

exports_files([
    "bazel_conformance_test_runner.sh",
    "failure_list_cpp.txt",
    "failure_list_csharp.txt",
    "failure_list_csharp_performance.txt",
    "failure_list_java.txt",
    "failure_list_java_lite.txt",
    "failure_list_objc.txt",
    "failure_list_php.txt",
    "failure_list_php_c.txt",
    "failure_list_python.txt",
    "failure_list_python_cpp.txt",
    "failure_list_python_upb.txt",
    "failure_list_ruby.txt",
    "failure_list_jruby.txt",
    "failure_list_jruby_ffi.txt",
    "text_format_failure_list_cpp.txt",
    "text_format_failure_list_java.txt",
    "text_format_failure_list_java_lite.txt",
    "text_format_failure_list_php.txt",
    "text_format_failure_list_php_c.txt",
    "text_format_failure_list_python.txt",
])

internal_csharp_proto_library(
    name = "conformance_csharp_proto",
    srcs = ["conformance.proto"],
    visibility = [
        "//csharp:__subpackages__",
    ],
)

internal_objc_proto_library(
    name = "conformance_objc_proto",
    srcs = ["conformance.proto"],
    visibility = [
        "//conformance:__pkg__",
        "//objc:__pkg__",
    ],
)

internal_py_proto_library(
    name = "conformance_py_proto",
    srcs = ["conformance.proto"],
    srcs_version = "PY2AND3",
    visibility = [
        "//python:__subpackages__",
    ],
)

internal_php_proto_library(
    name = "conformance_php_proto",
    srcs = ["conformance.proto"],
    outs = [
        "Conformance/ConformanceRequest.php",
        "Conformance/ConformanceResponse.php",
        "Conformance/FailureSet.php",
        "Conformance/JspbEncodingConfig.php",
        "Conformance/TestCategory.php",
        "Conformance/WireFormat.php",
        "GPBMetadata/Conformance.php",
    ],
    visibility = [
        "//conformance:__pkg__",
        "//php:__pkg__",
    ],
)

cc_binary(
    name = "conformance_cpp",
    testonly = 1,
    srcs = ["conformance_cpp.cc"],
    visibility = [
        "//pkg:__pkg__",
        "//src:__subpackages__",
    ],
    deps = [
        ":conformance_cc_proto",
        "//:any_cc_proto",
        "//:api_cc_proto",
        "//:duration_cc_proto",
        "//:empty_cc_proto",
        "//:field_mask_cc_proto",
        "//:protobuf",
        "//:struct_cc_proto",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//:timestamp_cc_proto",
        "//:type_cc_proto",
        "//:wrappers_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//conformance/test_protos:test_messages_edition_unstable_cc_proto",
        "//editions:test_messages_proto2_editions_cc_proto",
        "//editions:test_messages_proto3_editions_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:port",
        "//src/google/protobuf:protobuf_lite",
        "//src/google/protobuf/json",
        "//src/google/protobuf/stubs",
        "//src/google/protobuf/util:json_util",
        "//src/google/protobuf/util:type_resolver",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

java_binary(
    name = "conformance_java",
    testonly = 1,
    srcs = ["ConformanceJava.java"],
    main_class = "ConformanceJava",
    visibility = [
        "//java:__subpackages__",
    ],
    deps = [
        ":conformance_java_proto",
        "//:protobuf_java",
        "//:protobuf_java_util",
        "//:test_messages_proto2_java_proto",
        "//:test_messages_proto3_java_proto",
        "//conformance/test_protos:test_messages_edition2023_java_proto",
        "//conformance/test_protos:test_messages_edition_unstable_java_proto",
        "//editions:test_messages_proto2_editions_java_proto",
        "//editions:test_messages_proto3_editions_java_proto",
    ],
)

java_binary(
    name = "conformance_java_lite",
    testonly = 1,
    srcs = ["ConformanceJavaLite.java"],
    main_class = "ConformanceJavaLite",
    visibility = [
        "//java:__subpackages__",
    ],
    deps = [
        ":conformance_java_proto_lite",
        "//:protobuf_java_util",
        "//:protobuf_javalite",
        "//:test_messages_proto2_java_proto_lite",
        "//:test_messages_proto3_java_proto_lite",
        "//conformance/test_protos:test_messages_edition2023_java_proto_lite",
        "//conformance/test_protos:test_messages_edition_unstable_java_proto_lite",
        "//editions:test_messages_proto2_editions_java_proto_lite",
        "//editions:test_messages_proto3_editions_java_proto_lite",
    ],
)

py_binary(
    name = "conformance_python",
    testonly = 1,
    srcs = ["conformance_python.py"],
    imports = [
        "..",
        "../python",
    ],
    srcs_version = "PY2AND3",
    visibility = ["//python:__subpackages__"],
    deps = [
        ":conformance_py_proto",
        "//:protobuf_python",
        "//conformance/test_protos:test_messages_edition2023_py_pb2",
        "//conformance/test_protos:test_messages_edition_unstable_py_pb2",
        "//editions:test_messages_proto2_editions_py_pb2",
        "//editions:test_messages_proto3_editions_py_pb2",
        "//python:_message",  # Make upb visible if we need it.
        "//python:conformance_test_py_proto",
    ],
)

py_binary(
    name = "update_failure_list",
    srcs = ["update_failure_list.py"],
)

inline_sh_binary(
    name = "conformance_php",
    testonly = 1,
    srcs = [
        "autoload.php",
        "conformance_php.php",
    ],
    cmd = """
        php -d include_path=php/generated:conformance:src/google/protobuf:editions/golden \\
            -d auto_prepend_file=$(rootpath autoload.php) \\
            $(rootpath conformance_php.php)
    """,
    visibility = ["//php:__subpackages__"],
    deps = [
        ":conformance_php_proto",
        "//:test_messages_proto3_php_proto",
        "//editions:test_messages_proto3_editions_php_proto",
        "//php:source_files",
    ],
)

inline_sh_binary(
    name = "conformance_php_c",
    testonly = 1,
    srcs = [
        "conformance_php.php",
        "//php:extension",
    ],
    cmd = """
        php -dextension=$(rootpath //php:extension) \\
            -d include_path=conformance:src/google/protobuf:editions/golden \\
            $(rootpath conformance_php.php)
    """,
    visibility = ["//php:__subpackages__"],
    deps = [
        ":conformance_php_proto",
        "//:test_messages_proto3_php_proto",
        "//editions:test_messages_proto3_editions_php_proto",
    ],
)

inline_sh_binary(
    name = "conformance_csharp",
    testonly = 1,
    srcs = ["//csharp/src/Google.Protobuf.Conformance:conformance_dll"],
    cmd = "dotnet $(rootpath //csharp/src/Google.Protobuf.Conformance:conformance_dll)",
    visibility = ["//csharp:__subpackages__"],
    deps = [
        "//conformance/test_protos:test_messages_edition2023_csharp_proto",
        "//conformance/test_protos:test_messages_edition_unstable_csharp_proto",
        "//csharp/src/Google.Protobuf.Conformance:conformance_runfiles",
    ],
)

objc_library(
    name = "conformance_objc_lib",
    testonly = 1,
    non_arc_srcs = ["conformance_objc.m"],
    # See https://github.com/bazelbuild/bazel/issues/12897.
    tags = ["manual"],
    deps = [
        ":conformance_objc_proto",
        "//:test_messages_proto2_objc_proto",
        "//:test_messages_proto3_objc_proto",
        "//conformance/test_protos:test_messages_edition2023_objc_proto",
        "//conformance/test_protos:test_messages_edition_unstable_objc_proto",
        "//editions:test_messages_proto2_editions_objc_proto",
        "//editions:test_messages_proto3_editions_objc_proto",
    ],
)

cc_binary(
    name = "conformance_objc",
    testonly = 1,
    # See https://github.com/bazelbuild/bazel/issues/12897.
    tags = ["manual"],
    visibility = ["//objectivec:__subpackages__"],
    deps = [":conformance_objc_lib"],
)
