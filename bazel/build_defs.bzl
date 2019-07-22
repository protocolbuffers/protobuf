"""Internal rules for building upb."""

load(":upb_proto_library.bzl", "GeneratedSrcsInfo")

def _librule(name):
    return name + "_lib"

def _get_real_short_path(file):
    # For some reason, files from other archives have short paths that look like:
    #   ../com_google_protobuf/google/protobuf/descriptor.proto
    short_path = file.short_path
    if short_path.startswith("../"):
        second_slash = short_path.index("/", 3)
        short_path = short_path[second_slash + 1:]
    return short_path

def _get_real_root(file):
    real_short_path = _get_real_short_path(file)
    return file.path[:-len(real_short_path) - 1]

def _get_real_roots(files):
    roots = {}
    for file in files:
        real_root = _get_real_root(file)
        if real_root:
            roots[real_root] = True
    return roots.keys()

def lua_cclibrary(name, srcs, hdrs = [], deps = [], luadeps = []):
    lib_rule = name + "_lib"
    so_rule = "lib" + name + ".so"
    so_file = _remove_prefix(name, "lua/") + ".so"

    native.cc_library(
        name = _librule(name),
        hdrs = hdrs,
        srcs = srcs,
        deps = deps + [_librule(dep) for dep in luadeps] + ["@lua//:liblua_headers"],
    )

    native.cc_binary(
        name = so_rule,
        linkshared = True,
        deps = [_librule(name)],
        linkopts = select({
            ":darwin": [
                "-undefined dynamic_lookup",
            ],
            "//conditions:default": [],
        }),
    )

    native.genrule(
        name = name + "_copy",
        srcs = [":" + so_rule],
        outs = [so_file],
        cmd = "cp $< $@",
    )

    native.filegroup(
        name = name,
        data = [so_file],
    )

def _remove_prefix(str, prefix):
    if not str.startswith(prefix):
        fail("%s doesn't start with %s" % (str, prefix))
    return str[len(prefix):]

def _remove_suffix(str, suffix):
    if not str.endswith(suffix):
        fail("%s doesn't end with %s" % (str, suffix))
    return str[:-len(suffix)]

def lua_library(name, srcs, strip_prefix, luadeps = []):
    outs = [_remove_prefix(src, strip_prefix + "/") for src in srcs]
    native.genrule(
        name = name + "_copy",
        srcs = srcs,
        outs = outs,
        cmd = "cp $(SRCS) $(@D)",
    )

    native.filegroup(
        name = name,
        data = outs + luadeps,
    )

def make_shell_script(name, contents, out):
    contents = contents.replace("$", "$$")
    native.genrule(
        name = "gen_" + name,
        outs = [out],
        cmd = "(cat <<'HEREDOC'\n%s\nHEREDOC\n) > $@" % contents,
    )

def _lua_binary_or_test(name, luamain, luadeps, rule):
    script = name + ".sh"

    make_shell_script(
        name = "gen_" + name,
        out = script,
        contents = """
BASE=$(dirname $(rlocation upb/upb_c.so))
export LUA_CPATH="$BASE/?.so"
export LUA_PATH="$BASE/?.lua"
$(rlocation lua/lua) $(rlocation upb/tools/upbc.lua) "$@"
""",
    )

    rule(
        name = name,
        srcs = [script],
        data = ["@lua//:lua", luamain] + luadeps,
    )

def lua_binary(name, luamain, luadeps = []):
    _lua_binary_or_test(name, luamain, luadeps, native.sh_binary)

def lua_test(name, luamain, luadeps = []):
    _lua_binary_or_test(name, luamain, luadeps, native.sh_test)

def generated_file_staleness_test(name, outs, generated_pattern):
    """Tests that checked-in file(s) match the contents of generated file(s).

    The resulting test will verify that all output files exist and have the
    correct contents.  If the test fails, it can be invoked with --fix to
    bring the checked-in files up to date.

    Args:
      name: Name of the rule.
      outs: the checked-in files that are copied from generated files.
      generated_pattern: the pattern for transforming each "out" file into a
        generated file.  For example, if generated_pattern="generated/%s" then
        a file foo.txt will look for generated file generated/foo.txt.
    """

    script_name = name + ".py"
    script_src = "//:tools/staleness_test.py"

    # Filter out non-existing rules so Blaze doesn't error out before we even
    # run the test.
    existing_outs = native.glob(include = outs)

    # The file list contains a few extra bits of information at the end.
    # These get unpacked by the Config class in staleness_test_lib.py.
    file_list = outs + [generated_pattern, native.package_name() or ".", name]

    native.genrule(
        name = name + "_makescript",
        outs = [script_name],
        srcs = [script_src],
        testonly = 1,
        cmd = "cat $(location " + script_src + ") > $@; " +
              "sed -i.bak -e 's|INSERT_FILE_LIST_HERE|" + "\\\n  ".join(file_list) + "|' $@",
    )

    native.py_test(
        name = name,
        srcs = [script_name],
        data = existing_outs + [generated_pattern % file for file in outs],
        deps = [
            "//:staleness_test_lib",
        ],
    )

# upb_amalgamation() rule, with file_list aspect.

SrcList = provider(
    fields = {
        "srcs": "list of srcs",
    },
)

def _file_list_aspect_impl(target, ctx):
    if GeneratedSrcsInfo in target:
        srcs = target[GeneratedSrcsInfo]
        return [SrcList(srcs = srcs.srcs + srcs.hdrs)]

    srcs = []
    for src in ctx.rule.attr.srcs:
        srcs += src.files.to_list()
    for hdr in ctx.rule.attr.hdrs:
        srcs += hdr.files.to_list()
    for hdr in ctx.rule.attr.textual_hdrs:
        srcs += hdr.files.to_list()
    return [SrcList(srcs = srcs)]

_file_list_aspect = aspect(
    implementation = _file_list_aspect_impl,
)

def _upb_amalgamation(ctx):
    inputs = []
    for lib in ctx.attr.libs:
        inputs += lib[SrcList].srcs
    srcs = [src for src in inputs if src.path.endswith("c")]
    ctx.actions.run(
        inputs = inputs,
        outputs = ctx.outputs.outs,
        arguments = [ctx.bin_dir.path + "/"] + [f.path for f in srcs] + ["-I" + root for root in _get_real_roots(inputs)],
        progress_message = "Making amalgamation",
        executable = ctx.executable.amalgamator,
    )
    return []

upb_amalgamation = rule(
    attrs = {
        "amalgamator": attr.label(
            executable = True,
            cfg = "host",
        ),
        "libs": attr.label_list(aspects = [_file_list_aspect]),
        "outs": attr.output_list(),
    },
    implementation = _upb_amalgamation,
)

def licenses(*args):
    # No-op (for Google-internal usage).
    pass
