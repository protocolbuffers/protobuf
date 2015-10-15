# -*- mode: python; -*- PYTHON-PREPROCESSING-REQUIRED

def _gen_dir(ctx):
  if not ctx.attr.prefix:
    return ctx.label.package
  if not ctx.label.package:
    return ctx.attr.prefix
  return ctx.label.package + '/' + ctx.attr.prefix

def CcOuts(srcs):
  return [s[:-len(".proto")] +  ".pb.h" for s in srcs] + \
         [s[:-len(".proto")] +  ".pb.cc" for s in srcs]

def PyOuts(srcs):
  return [s[:-len(".proto")] +  "_pb2.py" for s in srcs]

def _proto_srcs_impl(ctx):
  """General implementation for calculating proto srcs"""
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
        inputs= srcs + deps,
        outputs=ctx.outputs.outs,
        arguments= args + import_flags + [s.path for s in srcs],
        executable=ctx.executable.protoc
    )

  return struct(
      proto=struct(
            srcs = srcs,
            import_flags = import_flags,
            deps = deps,
          ),
      )

_proto_srcs = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label_list(providers = ["proto"]),
        "prefix": attr.string(),
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
    implementation = _proto_srcs_impl,
)

def cc_proto_library(
    name,
    srcs=[],
    protoc=":protoc",
    internal_bootstrap_hack=False,
    prefix="",
    proto_deps=[],
    deps=[],
    **kargs):

  if internal_bootstrap_hack:
    # For pre-checked-in generated files, we add the internal_bootstrap_hack
    # which will skip the codegen action.
    _proto_srcs(
        name = name + "_genproto",
        srcs = srcs,
        deps = [s + "_genproto" for s in proto_deps],
        prefix = prefix,
        protoc = protoc,
    )
    # An empty cc_library to make rule dependency consistent.
    native.cc_library(
        name = name,
        **kargs)
    return

  outs = CcOuts(srcs)
  _proto_srcs(
      name = name + "_genproto",
      srcs = srcs,
      deps = [s + "_genproto" for s in proto_deps],
      prefix = prefix,
      protoc = protoc,
      gen_cc = 1,
      outs = outs,
  )

  native.cc_library(
      name = name,
      srcs = outs,
      deps = deps + proto_deps,
      includes = [prefix],
      **kargs)
