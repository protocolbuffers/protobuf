"""ProtoInfo"""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/private:proto_info.bzl", _ProtoInfo = "ProtoInfo")  # buildifier: disable=bzl-visibility

# This resolves to Starlark ProtoInfo in Bazel 8 or with --incompatible_enable_autoload flag
ProtoInfo = getattr(bazel_features.globals, "ProtoInfo", None) or _ProtoInfo
