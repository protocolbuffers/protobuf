"""ProtoInfo"""

load("//bazel/private:proto_info.bzl", _ProtoInfo = "ProtoInfo")

# This resolves to Starlark ProtoInfo in Bazel 8 or with --incompatible_enable_autoload flag
ProtoInfo = _ProtoInfo
