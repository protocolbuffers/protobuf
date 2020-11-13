load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)
load(
    ":build_defs.bzl",
    "tmpl_cc_binary",
    "cc_optimizefor_proto_library",
    "expand_suffixes",
    "proto_library",
)

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
    tools = [":gen_synthetic_protos"],
    outs = [
        "100_msgs.proto",
        "200_msgs.proto",
        "100_fields.proto",
        "200_fields.proto",
    ],
    cmd = "$(execpath :gen_synthetic_protos) $(RULEDIR)",
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
    gen = ":gen_upb_binary_c",
    args = [
        package_name() + "/" + k + ".upb.h",
        "upb_benchmark_" + v,
    ],
    deps = [
        ":" + k + "_upb_proto",
    ],
),
tmpl_cc_binary(
    name = k + "_protobuf_binary",
    testonly = 1,
    gen = ":gen_protobuf_binary_cc",
    args = [
        package_name() + "/" + k + ".pb.h",
        "upb_benchmark::" + v,
    ],
    deps = [
        ":" + k + "_cc_proto",
    ],
),
cc_optimizefor_proto_library(
    srcs = [k + ".proto"],
    outs = [k + "_lite.proto"],
    name = k + "_cc_lite_proto",
    optimize_for = "LITE_RUNTIME",
),
tmpl_cc_binary(
    name = k + "_lite_protobuf_binary",
    testonly = 1,
    gen = ":gen_protobuf_binary_cc",
    args = [
        package_name() + "/" + k + "_lite.pb.h",
        "upb_benchmark::" + v,
    ],
    deps = [
        ":" + k + "_cc_lite_proto",
    ],
),
cc_optimizefor_proto_library(
    srcs = [k + ".proto"],
    outs = [k + "_codesize.proto"],
    name = k + "_cc_codesize_proto",
    optimize_for = "CODE_SIZE",
),
tmpl_cc_binary(
    name = k + "_codesize_protobuf_binary",
    testonly = 1,
    gen = ":gen_protobuf_binary_cc",
    args = [
        package_name() + "/" + k + "_codesize.pb.h",
        "upb_benchmark::" + v,
    ],
    deps = [
        ":" + k + "_cc_codesize_proto",
    ],
)
) for k, v in SIZE_BENCHMARKS.items()]

genrule(
    testonly = 1,
    name = "size_data",
    srcs = expand_suffixes(
        SIZE_BENCHMARKS.keys(),
        suffixes = ["_upb_binary", "_protobuf_binary", "_lite_protobuf_binary", "_codesize_protobuf_binary"],
    ),
    outs = ["size_data.txt"],
    # We want --format=GNU which counts rodata with data, not text.
    cmd = "size $$($$OSTYPE == 'linux-gnu' ? '--format=GNU -d' : '') $(SRCS) > $@",
    # "size" sometimes isn't available remotely.
    local = 1,
)
