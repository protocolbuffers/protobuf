load("//java:testing.bzl", "junit_tests")
load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain")

exports_files(["lite.awk"], visibility = ["//java/core:__pkg__"])

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
        "lite_tests",
        "//java/core:lite_tests",
    ],
)

junit_tests(
    name = "lite_tests",
    srcs = glob(["src/test/**/*.java"]),
    size = "small",
    deps = [
        ":lite",
        "//external:junit",
        "//java/core:generic_test_protos_java_proto_lite",
        "//java/core:java_test_protos_java_proto_lite",
        "//java/core:test_util_lite",
    ],
)
