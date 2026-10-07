load("@rules_java//java:java_library.bzl", "java_library")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_lite_proto_library.bzl", "java_lite_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "indirect_proto",
    srcs = ["indirect.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "direct_proto",
    srcs = ["direct.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":indirect_proto"],
)

java_lite_proto_library(
    name = "direct_java_proto_lite",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":direct_proto"],
)

java_lite_proto_library(
    name = "indirect_java_proto_lite",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":indirect_proto"],
)

java_library(
    name = "javalib",
    srcs = ["Foo.java"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":direct_java_proto_lite"],
)

bzl_test_suite(
    name = "tests",
    tests = TESTS,
)
