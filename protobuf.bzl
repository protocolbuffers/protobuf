# -*- mode: python; -*- PYTHON-PREPROCESSING-REQUIRED

def _GetPath(ctx, path):
  if ctx.label.workspace_root:
    return ctx.label.workspace_root + '/' + path
  else:
    return path

def _GenDir(ctx):
  if not ctx.attr.includes:
    return ctx.label.workspace_root
  if not ctx.attr.includes[0]:
    return _GetPath(ctx, ctx.label.package)
  if not ctx.label.package:
    return _GetPath(ctx, ctx.attr.includes[0])
  return _GetPath(ctx, ctx.label.package + '/' + ctx.attr.includes[0])

def _CcOuts(srcs, use_grpc_plugin=False):
  ret = [s[:-len(".proto")] + ".pb.h" for s in srcs] + \
        [s[:-len(".proto")] + ".pb.cc" for s in srcs]
  if use_grpc_plugin:
    ret += [s[:-len(".proto")] + ".grpc.pb.h" for s in srcs] + \
           [s[:-len(".proto")] + ".grpc.pb.cc" for s in srcs]
  return ret

def _PyOuts(srcs):
  return [s[:-len(".proto")] + "_pb2.py" for s in srcs]

def _RelativeOutputPath(path, include):
  if include == None:
    return path

  if not path.startswith(include):
    fail("Include path %s isn't part of the path %s." % (include, path))

  if include and include[-1] != '/':
    include = include + '/'

  path = path[len(include):]

  if not path.startswith(PACKAGE_NAME):
    fail("The package %s is not within the path %s" % (PACKAGE_NAME, path))

  if not PACKAGE_NAME:
    return path

  return path[len(PACKAGE_NAME)+1:]

def _proto_gen_impl(ctx):
  """General implementation for generating protos"""
  srcs = ctx.files.srcs
  deps = []
  deps += ctx.files.srcs
  gen_dir = _GenDir(ctx)
  if gen_dir:
    import_flags = ["-I" + gen_dir]
  else:
    import_flags = ["-I."]

  for dep in ctx.attr.deps:
    import_flags += dep.proto.import_flags
    deps += dep.proto.deps

  args = []
  if ctx.attr.gen_cc:
    args += ["--cpp_out=" + ctx.var["GENDIR"] + "/" + gen_dir]
  if ctx.attr.gen_py:
    args += ["--python_out=" + ctx.var["GENDIR"] + "/" + gen_dir]

  if ctx.executable.grpc_cpp_plugin:
    args += ["--plugin=protoc-gen-grpc=" + ctx.executable.grpc_cpp_plugin.path]
    args += ["--grpc_out=" + ctx.var["GENDIR"] + "/" + gen_dir]

  if args:
    ctx.action(
        inputs=srcs + deps,
        outputs=ctx.outputs.outs,
        arguments=args + import_flags + [s.path for s in srcs],
        executable=ctx.executable.protoc,
    )

  return struct(
      proto=struct(
          srcs=srcs,
          import_flags=import_flags,
          deps=deps,
      ),
  )

_proto_gen = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label_list(providers = ["proto"]),
        "includes": attr.string_list(),
        "protoc": attr.label(
            cfg = HOST_CFG,
            executable = True,
            single_file = True,
            mandatory = True,
        ),
        "grpc_cpp_plugin": attr.label(
            cfg = HOST_CFG,
            executable = True,
            single_file = True,
        ),
        "gen_cc": attr.bool(),
        "gen_py": attr.bool(),
        "outs": attr.output_list(),
    },
    output_to_genfiles = True,
    implementation = _proto_gen_impl,
)

