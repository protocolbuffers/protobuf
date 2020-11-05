
def tmpl_cc_binary(name, srcs, replacements = [], **kwargs):
    if len(srcs) != 1:
        fail("Currently srcs must have exactly 1 element")
    src = srcs[0]
    if not src.endswith(".tmpl"):
        fail("srcs of tmpl_cc_binary must end with .tmpl")
    outs = [name + "_" + src[:-5]]
    sed_cmds = ["s,{},{},g".format(k, v) for k, v in replacements.items()]
    cmd = "sed -e '{}' $< > $@".format("; ".join(sed_cmds))

    native.genrule(
        name = name + "_gen_srcs",
        srcs = [src],
        outs = outs,
        cmd = cmd,
    )

    native.cc_binary(
        name = name,
        srcs = outs,
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
