# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Forwarding shim for the Bazel autoloader."""

load(":cc_proto_aspect.bzl", _cc_proto_aspect = "cc_proto_aspect")

# This entry point is for the Bazel autoloader only.  We can't easily change it since the file
# name is hardcoded in the Bazel binary.
cc_proto_aspect = _cc_proto_aspect
