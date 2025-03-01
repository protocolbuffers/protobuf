"""The implementation of the `py_proto_library` rule and its aspect."""

load("@rules_python//python:py_info.bzl", "PyInfo")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")

_PY_PROTO_TOOLCHAIN = Label("//bazel/private:python_toolchain_type")

_PyProtoInfo = provider(
    doc = "Encapsulates information needed by the Python proto rules.",
    fields = {
        "imports": """
            (depset[str]) The field forwarding PyInfo.imports coming from
            the proto language runtime dependency.""",
        "runfiles_from_proto_deps": """
            (depset[File]) Files from the transitive closure implicit proto
            dependencies""",
        "transitive_sources": """(depset[File]) The Python sources.""",
    },
)

def _filter_provider(provider, *attrs):
    return [dep[provider] for attr in attrs for dep in attr if provider in dep]

def _py_proto_aspect_impl(target, ctx):
    """Generates and compiles Python code for a proto_library.

    The function runs protobuf compiler on the `proto_library` target generating
    a .py file for each .proto file.

    Args:
      target: (Target) A target providing `ProtoInfo`. Usually this means a
         `proto_library` target, but not always; you must expect to visit
         non-`proto_library` targets, too.
      ctx: (RuleContext) The rule context.

    Returns:
      ([_PyProtoInfo]) Providers collecting transitive information about
      generated files.
    """

    _proto_library = ctx.rule.attr

    # Check Proto file names
    for proto in target[ProtoInfo].direct_sources:
        import_path = proto_common.get_import_path(proto)
        if proto.is_source and "-" in import_path:
            fail("Cannot generate Python code for a .proto whose python import path contains '-' ({}).".format(
                proto.path,
            ))

    if proto_common.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION:
        toolchain = ctx.toolchains[_PY_PROTO_TOOLCHAIN]
        if not toolchain:
            fail("No toolchains registered for '%s'." % _PY_PROTO_TOOLCHAIN)
        proto_lang_toolchain_info = toolchain.proto
    else:
        proto_lang_toolchain_info = getattr(ctx.attr, "_aspect_proto_toolchain")[proto_common.ProtoLangToolchainInfo]

    api_deps = [proto_lang_toolchain_info.runtime]

    generated_sources = []
    proto_info = target[ProtoInfo]
    proto_root = proto_info.proto_source_root
    if proto_info.direct_sources:
        # Generate py files
        generated_sources = proto_common.declare_generated_files(
            actions = ctx.actions,
            proto_info = proto_info,
            extension = "_pb2.py",
            name_mapper = lambda name: name.replace("-", "_").replace(".", "/"),
        )

        # Handles multiple repository and virtual import cases
        if proto_root.startswith(ctx.bin_dir.path):
            proto_root = proto_root[len(ctx.bin_dir.path) + 1:]

        plugin_output = ctx.bin_dir.path + "/" + proto_root

        # Import path within the runfiles tree
        if proto_root.startswith("external/"):
            proto_root = proto_root[len("external") + 1:]
        else:
            proto_root = ctx.workspace_name + "/" + proto_root

        proto_common.compile(
            actions = ctx.actions,
            proto_info = proto_info,
            proto_lang_toolchain_info = proto_lang_toolchain_info,
            generated_files = generated_sources,
            plugin_output = plugin_output,
        )

    # Generated sources == Python sources
    python_sources = generated_sources

    deps = _filter_provider(_PyProtoInfo, getattr(_proto_library, "deps", []))
    runfiles_from_proto_deps = depset(
        transitive = [dep[DefaultInfo].default_runfiles.files for dep in api_deps] +
                     [dep.runfiles_from_proto_deps for dep in deps],
    )
    transitive_sources = depset(
        direct = python_sources,
        transitive = [dep.transitive_sources for dep in deps],
    )

    return [
        _PyProtoInfo(
            imports = depset(
                # Adding to PYTHONPATH so the generated modules can be
                # imported.  This is necessary when there is
                # strip_import_prefix, the Python modules are generated under
                # _virtual_imports. But it's undesirable otherwise, because it
                # will put the repo root at the top of the PYTHONPATH, ahead of
                # directories added through `imports` attributes.
                [proto_root] if "_virtual_imports" in proto_root else [],
                transitive = [dep[PyInfo].imports for dep in api_deps] + [dep.imports for dep in deps],
            ),
            runfiles_from_proto_deps = runfiles_from_proto_deps,
            transitive_sources = transitive_sources,
        ),
    ]

_py_proto_aspect = aspect(
    implementation = _py_proto_aspect_impl,
    attrs = toolchains.if_legacy_toolchain({
        "_aspect_proto_toolchain": attr.label(
            default = "//python:python_toolchain",
        ),
    }),
    attr_aspects = ["deps"],
    required_providers = [ProtoInfo],
    provides = [_PyProtoInfo],
    toolchains = toolchains.use_toolchain(_PY_PROTO_TOOLCHAIN),
)

def _py_proto_library_rule(ctx):
    """Merges results of `py_proto_aspect` in `deps`.

    Args:
      ctx: (RuleContext) The rule context.
    Returns:
      ([PyInfo, DefaultInfo, OutputGroupInfo])
    """
    if not ctx.attr.deps:
        fail("'deps' attribute mustn't be empty.")

    pyproto_infos = _filter_provider(_PyProtoInfo, ctx.attr.deps)
    default_outputs = depset(
        transitive = [info.transitive_sources for info in pyproto_infos],
    )

    return [
        DefaultInfo(
            files = default_outputs,
            default_runfiles = ctx.runfiles(transitive_files = depset(
                transitive =
                    [default_outputs] +
                    [info.runfiles_from_proto_deps for info in pyproto_infos],
            )),
        ),
        OutputGroupInfo(
            default = depset(),
        ),
        PyInfo(
            transitive_sources = default_outputs,
            imports = depset(transitive = [info.imports for info in pyproto_infos]),
            # Proto always produces 2- and 3- compatible source files
            has_py2_only_sources = False,
            has_py3_only_sources = False,
        ),
    ]

py_proto_library = rule(
    implementation = _py_proto_library_rule,
    doc = """
      Use `py_proto_library` to generate Python libraries from `.proto` files.

      The convention is to name the `py_proto_library` rule `foo_py_pb2`,
      when it is wrapping `proto_library` rule `foo_proto`.

      `deps` must point to a `proto_library` rule.

      Example:

```starlark
py_library(
    name = "lib",
    deps = [":foo_py_pb2"],
)

py_proto_library(
    name = "foo_py_pb2",
    deps = [":foo_proto"],
)

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
)
```""",
    attrs = {
        "deps": attr.label_list(
            doc = """
              The list of `proto_library` rules to generate Python libraries for.

              Usually this is just the one target: the proto library of interest.
              It can be any target providing `ProtoInfo`.""",
            providers = [ProtoInfo],
            aspects = [_py_proto_aspect],
        ),
    },
    provides = [PyInfo],
)
