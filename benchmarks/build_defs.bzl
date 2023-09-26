# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# begin:google_only
# load("@rules_cc//cc:defs.bzl", _cc_proto_library = "cc_proto_library")
#
# _is_google3 = True
# end:google_only

# begin:github_only
_cc_proto_library = native.cc_proto_library
_is_google3 = False
# end:github_only

def proto_library(**kwargs):
    if _is_google3:
        kwargs["cc_api_version"] = 2
    native.proto_library(
        **kwargs
    )

def tmpl_cc_binary(name, gen, args, replacements = [], **kwargs):
    srcs = [name + ".cc"]
    native.genrule(
        name = name + "_gen_srcs",
        tools = [gen],
        outs = srcs,
        cmd = "$(location " + gen + ") " + " ".join(args) + " > $@",
    )

    if _is_google3:
        kwargs["malloc"] = "//base:system_malloc"
        kwargs["features"] = ["-static_linking_mode"]
    native.cc_binary(
        name = name,
        srcs = srcs,
        **kwargs
    )

def cc_optimizefor_proto_library(name, srcs, outs, optimize_for):
    if len(srcs) != 1:
        fail("Currently srcs must have exactly 1 element")

    native.genrule(
        name = name + "_gen_proto",
        srcs = srcs,
        outs = outs,
        cmd = "cp $< $@ && chmod a+w $@ && echo 'option optimize_for = " + optimize_for + ";' >> $@",
    )

    proto_library(
        name = name + "_proto",
        srcs = outs,
    )

    _cc_proto_library(
        name = name,
        deps = [":" + name + "_proto"],
    )

def expand_suffixes(vals, suffixes):
    ret = []
    for val in vals:
        for suffix in suffixes:
            ret.append(val + suffix)
    return ret
