# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Temporary alias to repository rule for using Python 3.x headers from the system."""

load(
    "//python/dist:system_python.bzl",
    _system_python = "system_python",
)

# TODO: Temporary alias. This is deprecated and to be removed in a future
# release. Users should now get system_python from protobuf_deps.bzl.
system_python = _system_python
