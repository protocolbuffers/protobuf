"""Defines a test suite for bzl analysis tests."""

load("@rules_cc//tests/cc/testutil:cc_analysis_test.bzl", "cc_analysis_test")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test")
load("@rules_testing//lib:util.bzl", "testing_aspect")

def cc_bzl_test_suite(
        name,
        tests,
        attrs = {},
        testing_aspect = testing_aspect,
        provider_subject_factories = [],
        config_settings = {},
        **kwargs):
    """Defines a cc test suite for bzl analysis tests.

    Args:
      name: The name of the test suite.
      tests: A dictionary where the key is the build target and the value is a list of
        analysis test implementation functions using that target.
      attrs: A dictionary of attributes to apply to the testing aspect.
      testing_aspect: The testing aspect to use in the test suite.
      provider_subject_factories: An array of subject factories to use in the test suite.
      config_settings: A dictionary of config settings to apply to the test suite.
      **kwargs: Additional keyword arguments passed through to the underlying analysis tests.
    """

    actual_config_settings = dict(config_settings)
    if "//command_line_option:extra_toolchains" not in actual_config_settings:
        actual_config_settings["//command_line_option:extra_toolchains"] = ",".join([
            "//bazel/tests:cc-toolchain-k8-portable-toolchain",
            "@rules_cc//tests/cc/testutil/toolchains:cc-toolchain-macos-compiler",
            "@rules_cc//tests/cc/testutil/toolchains:mock_go_toolchain",
        ])

    user_test_features = kwargs.pop("test_features", [])
    user_with_features = kwargs.pop("with_features", None)
    user_config_features = [
        f[1:] if f.startswith("-") else f
        for f in actual_config_settings.get("//command_line_option:features", [])
        if f != "proto_dynamic_mode_static_link" and f != "-proto_dynamic_mode_static_link"
    ]
    if user_with_features != None:
        with_features = user_with_features
    else:
        with_features = user_test_features + user_config_features

    test_names = []
    for target, impl_list in tests.items():
        for impl in impl_list:
            impl_name = get_function_name(impl)
            test_name = create_test_name(impl_name, name)
            cc_analysis_test(
                name = test_name,
                target = target,
                impl = impl,
                provider_subject_factories = provider_subject_factories,
                config_settings = actual_config_settings,
                testing_aspect = testing_aspect,
                attrs = attrs,
                test_features = user_test_features,
                with_features = with_features,
                **kwargs
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
