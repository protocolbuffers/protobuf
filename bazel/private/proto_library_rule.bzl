# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""
Implementation of proto_library rule.
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")

STRICT_DEPS_FLAG_TEMPLATE = (
    #
    "--direct_dependencies_violation_msg=" +
    "%%s is imported, but %s doesn't directly depend on a proto_library that 'srcs' it."
)

def _check_srcs_package(target_package, srcs):
    """Check that .proto files in sources are from the same package.

    This is done to avoid clashes with the generated sources."""

    #TODO: this does not work with filegroups that contain files that are not in the package
    for src in srcs:
        if target_package != src.label.package:
            fail("Proto source with label '%s' must be in same package as consuming rule." % src.label)

def _get_import_prefix(ctx):
    """Gets and verifies import_prefix attribute if it is declared."""

    import_prefix = ctx.attr.import_prefix

    if not paths.is_normalized(import_prefix):
        fail("should be normalized (without uplevel references or '.' path segments)", attr = "import_prefix")
    if paths.is_absolute(import_prefix):
        fail("should be a relative path", attr = "import_prefix")

    return import_prefix

def _get_strip_import_prefix(ctx):
    """Gets and verifies strip_import_prefix."""

    strip_import_prefix = ctx.attr.strip_import_prefix

    if not paths.is_normalized(strip_import_prefix):
        fail("should be normalized (without uplevel references or '.' path segments)", attr = "strip_import_prefix")

    if paths.is_absolute(strip_import_prefix):
        strip_import_prefix = strip_import_prefix[1:]
    else:  # Relative to current package
        strip_import_prefix = _join(ctx.label.package, strip_import_prefix)

    return strip_import_prefix.removesuffix("/")

def _proto_library_impl(ctx):
    # Verifies attributes.
    _check_srcs_package(ctx.label.package, ctx.attr.srcs)
    srcs = ctx.files.srcs
    deps = [dep[ProtoInfo] for dep in ctx.attr.deps]
    exports = [dep[ProtoInfo] for dep in ctx.attr.exports]
    import_prefix = _get_import_prefix(ctx)
    strip_import_prefix = _get_strip_import_prefix(ctx)
    check_for_reexport = deps + exports if not srcs else exports
    _PackageSpecificationInfo = bazel_features.globals.PackageSpecificationInfo
    for proto in check_for_reexport:
        if getattr(proto, "allow_exports", None):
            if not _PackageSpecificationInfo:
                fail("Allowlist checks not supported before Bazel 6.4.0")
            if not proto.allow_exports[_PackageSpecificationInfo].contains(ctx.label):
                fail("proto_library '%s' can't be reexported in package '//%s'" % (proto.direct_descriptor_set.owner, ctx.label.package))

    proto_path, virtual_srcs = _process_srcs(ctx, srcs, import_prefix, strip_import_prefix)
    descriptor_set = ctx.actions.declare_file(ctx.label.name + "-descriptor-set.proto.bin")
    proto_info = ProtoInfo(
        srcs = virtual_srcs,
        deps = deps,
        descriptor_set = descriptor_set,
        proto_path = proto_path,
        workspace_root = ctx.label.workspace_root,
        bin_dir = ctx.bin_dir.path,
        allow_exports = ctx.attr.allow_exports,
    )

    _write_descriptor_set(ctx, proto_info, deps, exports, descriptor_set)

    # We assume that the proto sources will not have conflicting artifacts
    # with the same root relative path
    data_runfiles = ctx.runfiles(
        files = [proto_info.direct_descriptor_set],
        transitive_files = depset(transitive = [proto_info.transitive_sources]),
    )
    return [
        proto_info,
        DefaultInfo(
            files = depset([proto_info.direct_descriptor_set]),
            default_runfiles = ctx.runfiles(),  # empty
            data_runfiles = data_runfiles,
        ),
    ]

def _process_srcs(ctx, srcs, import_prefix, strip_import_prefix):
    """Returns proto_path and sources, optionally symlinking them to _virtual_imports.

    Returns:
      (str, [File]) A pair of proto_path and virtual_sources.
    """
    if import_prefix != "" or strip_import_prefix != "":
        # Use virtual source roots
        return _symlink_to_virtual_imports(ctx, srcs, import_prefix, strip_import_prefix)
    else:
        # No virtual source roots
        return "", srcs

def _join(*path):
    return "/".join([p for p in path if p != ""])

