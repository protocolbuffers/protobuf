"""Exposes cc_proto_aspect to rules_rust"""

load("//bazel/private:bazel_cc_proto_library.bzl", _cc_proto_aspect = "cc_proto_aspect")  # buildifier: disable=bzl-visibility
load("//bazel/private:native.bzl", _native_cc_proto_aspect = "native_cc_proto_aspect")  # buildifier: disable=bzl-visibility

cc_proto_aspect = _cc_proto_aspect if not hasattr(native, "cc_proto_library") else _native_cc_proto_aspect
