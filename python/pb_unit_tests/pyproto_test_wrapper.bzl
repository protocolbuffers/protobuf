"""Wrapper for another py_test to run with upb, possibly with a set of expected failures."""

def pyproto_test_wrapper(name, deps = []):
    src = name + "_wrapper.py"
    native.py_test(
        name = name,
        srcs = [src],
        legacy_create_init = False,
        main = src,
        data = ["//src/google/protobuf:testdata"],
        deps = [
            "//python:_message",
            "//:python_common_test_protos",
            "//:python_specific_test_protos",
            "//:python_test_srcs",
            "//:python_srcs",
        ] + deps,
        target_compatible_with = select({
            "@system_python//:supported": [],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
    )
