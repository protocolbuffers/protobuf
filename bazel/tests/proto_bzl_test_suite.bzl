"""Defines a test suite for bzl analysis tests."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test")
load("@rules_testing//lib:util.bzl", "testing_aspect")

def default_config_settings():
    """Returns the default config settings for bzl analysis tests."""
    return {
        "//command_line_option:features": [
            "supports_dynamic_linker",
            "supports_pic",
        ],
    }

def bzl_test_suite(
        name,
        tests,
        attrs = {},
        testing_aspect = testing_aspect,
        provider_subject_factories = [],
        config_settings = {}):
    """Defines a test suite for bzl analysis tests.

    Args:
      name: The name of the test suite.
      tests: A dictionary where the key is the build target and the value is a list of
        analysis test implementation functions using that target.
      attrs: A dictionary of attributes to apply to the testing aspect.
      testing_aspect: The testing aspect to use in the test suite.
      provider_subject_factories: An array of subject factories to use in the test suite.
      config_settings: A dictionary of config settings to apply to the test suite.
    """

    test_names = []
    for target, impl_list in tests.items():
        for impl in impl_list:
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
