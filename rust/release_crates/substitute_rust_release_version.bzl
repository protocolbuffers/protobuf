# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""A rule to textually replace {{VERSION}} with the Rust release version in files."""

load("//:protobuf_version.bzl", "PROTOBUF_LEGACY_RUST_VERSION", "PROTOBUF_RUST_VERSION")

def substitute_rust_release_version(src, out, name = None):
    native.genrule(
        name = name or ("gen_%s" % out),
        srcs = [src],
        outs = [out],
        cmd = "cat $(SRCS) | sed -e 's/{{PROTOBUF_RUST_VERSION}}/{0}/g' -e 's/{{PROTOBUF_LEGACY_VERSION}}/{1}/g' > $(OUTS)".format(PROTOBUF_RUST_VERSION, PROTOBUF_LEGACY_RUST_VERSION),
    )
