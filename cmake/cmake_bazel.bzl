# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""`cmake_bazel()`: a CMake build split into fine-grained, cacheable actions.

A CMake build run as a single Bazel action (e.g. rules_foreign_cc's `cmake()`)
rebuilds the whole project whenever any input changes. `cmake_bazel()` builds
the same CMake project, with the same compiler and linker command lines
CMake generates, but as many small actions that Bazel caches (locally, in a
remote cache, or on remote execution) independently:

  configure  1 action. Runs `cmake -G Ninja` over the CMake files only; every
             other source file is an empty stub. Records the compile and link
             commands Ninja would run. Re-runs only when a CMake file, the
             cache entries, the toolchain, a dependency, or the set of source
             file *names* changes -- not when a source file is edited.
  compile    1 action per .c/.cc file in `srcs` (except `textual_srcs`). Runs
             the compile command(s) CMake recorded for it, from its own spec
             file. Inputs are that file and the headers it can include (from
             `header_deps`, see below; otherwise all headers in `srcs`).
  link       1 action. Replays CMake's archive/link commands over all objects.
  package    1 action. `cmake --install` into the output directory.

CMake itself never builds anything: Ninja is only queried for the commands it
would run, and the actions above run them.

Dependencies are other `cmake_bazel()` targets. Their install directories (and
those of their dependencies) are put on `CMAKE_PREFIX_PATH`, so the project
finds them with `find_package(... CONFIG)`, as a downstream CMake user would.

CMake and Ninja are the official release binaries pinned in MODULE.bazel; the
compiler is Bazel's C++ toolchain.

Recorded commands use paths relative to the build directory (../src, ../deps),
so they are the same on every run and machine.

Limitations: Linux only; no generated sources (CMake custom commands). Without
`header_deps`, editing any header in `srcs` recompiles every source.
"""

load("@rules_cc//cc:action_names.bzl", "ACTION_NAMES")
load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cc_toolchain", "use_cc_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

CmakeBazelInfo = provider(
    doc = "A CMake install tree built by cmake_bazel().",
    fields = {
        "install_dir": "Tree artifact: this project's CMake install prefix.",
        "install_dirs": "Depset of the install prefixes of this project and " +
                        "all its transitive dependencies.",
    },
)

_SOURCE_EXTENSIONS = (".c", ".cc", ".cpp", ".cxx")
_HEADER_EXTENSIONS = (".h", ".hh", ".hpp", ".hxx", ".inc")

def _rel_path(f):
    """Path of `f` relative to the root of the repository that contains it."""
    if f.owner and f.owner.workspace_name:
        prefix = "../" + f.owner.workspace_name + "/"
        if f.short_path.startswith(prefix):
            return f.short_path[len(prefix):]
    return f.short_path

def _src_map(f):
    return "%s=%s" % (f.path, _rel_path(f))

def _find_file(files, suffix, what):
    for f in files.to_list():
        if f.path.endswith(suffix):
            return f
    fail("cmake_bazel: no %s (a file ending in %s)" % (what, suffix))

# C++ toolchain features that only make sense for Bazel's own cc_* compiles:
# they need per-target variables (e.g. `module_name`, `module_map_file`) that
# do not exist here, and CMake's compiles have no module maps anyway.
# protobuf's .bazelrc enables layering_check globally.
_UNSUPPORTED_FEATURES = [
    "header_modules",
    "layering_check",
    "module_maps",
    "parse_headers",
    "use_header_modules",
]

def _resolve_cc_tools(ctx):
    cc_toolchain = find_cc_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = [f for f in ctx.features if f not in _UNSUPPORTED_FEATURES],
        unsupported_features = ctx.disabled_features + _UNSUPPORTED_FEATURES,
    )

    def tool(action):
        return cc_common.get_tool_for_action(
            feature_configuration = feature_configuration,
            action_name = action,
        )

    def compile_flags(action, user_flags):
        return cc_common.get_memory_inefficient_command_line(
            feature_configuration = feature_configuration,
            action_name = action,
            variables = cc_common.create_compile_variables(
                feature_configuration = feature_configuration,
                cc_toolchain = cc_toolchain,
                user_compile_flags = user_flags,
            ),
        )

    cpp = ctx.fragments.cpp
    return struct(
        cc = tool(ACTION_NAMES.c_compile),
        cxx = tool(ACTION_NAMES.cpp_compile),
        ar = tool(ACTION_NAMES.cpp_link_static_library),
        cflags = compile_flags(ACTION_NAMES.c_compile, cpp.copts + cpp.conlyopts),
        cxxflags = compile_flags(ACTION_NAMES.cpp_compile, cpp.copts + cpp.cxxopts),
        ldflags = cc_common.get_memory_inefficient_command_line(
            feature_configuration = feature_configuration,
            action_name = ACTION_NAMES.cpp_link_executable,
            variables = cc_common.create_link_variables(
                feature_configuration = feature_configuration,
                cc_toolchain = cc_toolchain,
                is_using_linker = True,
                user_link_flags = cpp.linkopts,
            ),
        ),
        files = cc_toolchain.all_files,
    )

def _params_args(ctx):
    args = ctx.actions.args()
    args.use_param_file("@%s", use_always = True)
    args.set_param_file_format("multiline")
    return args

def _write_list(ctx, name, values, map_each = None):
    """Writes one entry per line into a file, for large lists."""
    out = ctx.actions.declare_file("%s/%s.txt" % (ctx.label.name, name))
    args = ctx.actions.args()
    args.set_param_file_format("multiline")
    args.add_all(values, map_each = map_each)
    ctx.actions.write(out, args)
    return out

_CmakeHeadersInfo = provider(
    doc = "Per-source header sets, collected from a Bazel cc_library graph.",
    fields = {
        "by_src": "Dict: repository-relative path of each .c/.cc compiled by this " +
                  "target or its transitive deps -> depset of the headers Bazel's " +
                  "own compile of that file gets (this repository's source files only).",
    },
)

def _is_local_source(f):
    return f.is_source and not f.owner.workspace_name

def _cmake_headers_aspect_impl(target, ctx):
    by_src = {}
    for attr_name in ("deps", "implementation_deps"):
        for d in getattr(ctx.rule.attr, attr_name, []):
            if _CmakeHeadersInfo in d:
                by_src.update(d[_CmakeHeadersInfo].by_src)
    if target.label.workspace_name:
        return [_CmakeHeadersInfo(by_src = by_src)]

    own_headers = []
    own_sources = []
    for attr_name in ("srcs", "hdrs", "textual_hdrs"):
        for f in getattr(ctx.rule.files, attr_name, []):
            if attr_name == "srcs" and f.basename.endswith(_SOURCE_EXTENSIONS):
                if _is_local_source(f):
                    own_sources.append(f)
            elif f.basename.endswith(_HEADER_EXTENSIONS + _SOURCE_EXTENSIONS):
                # Headers, and sources #included textually (textual_hdrs).
                own_headers.append(f)
    if own_sources:
        # What Bazel gives this library's compiles: its own headers plus the
        # CcInfo headers of its deps. That includes headers that rules add
        # internally (e.g. upb_c_proto_library adds upb runtime headers), not
        # only those reachable through `deps` attributes. Keep only this
        # repository's checked-in files: Bazel-generated headers (e.g. a
        # generated descriptor.upb.h) are not built for this; the CMake build
        # uses checked-in copies of those (see `common_hdrs`).
        all_headers = depset(own_headers, transitive = [
            d[CcInfo].compilation_context.headers
            for attr_name in ("deps", "implementation_deps")
            for d in getattr(ctx.rule.attr, attr_name, [])
            if CcInfo in d
        ])
        headers = depset([f for f in all_headers.to_list() if _is_local_source(f)])
        for f in own_sources:
            by_src[_rel_path(f)] = headers
    return [_CmakeHeadersInfo(by_src = by_src)]

# Walks a cc_library graph (through cc_dist_library / cc_library `deps`) to
# find, for each source file, the headers Bazel's own compile of it gets. With
# layering_check, Bazel enforces that these cover every #include.
_cmake_headers_aspect = aspect(
    implementation = _cmake_headers_aspect_impl,
    attr_aspects = ["deps", "implementation_deps"],
)

def _dep_dir_arg(d):
    # A stable, unique name per dependency install tree, so the recorded
    # commands refer to it as ../deps/<name>.
    return "%s=%s" % (d.path, d.short_path.replace("/", "_").replace(".", "_"))

def _cmake_bazel_impl(ctx):
    name = ctx.label.name
    cmake_files = ctx.attr._cmake[DefaultInfo].files
    cmake = _find_file(cmake_files, "/bin/cmake", "cmake binary").path
    ninja_files = depset([ctx.file._ninja])
    ninja = ctx.file._ninja.path
    cc = _resolve_cc_tools(ctx)

    dep_dirs = depset(transitive = [
        d[CmakeBazelInfo].install_dirs
        for d in ctx.attr.deps
    ])

    def add_dep_dirs(args):
        args.add_all(
            dep_dirs,
            map_each = _dep_dir_arg,
            format_each = "--dep-dir=%s",
            expand_directories = False,
        )

    cmake_srcs = ctx.files.cmake_srcs
    cmake_src_paths = {_rel_path(f): True for f in cmake_srcs}
    textual_srcs = ctx.files.textual_srcs
    textual_paths = {_rel_path(f): True for f in textual_srcs}
    srcs = ctx.files.srcs
    sources = [
        f
        for f in srcs
        if f.basename.endswith(_SOURCE_EXTENSIONS) and _rel_path(f) not in textual_paths
    ]
    headers = [f for f in srcs if f.basename.endswith(_HEADER_EXTENSIONS)] + textual_srcs

    headers_by_src = {}
    for d in ctx.attr.header_deps:
        headers_by_src.update(d[_CmakeHeadersInfo].by_src)

    # One compile spec per source, written by configure.
    specs = {
        _rel_path(src): ctx.actions.declare_file("%s/specs/%s.json" % (name, _rel_path(src)))
        for src in sources
    }

    # 1. Configure.
    link_spec = ctx.actions.declare_file(name + "/link_spec.json")
    build_tree = ctx.actions.declare_file(name + "/build_tree.tar")
    stubs = _write_list(ctx, "stubs", [
        _rel_path(f)
        for f in srcs
        if _rel_path(f) not in cmake_src_paths
    ])
    spec_map = _write_list(ctx, "spec_map", [
        "%s=%s" % (rel, spec.path)
        for rel, spec in specs.items()
    ])

    args = _params_args(ctx)
    args.add("configure")
    args.add("--cmake", cmake)
    args.add("--ninja", ninja)
    args.add("--cc", cc.cc)
    args.add("--cxx", cc.cxx)
    args.add("--ar", cc.ar)
    args.add_all(cc.cflags, format_each = "--cflag=%s")
    args.add_all(cc.cxxflags, format_each = "--cxxflag=%s")
    args.add_all(cc.ldflags, format_each = "--ldflag=%s")
    args.add_all(cmake_srcs, map_each = _src_map, format_each = "--cmake-src=%s")
    args.add("--stubs", stubs)
    args.add("--spec-map", spec_map)
    add_dep_dirs(args)
    args.add_all(
        ["%s=%s" % (k, v) for k, v in sorted(ctx.attr.cache_entries.items())],
        format_each = "--cache-entry=%s",
    )
    args.add("--out-link-spec", link_spec)
    args.add("--out-build-tree", build_tree)
    ctx.actions.run(
        executable = ctx.executable._runner,
        arguments = [args],
        inputs = depset(cmake_srcs + [stubs, spec_map], transitive = [dep_dirs]),
        tools = depset(transitive = [cc.files, cmake_files, ninja_files]),
        outputs = [link_spec, build_tree] + specs.values(),
        mnemonic = "CMakeBazelConfigure",
        progress_message = "Configuring CMake for %{label}",
    )

    # 2. Compile: one action per source file, with only its own compile spec
    # and the headers it can include: those of its cc_library in
    # `header_deps` plus `common_hdrs`, or else every header in `srcs`.
    all_headers = depset(headers)
    all_header_map = _write_list(ctx, "headers", all_headers, map_each = _src_map)
    common_hdrs = depset(ctx.files.common_hdrs + textual_srcs)
    obj_tars = []
    for src in sources:
        rel = _rel_path(src)
        if rel in headers_by_src:
            src_headers = depset(transitive = [headers_by_src[rel], common_hdrs])
            header_map = ctx.actions.declare_file("%s/headers/%s.txt" % (name, rel))
            header_args = ctx.actions.args()
            header_args.set_param_file_format("multiline")
            header_args.add_all(src_headers, map_each = _src_map)
            ctx.actions.write(header_map, header_args)
        else:
            src_headers = all_headers
            header_map = all_header_map
        obj_tar = ctx.actions.declare_file("%s/objs/%s.tar" % (name, rel))
        obj_tars.append(obj_tar)
        args = ctx.actions.args()
        args.add("compile")
        args.add("--spec", specs[rel])
        args.add("--src", _src_map(src))
        args.add("--headers", header_map)
        add_dep_dirs(args)
        args.add("--out-tar", obj_tar)
        ctx.actions.run(
            executable = ctx.executable._runner,
            arguments = [args],
            inputs = depset(
                [src, specs[rel], header_map],
                transitive = [src_headers, dep_dirs],
            ),
            tools = cc.files,
            outputs = [obj_tar],
            mnemonic = "CMakeBazelCompile",
            progress_message = "Compiling %s (CMake) for %%{label}" % rel,
        )

    # 3. Link.
    libs_tar = ctx.actions.declare_file(name + "/libs.tar")
    obj_tar_list = _write_list(ctx, "obj_tars", obj_tars)
    args = _params_args(ctx)
    args.add("link")
    args.add("--spec", link_spec)
    args.add_all(cmake_srcs, map_each = _src_map, format_each = "--cmake-src=%s")
    args.add("--obj-tars", obj_tar_list)
    add_dep_dirs(args)
    args.add("--out-tar", libs_tar)
    ctx.actions.run(
        executable = ctx.executable._runner,
        arguments = [args],
        inputs = depset([link_spec, obj_tar_list] + obj_tars + cmake_srcs, transitive = [dep_dirs]),
        tools = depset(transitive = [cc.files, cmake_files]),
        outputs = [libs_tar],
        mnemonic = "CMakeBazelLink",
        progress_message = "Linking CMake targets for %{label}",
    )

    # 4. Package.
    install_dir = ctx.actions.declare_directory(name + "/install")
    all_src_map = _write_list(ctx, "all_srcs", srcs + cmake_srcs, map_each = _src_map)
    args = ctx.actions.args()
    args.add("package")
    args.add("--cmake", cmake)
    args.add("--build-tree", build_tree)
    args.add("--libs-tar", libs_tar)
    args.add("--src-map", all_src_map)
    add_dep_dirs(args)
    args.add("--out-dir", install_dir.path)
    ctx.actions.run(
        executable = ctx.executable._runner,
        arguments = [args],
        inputs = depset([build_tree, libs_tar, all_src_map] + srcs + cmake_srcs, transitive = [dep_dirs]),
        tools = cmake_files,
        outputs = [install_dir],
        mnemonic = "CMakeBazelPackage",
        progress_message = "Installing CMake package for %{label}",
    )

    return [
        DefaultInfo(files = depset([install_dir])),
        CmakeBazelInfo(
            install_dir = install_dir,
            install_dirs = depset([install_dir], transitive = [dep_dirs]),
        ),
        OutputGroupInfo(
            objects = depset(obj_tars),
            libs = depset([libs_tar]),
            spec = depset([link_spec, build_tree] + specs.values()),
        ),
    ]

cmake_bazel = rule(
    implementation = _cmake_bazel_impl,
    doc = "Builds a CMake project as fine-grained, cacheable Bazel actions " +
          "(configure, one compile per source, link, install). See the " +
          "documentation at the top of cmake_bazel.bzl.",
    attrs = {
        "cache_entries": attr.string_dict(
            doc = "CMake cache entries, passed to configure as `-D<key>=<value>`.",
        ),
        "cmake_srcs": attr.label_list(
            doc = "The files CMake reads while configuring: CMakeLists.txt, *.cmake, " +
                  "configure_file() templates, and files named in link options. Editing " +
                  "these re-runs configure.",
            allow_files = True,
            mandatory = True,
        ),
        "common_hdrs": attr.label_list(
            doc = "Headers added to every compile that uses `header_deps`: files the " +
                  "CMake build includes that are not sources of any cc_library there " +
                  "(e.g. checked-in generated headers used only by CMake).",
            allow_files = True,
        ),
        "deps": attr.label_list(
            doc = "Other `cmake_bazel()` targets whose packages this project finds with " +
                  "find_package().",
            providers = [CmakeBazelInfo],
        ),
        "header_deps": attr.label_list(
            doc = "Bazel C++ targets (cc_library or cc_dist_library) of this project that " +
                  "compile the same sources as the CMake build. Each source compiled by " +
                  "one of their transitive cc_librarys gets, as compile inputs, the " +
                  "headers Bazel's own compile of it gets (that library's headers and " +
                  "its deps' CcInfo headers; checked-in files only), plus `common_hdrs`; " +
                  "other sources get every header in `srcs`.",
            aspects = [_cmake_headers_aspect],
        ),
        "srcs": attr.label_list(
            doc = "Every other file the CMake build reads: sources, headers, and files it " +
                  "installs. Each .c/.cc file gets its own compile action. Paths are " +
                  "relative to the root of the repository containing each file, and must " +
                  "match where CMake expects them.",
            allow_files = True,
            mandatory = True,
        ),
        "textual_srcs": attr.label_list(
            doc = "Source files that are #included by other sources rather than compiled " +
                  "on their own (e.g. GoogleTest's gtest-all.cc includes the other " +
                  "src/*.cc). They are inputs of every compile action and get no compile " +
                  "action of their own. May overlap with `srcs`.",
            allow_files = True,
        ),
        "_cmake": attr.label(
            default = Label("@cmake_bazel_cmake//:cmake"),
            cfg = "exec",
        ),
        "_ninja": attr.label(
            default = Label("@cmake_bazel_ninja//:ninja"),
            allow_single_file = True,
            cfg = "exec",
        ),
        "_runner": attr.label(
            default = Label("//cmake:cmake_bazel_runner"),
            executable = True,
            cfg = "exec",
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cc_toolchain(),
)
