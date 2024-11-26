# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# https:#developers.google.com/protocol-buffers/
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

load("//build_defs:cpp_opts.bzl", "COPTS")

# This works around https://github.com/bazelbuild/bazel/issues/19124 by using a generated header to
# inject the Bazel path of the test plugins.
# TODO: Replace this with simpler alternative injecting these via copts once we drop
# support for Bazel 6.3.0.
def inject_plugin_paths(name):
    hdr = name + ".h"
    native.genrule(
        name = "test_plugin_paths_gen",
        outs = [hdr],
        srcs = [
            ":fake_plugin",
            ":test_plugin",
        ],
        cmd = """
cat <<'EOF' >$(OUTS)
#ifndef GOOGLE_PROTOBUF_COMPILER_TEST_PLUGIN_PATHS_H__
#define GOOGLE_PROTOBUF_COMPILER_TEST_PLUGIN_PATHS_H__

#define GOOGLE_PROTOBUF_TEST_PLUGIN_PATH "$(rootpath :test_plugin)"
#define GOOGLE_PROTOBUF_FAKE_PLUGIN_PATH "$(rootpath :fake_plugin)"

#endif  // GOOGLE_PROTOBUF_COMPILER_TEST_PLUGIN_PATHS_H__
EOF
""",
        visibility = ["//visibility:private"],
        testonly = True,
    )

    native.cc_library(
        name = name,
        hdrs = [hdr],
        strip_include_prefix = "/src",
        copts = COPTS,
        testonly = True,
    )
