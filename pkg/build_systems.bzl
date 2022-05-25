# Starlark utilities for working with other build systems

load("@rules_pkg//:providers.bzl", "PackageFilegroupInfo", "PackageFilesInfo")
load(
    ":file_list_aspects.bzl",
    "CcFileList",
    "ProtoFileList",
    "cc_file_list_aspect",
    "proto_file_list_aspect",
)

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
    gen_automake_file_lists(
        name = name + "_automake",
        out = out_stem + ".am",
        source_prefix = "$(top_srcdir)/",
        **kwargs
    )
    native.filegroup(
        name = name,
        srcs = [
            out_stem + ".cmake",
            out_stem + ".am",
        ],
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
    for srcrule, libname in ctx.attr.src_libs.items():
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
                    proto_file_list.srcs,
                ),
                fragment_generator(
                    srcrule.label,
                    libname + "_hdrs",
                    ctx.attr.source_prefix,
                    proto_file_list.hdrs,
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

    ctx.actions.write(
        output = out,
        content = (ctx.attr._header % ctx.label) + "\n".join(fragments),
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
            "A dict, {target: libname} of libraries to include. " +
            "Targets can be " +
            "files, `filegroup` rules, " +
            "`proto_library` rules, " +
            "`cc_library` (or similar C++) rules, " +
            "`cc_dist_library` rules, " +
            "`pkg_files` rules, `pkg_filegroup` rules, " +
            "or `test_suite` rules that expand to `cc_test`s. " +
            "The libname is a string, and used to construct the variable " +
            "name in the `out` file holding the target's sources. " +
            "For generated files, the output root (like `bazel-bin/`) is not " +
            "included. " +
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
        aspects = [cc_file_list_aspect, proto_file_list_aspect],
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
        "# {owner}\n" +
        "set({varname}\n" +
        "{entries}\n" +
        ")\n"
    ).format(
        owner = owner,
        varname = varname,
        entries = "\n".join(["  %s%s" % (prefix, f) for f in entries]),
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

################################################################################
# Automake source lists generation
################################################################################

def _automake_var_fragment(owner, varname, prefix, entries):
    """Returns a single variable assignment fragment (Automake syntax).

    Args:
      owner: Label, the rule that owns these srcs.
      varname: str, the var name to set.
      prefix: str, prefix to prepend to each of `entries`.
      entries: [str], the entries in the list.

    Returns:
      A string.
    """
    if len(entries) == 0:
        # A backslash followed by a blank line is illegal. We still want
        # to emit the variable, though.
        return "# {owner}\n{varname} =\n".format(
            owner = owner,
            varname = varname,
        )
    fragment = (
        "# {owner}\n" +
        "{varname} = \\\n" +
        "{entries}"
    ).format(
        owner = owner,
        varname = varname,
        entries = " \\\n".join(["  %s%s" % (prefix, f) for f in entries]),
    )
    return fragment.rstrip("\\ ") + "\n"

def _automake_file_list_impl(ctx):
    _create_file_list_impl(ctx, _automake_var_fragment)

gen_automake_file_lists = rule(
    doc = """
Generates an Automake-syntax file with lists of files.

The generated file defines variables with lists of files from `srcs`. The
intent is for these files to be included from a non-generated Makefile.am
file which actually defines the libraries based on these lists.

For C++ rules, the following are generated:
    {libname}_srcs: contains srcs.
    {libname}_hdrs: contains hdrs and textual_hdrs.

For proto_library, the following are generated:
    {libname}_proto_srcs: contains the srcs from the `proto_library` rule.
    {libname}_srcs: contains syntesized paths for generated C++ sources.
    {libname}_hdrs: contains syntesized paths for generated C++ headers.

""",
    implementation = _automake_file_list_impl,
    attrs = dict(
        _source_list_common_attrs.items(),
        _header = attr.string(
            default = """\
# Auto-generated by %s
#
# This file contains lists of sources based on Bazel rules. It should
# be included from a hand-written Makefile.am that defines targets.
#
# Changes to this file will be overwritten based on Bazel definitions.

""",
        ),
    ),
)
