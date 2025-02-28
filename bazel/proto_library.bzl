# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""
Macro of proto_library rule.
"""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/private:bazel_proto_library_rule.bzl", _proto_library = "proto_library")

def proto_library(**kwattrs):
    # This condition causes Starlark rules to be used only on Bazel >=7.0.0
    if bazel_features.proto.starlark_proto_info:
        _proto_library(**kwattrs)
    else:
        # On older Bazel versions keep using native rules, so that mismatch in ProtoInfo doesn't happen
        native.proto_library(**kwattrs)  # buildifier: disable=native-proto
