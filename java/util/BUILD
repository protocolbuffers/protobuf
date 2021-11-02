load("@rules_java//java:defs.bzl", "java_proto_library")
load("@rules_jvm_external//:defs.bzl", "java_export")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("//:protobuf_version.bzl", "PROTOBUF_VERSION")
load("//java/internal:testing.bzl", "junit_tests")

java_library(
    name = "util",
    srcs = glob([
        "src/main/java/com/google/protobuf/util/*.java",
    ]),
    visibility = ["//visibility:public"],
    deps = [
        "//java/core",
        "//java/lite",
        "@maven//:com_google_code_findbugs_jsr305",
        "@maven//:com_google_code_gson_gson",
        "@maven//:com_google_errorprone_error_prone_annotations",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_j2objc_j2objc_annotations",
    ],
)

# Bazel users, don't depend on this target, use :util.
java_export(
    name = "util_mvn",
    maven_coordinates = "com.google.protobuf:protobuf-java-util:%s" % PROTOBUF_VERSION,
    pom_template = "pom_template.xml",
    visibility = ["//java:__pkg__"],
    runtime_deps = [":util"],
)

filegroup(
    name = "release",
    srcs = [
        ":util_mvn-docs",
        ":util_mvn-maven-source",
        ":util_mvn-pom",
        ":util_mvn-project",
    ],
    visibility = ["//java:__pkg__"],
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
    package_name = "com.google.protobuf.util",
    srcs = glob(["src/test/java/**/*.java"]),
    deps = [
        ":test_protos_java_proto",
        ":util",
        "//java/core",
        "//java/core:generic_test_protos_java_proto",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)
