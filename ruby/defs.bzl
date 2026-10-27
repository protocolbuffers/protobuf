# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Wrapper around internal_ruby_proto_library to supply our rules_ruby"""

load("@rules_ruby//ruby:defs.bzl", "rb_library")
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
      **kwargs: other keyword arguments that are passed to rb_library.

    """
    _internal_ruby_proto_library(
        name,
        rb_library,
        **kwargs
    )
