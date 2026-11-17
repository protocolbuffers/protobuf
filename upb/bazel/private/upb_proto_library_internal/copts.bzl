# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Dummy rule to expose select() copts to aspects."""

UpbProtoLibraryCoptsInfo = provider(
    "Provides copts for upb proto targets",
    fields = {
        "copts": "copts for upb_proto_library()",
    },
)

def upb_proto_library_copts_impl(ctx):
    return UpbProtoLibraryCoptsInfo(copts = ctx.attr.copts)

upb_proto_library_copts = rule(
    implementation = upb_proto_library_copts_impl,
    attrs = {"copts": attr.string_list(default = [])},
)
