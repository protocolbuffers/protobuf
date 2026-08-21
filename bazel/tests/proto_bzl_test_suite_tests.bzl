"""Tests for proto_bzl_test_suite.bzl."""

load("@rules_testing//lib:unittest.bzl", "loadingtest")
load(":proto_bzl_test_suite.bzl", "create_test_name", "get_function_name")

def _some_test_impl():
    pass

def _another_test():
    pass

def public_test():
    pass

def proto_bzl_test_suite_tests(name):
    """Tests for creating test names with proto_bzl_test_suite.

    Args:
        name: The name of the test suite.
    """
    env = loadingtest.make(name)

    loadingtest.equals(
        env,
        "get_function_name",
        "_some_test_impl",
        get_function_name(_some_test_impl),
    )

    loadingtest.equals(
        env,
        "basic_test_name",
        "some_test_impl_tests",
        create_test_name(get_function_name(_some_test_impl), "tests"),
    )

    loadingtest.equals(
        env,
        "another_test_name",
        "another_test_tests",
        create_test_name(get_function_name(_another_test), "tests"),
    )

    # Verifies that public functions (no leading '_') are also supported.
    loadingtest.equals(
        env,
        "public_test_name",
        "public_test_tests",
        create_test_name(get_function_name(public_test), "tests"),
    )

    def _inner_test():
        pass

    loadingtest.equals(
        env,
        "inner_test_name",
        "inner_test_tests",
        create_test_name(get_function_name(_inner_test), "tests"),
    )
