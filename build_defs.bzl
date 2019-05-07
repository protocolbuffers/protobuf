
load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")

_shell_find_runfiles = """
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
    script_contents = (_shell_find_runfiles + contents).replace("$", "$$")
    native.genrule(
        name = "gen_" + name,
        outs = [out],
        cmd = "(cat <<'HEREDOC'\n%s\nHEREDOC\n) > $@" % script_contents,
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
        data = ["@lua//:lua", "@bazel_tools//tools/bash/runfiles", luamain] + luadeps,
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
        "hdrs": "list of hdrs",
    },
)

def _file_list_aspect_impl(target, ctx):
    if SrcList in target:
        return []

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
    inputs = []
    srcs = []
    for lib in ctx.attr.libs:
        inputs += lib[SrcList].srcs
        inputs += lib[SrcList].hdrs
        srcs += [src for src in lib[SrcList].srcs if src.path.endswith("c")]
    ctx.actions.run(
        inputs = inputs,
        outputs = ctx.outputs.outs,
        arguments = [ctx.bin_dir.path + "/"] + [f.path for f in srcs] + ["-I" + root for root in _get_real_roots(inputs)],
        progress_message = "Making amalgamation",
        executable = ctx.executable.amalgamator,
    )

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

is_bazel = not hasattr(native, "genmpm")

google3_dep_map = {
    "@absl//absl/base:core_headers": "//third_party/absl/base:core_headers",
    "@absl//absl/strings": "//third_party/absl/strings",
    "@com_google_protobuf//:protoc": "//third_party/protobuf:protoc",
    "@com_google_protobuf//:protobuf": "//third_party/protobuf:protobuf",
    "@com_google_protobuf//:protoc_lib": "//third_party/protobuf:libprotoc",
}

def map_dep(dep):
    if is_bazel:
        return dep
    else:
        return google3_dep_map[dep]

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

def _generate_output_file(ctx, src, extension):
    real_short_path = _get_real_short_path(src)
    output_filename = paths.replace_extension(real_short_path, extension)
    ret = ctx.new_file(ctx.genfiles_dir, output_filename)
    return ret

def filter_none(elems):
    out = []
    for elem in elems:
        if elem:
            out.append(elem)
    return out

# upb_proto_library() rule

def cc_library_func(ctx, hdrs, srcs, deps):
    compilation_contexts = []
    linking_contexts = []
    for dep in deps:
        if CcInfo in dep:
            linking_contexts.append(dep[CcInfo].linking_context)
            compilation_contexts.append(dep[CcInfo].compilation_context)
    toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        cc_toolchain = toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    compilation_info = cc_common.compile(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        name = "upb_lib",
        srcs = srcs,
        public_hdrs = hdrs,
        compilation_contexts = compilation_contexts,
    )
    linking_info = cc_common.link(
        actions = ctx.actions,
        name = "upb_lib",
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        cc_compilation_outputs = compilation_info.cc_compilation_outputs,
        linking_contexts = linking_contexts,
    )

    return CcInfo(
        compilation_context = compilation_info.compilation_context,
        linking_context = linking_info.linking_context,
    )

def _compile_upb_protos(ctx, proto_info, proto_sources, ext):
    srcs = [_generate_output_file(ctx, name, ext + ".c") for name in proto_sources]
    hdrs = [_generate_output_file(ctx, name, ext + ".h") for name in proto_sources]
    transitive_sets = list(proto_info.transitive_descriptor_sets)
    ctx.actions.run(
        inputs = depset(
            direct = [ctx.executable._upbc, proto_info.direct_descriptor_set],
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        outputs = srcs + hdrs,
        executable = ctx.executable._protoc,
        arguments = [
                        "--upb_out=" + _get_real_root(srcs[0]),
                        "--plugin=protoc-gen-upb=" + ctx.executable._upbc.path,
                        "--descriptor_set_in=" + ":".join([f.path for f in transitive_sets]),
                    ] +
                    [_get_real_short_path(file) for file in proto_sources],
                    progress_message = "Generating upb protos for :" + ctx.label.name,
    )
    return SrcList(srcs = srcs, hdrs = hdrs)

# upb_proto_library() shared code #############################################

# Can share these with upb_proto_library() once cc_common.link() supports name
# param.

def _upb_proto_rule_impl(ctx):
    if len(ctx.attr.deps) != 1:
        fail("only one deps dependency allowed.")
    dep = ctx.attr.deps[0]
    if CcInfo not in dep:
        fail("proto_library rule must generate CcInfo (aspect should have handled this).")
    lib = dep[CcInfo].linking_context.libraries_to_link[0]
    files = filter_none([
        lib.static_library,
        lib.pic_static_library,
        lib.dynamic_library,
    ])
    return [
        DefaultInfo(files = depset(files)),
        dep[CcInfo],
    ]

def _upb_proto_aspect_impl(target, ctx):
    proto_info = target[ProtoInfo]
    files = _compile_upb_protos(ctx, proto_info, proto_info.direct_sources, ctx.attr._ext)
    cc_info = cc_library_func(
        ctx = ctx,
        hdrs = files.hdrs,
        srcs = files.srcs,
        deps = ctx.rule.attr.deps + [ctx.attr._upb],
    )
    return [cc_info]

# upb_proto_library() ##########################################################

_upb_proto_library_aspect = aspect(
    attrs = {
        "_upbc": attr.label(
            executable = True,
            cfg = "host",
            default = ":protoc-gen-upb",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "host",
            default = map_dep("@com_google_protobuf//:protoc"),
        ),
        "_cc_toolchain": attr.label(default = "@bazel_tools//tools/cpp:current_cc_toolchain"),
        "_upb": attr.label(default = ":upb"),
        "_ext": attr.string(default = ".upb"),
    },
    implementation = _upb_proto_aspect_impl,
    attr_aspects = ["deps"],
    fragments = ["cpp"],
)

upb_proto_library = rule(
    output_to_genfiles = True,
    implementation = _upb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_upb_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)

# upb_proto_srcs() #############################################################

def _upb_proto_srcs_impl(ctx):
    srcs = []
    hdrs = []
    for dep in ctx.attr.deps:
        if hasattr(dep, "proto"):
            proto_info = dep[ProtoInfo]
            files = _compile_upb_protos(ctx, proto_info, proto_info.transitive_sources, ctx.attr.ext)
            srcs += files.srcs
            hdrs += files.hdrs
    return [
        SrcList(srcs = srcs, hdrs = hdrs),
        DefaultInfo(files = depset(srcs + hdrs)),
    ]

upb_proto_srcs = rule(
    attrs = {
        "_upbc": attr.label(
            executable = True,
            cfg = "host",
            default = ":protoc-gen-upb",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "host",
            default = map_dep("@com_google_protobuf//:protoc"),
        ),
        "deps": attr.label_list(),
        "ext": attr.string(default = ".upb")
    },
    implementation = _upb_proto_srcs_impl,
)

# upb_proto_reflection_library() ###############################################

def upb_proto_reflection_library(name, deps):
    srcs_rule = name + "_defsrcs.cc"
    upb_proto_srcs(
        name = srcs_rule,
        deps = deps,
        ext = ".upbdefs",
    )
    native.cc_library(
        name = name,
        srcs = [":" + srcs_rule],
        deps = [":upb", ":reflection"],
        copts = ["-Ibazel-out/k8-fastbuild/bin"],
    )

def licenses(*args):
    # No-op (for Google-internal usage).
    pass
