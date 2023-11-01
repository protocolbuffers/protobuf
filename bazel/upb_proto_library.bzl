# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Public rules for using upb protos:
  - upb_proto_library()
  - upb_proto_reflection_library()
"""

load(
    "//bazel:upb_c_proto_library.bzl",
    _UpbWrappedCcInfo = "UpbWrappedCcInfo",
    _upb_c_proto_library = "upb_c_proto_library",
    _upb_c_proto_library_aspect = "upb_c_proto_library_aspect",
)
load(
    "//bazel:upb_proto_library_internal/aspect.bzl",
    _GeneratedSrcsInfo = "GeneratedSrcsInfo",
)
load(
    "//bazel:upb_proto_reflection_library.bzl",
    _upb_proto_reflection_library = "upb_proto_reflection_library",
)

# Temporary alias, see b/291827469.
upb_proto_library = _upb_c_proto_library

upb_c_proto_library = _upb_c_proto_library
upb_proto_reflection_library = _upb_proto_reflection_library
GeneratedSrcsInfo = _GeneratedSrcsInfo
UpbWrappedCcInfo = _UpbWrappedCcInfo
upb_proto_library_aspect = _upb_c_proto_library_aspect
