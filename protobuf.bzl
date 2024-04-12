load("@bazel_skylib//lib:versions.bzl", "versions")
load("@rules_cc//cc:defs.bzl", "objc_library")
load("@rules_proto//proto:defs.bzl", "ProtoInfo")
load("@rules_python//python:defs.bzl", "py_library")
load("@rules_ruby//ruby:defs.bzl", "ruby_library")

def _GetPath(ctx, path):
    if ctx.label.workspace_root:
        return ctx.label.workspace_root + "/" + path
    else:
        return path

def _IsNewExternal(ctx):
    # Bazel 0.4.4 and older have genfiles paths that look like:
    #   bazel-out/local-fastbuild/genfiles/external/repo/foo
    # After the exec root rearrangement, they look like:
    #   ../repo/bazel-out/local-fastbuild/genfiles/foo
    return ctx.label.workspace_root.startswith("../")

def _GenDir(ctx):
    if _IsNewExternal(ctx):
        # We are using the fact that Bazel 0.4.4+ provides repository-relative paths
        # for ctx.genfiles_dir.
        return ctx.genfiles_dir.path + (
            "/" + ctx.attr.includes[0] if ctx.attr.includes and ctx.attr.includes[0] else ""
        )

    # This means that we're either in the old version OR the new version in the local repo.
    # Either way, appending the source path to the genfiles dir works.
    return ctx.var["GENDIR"] + "/" + _SourceDir(ctx)

def _SourceDir(ctx):
    if not ctx.attr.includes:
        return ctx.label.workspace_root
    if not ctx.attr.includes[0]:
        return _GetPath(ctx, ctx.label.package)
    if not ctx.label.package:
        return _GetPath(ctx, ctx.attr.includes[0])
    return _GetPath(ctx, ctx.label.package + "/" + ctx.attr.includes[0])

def _ObjcBase(srcs):
    return [
        "".join([token.capitalize() for token in src[:-len(".proto")].split("_")])
        for src in srcs
    ]

def _ObjcHdrs(srcs):
    return [src + ".pbobjc.h" for src in _ObjcBase(srcs)]

def _ObjcSrcs(srcs):
    return [src + ".pbobjc.m" for src in _ObjcBase(srcs)]

def _ObjcOuts(srcs, out_type):
    if out_type == "hdrs":
        return _ObjcHdrs(srcs)
    if out_type == "srcs":
        return _ObjcSrcs(srcs)
    return _ObjcHdrs(srcs) + _ObjcSrcs(srcs)

def _PyOuts(srcs, use_grpc_plugin = False):
    ret = [s[:-len(".proto")] + "_pb2.py" for s in srcs]
    if use_grpc_plugin:
        ret += [s[:-len(".proto")] + "_pb2_grpc.py" for s in srcs]
    return ret

def _RubyOuts(srcs):
    return [s[:-len(".proto")] + "_pb.rb" for s in srcs]

def _CsharpOuts(srcs):
    return [
        "".join([token.capitalize() for token in src[:-len(".proto")].split("_")]) + ".cs"
        for src in srcs
    ]

ProtoGenInfo = provider(
    fields = ["srcs", "import_flags", "deps"],
)

