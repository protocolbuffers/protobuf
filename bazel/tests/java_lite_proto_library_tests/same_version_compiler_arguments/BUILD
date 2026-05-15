load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_lite_proto_library_tests/test_utils.bzl", "JAVA_LITE_PROTO_TESTING_ASPECT", "java_info_subject_factory")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "baz_proto",
    srcs = ["baz.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

bzl_test_suite(
    name = "tests",
    provider_subject_factories = [java_info_subject_factory],
    testing_aspect = JAVA_LITE_PROTO_TESTING_ASPECT,
    tests = TESTS,
)
