# -*- mode: python; -*- PYTHON-PREPROCESSING-REQUIRED

def _gen_dir(ctx):
  if not ctx.attr.include:
    return ctx.label.package
  if not ctx.label.package:
    return ctx.attr.include
  return ctx.label.package + '/' + ctx.attr.include

def _cc_outs(srcs):
  return [s[:-len(".proto")] +  ".pb.h" for s in srcs] + \
         [s[:-len(".proto")] + ".pb.cc" for s in srcs]

def _py_outs(srcs):
  return [s[:-len(".proto")] + "_pb2.py" for s in srcs]

def _proto_gen_impl(ctx):
  """General implementation for generating protos"""
  srcs = ctx.files.srcs
  deps = []
  deps += ctx.files.srcs
  gen_dir = _gen_dir(ctx)
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
        include="",
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

  outs = _cc_outs(srcs)
  _proto_gen(
      name=name + "_genproto",
      srcs=srcs,
      deps=[s + "_genproto" for s in deps],
      include=include,
      protoc=protoc,
      gen_cc=1,
      outs=outs,
  )

  native.cc_library(
      name=name,
      srcs=outs,
      deps=cc_libs + deps,
      includes=[include],
      **kargs)
