# begin:github_only

def pyproto_test_wrapper(name, deps = []):
    src = name + "_wrapper.py"
    native.py_test(
        name = name,
        srcs = [src],
        legacy_create_init = False,
        main = src,
        data = ["@com_google_protobuf//src/google/protobuf:testdata"],
        deps = [
            "//python:_message",
            "@com_google_protobuf//:python_common_test_protos",
            "@com_google_protobuf//:python_specific_test_protos",
            "@com_google_protobuf//:python_test_srcs",
            "@com_google_protobuf//:python_srcs",
        ] + deps,
        target_compatible_with = select({
            "@system_python//:supported": [],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
    )

# end:github_only

# begin:google_only
#
# def pyproto_test_wrapper(name):
#     src = name + "_wrapper.py"
#     native.py_test(
#         name = name,
#         srcs = [src],
#         main = src,
#         deps = [
#             "//third_party/py/google/protobuf/internal:" + name + "_for_deps",
#             "//net/proto2/python/public:use_upb_protos",
#         ],
#         target_compatible_with = select({
#             "@platforms//os:windows": ["@platforms//:incompatible"],
#             "//conditions:default": [],
#         }),
#     )
#
# end:google_only
