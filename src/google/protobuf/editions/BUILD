load("@rules_cc//cc:defs.bzl", "cc_proto_library")

proto_library(
    name = "test_messages_proto2_proto",
    srcs = ["golden/test_messages_proto2.proto"],
    strip_import_prefix = "/src",
    deps = ["//src/google/protobuf:cpp_features_proto"],
)

proto_library(
    name = "test_messages_proto3_proto",
    srcs = ["golden/test_messages_proto3.proto"],
    strip_import_prefix = "/src",
    deps = [
        "//:any_proto",
        "//:duration_proto",
        "//:field_mask_proto",
        "//:struct_proto",
        "//:timestamp_proto",
        "//:wrappers_proto",
        "//src/google/protobuf:cpp_features_proto",
    ],
)

cc_proto_library(
    name = "test_messages_proto2_cc_proto",
    deps = [":test_messages_proto2_proto"],
)

cc_proto_library(
    name = "test_messages_proto3_cc_proto",
    deps = [":test_messages_proto3_proto"],
)

cc_test(
    name = "generated_files_test",
    srcs = ["generated_files_test.cc"],
    deps = [
        ":test_messages_proto2_cc_proto",
        ":test_messages_proto3_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:test_textproto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "generated_reflection_test",
    srcs = ["generated_reflection_test.cc"],
    deps = [
        ":test_messages_proto2_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
