"""Exposes cc_proto_aspect name to select paths"""

visibility([
    "//third_party/crubit/rs_bindings_from_cc/bazel_support",
])

# Due to an unusual edge case, the Crubit aspect needs to know the name of the C++ proto aspect.
cc_proto_aspect_name = "//bazel/private/google:cc_proto_library.bzl%cc_proto_aspect"
