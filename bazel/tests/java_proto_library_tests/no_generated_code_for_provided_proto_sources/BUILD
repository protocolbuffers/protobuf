load("//bazel/tests:java_proto_library_tests/test_utils.bzl", "JAVA_PROTO_TESTING_ASPECT")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

bzl_test_suite(
    name = "tests",
    testing_aspect = JAVA_PROTO_TESTING_ASPECT,
    tests = TESTS,
)
