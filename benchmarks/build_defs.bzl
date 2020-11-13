
# copybara:insert_for_google3_begin
# load("//tools/build_defs/proto/cpp:cc_proto_library.bzl", _cc_proto_library="cc_proto_library")
# copybara:insert_end

# copybara:strip_for_google3_begin
_cc_proto_library = native.cc_proto_library
# copybara:strip_end

def proto_library(**kwargs):
    native.proto_library(
        # copybara:insert_for_google3_begin
        # cc_api_version = 2,
        # copybara:insert_end
        **kwargs,
    )

def tmpl_cc_binary(name, gen, args, replacements = [], **kwargs):
    srcs = [name + ".cc"]
    native.genrule(
        name = name + "_gen_srcs",
        tools = [gen],
        outs = srcs,
        cmd = "$(location " + gen + ") " + " ".join(args) + " > $@",
    )

    native.cc_binary(
        # copybara:insert_for_google3_begin
        # malloc="//base:system_malloc",
        # features = ["-static_linking_mode"],
        # copybara:insert_end
        name = name,
        srcs = srcs,
        **kwargs,
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
