# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""cc_proto_library rule"""

load("//bazel/private/oss:cc_proto_library.bzl", _cc_proto_library = "cc_proto_library")

def cc_proto_library(**kwattrs):
    # Only use Starlark rules when they are removed from Bazel
    if not hasattr(native, "cc_proto_library"):
        _cc_proto_library(**kwattrs)
    else:
        native.cc_proto_library(**kwattrs)  # buildifier: disable=native-cc-proto
