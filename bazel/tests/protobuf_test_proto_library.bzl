"""This module defines a wrapper around proto_library for Bazel versions that do not support certain attributes yet."""

load("//bazel:proto_library.bzl", "proto_library")

def protobuf_test_proto_library(**kwattrs):
    """
    Creates a proto library, handling any attributes that are not supported by the proto_library rule.

    Args:
        **kwattrs: Additional arguments to pass to the proto_library rule.
    """
    kwargs = dict(kwattrs)

    # TODO: Bazel 7's proto_library rule does not support option_deps, so we handle it by putting it in deps instead.
    if "option_deps" in kwargs and hasattr(native, "proto_library"):
        deps = kwargs.pop("deps", [])
        option_deps = kwargs.pop("option_deps")
        kwargs["deps"] = depset(deps + option_deps).to_list()

    proto_library(**kwargs)
