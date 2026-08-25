load("@rules_java//:test/java/testutil/mock_java_toolchain.bzl", "mock_java_toolchain")
load("@rules_java//java/toolchains:java_package_configuration.bzl", "java_package_configuration")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_lite_proto_library_tests/test_utils.bzl", "JAVA_LITE_PROTO_TESTING_ASPECT")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite", "package_label_string")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "input_proto",
    srcs = [
        "input1.proto",
        "input2.proto",
    ],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

package_group(
    name = "package_javacopts_packages",
    packages = ["//bazel/tests/java_lite_proto_library_tests/package_javac_opts/..."],
)

java_package_configuration(
    name = "package_javacopts",
    javacopts = ["-packageJavacopt"],
    packages = [":package_javacopts_packages"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

mock_java_toolchain(
    name = "java_toolchain",
    package_configuration = [":package_javacopts"],
    source_version = "11",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    target_version = "11",
)

bzl_test_suite(
    name = "tests",
    config_settings = {
        "//command_line_option:extra_toolchains": [package_label_string(label_str = ":java_toolchain")],
    },
    testing_aspect = JAVA_LITE_PROTO_TESTING_ASPECT,
    tests = TESTS,
)
