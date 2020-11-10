load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)
load(
    ":build_defs.bzl",
    "tmpl_cc_binary",
    "cc_lite_proto_library",
)

licenses(["notice"])

proto_library(
    name = "benchmark_descriptor_proto",
    srcs = ["descriptor.proto"],
)

upb_proto_library(
    name = "benchmark_descriptor_upb_proto",
    deps = [":benchmark_descriptor_proto"],
)

upb_proto_reflection_library(
    name = "benchmark_descriptor_upb_proto_reflection",
    deps = [":benchmark_descriptor_proto"],
)

upb_proto_reflection_library(
    name = "ads_upb_proto_reflection",
    deps = ["@com_google_googleapis//:ads_proto"],
)

cc_proto_library(
    name = "benchmark_descriptor_cc_proto",
    deps = [":benchmark_descriptor_proto"],
)

proto_library(
    name = "benchmark_descriptor_sv_proto",
    srcs = ["descriptor_sv.proto"],
)

cc_proto_library(
    name = "benchmark_descriptor_sv_cc_proto",
    deps = [":benchmark_descriptor_sv_proto"],
)

cc_binary(
    name = "benchmark",
    testonly = 1,
    srcs = ["benchmark.cc"],
    deps = [
        ":ads_upb_proto_reflection",
        ":benchmark_descriptor_cc_proto",
        ":benchmark_descriptor_sv_cc_proto",
        ":benchmark_descriptor_upb_proto",
        ":benchmark_descriptor_upb_proto_reflection",
        "//:descriptor_upb_proto",
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
    "100_msgs": "Message99",
}

py_binary(
    name = "gen_benchmark_proto",
    srcs = ["gen_benchmark_proto.py"],
)

genrule(
    name = "gen_100_msgs",
    tools = [":gen_benchmark_proto"],
    outs = ["100_msgs.proto"],
    cmd = "$(execpath :gen_benchmark_proto) $@",
)

[(
proto_library(
    name = k + "_proto",
    srcs = [k + ".proto"],
),
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
    srcs = ["upb_binary.c.tmpl"],
    replacements = {
        "PROTO": "upb_benchmark_" + v,
        "INCLUDE": "benchmarks/" + k + ".upb.h",
    },
    deps = [
        ":" + k + "_upb_proto",
    ],
),
tmpl_cc_binary(
    name = k + "_protobuf_binary",
    testonly = 1,
    srcs = ["protobuf_binary.cc.tmpl"],
    replacements = {
        "PROTO": "upb_benchmark::" + v,
        "INCLUDE": "benchmarks/" + k + ".pb.h",
    },
    deps = [
        ":" + k + "_cc_proto",
    ],
),
cc_lite_proto_library(
    srcs = [k + ".proto"],
    outs = [k + "_lite.proto"],
    name = k + "_cc_lite_proto",
),
tmpl_cc_binary(
    name = k + "_lite_protobuf_binary",
    testonly = 1,
    srcs = ["protobuf_binary.cc.tmpl"],
    replacements = {
        "PROTO": "upb_benchmark::" + v,
        "INCLUDE": "benchmarks/" + k + "_lite.pb.h",
    },
    deps = [
        ":" + k + "_cc_lite_proto",
    ],
)) for k, v in SIZE_BENCHMARKS.items()]

