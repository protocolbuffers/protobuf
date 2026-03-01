"""Defines a test suite for bzl analysis tests."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test")
load("@rules_testing//lib:util.bzl", "testing_aspect")

def bzl_test_suite(
        tests,
        config_settings = [],
        testing_aspect = testing_aspect,
        name = "tests"):
    """Defines a test suite for bzl analysis tests.

    Args:
      tests: A list of tuples, where each tuple contains an analysis test
        implementation function and a target to test.
      config_settings: A list of config settings to apply to the test suite.
      testing_aspect: The testing aspect to use.
      name: The name of the test suite.
    """
    test_names = []
    for (impl, target) in tests:
        # Starlark currently stringifies a function as "<function NAME>", so we use
        # that knowledge to parse the "NAME" portion out.
        impl_name = str(impl).partition("<function _")[2].partition(" ")[0].partition(">")[0]
        test_name = impl_name + "_" + name
        analysis_test(
            name = test_name,
            target = target,
            impl = impl,
            config_settings = config_settings,
            testing_aspect = testing_aspect,
        )
        test_names.append(test_name)

    native.test_suite(
        name = name,
        tests = test_names,
    )