def _proto_gen_impl(ctx):
    """General implementation for generating protos"""
    srcs = ctx.files.srcs
    langs = ctx.attr.langs or []
    out_type = ctx.attr.out_type
    deps = depset(direct = ctx.files.srcs)
    source_dir = _SourceDir(ctx)
    gen_dir = _GenDir(ctx).rstrip("/")
    import_flags = []

    if source_dir:
        has_sources = any([src.is_source for src in srcs])
        if has_sources:
            import_flags += ["-I" + source_dir]
    else:
        import_flags += ["-I."]

    has_generated = any([not src.is_source for src in srcs])
    if has_generated:
        import_flags += ["-I" + gen_dir]

    if ctx.attr.includes:
        for include in ctx.attr.includes:
            import_flags += ["-I" + _GetPath(ctx, include)]

    import_flags = depset(direct = import_flags)

    for dep in ctx.attr.deps:
        dep_proto = dep[ProtoGenInfo]
        if type(dep_proto.import_flags) == "list":
            import_flags = depset(
                transitive = [import_flags],
                direct = dep_proto.import_flags,
            )
        else:
            import_flags = depset(
                transitive = [import_flags, dep_proto.import_flags],
            )
        if type(dep_proto.deps) == "list":
            deps = depset(transitive = [deps], direct = dep_proto.deps)
        else:
            deps = depset(transitive = [deps, dep_proto.deps])

    if not langs and not ctx.executable.plugin:
        return [
            ProtoGenInfo(
                srcs = srcs,
                import_flags = import_flags,
                deps = deps,
            ),
        ]

    generated_files = []
    for src in srcs:
        args = []

        in_gen_dir = src.root.path == gen_dir
        if in_gen_dir:
            import_flags_real = []
            for f in import_flags.to_list():
                path = f.replace("-I", "")
                import_flags_real.append("-I$(realpath -s %s)" % path)

        use_grpc_plugin = (ctx.attr.plugin_language == "grpc" and ctx.attr.plugin)
        path_tpl = "$(realpath %s)" if in_gen_dir else "%s"

        outs = []
        for lang in langs:
            if lang == "csharp":
                outs.extend(_CsharpOuts([src.basename]))
            elif lang == "objc":
                outs.extend(_ObjcOuts([src.basename], out_type = out_type))
            elif lang == "python":
                outs.extend(_PyOuts([src.basename], use_grpc_plugin = use_grpc_plugin))
            elif lang == "ruby":
                outs.extend(_RubyOuts([src.basename]))

            # Otherwise, rely on user-supplied outs.
            args += [("--%s_out=" + path_tpl) % (lang, gen_dir)]

        if ctx.attr.outs:
            outs.extend(ctx.attr.outs)
        outs = [ctx.actions.declare_file(out, sibling = src) for out in outs]
        generated_files.extend(outs)

        inputs = [src] + deps.to_list()
        tools = [ctx.executable.protoc]
        if ctx.executable.plugin:
            plugin = ctx.executable.plugin
            lang = ctx.attr.plugin_language
            if not lang and plugin.basename.startswith("protoc-gen-"):
                lang = plugin.basename[len("protoc-gen-"):]
            if not lang:
                fail("cannot infer the target language of plugin", "plugin_language")

            outdir = "." if in_gen_dir else gen_dir

            if ctx.attr.plugin_options:
                outdir = ",".join(ctx.attr.plugin_options) + ":" + outdir
            args += [("--plugin=protoc-gen-%s=" + path_tpl) % (lang, plugin.path)]
            args += ["--%s_out=%s" % (lang, outdir)]
            tools.append(plugin)

        if not in_gen_dir:
            ctx.actions.run(
                inputs = inputs,
                tools = tools,
                outputs = outs,
                arguments = args + import_flags.to_list() + [src.path],
                executable = ctx.executable.protoc,
                mnemonic = "ProtoCompile",
                use_default_shell_env = True,
            )
        else:
            for out in outs:
                orig_command = " ".join(
                    ["$(realpath %s)" % ctx.executable.protoc.path] + args +
                    import_flags_real + [src.basename],
                )
                command = ";".join([
                    'CMD="%s"' % orig_command,
                    "cd %s" % src.dirname,
                    "${CMD}",
                    "cd -",
                ])
                generated_out = "/".join([gen_dir, out.basename])
                if generated_out != out.path:
                    command += ";mv %s %s" % (generated_out, out.path)
                ctx.actions.run_shell(
                    inputs = inputs,
                    outputs = [out],
                    command = command,
                    mnemonic = "ProtoCompile",
                    tools = tools,
                    use_default_shell_env = True,
                )

    return [
        ProtoGenInfo(
            srcs = srcs,
            import_flags = import_flags,
            deps = deps,
        ),
        DefaultInfo(files = depset(generated_files)),
    ]

"""Generates codes from Protocol Buffers definitions.

This rule helps you to implement Skylark macros specific to the target
language. You should prefer more specific `cc_proto_library `,
`py_proto_library` and others unless you are adding such wrapper macros.

Args:
  srcs: Protocol Buffers definition files (.proto) to run the protocol compiler
    against.
  deps: a list of dependency labels; must be other proto libraries.
  includes: a list of include paths to .proto files.
  protoc: the label of the protocol compiler to generate the sources.
  plugin: the label of the protocol compiler plugin to be passed to the protocol
    compiler.
  plugin_language: the language of the generated sources
  plugin_options: a list of options to be passed to the plugin
  langs: generates sources in addition to the ones from the plugin for each
    specified language.
  outs: a list of labels of the expected outputs from the protocol compiler.
  out_type: only generated a single type of source file for languages that have
    split sources (e.g. *.h and *.cc in C++)
"""
_proto_gen = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label_list(providers = [ProtoGenInfo]),
        "includes": attr.string_list(),
        "protoc": attr.label(
            cfg = "exec",
            executable = True,
            allow_single_file = True,
            mandatory = True,
        ),
        "plugin": attr.label(
            cfg = "exec",
            allow_files = True,
            executable = True,
        ),
        "plugin_language": attr.string(),
        "plugin_options": attr.string_list(),
        "langs": attr.string_list(),
        "outs": attr.string_list(),
        "out_type": attr.string(
            default = "all",
        ),
    },
    output_to_genfiles = True,
    implementation = _proto_gen_impl,
)

