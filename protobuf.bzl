# -*- mode: python; -*- PYTHON-PREPROCESSING-REQUIRED

def _GenDir(ctx):
  if ctx.attr.include == None:
    return ""
  if not ctx.attr.include:
    return ctx.label.package
  if not ctx.label.package:
    return ctx.attr.include
  return ctx.label.package + '/' + ctx.attr.include

def _CcOuts(srcs):
  return [s[:-len(".proto")] +  ".pb.h" for s in srcs] + \
         [s[:-len(".proto")] + ".pb.cc" for s in srcs]

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
  import_flags = ["-I" + gen_dir]
  for dep in ctx.attr.deps:
    import_flags += dep.proto.import_flags
    deps += dep.proto.deps

  args = []
  if ctx.attr.gen_cc:
    args += ["--cpp_out=" + ctx.var["GENDIR"] + "/" + gen_dir]
  if ctx.attr.gen_py:
    args += ["--python_out=" + ctx.var["GENDIR"] + "/" + gen_dir]

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
        "include": attr.string(),
        "protoc": attr.label(
            executable = True,
            single_file = True,
            mandatory = True,
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
        protoc=":protoc",
        internal_bootstrap_hack=False,
        **kargs):
  """Bazel rule to create a C++ protobuf library from proto source files

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
    **kargs: other keyword arguments that are passed to cc_library.

  """

  if internal_bootstrap_hack:
    # For pre-checked-in generated files, we add the internal_bootstrap_hack
    # which will skip the codegen action.
    _proto_gen(
        name=name + "_genproto",
        srcs=srcs,
        deps=[s + "_genproto" for s in deps],
        include=include,
        protoc=protoc,
    )
    # An empty cc_library to make rule dependency consistent.
    native.cc_library(
        name=name,
        **kargs)
    return

  outs = _CcOuts(srcs)
  _proto_gen(
      name=name + "_genproto",
      srcs=srcs,
      deps=[s + "_genproto" for s in deps],
      include=include,
      protoc=protoc,
      gen_cc=1,
      outs=outs,
  )

  includes = []
  if include != None:
    includes = [include]

  native.cc_library(
      name=name,
      srcs=outs,
      deps=cc_libs + deps,
      includes=includes,
      **kargs)


def copied_srcs(
        name,
        srcs,
        include,
        **kargs):
  outs = [_RelativeOutputPath(s, include) for s in srcs]

  native.genrule(
      name=name+"_genrule",
      srcs=srcs,
      outs=outs,
      cmd=";".join(["cp $(location %s) $(location %s)" % \
                    (s, _RelativeOutputPath(s, include)) \
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
        protoc=":protoc",
        **kargs):
  outs = _PyOuts(srcs)
  _proto_gen(
      name=name + "_genproto",
      srcs=srcs,
      deps=[s + "_genproto" for s in deps],
      include=include,
      protoc=protoc,
      gen_py=1,
      outs=outs,
  )

  copied_srcs_name=name + "_copied_srcs"
  if include != None:
    copied_srcs(
        name=copied_srcs_name,
        srcs=outs,
        include=include)
    srcs=[copied_srcs_name]

  native.py_library(
      name=name,
      srcs=srcs+py_extra_srcs,
      deps=py_libs,
      **kargs)

def internal_protobuf_py_tests(
    name,
    modules=[],
    **kargs):
  for m in modules:
    native.py_test(
        name="py_%s" % m,
        srcs=["google/protobuf/internal/%s.py" % m],
        main="google/protobuf/internal/%s.py" % m,
        **kargs)
