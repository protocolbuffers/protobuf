load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_lite_proto_library.bzl", "java_lite_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_lite_proto_library_tests/test_utils.bzl", "java_info_subject_factory")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "baz_proto",
    srcs = ["baz.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "foo_proto",
    srcs = [
        "bar.proto",
        "foo.proto",
    ],
    option_deps = [":baz_proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

java_lite_proto_library(
    name = "foo_java_proto_lite",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

bzl_test_suite(
    name = "tests",
    provider_subject_factories = [java_info_subject_factory],
    tests = TESTS,
)
