# begin:github_only

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

# end:github_only

# begin:google_only
#
# load("//third_party/bazel_rules/rules_python/python:py_test.bzl", "py_test")
#
# def pyproto_test_wrapper(name):
#     src = name + "_wrapper.py"
#     py_test(
#         name = name,
#         srcs = [src],
#         main = src,
#         deps = [
#             "//third_party/py/google/protobuf/internal:" + name + "_for_deps",
#             "//third_party/py/google/protobuf:use_upb_protos",
#         ],
#         target_compatible_with = select({
#             "@platforms//os:windows": ["@platforms//:incompatible"],
#             "//conditions:default": [],
#         }),
#     )
#
# end:google_only
