# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Exposes Java proto aspect names to select paths"""

visibility([
    "@rules_java//java/google/rules",
])

java_lite_proto_aspect_name = "//bazel/private:java_lite_proto_library.bzl%_java_lite_proto_aspect"
java_proto_aspect_name = "//bazel/private:java_proto_library.bzl%java_proto_aspect"
java_mutable_proto_aspect_names = [
    "//bazel/private/google:java_mutable_proto_library.bzl%google_java_mutable_proto_aspect_services",
    "//bazel/private/google:java_mutable_proto_library.bzl%google_java_mutable_proto_aspect_noservices",
]
