"""Exposes cc_proto_aspect to rules_rust"""

load("//bazel/private:bazel_cc_proto_library.bzl", _cc_proto_aspect = "cc_proto_aspect")

cc_proto_aspect = _cc_proto_aspect
