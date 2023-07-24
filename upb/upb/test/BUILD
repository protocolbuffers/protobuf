# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

cc_library(
    name = "parse_text_proto",
    testonly = 1,
    hdrs = ["parse_text_proto.h"],
    visibility = ["//:__subpackages__"],
    deps = [
        "@com_google_googletest//:gtest",
        "@com_google_protobuf//:protobuf",
    ],
)

proto_library(
    name = "empty_proto",
    srcs = ["empty.proto"],
)

upb_proto_reflection_library(
    name = "empty_upb_proto_reflection",
    testonly = 1,
    deps = [":empty_proto"],
)

proto_library(
    name = "proto3_test_proto",
    testonly = 1,
    srcs = ["proto3_test.proto"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

upb_proto_library(
    name = "proto3_test_upb_proto",
    testonly = 1,
    deps = [":proto3_test_proto"],
)

upb_proto_reflection_library(
    name = "proto3_test_upb_proto_reflection",
    testonly = 1,
    deps = [":proto3_test_proto"],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["test.proto"],
)

upb_proto_library(
    name = "test_upb_proto",
    testonly = 1,
    visibility = ["//:__subpackages__"],
    deps = [":test_proto"],
)

proto_library(
    name = "test_cpp_proto",
    srcs = ["test_cpp.proto"],
    deps = ["@com_google_protobuf//:timestamp_proto"],
)

upb_proto_library(
    name = "test_cpp_upb_proto",
    deps = ["test_cpp_proto"],
)

upb_proto_reflection_library(
    name = "test_cpp_upb_proto_reflection",
    deps = ["test_cpp_proto"],
)

upb_proto_library(
    name = "test_messages_proto2_upb_proto",
    testonly = 1,
    visibility = ["//:__subpackages__"],
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto2_proto"],
)

upb_proto_library(
    name = "test_messages_proto3_upb_proto",
    testonly = 1,
    visibility = ["//:__subpackages__"],
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto3_proto"],
)

upb_proto_library(
    name = "timestamp_upb_proto",
    deps = ["@com_google_protobuf//:timestamp_proto"],
)

upb_proto_reflection_library(
    name = "timestamp_upb_proto_reflection",
    deps = ["@com_google_protobuf//:timestamp_proto"],
)

cc_library(
    name = "fuzz_util",
    testonly = 1,
    srcs = ["fuzz_util.cc"],
    hdrs = ["fuzz_util.h"],
    visibility = ["//:__subpackages__"],
    deps = [
        "//:upb",
        "//:mini_table",
        "//:mini_table_internal",
        "//:port",
        "//upb/mini_descriptor",
    ],
)

cc_test(
    name = "proto3_test",
    srcs = ["proto3_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":proto3_test_upb_proto",
        ":proto3_test_upb_proto_reflection",
        "//:reflection",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_cpp",
    srcs = ["test_cpp.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":test_cpp_upb_proto",
        ":test_cpp_upb_proto_reflection",
        ":timestamp_upb_proto",
        ":timestamp_upb_proto_reflection",
        "//:upb",
        "//:json",
        "//:port",
        "//:reflection",
        "//upb/json",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_generated_code",
    srcs = [
        "test_generated_code.cc",
    ],
    deps = [
        ":empty_upb_proto_reflection",
        ":test_messages_proto2_upb_proto",
        ":test_messages_proto3_upb_proto",
        ":test_upb_proto",
        "//:upb",
        "//:port",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_mini_table_oneof",
    srcs = [
        "test_mini_table_oneof.cc",
    ],
    deps = [
        ":empty_upb_proto_reflection",
        ":test_messages_proto2_upb_proto",
        ":test_messages_proto3_upb_proto",
        ":test_upb_proto",
        "//:upb",
        "//:mini_table",
        "//:mini_table_internal",
        "//:port",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "empty_srcs_proto",
    testonly = 1,
    srcs = [],
    deps = [":test_proto"],
)

proto_library(
    name = "empty_srcs_2_proto",
    testonly = 1,
    srcs = [],
    deps = [":test_cpp_proto"],
)

proto_library(
    name = "empty_srcs_3_proto",
    testonly = 1,
    srcs = [],
    deps = [
        ":empty_srcs_2_proto",
        ":empty_srcs_proto",
    ],
)

proto_library(
    name = "test_import_empty_srcs_proto",
    testonly = 1,
    srcs = ["test_import_empty_srcs.proto"],
    deps = [":empty_srcs_3_proto"],
)

upb_proto_library(
    name = "test_import_empty_srcs_upb_proto",
    testonly = 1,
    deps = [":test_import_empty_srcs_proto"],
)

cc_test(
    name = "test_import_empty_srcs",
    srcs = ["test_import_empty_srcs.cc"],
    deps = [
        ":test_import_empty_srcs_upb_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
