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
        "test_util.h",
        "upb_test.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        "//:handlers",
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
    name = "test_decoder_proto",
    srcs = [
        "pb/test_decoder.proto",
    ],
)

upb_proto_reflection_library(
    name = "test_decoder_upb_proto",
    deps = [":test_decoder_proto"],
)

cc_test(
    name = "test_decoder",
    srcs = ["pb/test_decoder.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":test_decoder_upb_proto",
        ":upb_test",
        "//:handlers",
        "//:port",
        "//:upb",
        "//:upb_pb",
    ],
)

proto_library(
    name = "test_cpp_proto",
    srcs = [
        "test_cpp.proto",
    ],
)

upb_proto_reflection_library(
    name = "test_cpp_upb_proto",
    deps = ["test_cpp_proto"],
)

cc_test(
    name = "test_cpp",
    srcs = ["test_cpp.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":test_cpp_upb_proto",
        ":upb_test",
        "//:handlers",
        "//:port",
        "//:reflection",
        "//:upb",
        "//:upb_pb",
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

# copybara:strip_for_google3_begin
cc_test(
    name = "test_encoder",
    srcs = ["pb/test_encoder.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":upb_test",
        "//:descriptor_upb_proto",
        "//:descriptor_upb_proto_reflection",
        "//:upb",
        "//:upb_pb",
    ],
)

proto_library(
    name = "test_json_enum_from_separate",
    srcs = ["json/enum_from_separate_file.proto"],
    deps = [":test_json_proto"],
)

proto_library(
    name = "test_json_proto",
    srcs = ["json/test.proto"],
)

upb_proto_reflection_library(
    name = "test_json_upb_proto_reflection",
    deps = ["test_json_proto"],
)

upb_proto_library(
    name = "test_json_enum_from_separate_upb_proto",
    deps = [":test_json_enum_from_separate"],
)

upb_proto_library(
    name = "test_json_upb_proto",
    deps = [":test_json_proto"],
)

cc_test(
    name = "test_json",
    srcs = [
        "json/test_json.cc",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":test_json_upb_proto",
        ":test_json_upb_proto_reflection",
        ":upb_test",
        "//:upb_json",
    ],
)
# copybara:strip_end

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
