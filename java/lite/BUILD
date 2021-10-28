load("@bazel_skylib//rules:build_test.bzl", "build_test")
load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain")
load("//:internal.bzl", "conformance_test")
load("//java/internal:testing.bzl", "junit_tests")

exports_files(
    ["lite.awk"],
    visibility = ["//java/core:__pkg__"],
)

exports_files(
    ["pom_template.xml"],
    visibility = ["//java/core:__pkg__"],
)

alias(
    name = "lite",
    actual = "//java/core:lite",
    visibility = ["//visibility:public"],
)

proto_lang_toolchain(
    name = "toolchain",
    command_line = "--java_out=lite:$(OUT)",
    runtime = ":lite",
    visibility = ["//visibility:public"],
)

test_suite(
    name = "tests",
    tests = [
        "conformance_test",
        "lite_build_test",
        "lite_tests",
        "//java/core:lite_tests",
    ],
)

build_test(
    name = "lite_build_test",
    targets = [
        ":lite",
    ],
)

conformance_test(
    name = "conformance_test",
    failure_list = "//:conformance/failure_list_java_lite.txt",
    testee = "//:conformance_java_lite",
    text_format_failure_list = "//:conformance/text_format_failure_list_java_lite.txt",
)

junit_tests(
    name = "lite_tests",
    size = "small",
    srcs = glob(["src/test/**/*.java"]),
    deps = [
        ":lite",
        "//java/core:generic_test_protos_java_proto_lite",
        "//java/core:java_test_protos_java_proto_lite",
        "//java/core:test_util_lite",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)
