"""Starlark utilities for working with other build systems."""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@rules_pkg//:providers.bzl", "PackageFilegroupInfo", "PackageFilesInfo")
load(":cc_dist_library.bzl", "CcFileList")

################################################################################
# Macro to create CMake and Automake source lists.
################################################################################

def gen_file_lists(name, out_stem, **kwargs):
    gen_cmake_file_lists(
        name = name + "_cmake",
        out = out_stem + ".cmake",
        source_prefix = "${protobuf_SOURCE_DIR}/",
        **kwargs
    )
    native.filegroup(
        name = name,
        srcs = [
            out_stem + ".cmake",
        ],
        visibility = ["//src:__pkg__"],
    )

################################################################################
# Aspect that extracts srcs, hdrs, etc.
################################################################################

ProtoFileList = provider(
    doc = "List of proto files and generated code to be built into a library.",
    fields = {
        # Proto files:
        "proto_srcs": "proto file sources",

        # Generated sources:
        "hdrs": "header files that are expected to be generated",
        "srcs": "source files that are expected to be generated",
    },
)

def _flatten_target_files(targets):
    files = []
    for target in targets:
        for tfile in target.files.to_list():
            files.append(tfile)
    return files

def _file_list_aspect_impl(target, ctx):
    # We're going to reach directly into the attrs on the traversed rule.
    rule_attr = ctx.rule.attr
    providers = []

    # Extract sources from a `proto_library`:
    if ProtoInfo in target:
        proto_srcs = []
        srcs = []
        hdrs = []
        for src in _flatten_target_files(rule_attr.srcs):
            proto_srcs.append(src)
            srcs.append("%s/%s.pb.cc" % (src.dirname, src.basename))
            hdrs.append("%s/%s.pb.h" % (src.dirname, src.basename))

        providers.append(ProtoFileList(
            proto_srcs = proto_srcs,
            srcs = srcs,
            hdrs = hdrs,
        ))

    return providers

file_list_aspect = aspect(
    doc = """
Aspect to provide the list of sources and headers from a rule.

Output is CcFileList and/or ProtoFileList. Example:

  cc_library(
      name = "foo",
      srcs = [
          "foo.cc",
          "foo_internal.h",
      ],
      hdrs = ["foo.h"],
      textual_hdrs = ["foo_inl.inc"],
  )
  # produces:
  # CcFileList(
  #     hdrs = [File("foo.h")],
  #     textual_hdrs = [File("foo_inl.inc")],
  #     internal_hdrs = [File("foo_internal.h")],
  #     srcs = [File("foo.cc")],
  # )

  proto_library(
      name = "bar_proto",
      srcs = ["bar.proto"],
  )
  # produces:
  # ProtoFileList(
  #     proto_srcs = ["bar.proto"],
  #     # Generated filenames are synthesized:
  #     hdrs = ["bar.pb.h"],
  #     srcs = ["bar.pb.cc"],
  # )
""",
    implementation = _file_list_aspect_impl,
)

################################################################################
# Generic source lists generation
#
# This factory creates a rule implementation that is parameterized by a
# fragment generator function.
################################################################################

def _create_file_list_impl(ctx, fragment_generator):
    # `fragment_generator` is a function like:
    #     def fn(originating_rule: Label,
    #            varname: str,
    #            source_prefix: str,
    #            path_strings: [str]) -> str
    #
    # It returns a string that defines `varname` to `path_strings`, each
    # prepended with `source_prefix`.
    #
    # When dealing with `File` objects, the `short_path` is used to strip
    # the output prefix for generated files.

    out = ctx.outputs.out

    fragments = []
    for srcrule, value in ctx.attr.src_libs.items():
        split_value = value.split(",")
        libname = split_value[0]
        gencode_dir = split_value[1] if len(split_value) == 2 else ""
        if CcFileList in srcrule:
            cc_file_list = srcrule[CcFileList]

            # Turn depsets of files into sorted lists.
            srcs = sorted(cc_file_list.srcs.to_list())
            hdrs = sorted(
                depset(transitive = [
                    cc_file_list.textual_hdrs,
                    cc_file_list.hdrs,
                ]).to_list(),
            )

            fragments.extend([
                fragment_generator(
                    srcrule.label,
                    libname + "_srcs",
                    ctx.attr.source_prefix,
                    [f.short_path for f in srcs],
                ),
                fragment_generator(
                    srcrule.label,
                    libname + "_hdrs",
                    ctx.attr.source_prefix,
                    [f.short_path for f in hdrs],
                ),
            ])

        if ProtoFileList in srcrule:
            proto_file_list = srcrule[ProtoFileList]
            fragments.extend([
                fragment_generator(
                    srcrule.label,
                    libname + "_proto_srcs",
                    ctx.attr.source_prefix,
                    [f.short_path for f in proto_file_list.proto_srcs],
                ),
                fragment_generator(
                    srcrule.label,
                    libname + "_srcs",
                    ctx.attr.source_prefix,
                    [gencode_dir + paths.basename(s) if gencode_dir else s for s in proto_file_list.srcs],
                ),
                fragment_generator(
                    srcrule.label,
                    libname + "_hdrs",
                    ctx.attr.source_prefix,
                    [gencode_dir + paths.basename(s) if gencode_dir else s for s in proto_file_list.hdrs],
                ),
            ])

        files = {}

        if PackageFilegroupInfo in srcrule:
            for pkg_files_info, origin in srcrule[PackageFilegroupInfo].pkg_files:
                # keys are the destination path:
                files.update(pkg_files_info.dest_src_map)

        if PackageFilesInfo in srcrule:
            # keys are the destination:
            files.update(srcrule[PackageFilesInfo].dest_src_map)

        if files == {} and DefaultInfo in srcrule and CcFileList not in srcrule:
            # This could be an individual file or filegroup.
            # We explicitly ignore rules with CcInfo, since their
            # output artifacts are libraries or binaries.
            files.update(
                {
                    f.short_path: 1
                    for f in srcrule[DefaultInfo].files.to_list()
                },
            )

        if files:
            fragments.append(
                fragment_generator(
                    srcrule.label,
                    libname + "_files",
                    ctx.attr.source_prefix,
                    sorted(files.keys()),
                ),
            )

    generator_label = "@//%s:%s" % (ctx.label.package, ctx.label.name)
    ctx.actions.write(
        output = out,
        content = (ctx.attr._header % generator_label) + "\n".join(fragments),
    )

    return [DefaultInfo(files = depset([out]))]

