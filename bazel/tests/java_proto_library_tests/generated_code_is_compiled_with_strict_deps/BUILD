load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_proto_library_tests/test_utils.bzl", "JAVA_PROTO_TESTING_ASPECT")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "baz_proto",
    srcs = ["baz.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "bar_proto",
    srcs = ["bar.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":baz_proto"],
)

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":bar_proto"],
)

java_proto_library(
    name = "foo_java_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

bzl_test_suite(
    name = "tests",
    # Used to read the runtime jars from baz_proto's (no proto deps) compile action.
    attr_values = {"baz_proto": ":baz_proto"},
    attrs = {"baz_proto": {}},
    testing_aspect = JAVA_PROTO_TESTING_ASPECT,
    tests = TESTS,
)