def cc_proto_library(
        name,
        srcs=[],
        deps=[],
        cc_libs=[],
        include=None,
        protoc="//google/protobuf:protoc",
        internal_bootstrap_hack=False,
        use_grpc_plugin=False,
        default_runtime="//google/protobuf:protobuf",
        **kargs):
  """Bazel rule to create a C++ protobuf library from proto source files

  NOTE: the rule is only an internal workaround to generate protos. The
  interface may change and the rule may be removed when bazel has introduced
  the native rule.

  Args:
    name: the name of the cc_proto_library.
    srcs: the .proto files of the cc_proto_library.
    deps: a list of dependency labels; must be cc_proto_library.
    cc_libs: a list of other cc_library targets depended by the generated
        cc_library.
    include: a string indicating the include path of the .proto files.
    protoc: the label of the protocol compiler to generate the sources.
    internal_bootstrap_hack: a flag indicate the cc_proto_library is used only
        for bootstraping. When it is set to True, no files will be generated.
        The rule will simply be a provider for .proto files, so that other
        cc_proto_library can depend on it.
    use_grpc_plugin: a flag to indicate whether to call the grpc C++ plugin
        when processing the proto files.
    default_runtime: the implicitly default runtime which will be depended on by
        the generated cc_library target.
    **kargs: other keyword arguments that are passed to cc_library.

  """

  includes = []
  if include != None:
    includes = [include]

  if internal_bootstrap_hack:
    # For pre-checked-in generated files, we add the internal_bootstrap_hack
    # which will skip the codegen action.
    _proto_gen(
        name=name + "_genproto",
        srcs=srcs,
        deps=[s + "_genproto" for s in deps],
        includes=includes,
        protoc=protoc,
        visibility=["//visibility:public"],
    )
    # An empty cc_library to make rule dependency consistent.
    native.cc_library(
        name=name,
        **kargs)
    return

  grpc_cpp_plugin = None
  if use_grpc_plugin:
    grpc_cpp_plugin = "//external:grpc_cpp_plugin"

  outs = _CcOuts(srcs, use_grpc_plugin)

  _proto_gen(
      name=name + "_genproto",
      srcs=srcs,
      deps=[s + "_genproto" for s in deps],
      includes=includes,
      protoc=protoc,
      grpc_cpp_plugin=grpc_cpp_plugin,
      gen_cc=1,
      outs=outs,
      visibility=["//visibility:public"],
  )

  if default_runtime and not default_runtime in cc_libs:
    cc_libs += [default_runtime]
  if use_grpc_plugin:
    cc_libs += ["//external:grpc_lib"]

  native.cc_library(
      name=name,
      srcs=outs,
      deps=cc_libs + deps,
      includes=includes,
      **kargs)

def internal_copied_filegroup(
        name,
        srcs,
        include,
        **kargs):
  """Bazel rule to fix sources file to workaround with python path issues.

  Args:
    name: the name of the internal_copied_filegroup rule, which will be the
        name of the generated filegroup.
    srcs: the source files to be copied.
    include: the expected import root of the source.
    **kargs: extra arguments that will be passed into the filegroup.
  """
  outs = [_RelativeOutputPath(s, include) for s in srcs]

  native.genrule(
      name=name+"_genrule",
      srcs=srcs,
      outs=outs,
      cmd=" && ".join(["cp $(location %s) $(location %s)" %
                       (s, _RelativeOutputPath(s, include))
                       for s in srcs]))

  native.filegroup(
      name=name,
      srcs=outs,
      **kargs)

def py_proto_library(
        name,
        srcs=[],
        deps=[],
        py_libs=[],
        py_extra_srcs=[],
        include=None,
        default_runtime="//google/protobuf:protobuf_python",
        protoc="//google/protobuf:protoc",
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
    **kargs: other keyword arguments that are passed to cc_library.

  """
  outs = _PyOuts(srcs)

  includes = []
  if include != None:
    includes = [include]

  _proto_gen(
      name=name + "_genproto",
      srcs=srcs,
      deps=[s + "_genproto" for s in deps],
      includes=includes,
      protoc=protoc,
      gen_py=1,
      outs=outs,
      visibility=["//visibility:public"],
  )

  if include != None:
    # Copy the output files to the desired location to make the import work.
    internal_copied_filegroup_name=name + "_internal_copied_filegroup"
    internal_copied_filegroup(
        name=internal_copied_filegroup_name,
        srcs=outs,
        include=include)
    outs=[internal_copied_filegroup_name]

  if default_runtime and not default_runtime in py_libs + deps:
    py_libs += [default_runtime]

  native.py_library(
      name=name,
      srcs=outs+py_extra_srcs,
      deps=py_libs+deps,
      **kargs)

def internal_protobuf_py_tests(
    name,
    modules=[],
    **kargs):
  """Bazel rules to create batch tests for protobuf internal.

  Args:
    name: the name of the rule.
    modules: a list of modules for tests. The macro will create a py_test for
        each of the parameter with the source "google/protobuf/%s.py"
    kargs: extra parameters that will be passed into the py_test.

  """
  for m in modules:
    s = _RelativeOutputPath(
        "python/google/protobuf/internal/%s.py" % m, "python")
    native.py_test(
        name="py_%s" % m,
        srcs=[s],
        main=s,
        **kargs)
