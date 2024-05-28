"""Setup dependencies needed to compile the protobuf library as a 3rd-party consumer."""

load("@bazel_features//:deps.bzl", "bazel_features_deps")

def protobuf_setup():
    bazel_features_deps()
