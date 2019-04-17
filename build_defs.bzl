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

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "CPP_LINK_STATIC_LIBRARY_ACTION_NAME")

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
        arguments = ["", ctx.bin_dir.path + "/"] + [f.path for f in srcs],
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

# upb_proto_library() rule

def _remove_up(string):
    if string.startswith("../"):
        string = string[3:]
        pos = string.find("/")
        string = string[pos + 1:]

    return _remove_suffix(string, ".proto")

def _upb_proto_srcs_impl(ctx, suffix):
    sources = []
    outs = []
    include_dirs = {}
    for dep in ctx.attr.deps:
        if hasattr(dep, "proto"):
            for src in dep.proto.transitive_sources:
                sources.append(src)
                include_dir = _remove_suffix(src.path, _remove_up(src.short_path) + "." + src.extension)
                if include_dir:
                    include_dirs[include_dir] = True
                outs.append(ctx.actions.declare_file(_remove_up(src.short_path) + suffix + ".h"))
                outs.append(ctx.actions.declare_file(_remove_up(src.short_path) + suffix + ".c"))
                outdir = _remove_suffix(outs[-1].path, _remove_up(src.short_path) + suffix + ".c")

    source_paths = [d.path for d in sources]
    include_args = ["-I" + root for root in include_dirs.keys()]

    ctx.actions.run(
        inputs = [ctx.executable.upbc] + sources,
        outputs = outs,
        executable = ctx.executable.protoc,
        arguments = ["--upb_out", outdir, "--plugin=protoc-gen-upb=" + ctx.executable.upbc.path] + include_args + source_paths,
        progress_message = "Generating upb protos",
    )

    return [DefaultInfo(files = depset(outs))]

def _upb_proto_library_srcs_impl(ctx):
    return _upb_proto_srcs_impl(ctx, ".upb")

def _upb_proto_reflection_library_srcs_impl(ctx):
    return _upb_proto_srcs_impl(ctx, ".upbdefs")

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
    return file.path[:-len(real_short_path)]

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
    real_genfiles_dir = ret.path[:-len(output_filename)]
    return ret, real_genfiles_dir

def _generate_output_files(ctx, package, file_names, file_types):
    result = {}
    real_genfiles_dir = None
    for key in file_types.keys():
        arr = []
        for name in file_names:
            file, real_genfiles_dir = _generate_output_file(ctx, name, file_types[key])
            arr.append(file)
        result[key] = arr
    return result, real_genfiles_dir

def cc_library_func(ctx, hdrs, srcs, deps):
    compilation_contexts = []
    cc_infos = []
    for dep in deps:
        if CcInfo in dep:
            cc_infos.append(dep[CcInfo])
            compilation_contexts.append(dep[CcInfo].compilation_context)
    toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        cc_toolchain = toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    compilation_info = cc_common.compile(
        ctx = ctx,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        srcs = srcs,
        hdrs = hdrs,
        compilation_contexts = compilation_contexts,
    )
    output_file = ctx.new_file(ctx.bin_dir, "lib" + ctx.rule.attr.name + ".a")
    library_to_link = cc_common.create_library_to_link(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        static_library = output_file,
    )
    archiver_path = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
    )
    archiver_variables = cc_common.create_link_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        output_file = output_file.path,
        is_using_linker = False,
    )
    command_line = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
        variables = archiver_variables,
    )

    # Non-PIC objects only get emitted in opt builds.
    use_pic = True
    if ctx.var.get("COMPILATION_MODE") == "opt":
        use_pic = False

    object_files = compilation_info.cc_compilation_outputs.object_files(use_pic = use_pic)
    args = ctx.actions.args()
    args.add_all(command_line)
    args.add_all(object_files)

    env = cc_common.get_environment_variables(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
        variables = archiver_variables,
    )

    ctx.actions.run(
        executable = archiver_path,
        arguments = [args],
        env = env,
        inputs = depset(
            direct = object_files,
            transitive = [
                # TODO: Use CcToolchainInfo getters when available
                # See https://github.com/bazelbuild/bazel/issues/7427.
                ctx.attr._cc_toolchain.files,
            ],
        ),
        outputs = [output_file],
    )
    linking_context = cc_common.create_linking_context(
        libraries_to_link = [library_to_link],
    )
    info = CcInfo(
        compilation_context = compilation_info.compilation_context,
        linking_context = linking_context,
    )
    return cc_common.merge_cc_infos(cc_infos = [info] + cc_infos)

def _upb_proto_library_aspect_impl(target, ctx):
    proto_sources = target[ProtoInfo].direct_sources
    types = {
        "srcs": ".upb.c",
        "hdrs": ".upb.h",
    }
    files, real_genfiles_dir = _generate_output_files(
        ctx = ctx,
        package = ctx.label.package,
        file_names = proto_sources,
        file_types = types,
    )
    ctx.actions.run(
        inputs = depset(
            direct = target[ProtoInfo].direct_sources + [ctx.executable._upbc],
            transitive = [target[ProtoInfo].transitive_sources],
        ),
        outputs = files["srcs"] + files["hdrs"],
        executable = ctx.executable._protoc,
        arguments = ["--upb_out=" + _get_real_root(files["srcs"][0]),
                     "--plugin=protoc-gen-upb=" + ctx.executable._upbc.path] +
                    ["-I" + path for path in _get_real_roots(target[ProtoInfo].direct_sources + list(target[ProtoInfo].transitive_sources))] +
                    [file.path for file in proto_sources],
        progress_message = "Generating upb protos",
    )
    cc_info = cc_library_func(
        ctx = ctx,
        hdrs = files["hdrs"],
        srcs = files["srcs"],
        deps = ctx.rule.attr.deps + [ctx.attr._upb],
    )
    return [cc_info]

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
    },
    implementation = _upb_proto_library_aspect_impl,
    attr_aspects = ["deps"],
)

def _upb_proto_library_impl(ctx):
    if len(ctx.attr.deps) != 1:
        fail("only one deps dependency allowed.")
    dep = ctx.attr.deps[0]
    if CcInfo not in dep:
        fail("proto_library rule must generate CcInfo (have cc_api_version>0).")
    return [
        DefaultInfo(
            files = depset(
                [dep[CcInfo].linking_context.libraries_to_link[0].static_library],
            ),
        ),
        dep[CcInfo],
    ]

upb_proto_library = rule(
    output_to_genfiles = True,
    implementation = _upb_proto_library_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_upb_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)

_upb_proto_reflection_library_srcs = rule(
    attrs = {
        "upbc": attr.label(
            executable = True,
            cfg = "host",
        ),
        "protoc": attr.label(
            executable = True,
            cfg = "host",
            default = map_dep("@com_google_protobuf//:protoc"),
        ),
        "deps": attr.label_list(),
    },
    implementation = _upb_proto_reflection_library_srcs_impl,
)

def upb_proto_reflection_library(name, deps, upbc):
    srcs_rule = name + "_defsrcs.cc"
    _upb_proto_reflection_library_srcs(
        name = srcs_rule,
        upbc = upbc,
        deps = deps,
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
