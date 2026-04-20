"""fuzz_test() macro for upb"""

load("@rules_cc//cc:cc_test.bzl", "cc_test")

def _fuzz_test_impl(
        name,
        visibility = None,
        srcs = [],
        deps = [],
        **kwargs):
    base_tags = kwargs.get("tags", [])
    other_kwargs = {k: v for k, v in kwargs.items() if k != "tags"}

    test_deps = ["//testing/fuzzing:fuzztest", "@googletest//:gtest", "@googletest//:gtest_main"]

    pass

# fuzz_test() is a symbolic macro for upb fuzz tests.
#
# Its main purpose is to helpfully define a fuzz test target with custom options and OSS exclusion.
#
# The actual utility library (_impl) and regression test (_regression_test) are expanded in the BUILD file.
fuzz_test = macro(
    implementation = _fuzz_test_impl,
    attrs = {
        "srcs": attr.label_list(default = [], configurable = False),
        "deps": attr.label_list(default = [], configurable = False),
    },
)