def _internal_gen_well_known_protos_java_impl(ctx):
    args = ctx.actions.args()

    deps = [d[ProtoInfo] for d in ctx.attr.deps]

    srcjar = ctx.actions.declare_file("{}.srcjar".format(ctx.attr.name))
    if ctx.attr.javalite:
        java_out = "lite:%s" % srcjar.path
    else:
        java_out = srcjar

    args.add("--java_out", java_out)

    descriptors = depset(
        transitive = [dep.transitive_descriptor_sets for dep in deps],
    )
    args.add_joined(
        "--descriptor_set_in",
        descriptors,
        join_with = ctx.configuration.host_path_separator,
    )

    for dep in deps:
        if "." == dep.proto_source_root:
            args.add_all([src.path for src in dep.direct_sources])
        else:
            source_root = dep.proto_source_root
            offset = len(source_root) + 1  # + '/'.
            args.add_all([src.path[offset:] for src in dep.direct_sources])

    ctx.actions.run(
        executable = ctx.executable._protoc,
        inputs = descriptors,
        outputs = [srcjar],
        arguments = [args],
        mnemonic = "ProtoCompile",
        use_default_shell_env = True,
    )

    return [
        DefaultInfo(
            files = depset([srcjar]),
        ),
    ]

internal_gen_well_known_protos_java = rule(
    implementation = _internal_gen_well_known_protos_java_impl,
    attrs = {
        "deps": attr.label_list(
            mandatory = True,
            providers = [ProtoInfo],
        ),
        "javalite": attr.bool(
            default = False,
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "exec",
            default = "//:protoc",
        ),
    },
)

def _internal_gen_kt_protos(ctx):
    args = ctx.actions.args()

    deps = [d[ProtoInfo] for d in ctx.attr.deps]

    srcjar = ctx.actions.declare_file("{}.srcjar".format(ctx.attr.name))
    if ctx.attr.lite:
        out = "lite:%s" % srcjar.path
    else:
        out = srcjar

    args.add("--kotlin_out", out)

    descriptors = depset(
        transitive = [dep.transitive_descriptor_sets for dep in deps],
    )
    args.add_joined(
        "--descriptor_set_in",
        descriptors,
        join_with = ctx.configuration.host_path_separator,
    )

    for dep in deps:
        if "." == dep.proto_source_root:
            args.add_all([src.path for src in dep.direct_sources])
        else:
            source_root = dep.proto_source_root
            offset = len(source_root) + 1  # + '/'.
            args.add_all([src.path[offset:] for src in dep.direct_sources])

    ctx.actions.run(
        executable = ctx.executable._protoc,
        inputs = descriptors,
        outputs = [srcjar],
        arguments = [args],
        mnemonic = "ProtoCompile",
        use_default_shell_env = True,
    )

    return [
        DefaultInfo(
            files = depset([srcjar]),
        ),
    ]

internal_gen_kt_protos = rule(
    implementation = _internal_gen_kt_protos,
    attrs = {
        "deps": attr.label_list(
            mandatory = True,
            providers = [ProtoInfo],
        ),
        "lite": attr.bool(
            default = False,
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "exec",
            default = "//:protoc",
        ),
    },
)