# Common rule attrs for rules that use `_create_file_list_impl`:
# (note that `_header` is also required)
_source_list_common_attrs = {
    "out": attr.output(
        doc = (
            "The generated filename. This should usually have a build " +
            "system-specific extension, like `out.am` or `out.cmake`."
        ),
        mandatory = True,
    ),
    "src_libs": attr.label_keyed_string_dict(
        doc = (
            "A dict, {target: libname[,gencode_dir]} of libraries to include. " +
            "Targets can be C++ rules (like `cc_library` or `cc_test`), " +
            "`proto_library` rules, files, `filegroup` rules, `pkg_files` " +
            "rules, or `pkg_filegroup` rules. " +
            "The libname is a string, and used to construct the variable " +
            "name in the `out` file holding the target's sources. " +
            "For generated files, the output root (like `bazel-bin/`) is not " +
            "included. gencode_dir is used instead of target's location if provided." +
            "For `pkg_files` and `pkg_filegroup` rules, the destination path " +
            "is used."
        ),
        mandatory = True,
        providers = [
            [CcFileList],
            [DefaultInfo],
            [PackageFilegroupInfo],
            [PackageFilesInfo],
            [ProtoFileList],
        ],
        aspects = [file_list_aspect],
    ),
    "source_prefix": attr.string(
        doc = "String to prepend to each source path.",
    ),
}

################################################################################
# CMake source lists generation
################################################################################

def _cmake_var_fragment(owner, varname, prefix, entries):
    """Returns a single `set(varname ...)` fragment (CMake syntax).

    Args:
      owner: Label, the rule that owns these srcs.
      varname: str, the var name to set.
      prefix: str, prefix to prepend to each of `entries`.
      entries: [str], the entries in the list.

    Returns:
      A string.
    """
    return (
        "# @//{package}:{name}\n" +
        "set({varname}\n" +
        "{entries}\n" +
        ")\n"
    ).format(
        package = owner.package,
        name = owner.name,
        varname = varname,
        # Strip out "wkt/google/protobuf/" from the well-known type file paths.
        # This is currently necessary to allow checked-in and generated
        # versions of the well-known type generated code to coexist.
        entries = "\n".join(["  %s%s" % (prefix, f.replace("wkt/google/protobuf/", "")) for f in entries]),
    )

def _cmake_file_list_impl(ctx):
    _create_file_list_impl(ctx, _cmake_var_fragment)

gen_cmake_file_lists = rule(
    doc = """
Generates a CMake-syntax file with lists of files.

The generated file defines variables with lists of files from `srcs`. The
intent is for these files to be included from a non-generated CMake file
which actually defines the libraries based on these lists.

For C++ rules, the following are generated:
    {libname}_srcs: contains srcs.
    {libname}_hdrs: contains hdrs and textual_hdrs.

For proto_library, the following are generated:
    {libname}_proto_srcs: contains the srcs from the `proto_library` rule.
    {libname}_srcs: contains syntesized paths for generated C++ sources.
    {libname}_hdrs: contains syntesized paths for generated C++ headers.

""",
    implementation = _cmake_file_list_impl,
    attrs = dict(
        _source_list_common_attrs,
        _header = attr.string(
            default = """\
# Auto-generated by %s
#
# This file contains lists of sources based on Bazel rules. It should
# be included from a hand-written CMake file that defines targets.
#
# Changes to this file will be overwritten based on Bazel definitions.

if(${CMAKE_VERSION} VERSION_GREATER 3.10 OR ${CMAKE_VERSION} VERSION_EQUAL 3.10)
  include_guard()
endif()

""",
        ),
    ),
)
