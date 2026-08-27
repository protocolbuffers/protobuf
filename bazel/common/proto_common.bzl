# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Definition of proto_common module, together with bazel providers for proto rules."""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")
load("//bazel/private:proto_compile.bzl", "proto_compile")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")

def _check_collocated(label, proto_info, proto_lang_toolchain_info):
    """Checks if lang_proto_library is collocated with proto_library.

    Exceptions are allowed by an allowlist defined on `proto_lang_toolchain` and
    on an allowlist defined on `proto_library`'s `allow_exports` attribute.

    If checks are not successful the function fails.

    Args:
      label: (Label) The label of lang_proto_library
      proto_info: (ProtoInfo) The ProtoInfo from the proto_library dependency.
      proto_lang_toolchain_info: (ProtoLangToolchainInfo) The proto lang toolchain info.
        Obtained from a `proto_lang_toolchain` target.
    """
    _PackageSpecificationInfo = bazel_features.globals.PackageSpecificationInfo

    if (proto_info.direct_descriptor_set.owner.package != label.package and
        proto_lang_toolchain_info.allowlist_different_package):
        if not proto_lang_toolchain_info.allowlist_different_package[_PackageSpecificationInfo].contains(label):
            fail(("lang_proto_library '%s' may only be created in the same package " +
                  "as proto_library '%s'") % (label, proto_info.direct_descriptor_set.owner))
    if (proto_info.direct_descriptor_set.owner.package != label.package and
        hasattr(proto_info, "allow_exports")):
        if not proto_info.allow_exports[_PackageSpecificationInfo].contains(label):
            fail(("lang_proto_library '%s' may only be created in the same package " +
                  "as proto_library '%s'") % (label, proto_info.direct_descriptor_set.owner))

_BAZEL_TOOLS_PREFIX = "external/bazel_tools/"

def _experimental_filter_sources(proto_info, proto_lang_toolchain_info):
    if not proto_info.direct_sources:
        return [], []

    # Collect a set of provided protos
    provided_proto_sources = proto_lang_toolchain_info.provided_proto_sources
    provided_paths = {}
    for src in provided_proto_sources:
        path = src.path

        # For listed protos bundled with the Bazel tools repository, their exec paths start
        # with external/bazel_tools/. This prefix needs to be removed first, because the protos in
        # user repositories will not have that prefix.
        if path.startswith(_BAZEL_TOOLS_PREFIX):
            provided_paths[path[len(_BAZEL_TOOLS_PREFIX):]] = None
        else:
            provided_paths[path] = None

    # Filter proto files
    proto_files = proto_info.direct_sources
    excluded = []
    included = []
    for proto_file in proto_files:
        if proto_file.path in provided_paths:
            excluded.append(proto_file)
        else:
            included.append(proto_file)
    return included, excluded

def _experimental_should_generate_code(
        proto_info,
        proto_lang_toolchain_info,
        rule_name,
        target_label):
    """Checks if the code should be generated for the given proto_library.

    The code shouldn't be generated only when the toolchain already provides it
    to the language through its runtime dependency.

    It fails when the proto_library contains mixed proto files, that should and
    shouldn't generate code.

    Args:
      proto_info: (ProtoInfo) The ProtoInfo from proto_library to check the generation for.
      proto_lang_toolchain_info: (ProtoLangToolchainInfo) The proto lang toolchain info.
        Obtained from a `proto_lang_toolchain` target or constructed ad-hoc.
      rule_name: (str) Name of the rule used in the failure message.
      target_label: (Label) The label of the target used in the failure message.

    Returns:
      (bool) True when the code should be generated.
    """
    included, excluded = _experimental_filter_sources(proto_info, proto_lang_toolchain_info)

    if included and excluded:
        fail(("The 'srcs' attribute of '%s' contains protos for which '%s' " +
              "shouldn't generate code (%s), in addition to protos for which it should (%s).\n" +
              "Separate '%s' into 2 proto_library rules.") % (
            target_label,
            rule_name,
            ", ".join([f.short_path for f in excluded]),
            ", ".join([f.short_path for f in included]),
            target_label,
        ))

    return bool(included)

def _declare_generated_files(
        actions,
        proto_info,
        extension,
        name_mapper = None):
    """Declares generated files with a specific extension.

    Use this in lang_proto_library-es when protocol compiler generates files
    that correspond to .proto file names.

    The function removes ".proto" extension with given one (e.g. ".pb.cc") and
    declares new output files.

    Args:
      actions: (ActionFactory) Obtained by ctx.actions, used to declare the files.
      proto_info: (ProtoInfo) The ProtoInfo to declare the files for.
      extension: (str) The extension to use for generated files.
      name_mapper: (str->str) A function mapped over the base filename without
        the extension. Used it to replace characters in the name that
        cause problems in a specific programming language.

    Returns:
      (list[File]) The list of declared files.
    """
    proto_sources = proto_info.direct_sources
    outputs = []

    for src in proto_sources:
        basename_no_ext = src.basename[:-(len(src.extension) + 1)]

        if name_mapper:
            basename_no_ext = name_mapper(basename_no_ext)

        # Note that two proto_library rules can have the same source file, so this is actually a
        # shared action. NB: This can probably result in action conflicts if the proto_library rules
        # are not the same.
        outputs.append(actions.declare_file(basename_no_ext + extension, sibling = src))

    return outputs

proto_common = struct(
    compile = proto_compile.compile_from_descriptors,
    declare_generated_files = _declare_generated_files,
    check_collocated = _check_collocated,
    experimental_should_generate_code = _experimental_should_generate_code,
    experimental_filter_sources = _experimental_filter_sources,
    get_import_path = proto_compile.get_import_path,
    output_directory = proto_compile.output_directory,
    ProtoLangToolchainInfo = ProtoLangToolchainInfo,
    INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION = toolchains.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION,
    INCOMPATIBLE_PASS_TOOLCHAIN_TYPE = True,
)
