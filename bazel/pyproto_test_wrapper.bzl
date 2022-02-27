
# copybara:strip_for_google3_begin

def pyproto_test_wrapper(name):
    src = name + "_wrapper.py"
    native.py_test(
        name = name,
        srcs = [src],
        legacy_create_init = False,
        main = src,
        data = ["@com_google_protobuf//:testdata"],
        deps = [
            "//python:message_ext",
            "@com_google_protobuf//:python_common_test_protos",
            "@com_google_protobuf//:python_specific_test_protos",
            "@com_google_protobuf//:python_srcs",
        ],
    )

# copybara:replace_for_google3_begin
# 
# def pyproto_test_wrapper(name):
#     src = name + "_wrapper.py"
#     native.py_test(
#         name = name,
#         srcs = [src],
#         main = src,
#         deps = ["//net/proto2/python/internal:" + name + "_for_deps"],
#     )
# 
# copybara:replace_for_google3_end
