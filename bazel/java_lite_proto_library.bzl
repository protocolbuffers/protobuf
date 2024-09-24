# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""java_lite_proto_library rule"""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/private:java_lite_proto_library.bzl", _java_lite_proto_library = "java_lite_proto_library")  # buildifier: disable=bzl-visibility

def java_lite_proto_library(**kwattrs):
    # This condition causes Starlark rules to be used only on Bazel >=7.0.0
    if bazel_features.proto.starlark_proto_info:
        _java_lite_proto_library(**kwattrs)
    else:
        # On older Bazel versions keep using native rules, so that mismatch in ProtoInfo doesn't happen
        native.java_lite_proto_library(**kwattrs)
