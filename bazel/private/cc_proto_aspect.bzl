"""Exposes cc_proto_aspect to rules_rust"""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/private:bazel_cc_proto_library.bzl", _cc_proto_aspect = "cc_proto_aspect")  # buildifier: disable=bzl-visibility

# This resolves to Starlark cc_proto_aspect in Bazel 8 or with --incompatible_enable_autoload flag
cc_proto_aspect = getattr(bazel_features.globals, "cc_proto_aspect", None) or _cc_proto_aspect
