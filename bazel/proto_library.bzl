# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Definition of proto_library rule."""

load("//bazel/private:proto_library_rule.bzl", _proto_library = "proto_library")

def proto_library(**kwattrs):
    # Only use Starlark rules when they are removed from Bazel.
    if not hasattr(native, "proto_library"):
        _proto_library(**kwattrs)
    else:
        # On older Bazel versions keep using native rules, so that mismatch in ProtoInfo doesn't happen
        native.proto_library(**kwattrs)  # buildifier: disable=native-proto
