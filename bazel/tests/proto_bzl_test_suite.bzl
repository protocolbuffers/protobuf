"""Defines a test suite for bzl analysis tests."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test")
load("@rules_testing//lib:util.bzl", "testing_aspect")

def bzl_test_suite(
        name,
        tests,
        attrs = {},
        testing_aspect = testing_aspect,
        provider_subject_factories = [],
        config_settings = []):
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
        impl_name = get_function_name(impl)
        test_name = create_test_name(impl_name, name)
        analysis_test(
            name = test_name,
            target = target,
            impl = impl,
            provider_subject_factories = provider_subject_factories,
            config_settings = config_settings,
            testing_aspect = testing_aspect,
            attrs = attrs,
        )
        test_names.append(test_name)

    native.test_suite(
        name = name,
        tests = test_names,
    )

def get_function_name(fn):
    # Starlark currently stringifies a function as "<function NAME>", so we use
    # that knowledge to parse the "NAME" portion out.
    fn_name = str(fn).partition("<function ")[2].partition(" ")[0].partition(">")[0]
    return fn_name

def create_test_name(fn_name, name):
    if fn_name.startswith("_"):
        fn_name = fn_name.removeprefix("_")
    return fn_name + "_" + name

def package_label_string(label_str, name = None):
    """Returns the string repr of a label resolved relative to the current package being constructed."""

    # name is unused.
    return str(native.package_relative_label(label_str))
