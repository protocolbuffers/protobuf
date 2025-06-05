load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("//:protobuf.bzl", "internal_objc_proto_library", "internal_php_proto_library", "internal_py_proto_library")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:java_lite_proto_library.bzl", "java_lite_proto_library")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel:upb_proto_library.bzl", "upb_c_proto_library", "upb_proto_reflection_library")
load(":defaults.bzl", "compile_edition_defaults", "embed_edition_defaults")

bzl_library(
    name = "defaults",
    srcs = ["defaults.bzl"],
    visibility = ["//visibility:public"],
    deps = ["//bazel/common:proto_info_bzl"],
)

# Aggregate all the features owned by the Protobuf repo.
compile_edition_defaults(
    name = "protobuf_defaults",
    testonly = True,
    srcs = [
        "//java/core:java_features_proto",
        "//src/google/protobuf:cpp_features_proto",
    ],
    maximum_edition = "2023",
    minimum_edition = "2023",
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

embed_edition_defaults(
    name = "embed_test_defaults_base64",
    testonly = True,
    defaults = ":test_defaults_2023",
    encoding = "base64",
    output = "defaults_test_embedded_base64.h",
    placeholder = "DEFAULTS_VALUE",
    template = "defaults_test_embedded_base64.h.template",
)

cc_binary(
    name = "internal_defaults_escape",
    srcs = ["internal_defaults_escape.cc"],
    malloc = "@bazel_tools//tools/cpp:malloc",
    # This needs to be public for users of embed_edition_defaults.
    visibility = ["//visibility:public"],
    deps = [
        "//src/google/protobuf",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "defaults_test_embedded",
    hdrs = [
        "defaults_test_embedded.h",
        "defaults_test_embedded_base64.h",
    ],
)

cc_test(
    name = "defaults_test",
    srcs = ["defaults_test.cc"],
    data = [
        ":protobuf_defaults",
        ":test_defaults_2023",
        ":test_defaults_far_future",
        ":test_defaults_future",
    ],
    deps = [
        ":defaults_test_embedded",
        "//java/core:java_features_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:cpp_features_cc_proto",
        "//src/google/protobuf:port",
        "//src/google/protobuf:protobuf_lite",
        "//src/google/protobuf:test_textproto",
        "//src/google/protobuf:unittest_features_cc_proto",
        "//src/google/protobuf/stubs",
        "//src/google/protobuf/testing",
        "//src/google/protobuf/testing:file",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@bazel_tools//tools/cpp/runfiles",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_messages_proto2_editions_proto",
    testonly = True,
    srcs = ["golden/test_messages_proto2_editions.proto"],
)

cc_proto_library(
    name = "test_messages_proto2_editions_cc_proto",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto2_editions_proto"],
)

internal_objc_proto_library(
    name = "test_messages_proto2_editions_objc_proto",
    testonly = True,
    srcs = ["golden/test_messages_proto2_editions.proto"],
    visibility = ["//conformance:__pkg__"],
)

java_proto_library(
    name = "test_messages_proto2_editions_java_proto",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto2_editions_proto"],
)

java_lite_proto_library(
    name = "test_messages_proto2_editions_java_proto_lite",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto2_editions_proto"],
)

internal_py_proto_library(
    name = "test_messages_proto2_editions_py_pb2",
    testonly = True,
    srcs = ["golden/test_messages_proto2_editions.proto"],
    srcs_version = "PY2AND3",
    visibility = [
        "//conformance:__pkg__",
    ],
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

internal_objc_proto_library(
    name = "test_messages_proto3_editions_objc_proto",
    testonly = True,
    srcs = ["golden/test_messages_proto3_editions.proto"],
    includes = [
        ".",
        "src",
    ],
    proto_deps = ["//:well_known_type_protos"],
    visibility = ["//conformance:__pkg__"],
)

java_proto_library(
    name = "test_messages_proto3_editions_java_proto",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto3_editions_proto"],
)

java_lite_proto_library(
    name = "test_messages_proto3_editions_java_proto_lite",
    testonly = True,
    visibility = ["//conformance:__pkg__"],
    deps = [":test_messages_proto3_editions_proto"],
)

internal_php_proto_library(
    name = "test_messages_proto3_editions_php_proto",
    testonly = 1,
    srcs = ["golden/test_messages_proto3_editions.proto"],
    outs = [
        "GPBMetadata/TestMessagesProto3Editions.php",
        "Protobuf_test_messages/Editions/Proto3/EnumOnlyProto3.php",
        "Protobuf_test_messages/Editions/Proto3/EnumOnlyProto3/PBBool.php",
        "Protobuf_test_messages/Editions/Proto3/ForeignEnum.php",
        "Protobuf_test_messages/Editions/Proto3/ForeignMessage.php",
        "Protobuf_test_messages/Editions/Proto3/NullHypothesisProto3.php",
        "Protobuf_test_messages/Editions/Proto3/TestAllTypesProto3.php",
        "Protobuf_test_messages/Editions/Proto3/TestAllTypesProto3/AliasedEnum.php",
        "Protobuf_test_messages/Editions/Proto3/TestAllTypesProto3/NestedEnum.php",
        "Protobuf_test_messages/Editions/Proto3/TestAllTypesProto3/NestedMessage.php",
    ],
    enable_editions = True,
    includes = [
        "golden",
        "src",
    ],
    proto_deps = ["//:well_known_type_protos"],
    visibility = ["//conformance:__pkg__"],
)

internal_py_proto_library(
    name = "test_messages_proto3_editions_py_pb2",
    testonly = True,
    srcs = ["golden/test_messages_proto3_editions.proto"],
    srcs_version = "PY2AND3",
    visibility = [
        "//conformance:__pkg__",
    ],
    deps = ["//python:well_known_types_py_pb2"],
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

# Export these for conformance tests for ruby and C# codegen.
exports_files(
    [
        "golden/test_messages_proto2_editions.proto",
        "golden/test_messages_proto3_editions.proto",
    ],
    visibility = [
        "//ruby:__pkg__",
        "//src/google/protobuf/csharp:__pkg__",
    ],
)

proto_library(
    name = "test_editions_default_features_proto",
    testonly = True,
    srcs = ["proto/test_editions_default_features.proto"],
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
        "//src/google/protobuf",
        "//src/google/protobuf:test_textproto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "generated_reflection_test",
    srcs = ["generated_reflection_test.cc"],
    deps = [
        ":test_messages_proto2_editions_cc_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# Define a test suite to allow us to trigger these tests from //src expansions.
test_suite(name = "all_tests")
