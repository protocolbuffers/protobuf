load("@rules_java//java:defs.bzl", "java_library", "java_proto_library", "java_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "test_protos",
    srcs = glob([
        "proto/**/*.proto",
    ]),
    strip_import_prefix = "proto",
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
    name = "test_java_protos",
    deps = [
        ":test_protos",
    ],
)

java_library(
    name = "test",
    srcs = glob(["java/**/*.java"]),
    deps = [
        ":test_java_protos",
        "//:any_java_proto",
        "//:unittest_java_proto",
        "//java/core",
        "//java/lite",
        "//java/util",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

[
    java_test(
        name = src[:-len(".java")],
        runtime_deps = [
            ":test",
        ],
    )
    for src in glob(["java/**/*Test.java"])
]
