# Copyright (c) 2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Protobuf conformance tests.

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
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
load("//conformance:conformance.bzl", "conformance_suite", "conformance_test", "failure_lists")

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
# with one command line and one verdict.  See conformance_test_main.cc.
cc_library(
    name = "conformance_test_main",
    testonly = 1,
    srcs = ["conformance_test_main.cc"],
    deps = [
        # The gtest-based conformance suites, linked in as they are migrated.
        # The performance suite is linked from the start, so that the runner's
        # --performance selects its tests (see PerformanceGtestFilter()) as
        # soon as there are any.
        ":binary_conformance_tests",  # buildcleaner: keep
        ":json_conformance_tests",  # buildcleaner: keep
        ":performance_conformance_tests",  # buildcleaner: keep
        ":binary_json_conformance_suite",
        ":conformance_test_lib",
        ":filtering_test_runner",
        ":fork_pipe_runner",
        ":recording_test_runner",
        ":test_environment",
        ":test_manager",
        ":test_runner",
        ":text_conformance_tests",  # buildcleaner: keep
        ":text_format_conformance_suite",
        "//src/google/protobuf",
        "@googletest//:gtest",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@abseil-cpp//absl/types:span",
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
        "conformance_cpp_harness_test",
        "conformance_test_runner_test",
        "cpp_in_process_test_runner_test",
        "cpp_in_process_testee_test",
        "failure_list_trie_node_test",
        "filtering_test_runner_test",
        "fork_pipe_runner_test",
        "forked_testee_test",
        "in_process_test_runner_test",
        "in_process_testee_test",
        "matchers_test",
        "message_type_fixtures_test",
        "naming_test",
        "recording_test_runner_test",
        "recursion_limit_payloads_test",
        "test_environment_flags_test",
        "test_environment_test",
        "test_manager_test",
        "testee_test",
        "//conformance/bzl_tests:conformance_bzl_test",
    ],
)

cc_library(
    name = "binary_json_conformance_suite",
    testonly = 1,
    srcs = ["binary_json_conformance_suite.cc"],
    hdrs = ["binary_json_conformance_suite.h"],
    deps = [
        ":binary_test_util",
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
        "@abseil-cpp//absl/strings:string_view",
        "@jsoncpp",
    ],
)

# Helpers shared by the gtest-based binary conformance tests (field lookups,
# wire types, test-message parameter sets).
cc_library(
    name = "binary_test_util",
    testonly = 1,
    srcs = ["binary_test_util.cc"],
    hdrs = ["binary_test_util.h"],
    deps = [
        ":binary_wireformat",
        ":naming",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//editions:test_messages_proto2_editions_cc_proto",
        "//editions:test_messages_proto3_editions_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:protobuf_lite",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
    ],
)