def internal_objc_proto_library(
        name,
        srcs = [],
        deps = [],
        outs = [],
        proto_deps = [],
        includes = ["."],
        default_runtime = Label("//:protobuf_objc"),
        protoc = Label("//:protoc"),
        testonly = None,
        visibility = ["//visibility:public"],
        **kwargs):
    """Bazel rule to create a Objective-C protobuf library from proto source
    files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the objc_proto_library.
      srcs: the .proto files to compile.
      deps: a list of dependency labels; must be objc_proto_library.
      outs: a list of expected output files.
      proto_deps: a list of proto file dependencies that don't have a
        objc_proto_library rule.
      include: a string indicating the include path of the .proto files.
      default_runtime: the Objective-C Protobuf runtime
      protoc: the label of the protocol compiler to generate the sources.
      testonly: common rule attribute (see:
          https://bazel.build/reference/be/common-definitions#common-attributes)
      visibility: the visibility of the generated files.
      **kwargs: other keyword arguments that are passed to py_library.

    """
    full_deps = [d + "_genproto" for d in deps]

    if proto_deps:
        _proto_gen(
            name = name + "_deps_genproto",
            testonly = testonly,
            srcs = proto_deps,
            protoc = protoc,
            includes = includes,
        )
        full_deps.append(":%s_deps_genproto" % name)

    # Note: we need to run the protoc build twice to get separate targets for
    # the generated header and the source files.
    _proto_gen(
        name = name + "_genproto_hdrs",
        srcs = srcs,
        deps = full_deps,
        langs = ["objc"],
        out_type = "hdrs",
        includes = includes,
        protoc = protoc,
        testonly = testonly,
        visibility = visibility,
        tags = ["manual"],
    )

    _proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        deps = full_deps,
        langs = ["objc"],
        out_type = "srcs",
        includes = includes,
        protoc = protoc,
        testonly = testonly,
        visibility = visibility,
        tags = ["manual"],
    )

    objc_library(
        name = name,
        hdrs = [name + "_genproto_hdrs"],
        non_arc_srcs = [name + "_genproto"],
        deps = [default_runtime],
        includes = includes,
        testonly = testonly,
        visibility = visibility,
        # Don't auto-expand these targets until target_compatible_with
        # works.  See https://github.com/bazelbuild/bazel/issues/12897.
        tags = ["manual"],
        target_compatible_with = ["@platforms//os:osx"],
        **kwargs
    )

def internal_ruby_proto_library(
        name,
        srcs = [],
        deps = [],
        includes = ["."],
        default_runtime = "@com_google_protobuf//ruby:protobuf",
        protoc = "@com_google_protobuf//:protoc",
        testonly = None,
        visibility = ["//visibility:public"],
        **kwargs):
    """Bazel rule to create a Ruby protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the ruby_proto_library.
      srcs: the .proto files to compile.
      deps: a list of dependency labels; must be a internal_ruby_proto_library.
      includes: a string indicating the include path of the .proto files.
      default_runtime: the RubyProtobuf runtime
      protoc: the label of the protocol compiler to generate the sources.
      testonly: common rule attribute (see:
          https://bazel.build/reference/be/common-definitions#common-attributes)
      visibility: the visibility of the generated files.
      **kwargs: other keyword arguments that are passed to ruby_library.

    """

    # Note: we need to run the protoc build twice to get separate targets for
    # the generated header and the source files.
    _proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        deps = [s + "_genproto" for s in deps],
        langs = ["ruby"],
        includes = includes,
        protoc = protoc,
        testonly = testonly,
        visibility = visibility,
        tags = ["manual"],
    )

    deps = []
    if default_runtime:
        deps.append(default_runtime)
    ruby_library(
        name = name,
        srcs = [name + "_genproto"],
        deps = deps,
        testonly = testonly,
        visibility = visibility,
        includes = includes,
        **kwargs
    )

# When canonical labels are in use, use additional "@" prefix
_canonical_label_prefix = "@" if str(Label("//:protoc")).startswith("@@") else ""

def _to_label(label_str):
    """Converts a string to a label using the repository of the calling thread"""
    if type(label_str) == type(Label("//:foo")):
        return label_str
    return Label(_canonical_label_prefix + native.repository_name() + "//" + native.package_name() + ":foo").relative(label_str)

def internal_py_proto_library(
        name,
        srcs = [],
        deps = [],
        py_libs = [],
        py_extra_srcs = [],
        include = None,
        default_runtime = Label("//:protobuf_python"),
        protoc = Label("//:protoc"),
        use_grpc_plugin = False,
        testonly = None,
        **kargs):
    """Bazel rule to create a Python protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the py_proto_library.
      srcs: the .proto files of the py_proto_library.
      deps: a list of dependency labels; must be py_proto_library.
      py_libs: a list of other py_library targets depended by the generated
          py_library.
      py_extra_srcs: extra source files that will be added to the output
          py_library. This attribute is used for internal bootstrapping.
      include: a string indicating the include path of the .proto files.
      default_runtime: the implicitly default runtime which will be depended on by
          the generated py_library target.
      protoc: the label of the protocol compiler to generate the sources.
      use_grpc_plugin: a flag to indicate whether to call the Python C++ plugin
          when processing the proto files.
      testonly: common rule attribute (see:
          https://bazel.build/reference/be/common-definitions#common-attributes)
      **kargs: other keyword arguments that are passed to py_library.

    """
    includes = []
    if include != None:
        includes = [include]

    grpc_python_plugin = None
    if use_grpc_plugin:
        grpc_python_plugin = "//external:grpc_python_plugin"
        # Note: Generated grpc code depends on Python grpc module. This dependency
        # is not explicitly listed in py_libs. Instead, host system is assumed to
        # have grpc installed.

    _proto_gen(
        name = name + "_genproto",
        testonly = testonly,
        srcs = srcs,
        deps = [s + "_genproto" for s in deps],
        includes = includes,
        protoc = protoc,
        langs = ["python"],
        visibility = ["//visibility:public"],
        plugin = grpc_python_plugin,
        plugin_language = "grpc",
    )

    if default_runtime:
        # Resolve non-local labels
        labels = [_to_label(lib) for lib in py_libs + deps]
        if not _to_label(default_runtime) in labels:
            py_libs = py_libs + [default_runtime]

    py_library(
        name = name,
        testonly = testonly,
        srcs = [name + "_genproto"] + py_extra_srcs,
        deps = py_libs + deps,
        imports = includes,
        **kargs
    )

