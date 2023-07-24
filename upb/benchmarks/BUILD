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

# begin:google_only
# load("@rules_cc//cc:defs.bzl", "cc_proto_library")
# end:google_only

load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)
load(
    ":build_defs.bzl",
    "cc_optimizefor_proto_library",
    "expand_suffixes",
    "proto_library",
    "tmpl_cc_binary",
)

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

licenses(["notice"])

proto_library(
    name = "descriptor_proto",
    srcs = ["descriptor.proto"],
)

upb_proto_library(
    name = "benchmark_descriptor_upb_proto",
    deps = [":descriptor_proto"],
)

upb_proto_reflection_library(
    name = "benchmark_descriptor_upb_proto_reflection",
    deps = [":descriptor_proto"],
)

upb_proto_reflection_library(
    name = "ads_upb_proto_reflection",
    deps = ["@com_google_googleapis//:ads_proto"],
)

cc_proto_library(
    name = "benchmark_descriptor_cc_proto",
    deps = [":descriptor_proto"],
)

proto_library(
    name = "benchmark_descriptor_sv_proto",
    srcs = ["descriptor_sv.proto"],
)

cc_proto_library(
    name = "benchmark_descriptor_sv_cc_proto",
    deps = [":benchmark_descriptor_sv_proto"],
)

cc_test(
    name = "benchmark",
    testonly = 1,
    srcs = ["benchmark.cc"],
    deps = [
        ":ads_upb_proto_reflection",
        ":benchmark_descriptor_cc_proto",
        ":benchmark_descriptor_sv_cc_proto",
        ":benchmark_descriptor_upb_proto",
        ":benchmark_descriptor_upb_proto_reflection",
        "//:base",
        "//:descriptor_upb_proto",
        "//:mem",
        "//:reflection",
        "@com_github_google_benchmark//:benchmark_main",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_protobuf//:protobuf",
    ],
)

# Size benchmarks.

SIZE_BENCHMARKS = {
    "empty": "Empty",
    "descriptor": "FileDescriptorSet",
    "100_msgs": "Message100",
    "200_msgs": "Message200",
    "100_fields": "Message",
    "200_fields": "Message",
}

py_binary(
    name = "gen_synthetic_protos",
    srcs = ["gen_synthetic_protos.py"],
    python_version = "PY3",
)

py_binary(
    name = "gen_upb_binary_c",
    srcs = ["gen_upb_binary_c.py"],
    python_version = "PY3",
)

py_binary(
    name = "gen_protobuf_binary_cc",
    srcs = ["gen_protobuf_binary_cc.py"],
    python_version = "PY3",
)

genrule(
    name = "do_gen_synthetic_protos",
    outs = [
        "100_msgs.proto",
        "200_msgs.proto",
        "100_fields.proto",
        "200_fields.proto",
    ],
    cmd = "$(execpath :gen_synthetic_protos) $(RULEDIR)",
    tools = [":gen_synthetic_protos"],
)

proto_library(
    name = "100_msgs_proto",
    srcs = ["100_msgs.proto"],
)

proto_library(
    name = "200_msgs_proto",
    srcs = ["200_msgs.proto"],
)

proto_library(
    name = "100_fields_proto",
    srcs = ["100_fields.proto"],
)

proto_library(
    name = "200_fields_proto",
    srcs = ["200_fields.proto"],
)

proto_library(
    name = "empty_proto",
    srcs = ["empty.proto"],
)

[(
    upb_proto_library(
        name = k + "_upb_proto",
        deps = [":" + k + "_proto"],
    ),
    cc_proto_library(
        name = k + "_cc_proto",
        deps = [":" + k + "_proto"],
    ),
    tmpl_cc_binary(
        name = k + "_upb_binary",
        testonly = 1,
        args = [
            package_name() + "/" + k + ".upb.h",
            "upb_benchmark_" + v,
        ],
        gen = ":gen_upb_binary_c",
        deps = [
            ":" + k + "_upb_proto",
        ],
    ),
    tmpl_cc_binary(
        name = k + "_protobuf_binary",
        testonly = 1,
        args = [
            package_name() + "/" + k + ".pb.h",
            "upb_benchmark::" + v,
        ],
        gen = ":gen_protobuf_binary_cc",
        deps = [
            ":" + k + "_cc_proto",
        ],
    ),
    cc_optimizefor_proto_library(
        name = k + "_cc_lite_proto",
        srcs = [k + ".proto"],
        outs = [k + "_lite.proto"],
        optimize_for = "LITE_RUNTIME",
    ),
    tmpl_cc_binary(
        name = k + "_lite_protobuf_binary",
        testonly = 1,
        args = [
            package_name() + "/" + k + "_lite.pb.h",
            "upb_benchmark::" + v,
        ],
        gen = ":gen_protobuf_binary_cc",
        deps = [
            ":" + k + "_cc_lite_proto",
        ],
    ),
    cc_optimizefor_proto_library(
        name = k + "_cc_codesize_proto",
        srcs = [k + ".proto"],
        outs = [k + "_codesize.proto"],
        optimize_for = "CODE_SIZE",
    ),
    tmpl_cc_binary(
        name = k + "_codesize_protobuf_binary",
        testonly = 1,
        args = [
            package_name() + "/" + k + "_codesize.pb.h",
            "upb_benchmark::" + v,
        ],
        gen = ":gen_protobuf_binary_cc",
        deps = [
            ":" + k + "_cc_codesize_proto",
        ],
    ),
) for k, v in SIZE_BENCHMARKS.items()]

genrule(
    name = "size_data",
    testonly = 1,
    srcs = expand_suffixes(
        SIZE_BENCHMARKS.keys(),
        suffixes = [
            "_upb_binary",
            "_protobuf_binary",
            "_lite_protobuf_binary",
            "_codesize_protobuf_binary",
        ],
    ),
    outs = ["size_data.txt"],
    # We want --format=GNU which counts rodata with data, not text.
    cmd = "size $$($$OSTYPE == 'linux-gnu' ? '--format=GNU -d' : '') $(SRCS) > $@",
    # "size" sometimes isn't available remotely.
    local = 1,
    tags = ["no-remote-exec"],
)
