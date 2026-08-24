load("@rules_java//java/toolchains:java_runtime.bzl", "java_runtime")
load("@rules_java//java/toolchains:java_toolchain.bzl", "java_toolchain")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_proto_library_tests/test_utils.bzl", "JAVA_PROTO_TESTING_ASPECT")
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

java_runtime(
    name = "java_runtime",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

java_toolchain(
    name = "java_toolchain",
    compatible_javacopts = {
        "proto": ["-protoMarkerForTest 7"],
    },
    genclass = "genclass",
    header_compiler = "turbine_canary_deploy.jar",
    header_compiler_direct = "turbine_direct",
    ijar = "ijar",
    java_runtime = ":java_runtime",
    javabuilder = "JavaBuilder_deploy.jar",
    singlejar = "singlejar",
    source_version = "11",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    target_version = "11",
)

toolchain(
    name = "toolchain_definition",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    toolchain = ":java_toolchain",
    toolchain_type = "@bazel_tools//tools/jdk:toolchain_type",
)

bzl_test_suite(
    name = "tests",
    config_settings = {
        "//command_line_option:extra_toolchains": [package_label_string(label_str = ":toolchain_definition")],
    },
    testing_aspect = JAVA_PROTO_TESTING_ASPECT,
    tests = TESTS,
)
