# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Exposes cc_proto_aspect to select paths"""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/private/oss:cc_proto_library.bzl", _cc_proto_aspect = "cc_proto_aspect")  # buildifier: disable=bzl-visibility

# This resolves to Starlark cc_proto_aspect in Bazel 8 or with --incompatible_enable_autoload flag
cc_proto_aspect = getattr(bazel_features.globals, "cc_proto_aspect", None) or _cc_proto_aspect
