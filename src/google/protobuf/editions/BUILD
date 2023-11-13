load("@rules_python//python:proto.bzl", "py_proto_library")
load("//bazel:upb_proto_library.bzl", "upb_c_proto_library", "upb_proto_reflection_library")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load(":defaults.bzl", "compile_edition_defaults", "embed_edition_defaults")

bzl_library(
    name = "defaults",
    srcs = ["defaults.bzl"],
    visibility = ["//visibility:public"],
)

compile_edition_defaults(
    name = "test_defaults_2023",
    testonly = True,
    srcs = ["//src/google/protobuf:unittest_features_proto"],
    maximum_edition = "2023",
    minimum_edition = "2023",
)

compile_edition_defaults(
    name = "test_defaults_future",
    testonly = True,
    srcs = ["//src/google/protobuf:unittest_features_proto"],
    maximum_edition = "99997_TEST_ONLY",
    minimum_edition = "2023",
)

compile_edition_defaults(
    name = "test_defaults_far_future",
    testonly = True,
    srcs = ["//src/google/protobuf:unittest_features_proto"],
    maximum_edition = "99999_TEST_ONLY",
    minimum_edition = "99997_TEST_ONLY",
)

embed_edition_defaults(
    name = "embed_test_defaults",
    testonly = True,
    defaults = ":test_defaults_2023",
    output = "defaults_test_embedded.h",
    placeholder = "DEFAULTS_VALUE",
    template = "defaults_test_embedded.h.template",
)

cc_binary(
    name = "internal_defaults_escape",
    srcs = ["internal_defaults_escape.cc"],
    # This needs to be public for users of embed_edition_defaults.
    visibility = ["//visibility:public"],
    deps = [
        "//src/google/protobuf",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "defaults_test_embedded",
    hdrs = [
        "defaults_test_embedded.h",
    ],
    strip_include_prefix = "/src",
)

cc_test(
    name = "defaults_test",
    srcs = ["defaults_test.cc"],
    data = [
        ":test_defaults_2023",
        ":test_defaults_far_future",
        ":test_defaults_future",
    ],
    deps = [
        ":defaults_test_embedded",
        "//src/google/protobuf",
        "//src/google/protobuf:unittest_features_cc_proto",
        "//src/google/protobuf/stubs",
        "//src/google/protobuf/testing",
        "@bazel_tools//tools/cpp/runfiles",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_messages_proto2_editions_proto",
    testonly = True,
    srcs = ["golden/test_messages_proto2_editions.proto"],
    strip_import_prefix = "/src",
)

cc_proto_library(
    name = "test_messages_proto2_editions_cc_proto",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto2_editions_proto"],
)

py_proto_library(
    name = "test_messages_proto2_editions_py_pb2",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto2_editions_proto"],
)

upb_c_proto_library(
    name = "test_messages_proto2_editions_upb_proto",
    testonly = 1,
    deps = [":test_messages_proto2_editions_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto2_editions_upbdefs",
    testonly = 1,
    visibility = ["//upb/conformance:__pkg__"],
    deps = [":test_messages_proto2_editions_proto"],
)

proto_library(
    name = "test_messages_proto3_editions_proto",
    testonly = True,
    srcs = ["golden/test_messages_proto3_editions.proto"],
    strip_import_prefix = "/src",
    deps = [
        "//:any_proto",
        "//:duration_proto",
        "//:field_mask_proto",
        "//:struct_proto",
        "//:timestamp_proto",
        "//:wrappers_proto",
    ],
)

cc_proto_library(
    name = "test_messages_proto3_editions_cc_proto",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto3_editions_proto"],
)

py_proto_library(
    name = "test_messages_proto3_editions_py_pb2",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto3_editions_proto"],
)

upb_c_proto_library(
    name = "test_messages_proto3_editions_upb_proto",
    testonly = 1,
    deps = [":test_messages_proto3_editions_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto3_editions_upbdefs",
    testonly = 1,
    visibility = ["//upb/conformance:__pkg__"],
    deps = ["test_messages_proto3_editions_proto"],
)

# Export these for conformance tests until we support py_proto_library.
exports_files(
    [
        "golden/test_messages_proto2_editions.proto",
        "golden/test_messages_proto3_editions.proto",
    ],
    visibility = [
        "//python:__pkg__",
    ],
)

proto_library(
    name = "test_editions_default_features_proto",
    testonly = True,
    srcs = ["proto/test_editions_default_features.proto"],
    strip_import_prefix = "/src",
)

cc_proto_library(
    name = "test_editions_default_features_cc_proto",
    testonly = True,
    deps = [":test_editions_default_features_proto"],
)

cc_test(
    name = "generated_files_test",
    srcs = ["generated_files_test.cc"],
    deps = [
        ":test_editions_default_features_cc_proto",
        ":test_messages_proto2_editions_cc_proto",
        ":test_messages_proto3_editions_cc_proto",
        "//:protobuf",
        "//src/google/protobuf:test_textproto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "generated_reflection_test",
    srcs = ["generated_reflection_test.cc"],
    deps = [
        ":test_messages_proto2_editions_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