cc_test(
    name = "binary_test_util_test",
    srcs = ["binary_test_util_test.cc"],
    deps = [
        ":binary_test_util",
        ":binary_wireformat",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//conformance/test_protos:test_messages_edition_unstable_cc_proto",
        "//editions:test_messages_proto2_editions_cc_proto",
        "//editions:test_messages_proto3_editions_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:test_textproto",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "recursion_limit_payloads",
    testonly = 1,
    srcs = ["recursion_limit_payloads.cc"],
    hdrs = ["recursion_limit_payloads.h"],
    deps = [
        ":binary_wireformat",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "recursion_limit_payloads_test",
    srcs = ["recursion_limit_payloads_test.cc"],
    deps = [
        ":binary_wireformat",
        ":recursion_limit_payloads",
        "//:test_messages_proto2_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "message_type_fixtures",
    testonly = 1,
    srcs = ["message_type_fixtures.cc"],
    hdrs = ["message_type_fixtures.h"],
    deps = [
        ":binary_test_util",
        ":test_environment",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//src/google/protobuf",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "message_type_fixtures_test",
    srcs = ["message_type_fixtures_test.cc"],
    deps = [
        ":binary_test_util",
        ":message_type_fixtures",
        ":test_environment",
        ":test_runner",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//editions:test_messages_proto2_editions_cc_proto",
        "//src/google/protobuf",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The packages using conformance_test() (conformance.bzl), which links the
# suite libraries below, :test_environment_main and :forked_testee into each
# test.  Add a package here when it moves its conformance test to the macro.
package_group(
    name = "conformance_test_users",
    packages = [
        "//conformance/...",
    ],
)

# The gtest-based binary conformance suite (:binary_conformance_tests), one
# file per migrated group.  A single alwayslink library so that linking it into
# conformance_test_runner (or any binary using test_environment_main) registers
# every test; each newly migrated group just adds its file here.
conformance_suite(
    name = "binary",
    srcs = [
        "binary_delimited_field_test.cc",
        "binary_illegal_tags_test.cc",
        "binary_merge_test.cc",
        "binary_message_set_test.cc",
        "binary_oneof_zero_test.cc",
        "binary_premature_eof_test.cc",
        "binary_unknown_fields_test.cc",
        "binary_unknown_wire_type_test.cc",
        "binary_unmatched_group_test.cc",
        "binary_unstable_edition_test.cc",
        "binary_utf8_test.cc",
    ],
    # conformance_test() links :binary_conformance_tests into every test it
    # creates, so it must be visible wherever that macro is used (see
    # :conformance_test_users; a package moving its conformance test to the
    # macro is added there).
    visibility = [":conformance_test_users"],
    deps = [
        ":binary_test_util",
        ":binary_wireformat",
        ":matchers",
        ":message_type_fixtures",
        ":test_environment",
        ":testee",
        "//:test_messages_proto2_cc_proto",
        "//conformance/test_protos:test_messages_edition_unstable_cc_proto",
        "//src/google/protobuf",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

# The gtest-based performance conformance suite
# (:performance_conformance_tests): the tests that stress the testee with
# deeply nested and very large payloads, for every wire format that has them.
# Every fixture in it must be named `*PerformanceTest`; see conformance.bzl.
conformance_suite(
    name = "performance",
    srcs = [
        "binary_performance_test.cc",
        "binary_recursion_limit_test.cc",
    ],
    visibility = [":conformance_test_users"],
    deps = [
        ":binary_test_util",
        ":binary_wireformat",
        ":matchers",
        ":message_type_fixtures",
        ":recursion_limit_payloads",
        ":test_environment",
        ":testee",
        "//:test_messages_proto2_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//src/google/protobuf",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

# The gtest-based text-format conformance suite (:text_conformance_tests); see
# the binary suite above.
conformance_suite(
    name = "text",
    srcs = [
        "text_enum_test.cc",
        "text_hello_world_test.cc",
    ],
    # conformance_test() links :text_conformance_tests into every test it
    # creates, so it must be visible wherever that macro is used.  (Kept to
    # this package tree so that it survives the open-source export unchanged;
    # widen, e.g. for //third_party/upb, as users outside it appear.)
    visibility = ["//conformance:__subpackages__"],
    deps = [
        ":binary_test_util",
        ":matchers",
        ":message_type_fixtures",
        ":test_environment",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
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

# The gtest-based JSON conformance suite (:json_conformance_tests); see the
# binary suite above.
conformance_suite(
    name = "json",
    srcs = ["json_hello_world_test.cc"],
    # conformance_test() links :json_conformance_tests into every test it
    # creates, so it must be visible wherever that macro is used (see the
    # binary suite above).
    visibility = ["//conformance:__subpackages__"],
    deps = [
        ":binary_test_util",
        ":matchers",
        ":message_type_fixtures",
        ":test_environment",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "test_runner",
    hdrs = ["test_runner.h"],
    # An in-process testee library (testee_runner.h) subclasses
    # ConformanceTestRunner, so packages that define one need this directly.
    visibility = [":conformance_test_users"],
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
    name = "in_process_test_runner",
    testonly = 1,
    srcs = ["in_process_test_runner.cc"],
    hdrs = ["in_process_test_runner.h"],
    deps = [
        ":conformance_cc_proto",
        ":test_runner",
        "@abseil-cpp//absl/functional:any_invocable",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_test(
    name = "in_process_test_runner_test",
    srcs = ["in_process_test_runner_test.cc"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":in_process_test_runner",
        "//src/google/protobuf:test_textproto",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The request-handling logic of the C++ conformance testee, shared by the
# conformance_cpp binary and by in-process conformance tests.
cc_library(
    name = "conformance_cpp_harness",
    testonly = 1,
    srcs = ["conformance_cpp_harness.cc"],
    hdrs = ["conformance_cpp_harness.h"],
    deps = [
        ":conformance_cc_proto",
        "//:any_cc_proto",
        "//:api_cc_proto",
        "//:duration_cc_proto",
        "//:empty_cc_proto",
        "//:field_mask_cc_proto",
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
        "//src/google/protobuf/json",
        "//src/google/protobuf/util:json_util",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "conformance_cpp_harness_test",
    srcs = ["conformance_cpp_harness_test.cc"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":conformance_cpp_harness",
        "//:test_messages_proto3_cc_proto",
        "//src/google/protobuf:test_textproto",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# Glue between the C++ testee (conformance_cpp_harness) and the tester side
# (in_process_test_runner). Kept separate so conformance_cpp links no runner
# code.
cc_library(
    name = "cpp_in_process_test_runner",
    testonly = 1,
    srcs = ["cpp_in_process_test_runner.cc"],
    hdrs = ["cpp_in_process_test_runner.h"],
    deps = [
        ":conformance_cc_proto",
        ":conformance_cpp_harness",
        ":in_process_test_runner",
        ":test_runner",
    ],
)

cc_test(
    name = "cpp_in_process_test_runner_test",
    srcs = ["cpp_in_process_test_runner_test.cc"],
    deps = [
        ":conformance_cc_proto",
        ":conformance_cpp_harness",
        ":cpp_in_process_test_runner",
        ":test_runner",
        "//:test_messages_proto3_cc_proto",
        "//src/google/protobuf:test_textproto",
        "//src/google/protobuf/util:differencer",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/status:statusor",
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
        "//src/google/protobuf/json",
        "//src/google/protobuf/util:differencer",
        "@abseil-cpp//absl/base:nullability",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
        "@jsoncpp",
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
        "@jsoncpp",
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

# Defines the global --testee_binary, --failure_list, --fix, ... flags of a
# conformance test binary.  Only link it into binaries with a
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

cc_test(
    name = "test_environment_flags_test",
    srcs = ["test_environment_flags_test.cc"],
    deps = [
        ":test_environment",
        ":test_environment_flags",
        "//src/google/protobuf",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/flags:reflection",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The main() for gtest-based conformance suites.  Depend on this instead of a
# generic gtest main, together with exactly one library defining
# MakeTesteeRunner() (see :testee_runner): :forked_testee, or an in-process
# testee.  conformance_test() (conformance.bzl) adds them as implicit deps, so
# they must be visible wherever that macro is used (see
# :conformance_test_users).
cc_library(
    name = "test_environment_main",
    testonly = 1,
    srcs = ["test_environment_main.cc"],
    visibility = [":conformance_test_users"],
    deps = [
        ":test_environment",
        ":test_environment_flags",
        ":testee_runner",
        "@abseil-cpp//absl/flags:parse",  # buildcleaner: keep
        "@abseil-cpp//absl/types:span",
        "@googletest//:gtest",
    ],
    alwayslink = 1,
)

# The link-time seam through which test_environment_main gets its testee: the
# declaration of MakeTesteeRunner(), which an in-process testee library defines.
cc_library(
    name = "testee_runner",
    testonly = 1,
    hdrs = ["testee_runner.h"],
    visibility = [":conformance_test_users"],
    deps = [
        ":test_runner",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
    ],
)

# The default testee: --testee_binary, spawned as a child process.  Like every
# library defining MakeTesteeRunner() it is alwayslink, so that the definition
# is linked in wherever the library appears on the link line.
cc_library(
    name = "forked_testee",
    testonly = 1,
    srcs = ["forked_testee.cc"],
    visibility = [":conformance_test_users"],
    deps = [
        ":fork_pipe_runner",
        ":test_runner",
        ":testee_runner",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
    ],
    alwayslink = 1,
)

# The C++ runtime's in-process testee: defines MakeTesteeRunner() on top of
# cpp_in_process_test_runner.  conformance_test(in_process = True) links it in
# place of :forked_testee; see conformance.bzl for the one-per-test rule.
cc_library(
    name = "cpp_in_process_testee",
    testonly = 1,
    srcs = ["cpp_in_process_testee.cc"],
    deps = [
        ":cpp_in_process_test_runner",
        ":test_runner",
        ":testee_runner",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
    ],
    alwayslink = 1,
)

cc_test(
    name = "cpp_in_process_testee_test",
    srcs = ["cpp_in_process_testee_test.cc"],
    deps = [
        ":conformance_cc_proto",
        ":cpp_in_process_testee",
        ":test_runner",
        ":testee_runner",
        "//:test_messages_proto3_cc_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "forked_testee_test",
    srcs = ["forked_testee_test.cc"],
    deps = [
        ":forked_testee",
        ":test_runner",
        ":testee_runner",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# A conformance test binary whose testee is linked in (the test file defines
# MakeTesteeRunner() itself), run without --testee_binary: checks that
# test_environment_main hosts an in-process testee end to end.
cc_test(
    name = "in_process_testee_test",
    srcs = ["in_process_testee_test.cc"],
    deps = [
        ":binary_wireformat",
        ":conformance_cc_proto",
        ":matchers",
        ":test_environment",
        ":test_environment_main",
        ":test_runner",
        ":testee_runner",
        "//:test_messages_proto3_cc_proto",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
        "@googletest//:gtest",
    ],
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
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:optional",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

bzl_library(
    name = "conformance_bzl",
    srcs = ["conformance.bzl"],
    visibility = ["//conformance/bzl_tests:__pkg__"],
    deps = [
        "@rules_cc//cc:core_rules",
    ],
)

# The expected-failure lists of the conformance_test()s below, one file per
# test and suite (e.g. failure_lists/cpp_binary.txt).  Update them with
# `blaze run :<test>_<suite>_test -- --fix`; see conformance.bzl.
failure_lists(name = "failure_lists")

# The gtest-based conformance tests of the C++ testee: cpp_binary_test,
# cpp_text_test and cpp_performance_test (and one more per suite as they are
# migrated), grouped under the :cpp test_suite.  They host the testee
# in-process (:cpp_in_process_testee) rather than forking :conformance_cpp, so a
# testee crash or log shows up in the test itself.
# They run alongside the legacy conformance_cpp_test, which stays the
# authoritative test until the migration completes (the merged
# conformance_test_runner behind it also runs these suites).
# TODO: b/563657875 - drop conformance_cpp_test once every suite is migrated.
conformance_test(
    name = "cpp",
    failure_lists = ":failure_lists",
    in_process = True,
    maximum_edition = "2023",
    testee = ":cpp_in_process_testee",
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
        ":conformance_cpp_harness",
        "//:protobuf",
        "//src/google/protobuf",
        "//src/google/protobuf:endian",
        "//src/google/protobuf/stubs",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
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
