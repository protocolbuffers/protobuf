
shell_find_runfiles = """
  # --- begin runfiles.bash initialization ---
  # Copy-pasted from Bazel's Bash runfiles library (tools/bash/runfiles/runfiles.bash).
  set -euo pipefail
  if [[ ! -d "${RUNFILES_DIR:-/dev/null}" && ! -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
    if [[ -f "$0.runfiles_manifest" ]]; then
      export RUNFILES_MANIFEST_FILE="$0.runfiles_manifest"
    elif [[ -f "$0.runfiles/MANIFEST" ]]; then
      export RUNFILES_MANIFEST_FILE="$0.runfiles/MANIFEST"
    elif [[ -f "$0.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
      export RUNFILES_DIR="$0.runfiles"
    fi
  fi
  if [[ -f "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
    source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"
  elif [[ -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
    source "$(grep -m1 "^bazel_tools/tools/bash/runfiles/runfiles.bash " \
              "$RUNFILES_MANIFEST_FILE" | cut -d ' ' -f 2-)"
  else
    echo >&2 "ERROR: cannot find @bazel_tools//tools/bash/runfiles:runfiles.bash"
    exit 1
  fi
  # --- end runfiles.bash initialization ---
"""

def _librule(name):
    return name + "_lib"

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

def _lua_binary_or_test(name, luamain, luadeps, rule):
    script = name + ".sh"

    script_contents = (shell_find_runfiles + """
BASE=$(dirname $(rlocation upb/upb_c.so))
export LUA_CPATH="$BASE/?.so"
export LUA_PATH="$BASE/?.lua"
$(rlocation lua/lua) $(rlocation upb/tools/upbc.lua) "$@"
""").replace("$", "$$")

    native.genrule(
        name = "gen_" + name,
        outs = [script],
        cmd = "(cat <<'HEREDOC'\n%s\nHEREDOC\n) > $@" % script_contents,
    )
    rule(
        name = name,
        srcs = [script],
        data = ["@lua//:lua", "@bazel_tools//tools/bash/runfiles", luamain] + luadeps,
    )

def lua_binary(name, luamain, luadeps=[]):
    _lua_binary_or_test(name, luamain, luadeps, native.sh_binary)

def lua_test(name, luamain, luadeps=[]):
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
    script_src = "//:staleness_test.py"

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
              "sed -i 's|INSERT_FILE_LIST_HERE|" + "\\n  ".join(file_list) + "|' $@",
    )

    native.py_test(
        name = name,
        srcs = [script_name],
        data = existing_outs + [generated_pattern % file for file in outs],
        deps = [
            "//:staleness_test_lib",
        ],
    )

SrcList = provider(
    fields = {
        'srcs' : 'list of srcs',
        'hdrs' : 'list of hdrs',
    }
)

def _file_list_aspect_impl(target, ctx):
    srcs = []
    hdrs = []
    for src in ctx.rule.attr.srcs:
        srcs += src.files.to_list()
    for hdr in ctx.rule.attr.hdrs:
        hdrs += hdr.files.to_list()
    return [SrcList(srcs = srcs, hdrs = hdrs)]

_file_list_aspect = aspect(
    implementation = _file_list_aspect_impl,
)

def _upb_amalgamation(ctx):
    srcs = []
    hdrs = []
    for lib in ctx.attr.libs:
        srcs += lib[SrcList].srcs
        hdrs += lib[SrcList].hdrs
    ctx.actions.run(
        inputs = srcs + hdrs,
        outputs = ctx.outputs.outs,
        arguments = ["", ctx.bin_dir.path + "/"] + [f.path for f in srcs],
        progress_message = "Making amalgamation",
        executable = ctx.executable.amalgamator,
    )

upb_amalgamation = rule(
    implementation = _upb_amalgamation,
    attrs = {
        "amalgamator": attr.label(
            executable = True,
            cfg = "host",
        ),
        "libs": attr.label_list(aspects = [_file_list_aspect]),
        "outs": attr.output_list(),
    }
)
