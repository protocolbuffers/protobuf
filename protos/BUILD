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
    "//protos/bazel:upb_cc_proto_library.bzl",
    "upb_cc_proto_library_copts",
)

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

licenses(["notice"])

cc_library(
    name = "repeated_field",
    hdrs = [
        "repeated_field.h",
        "repeated_field_iterator.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":protos",
        ":protos_traits",
        "//:collections_internal",
        "//:message_copy",
        "//:mini_table",
        "//:port",
        "//:upb",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "protos",
    srcs = [
        "protos.cc",
    ],
    hdrs = [
        "protos.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//:message_copy",
        "//:message_promote",
        "//:mini_table",
        "//:upb",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

# Internally used type traits.
cc_library(
    name = "protos_traits",
    hdrs = [
        "protos_traits.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:private"],
)

cc_library(
    name = "protos_internal",
    hdrs = ["protos_internal.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":protos",
        "//:mini_table",
        "//:upb",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:str_format",
    ],
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_protos_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = [
        "protos_internal.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":protos",
        ":protos_internal",
        ":repeated_field",
    ],
)

cc_test(
    name = "protos_internal_test",
    srcs = ["protos_internal_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":protos_internal",
        "//:upb",
        "//protos_generator/tests:test_model_upb_cc_proto",
        "//protos_generator/tests:test_model_upb_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

upb_cc_proto_library_copts(
    name = "upb_cc_proto_library_copts__for_generated_code_only_do_not_use",
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
)

cc_test(
    name = "repeated_field_iterator_test",
    srcs = ["repeated_field_iterator_test.cc"],
    deps = [
        ":repeated_field",
        "@com_google_googletest//:gtest_main",
    ],
)
