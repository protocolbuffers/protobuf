# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Renames toplevel symbols so they can be exported in Starlark under the same name"""

load("@proto_bazel_features//:features.bzl", "bazel_features")

native_proto_common = getattr(native, "proto_common", None)
HAS_NATIVE_PROTO_FLAGS = bazel_features.rules.has_proto_fragment
