load("@rules_java//java:java_library.bzl", "java_library")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_proto_library_tests/test_utils.bzl", "JAVA_PROTO_TESTING_ASPECT", "java_info_subject_factory")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "bottom_proto",
    srcs = ["bottom.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "e2_proto",
    srcs = ["e2.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    exports = [":bottom_proto"],
    deps = [":bottom_proto"],
)

proto_library(
    name = "e1_proto",
    srcs = ["e1.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    exports = [":e2_proto"],
    deps = [":e2_proto"],
)

proto_library(
    name = "mid_proto",
    srcs = ["mid.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    exports = [":e1_proto"],
    deps = [":e1_proto"],
)

proto_library(
    name = "top_proto",
    srcs = ["top.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":mid_proto"],
)

java_proto_library(
    name = "top_java_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":top_proto"],
)

java_library(
    name = "java_lib",
    srcs = ["Foo.java"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":top_java_proto"],
)

bzl_test_suite(
    name = "tests",
    provider_subject_factories = [java_info_subject_factory],
    testing_aspect = JAVA_PROTO_TESTING_ASPECT,
    tests = TESTS,
)
