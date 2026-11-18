# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"Module extensions for use under bzlmod"

load("@bazel_skylib//lib:modules.bzl", "modules")
load("//toolchain:platforms.bzl", "PROTOBUF_PLATFORMS")
load(":protoc_toolchain.bzl", "prebuilt_protoc_repo")

def create_all_toolchain_repos(name = "prebuilt_protoc"):
    for platform in PROTOBUF_PLATFORMS.keys():
        prebuilt_protoc_repo(
            # We must replace hyphen with underscore to workaround rules_python py_proto_library constraint
            name = ".".join([name, platform.replace("-", "_")]),
            platform = platform,
        )

protoc = modules.as_extension(create_all_toolchain_repos)
