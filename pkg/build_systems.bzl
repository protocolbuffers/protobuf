# Starlark utilities for working with other build systems

load("@rules_pkg//:providers.bzl", "PackageFilegroupInfo", "PackageFilesInfo")

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
# Aspect that extracts srcs, hdrs, etc.
################################################################################

CcFileList = provider(
    doc = "List of files to be built into a library.",
    fields = {
        # As a rule of thumb, `hdrs` and `textual_hdrs` are the files that
        # would be installed along with a prebuilt library.
        "hdrs": "public header files, including those used by generated code",
        "textual_hdrs": "files which are included but are not self-contained",

        # The `internal_hdrs` are header files which appear in `srcs`.
        # These are only used when compiling the library.
        "internal_hdrs": "internal header files (only used to build .cc files)",
        "srcs": "source files",
    },
)

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

def _combine_cc_file_lists(file_lists):
    hdrs = {}
    textual_hdrs = {}
    internal_hdrs = {}
    srcs = {}
    for file_list in file_lists:
        hdrs.update({f: 1 for f in file_list.hdrs})
        textual_hdrs.update({f: 1 for f in file_list.textual_hdrs})
        internal_hdrs.update({f: 1 for f in file_list.internal_hdrs})
        srcs.update({f: 1 for f in file_list.srcs})
    return CcFileList(
        hdrs = sorted(hdrs.keys()),
        textual_hdrs = sorted(textual_hdrs.keys()),
        internal_hdrs = sorted(internal_hdrs.keys()),
        srcs = sorted(srcs.keys()),
    )

def _file_list_aspect_impl(target, ctx):
    # We're going to reach directly into the attrs on the traversed rule.
    rule_attr = ctx.rule.attr
    providers = []

    # Extract sources from a `cc_library` (or similar):
    if CcInfo in target:
        # CcInfo is a proxy for what we expect this rule to look like.
        # However, some deps may expose `CcInfo` without having `srcs`,
        # `hdrs`, etc., so we use `getattr` to handle that gracefully.

        internal_hdrs = []
        srcs = []

        # Filter `srcs` so it only contains source files. Headers will go
        # into `internal_headers`.
        for src in _flatten_target_files(getattr(rule_attr, "srcs", [])):
            if src.extension.lower() in ["c", "cc", "cpp", "cxx"]:
                srcs.append(src)
            else:
                internal_hdrs.append(src)

        providers.append(CcFileList(
            hdrs = _flatten_target_files(getattr(rule_attr, "hdrs", [])),
            textual_hdrs = _flatten_target_files(getattr(
                rule_attr,
                "textual_hdrs",
                [],
            )),
            internal_hdrs = internal_hdrs,
            srcs = srcs,
        ))

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

def _create_file_list_impl(fragment_generator):
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

    def _impl(ctx):
        out = ctx.outputs.out

        fragments = []
        for srcrule, libname in ctx.attr.src_libs.items():
            if CcFileList in srcrule:
                cc_file_list = srcrule[CcFileList]
                fragments.extend([
                    fragment_generator(
                        srcrule.label,
                        libname + "_srcs",
                        ctx.attr.source_prefix,
                        [f.short_path for f in cc_file_list.srcs],
                    ),
                    fragment_generator(
                        srcrule.label,
                        libname + "_hdrs",
                        ctx.attr.source_prefix,
                        [f.short_path for f in (cc_file_list.hdrs +
                                                cc_file_list.textual_hdrs)],
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

            if files == {} and DefaultInfo in srcrule and CcInfo not in srcrule:
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

    return _impl

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
            "Targets can be C++ rules (like `cc_library` or `cc_test`), " +
            "`proto_library` rules, files, `filegroup` rules, `pkg_files` " +
            "rules, or `pkg_filegroup` rules. " +
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
        "# {owner}\n" +
        "set({varname}\n" +
        "{entries}\n" +
        ")\n"
    ).format(
        owner = owner,
        varname = varname,
        entries = "\n".join(["  %s%s" % (prefix, f) for f in entries]),
    )

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
    implementation = _create_file_list_impl(_cmake_var_fragment),
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
    implementation = _create_file_list_impl(_automake_var_fragment),
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
