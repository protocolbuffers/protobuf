# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
