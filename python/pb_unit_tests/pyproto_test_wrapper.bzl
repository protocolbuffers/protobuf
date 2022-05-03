# begin:github_only

def pyproto_test_wrapper(name):
    src = name + "_wrapper.py"
    native.py_test(
        name = name,
        srcs = [src],
        legacy_create_init = False,
        main = src,
        data = ["@com_google_protobuf//:testdata"],
        deps = [
            "//python:_message",
            "@com_google_protobuf//:python_common_test_protos",
            "@com_google_protobuf//:python_specific_test_protos",
            "@com_google_protobuf//:python_test_srcs",
            "@com_google_protobuf//:python_srcs",
        ],
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
#             "//net/proto2/python/internal:" + name + "_for_deps",
#             "//net/proto2/python/public:use_upb_protos",
#         ],
#     )
#
# end:google_only
