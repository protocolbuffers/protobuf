
def tmpl_cc_binary(name, gen, args, replacements = [], **kwargs):
    srcs = [name + ".cc"]
    native.genrule(
        name = name + "_gen_srcs",
        tools = [gen],
        outs = srcs,
        cmd = "$(location " + gen + ") " + " ".join(args) + " > $@",
    )

    native.cc_binary(
        name = name,
        srcs = srcs,
        **kwargs,
    )

def cc_lite_proto_library(name, srcs, outs):
    if len(srcs) != 1:
        fail("Currently srcs must have exactly 1 element")

    native.genrule(
        name = name + "_gen_proto",
        srcs = srcs,
        outs = outs,
        cmd = "cp $< $@ && chmod a+w $@ && echo 'option optimize_for = LITE_RUNTIME;' >> $@",
    )

    native.proto_library(
        name = name + "_proto",
        srcs = outs,
    )

    native.cc_proto_library(
        name = name,
        deps = [":" + name + "_proto"],
    )

def expand_suffixes(vals, suffixes):
  ret = []
  for val in vals:
    for suffix in suffixes:
      ret.append(val + suffix)
  return ret
