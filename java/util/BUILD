load("@rules_java//java:defs.bzl", "java_library", "java_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("//java/internal:testing.bzl", "junit_tests")

java_library(
    name = "util",
    srcs = glob([
        "src/main/java/com/google/protobuf/util/*.java",
    ]),
    visibility = ["//visibility:public"],
    deps = [
        "//external:error_prone_annotations",
        "//external:gson",
        "//external:guava",
        "//java/core",
        "//java/lite",
    ],
)

proto_library(
    name = "test_protos",
    srcs = glob(["src/test/proto/**/*.proto"]),
    deps = [
        "//:any_proto",
        "//:duration_proto",
        "//:field_mask_proto",
        "//:struct_proto",
        "//:timestamp_proto",
        "//:wrappers_proto",
    ],
)

java_proto_library(
    name = "test_protos_java_proto",
    deps = [":test_protos"],
)

junit_tests(
    name = "tests",
    srcs = glob(["src/test/java/**/*.java"]),
    package_name = "com.google.protobuf.util",
    deps = [
        ":test_protos_java_proto",
        ":util",
        "//external:guava",
        "//external:junit",
        "//external:truth",
        "//java/core",
        "//java/core:generic_test_protos_java_proto",
    ],
)