def _symlink_to_virtual_imports(ctx, srcs, import_prefix, strip_import_prefix):
    """Symlinks srcs to _virtual_imports.

    Returns:
          A pair proto_path, directs_sources.
    """
    virtual_imports = _join("_virtual_imports", ctx.label.name)
    proto_path = _join(ctx.label.package, virtual_imports)

    if ctx.label.workspace_name == "":
        full_strip_import_prefix = strip_import_prefix
    else:
        full_strip_import_prefix = _join("..", ctx.label.workspace_name, strip_import_prefix)
    if full_strip_import_prefix:
        full_strip_import_prefix += "/"

    virtual_srcs = []
    for src in srcs:
        # Remove strip_import_prefix
        if not src.short_path.startswith(full_strip_import_prefix):
            fail(".proto file '%s' is not under the specified strip prefix '%s'" %
                 (src.short_path, full_strip_import_prefix))
        import_path = src.short_path[len(full_strip_import_prefix):]

        # Add import_prefix
        virtual_src = ctx.actions.declare_file(_join(virtual_imports, import_prefix, import_path))
        ctx.actions.symlink(
            output = virtual_src,
            target_file = src,
            progress_message = "Symlinking virtual .proto sources for %{label}",
        )
        virtual_srcs.append(virtual_src)
    return proto_path, virtual_srcs

def _write_descriptor_set(ctx, proto_info, deps, exports, descriptor_set):
    """Writes descriptor set."""
    if proto_info.direct_sources == []:
        ctx.actions.write(descriptor_set, "")
        return

    dependencies_descriptor_sets = depset(transitive = [dep.transitive_descriptor_sets for dep in deps])

    args = ctx.actions.args()

    if ctx.attr._experimental_proto_descriptor_sets_include_source_info[BuildSettingInfo].value:
        args.add("--include_source_info")
    args.add("--retain_options")

    strict_deps = ctx.attr._strict_proto_deps[BuildSettingInfo].value
    if strict_deps:
        if proto_info.direct_sources:
            strict_importable_sources = depset(
                direct = proto_info.direct_sources,
                transitive = [dep.check_deps_sources for dep in deps],
            )
        else:
            strict_importable_sources = None
        if strict_importable_sources:
            args.add_joined(
                "--direct_dependencies",
                strict_importable_sources,
                map_each = proto_common.get_import_path,
                join_with = ":",
            )
            # Example: `--direct_dependencies a.proto:b.proto`

        else:
            # The proto compiler requires an empty list to turn on strict deps checking
            args.add("--direct_dependencies=")

        # Set `-direct_dependencies_violation_msg=`
        args.add(ctx.label, format = STRICT_DEPS_FLAG_TEMPLATE)

    strict_imports = ctx.attr._strict_public_imports[BuildSettingInfo].value
    if strict_imports:
        public_import_protos = depset(transitive = [export.check_deps_sources for export in exports])
        if not public_import_protos:
            # This line is necessary to trigger the check.
            args.add("--allowed_public_imports=")
        else:
            args.add_joined(
                "--allowed_public_imports",
                public_import_protos,
                map_each = proto_common.get_import_path,
                join_with = ":",
            )
    if proto_common.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION:
        toolchain = ctx.toolchains[toolchains.PROTO_TOOLCHAIN]
        if not toolchain:
            fail("Protocol compiler toolchain could not be resolved.")
        proto_lang_toolchain_info = toolchain.proto
    else:
        proto_lang_toolchain_info = proto_common.ProtoLangToolchainInfo(
            out_replacement_format_flag = "--descriptor_set_out=%s",
            output_files = "single",
            mnemonic = "GenProtoDescriptorSet",
            progress_message = "Generating Descriptor Set proto_library %{label}",
            proto_compiler = ctx.executable._proto_compiler,
            protoc_opts = ctx.fragments.proto.experimental_protoc_opts,
            plugin = None,
        )

    proto_common.compile(
        ctx.actions,
        proto_info,
        proto_lang_toolchain_info,
        generated_files = [descriptor_set],
        additional_inputs = dependencies_descriptor_sets,
        additional_args = args,
    )

_extra_doc = ""

