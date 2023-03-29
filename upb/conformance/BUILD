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
    "make_shell_script",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

licenses(["notice"])

config_setting(
    name = "windows",
    constraint_values = ["@platforms//os:windows"],
    visibility = ["//visibility:public"],
)

upb_proto_library(
    name = "conformance_upb_proto",
    testonly = 1,
    deps = ["@com_google_protobuf//conformance:conformance_proto"],
)

upb_proto_reflection_library(
    name = "conformance_upb_proto_reflection",
    testonly = 1,
    deps = ["@com_google_protobuf//conformance:conformance_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto2_upbdefs",
    testonly = 1,
    visibility = ["//:__subpackages__"],
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto2_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto3_upbdefs",
    testonly = 1,
    visibility = ["//:__subpackages__"],
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto3_proto"],
)

cc_binary(
    name = "conformance_upb",
    testonly = 1,
    srcs = ["conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS,
    data = ["conformance_upb_failures.txt"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_proto",
        ":conformance_upb_proto_reflection",
        ":test_messages_proto2_upbdefs",
        ":test_messages_proto3_upbdefs",
        "//:json",
        "//:port",
        "//:reflection",
        "//:textformat",
        "//:wire",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb",
    out = "test_conformance_upb.sh",
    contents = "external/com_google_protobuf/conformance/conformance_test_runner" +
               " --enforce_recommended" +
               " --failure_list upb/conformance/conformance_upb_failures.txt" +
               " upb/conformance/conformance_upb",
)

sh_test(
    name = "test_conformance_upb",
    srcs = ["test_conformance_upb.sh"],
    data = [
        "conformance_upb_failures.txt",
        ":conformance_upb",
        "@com_google_protobuf//conformance:conformance_test_runner",
    ],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = ["@bazel_tools//tools/bash/runfiles"],
)

cc_binary(
    name = "conformance_upb_dynamic_minitable",
    testonly = 1,
    srcs = ["conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS + [
        "-DREBUILD_MINITABLES",
    ],
    data = ["conformance_upb_failures.txt"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_proto",
        ":conformance_upb_proto_reflection",
        ":test_messages_proto2_upbdefs",
        ":test_messages_proto3_upbdefs",
        "//:json",
        "//:port",
        "//:reflection",
        "//:textformat",
        "//:wire",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb_dynamic_minitable",
    out = "test_conformance_upb_dynamic_minitable.sh",
    contents = "external/com_google_protobuf/conformance/conformance_test_runner" +
               " --enforce_recommended" +
               " --failure_list upb/conformance/conformance_upb_failures.txt" +
               " upb/conformance/conformance_upb_dynamic_minitable",
)

sh_test(
    name = "test_conformance_upb_dynamic_minitable",
    srcs = ["test_conformance_upb_dynamic_minitable.sh"],
    data = [
        "conformance_upb_failures.txt",
        ":conformance_upb_dynamic_minitable",
        "@com_google_protobuf//conformance:conformance_test_runner",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
