"""Renames toplevel symbols so they can be exported in Starlark under the same name"""

load("@proto_bazel_features//:features.bzl", "bazel_features")

native_proto_common = proto_common_do_not_use
HAS_NATIVE_PROTO_FLAGS = bazel_features.rules.has_proto_fragment