def py_proto_library(
        *args,
        **kwargs):
    """Deprecated alias for use before Bazel 5.3.

    Args:
      *args: the name of the py_proto_library.
      **kwargs: other keyword arguments that are passed to py_library.

    Deprecated:
      This is provided for backwards compatibility only.  Bazel 5.3 will
      introduce support for py_proto_library, which should be used instead.
    """
    internal_py_proto_library(*args, **kwargs)

def _source_proto_library(
        name,
        srcs = [],
        deps = [],
        proto_deps = [],
        outs = [],
        lang = None,
        includes = ["."],
        protoc = Label("//:protoc"),
        testonly = None,
        visibility = ["//visibility:public"],
        **kwargs):
    """Bazel rule to create generated protobuf code from proto source files for
    languages not well supported by Bazel yet.  This will output the generated
    code as-is without any compilation.  This is most useful for interpreted
    languages that don't require it.

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      name: the name of the unsupported_proto_library.
      srcs: the .proto files to compile.  Note, that for languages where out
        needs to be provided, only a single source file is allowed.
      deps: a list of dependency labels; must be unsupported_proto_library.
      proto_deps: a list of proto file dependencies that don't have a
        unsupported_proto_library rule.
      lang: the language to (optionally) generate code for.
      outs: a list of expected output files.  This is only required for
        languages where we can't predict the outputs.
      includes: strings indicating the include path of the .proto files.
      protoc: the label of the protocol compiler to generate the sources.
      testonly: common rule attribute (see:
          https://bazel.build/reference/be/common-definitions#common-attributes)
      visibility: the visibility of the generated files.
      **kwargs: other keyword arguments that are passed to py_library.

    """
    if outs and len(srcs) != 1:
        fail("Custom outputs only allowed for single proto targets.")

    langs = []
    if lang != None:
        langs = [lang]

    full_deps = [d + "_genproto" for d in deps]

    if proto_deps:
        _proto_gen(
            name = name + "_deps_genproto",
            testonly = testonly,
            srcs = proto_deps,
            protoc = protoc,
            includes = includes,
        )
        full_deps.append(":%s_deps_genproto" % name)

    _proto_gen(
        name = name + "_genproto",
        srcs = srcs,
        deps = full_deps,
        langs = langs,
        outs = outs,
        includes = includes,
        protoc = protoc,
        testonly = testonly,
        visibility = visibility,
    )

    native.filegroup(
        name = name,
        srcs = [":%s_genproto" % name],
        testonly = testonly,
        visibility = visibility,
        **kwargs
    )

def internal_csharp_proto_library(**kwargs):
    """Bazel rule to create a C# protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      **kwargs: arguments that are passed to unsupported_proto_library.

    """

    _source_proto_library(
        lang = "csharp",
        **kwargs
    )

def internal_php_proto_library(**kwargs):
    """Bazel rule to create a PHP protobuf library from proto source files

    NOTE: the rule is only an internal workaround to generate protos. The
    interface may change and the rule may be removed when bazel has introduced
    the native rule.

    Args:
      **kwargs: arguments that are passed to unsupported_proto_library.

    """
    if not kwargs.get("outs"):
        fail("Unable to predict the outputs for php_proto_library.  Please specify them via `outs`.")

    _source_proto_library(
        lang = "php",
        **kwargs
    )

def check_protobuf_required_bazel_version():
    """For WORKSPACE files, to check the installed version of bazel.

    This ensures bazel supports our approach to proto_library() depending on a
    copied filegroup. (Fixed in bazel 0.5.4)
    """
    versions.check(minimum_bazel_version = "0.5.4")
