# Copyright (c) 2026, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# This is a temporary shim to unbreak cel-spec and googleapis which referenced this file. At some
# point in the future we can remove this once both are updated.

"""upb_c_proto_library rule"""

load("//upb/bazel:upb_c_proto_library.bzl", _upb_c_proto_library = "upb_c_proto_library")

upb_c_proto_library = _upb_c_proto_library
