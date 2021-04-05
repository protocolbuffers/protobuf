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
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
    "make_shell_script",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

licenses(["notice"])

config_setting(
    name = "fuzz",
    values = {"define": "fuzz=true"},
)

cc_library(
    name = "upb_test",
    testonly = 1,
    srcs = [
        "testmain.cc",
    ],
    hdrs = [
        "upb_test.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        "//:port",
        "//:upb",
    ],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["test.proto"],
)

upb_proto_library(
    name = "test_upb_proto",
    testonly = 1,
    deps = [":test_proto"],
)

cc_test(
    name = "test_generated_code",
    srcs = ["test_generated_code.c"],
    copts = UPB_DEFAULT_COPTS,
    deps = [
        ":empty_upbdefs_proto",
        ":test_messages_proto3_proto_upb",
        ":test_upb_proto",
        ":upb_test",
    ],
)

proto_library(
    name = "empty_proto",
    srcs = ["empty.proto"],
)

upb_proto_reflection_library(
    name = "empty_upbdefs_proto",
    testonly = 1,
    deps = [":empty_proto"],
)

upb_proto_library(
    name = "test_messages_proto3_proto_upb",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto3_proto"],
)

proto_library(
    name = "test_cpp_proto",
    srcs = [
        "test_cpp.proto",
    ],
)

upb_proto_library(
    name = "test_cpp_upb_proto",
    deps = ["test_cpp_proto"],
)

upb_proto_reflection_library(
    name = "test_cpp_upb_proto_reflection",
    deps = ["test_cpp_proto"],
)

cc_test(
    name = "test_cpp",
    srcs = ["test_cpp.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":test_cpp_upb_proto",
        ":test_cpp_upb_proto_reflection",
        ":upb_test",
        "//:json",
        "//:port",
        "//:reflection",
        "//:upb",
    ],
)

cc_test(
    name = "test_table",
    srcs = ["test_table.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":upb_test",
        "//:port",
        "//:table",
        "//:upb",
    ],
)

# OSS-Fuzz test
cc_binary(
    name = "file_descriptor_parsenew_fuzzer",
    testonly = 1,
    srcs = ["file_descriptor_parsenew_fuzzer.cc"],
    copts = UPB_DEFAULT_CPPOPTS + select({
        "//conditions:default": [],
        ":fuzz": ["-fsanitize=fuzzer,address"],
    }),
    defines = select({
        "//conditions:default": [],
        ":fuzz": ["HAVE_FUZZER"],
    }),
    deps = [
        "//:descriptor_upb_proto",
        "//:upb",
    ],
)

upb_proto_library(
    name = "conformance_proto_upb",
    testonly = 1,
    deps = ["@com_google_protobuf//:conformance_proto"],
)

upb_proto_reflection_library(
    name = "conformance_proto_upbdefs",
    testonly = 1,
    deps = ["@com_google_protobuf//:conformance_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto2_upbdefs",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto2_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto3_upbdefs",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto3_proto"],
)

cc_binary(
    name = "conformance_upb",
    testonly = 1,
    srcs = [
        "conformance_upb.c",
    ],
    copts = UPB_DEFAULT_COPTS,
    data = [
        "conformance_upb_failures.txt",
    ],
    deps = [
        ":conformance_proto_upb",
        ":conformance_proto_upbdefs",
        ":test_messages_proto2_upbdefs",
        ":test_messages_proto3_upbdefs",
        "//:json",
        "//:port",
        "//:reflection",
        "//:textformat",
        "//:upb",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb",
    out = "test_conformance_upb.sh",
    contents = "external/com_google_protobuf/conformance_test_runner " +
               " --enforce_recommended " +
               " --failure_list ./tests/conformance_upb_failures.txt" +
               " ./tests/conformance_upb",
)

sh_test(
    name = "test_conformance_upb",
    srcs = ["test_conformance_upb.sh"],
    data = [
        "conformance_upb_failures.txt",
        ":conformance_upb",
        "@com_google_protobuf//:conformance_test_runner",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
