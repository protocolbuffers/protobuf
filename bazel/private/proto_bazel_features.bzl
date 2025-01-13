# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Vendored version of bazel_features for protobuf, to keep a one-step setup"""

_PROTO_BAZEL_FEATURES = """bazel_features = struct(
  cc = struct(
    protobuf_on_allowlist = {protobuf_on_allowlist},
  ),
  proto = struct(
    starlark_proto_info = {starlark_proto_info},
  ),
  globals = struct(
    PackageSpecificationInfo = {PackageSpecificationInfo},
    ProtoInfo = getattr(getattr(native, 'legacy_globals', None), 'ProtoInfo', {ProtoInfo}),
    cc_proto_aspect = getattr(getattr(native, 'legacy_globals', None), 'cc_proto_aspect', {cc_proto_aspect}),
  ),
)
"""

def _proto_bazel_features_impl(rctx):
    # An empty string is treated as a "dev version", which is greater than anything.
    bazel_version = native.bazel_version or "999999.999999.999999"
    version_parts = bazel_version.split("-")[0].split(".")
    if len(version_parts) != 3:
        fail("invalid Bazel version '{}': got {} dot-separated segments, want 3".format(bazel_version, len(version_parts)))
    major_version_int = int(version_parts[0])
    minor_version_int = int(version_parts[1])

    starlark_proto_info = major_version_int >= 7
    PackageSpecificationInfo = major_version_int > 6 or (major_version_int == 6 and minor_version_int >= 4)

    protobuf_on_allowlist = major_version_int > 7
    ProtoInfo = "ProtoInfo" if major_version_int < 8 else "None"
    cc_proto_aspect = "cc_proto_aspect" if major_version_int < 8 else "None"

    rctx.file("BUILD.bazel", """
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
bzl_library(
    name = "features",
    srcs = ["features.bzl"],
    visibility = ["//visibility:public"],
)
exports_files(["features.bzl"])
""")
    rctx.file("features.bzl", _PROTO_BAZEL_FEATURES.format(
        starlark_proto_info = repr(starlark_proto_info),
        PackageSpecificationInfo = "PackageSpecificationInfo" if PackageSpecificationInfo else "None",
        protobuf_on_allowlist = repr(protobuf_on_allowlist),
        ProtoInfo = ProtoInfo,
        cc_proto_aspect = cc_proto_aspect,
    ))

proto_bazel_features = repository_rule(
    implementation = _proto_bazel_features_impl,
    # Force reruns on server restarts to keep native.bazel_version up-to-date.
    local = True,
)
