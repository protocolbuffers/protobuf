# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:defs.bzl", "cc_library")
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
    cc_library(
        name = name,
        hdrs = [hdr],
        strip_include_prefix = "/src",
        copts = COPTS,
        testonly = True,
    )
