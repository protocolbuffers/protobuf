"""cc_proto_library rule"""

load("//bazel/private:bazel_cc_proto_library.bzl", _cc_proto_aspect_hint = "cc_proto_aspect_hint", _cc_proto_library = "cc_proto_library")  # buildifier: disable=bzl-visibility

def cc_proto_library(**kwattrs):
    # Only use Starlark rules when they are removed from Bazel
    if not hasattr(native, "cc_proto_library"):
        _cc_proto_library(**kwattrs)
    else:
        native.cc_proto_library(**kwattrs)  # buildifier: disable=native-cc-proto

def cc_proto_aspect_hint(**kwattrs):
    if hasattr(native, "cc_proto_library"):
        fail("cc_proto_aspect_hint requires bazel 8.x+")
    else:
        _cc_proto_aspect_hint(**kwattrs)
