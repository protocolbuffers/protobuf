# Copyright (c) 2024 Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Public rules for using hpb:
  - hpb_proto_library()
"""

load("//hpb/bazel:upb_cc_proto_library.bzl", "upb_cc_proto_library")

hpb_proto_library = upb_cc_proto_library
