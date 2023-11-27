"""Wrapper around internal_ruby_proto_library to supply our rules_ruby"""

load("@rules_ruby//ruby:defs.bzl", "ruby_library")
load("//:protobuf.bzl", _internal_ruby_proto_library = "internal_ruby_proto_library")

def internal_ruby_proto_library(
        name,
        **kwargs):
    """Bazel rule to create a Ruby protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the ruby_proto_library.
      **kwargs: other keyword arguments that are passed to ruby_library.

    """
    _internal_ruby_proto_library(
        name,
        ruby_library,
        **kwargs
    )