proto_library = rule(
    implementation = _proto_library_impl,
    # TODO: proto_common docs are missing
    # TODO: ProtoInfo link doesn't work and docs are missing
    doc = """
<p>Use <code>proto_library</code> to define libraries of protocol buffers which
may be used from multiple languages. A <code>proto_library</code> may be listed
in the <code>deps</code> clause of supported rules, such as
<code>java_proto_library</code>.

<p>When compiled on the command-line, a <code>proto_library</code> creates a file
named <code>foo-descriptor-set.proto.bin</code>, which is the descriptor set for
the messages the rule srcs. The file is a serialized
<code>FileDescriptorSet</code>, which is described in
<a href="https://developers.google.com/protocol-buffers/docs/techniques#self-description">
https://developers.google.com/protocol-buffers/docs/techniques#self-description</a>.

<p>It only contains information about the <code>.proto</code> files directly
mentioned by a <code>proto_library</code> rule; the collection of transitive
descriptor sets is available through the
<code>[ProtoInfo].transitive_descriptor_sets</code> Starlark provider.
See documentation in <code>proto_info.bzl</code>.

<p>Recommended code organization:
<ul>
<li>One <code>proto_library</code> rule per <code>.proto</code> file.
<li>A file named <code>foo.proto</code> will be in a rule named <code>foo_proto</code>,
  which is located in the same package.
<li>A <code>[language]_proto_library</code> that wraps a <code>proto_library</code>
  named <code>foo_proto</code> should be called <code>foo_[language]_proto</code>,
  and be located in the same package.
</ul>
""" + _extra_doc,
    attrs = {
        "srcs": attr.label_list(
            allow_files = [".proto", ".protodevel"],
            flags = ["DIRECT_COMPILE_TIME_INPUT"],
            # TODO: Should .protodevel be advertised or deprecated?
            doc = """
The list of <code>.proto</code> and <code>.protodevel</code> files that are
processed to create the target. This is usually a non empty list. One usecase
where <code>srcs</code> can be empty is an <i>alias-library</i>. This is a
proto_library rule having one or more other proto_library in <code>deps</code>.
This pattern can be used to e.g. export a public api under a persistent name.""",
        ),
        "deps": attr.label_list(
            providers = [ProtoInfo],
            doc = """
The list of other <code>proto_library</code> rules that the target depends upon.
A <code>proto_library</code> may only depend on other <code>proto_library</code>
targets. It may not depend on language-specific libraries.""",
        ),
        "exports": attr.label_list(
            providers = [ProtoInfo],
            doc = """
List of proto_library targets that can be referenced via "import public" in the
proto source.
It's an error if you use "import public" but do not list the corresponding library
in the exports attribute.
Note that you have list the library both in deps and exports since not all
lang_proto_library implementations have been changed yet.""",
        ),
        "strip_import_prefix": attr.string(
            default = "/",
            doc = """
The prefix to strip from the paths of the .proto files in this rule.

<p>When set, .proto source files in the <code>srcs</code> attribute of this rule are
accessible at their path with this prefix cut off.

<p>If it's a relative path (not starting with a slash), it's taken as a package-relative
one. If it's an absolute one, it's understood as a repository-relative path.

<p>The prefix in the <code>import_prefix</code> attribute is added after this prefix is
stripped.""",
        ),
        "import_prefix": attr.string(
            doc = """
The prefix to add to the paths of the .proto files in this rule.

<p>When set, the .proto source files in the <code>srcs</code> attribute of this rule are
accessible at is the value of this attribute prepended to their repository-relative path.

<p>The prefix in the <code>strip_import_prefix</code> attribute is removed before this
prefix is added.""",
        ),
        "allow_exports": attr.label(
            cfg = "exec",
            providers = [bazel_features.globals.PackageSpecificationInfo] if bazel_features.globals.PackageSpecificationInfo else [],
            doc = """
An optional allowlist that prevents proto library to be reexported or used in
lang_proto_library that is not in one of the listed packages.""",
        ),
        "data": attr.label_list(
            allow_files = True,
            flags = ["SKIP_CONSTRAINTS_OVERRIDE"],
        ),
        # buildifier: disable=attr-license (calling attr.license())
        "licenses": attr.license() if hasattr(attr, "license") else attr.string_list(),
        "_experimental_proto_descriptor_sets_include_source_info": attr.label(
            default = "//bazel/private:experimental_proto_descriptor_sets_include_source_info",
        ),
        "_strict_proto_deps": attr.label(
            default =
                "//bazel/private:strict_proto_deps",
        ),
        "_strict_public_imports": attr.label(
            default = "//bazel/private:strict_public_imports",
        ),
    } | toolchains.if_legacy_toolchain({
        "_proto_compiler": attr.label(
            cfg = "exec",
            executable = True,
            allow_files = True,
            default = configuration_field("proto", "proto_compiler"),
        ),
    }),  # buildifier: disable=attr-licenses (attribute called licenses)
    fragments = [
        "proto",
    ],
    provides = [ProtoInfo],
    toolchains = toolchains.use_toolchain(toolchains.PROTO_TOOLCHAIN),
)
