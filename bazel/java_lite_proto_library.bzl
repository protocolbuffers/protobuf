# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""java_lite_proto_library rule"""

load("//bazel/private:java_lite_proto_library.bzl", _java_lite_proto_library = "java_lite_proto_library")  # buildifier: disable=bzl-visibility

def java_lite_proto_library(**kwattrs):
    # Only use Starlark rules when they are removed from Bazel
    if not hasattr(native, "java_lite_proto_library"):
        _java_lite_proto_library(**kwattrs)
    else:
        native.java_lite_proto_library(**kwattrs)  # buildifier: disable=native-java-lite-proto
