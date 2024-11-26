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
load("//bazel/private:native.bzl", "native_proto_common")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")

def _import_virtual_proto_path(path):
    """Imports all paths for virtual imports.

      They're of the form:
      'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e' or
      'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e'"""
    if path.count("/") > 4:
        return "-I%s" % path
    return None

def _import_repo_proto_path(path):
    """Imports all paths for generated files in external repositories.

      They are of the form:
      'bazel-out/k8-fastbuild/bin/external/foo' or
      'bazel-out/foo/k8-fastbuild/bin'"""
    path_count = path.count("/")
    if path_count > 2 and path_count <= 4:
        return "-I%s" % path
    return None

def _import_main_output_proto_path(path):
    """Imports all paths for generated files or source files in external repositories.

      They're of the form:
      'bazel-out/k8-fastbuild/bin'
      'external/foo'
      '../foo'
    """
    if path.count("/") <= 2 and path != ".":
        return "-I%s" % path
    return None

def _remove_repo(file):
    """Removes `../repo/` prefix from path, e.g. `../repo/package/path -> package/path`"""
    short_path = file.short_path
    workspace_root = file.owner.workspace_root
    if workspace_root:
        if workspace_root.startswith("external/"):
            workspace_root = "../" + workspace_root.removeprefix("external/")
        return short_path.removeprefix(workspace_root + "/")
    return short_path

def _get_import_path(proto_file):
    """Returns the import path of a .proto file

    This is the path as used for the file that can be used in an `import` statement in another
    .proto file.

    Args:
      proto_file: (File) The .proto file

    Returns:
      (str) import path
    """
    repo_path = _remove_repo(proto_file)
    index = repo_path.find("_virtual_imports/")
    if index >= 0:
        index = repo_path.find("/", index + len("_virtual_imports/"))
        repo_path = repo_path[index + 1:]
    return repo_path

def _output_directory(proto_info, root):
    proto_source_root = proto_info.proto_source_root
    if proto_source_root.startswith(root.path):
        #TODO: remove this branch when bin_dir is removed from proto_source_root
        proto_source_root = proto_source_root.removeprefix(root.path).removeprefix("/")

    if proto_source_root == "" or proto_source_root == ".":
        return root.path

    return root.path + "/" + proto_source_root

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
    if not _PackageSpecificationInfo:
        if proto_lang_toolchain_info.allowlist_different_package or getattr(proto_info, "allow_exports", None):
            fail("Allowlist checks not supported before Bazel 6.4.0")
        return

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

def _compile(
        actions,
        proto_info,
        proto_lang_toolchain_info,
        generated_files,
        plugin_output = None,
        additional_args = None,
        additional_tools = [],
        additional_inputs = depset(),
        additional_proto_lang_toolchain_info = None,
        resource_set = None,
        experimental_exec_group = None,
        experimental_progress_message = None,
        experimental_output_files = "legacy"):
    """Creates proto compile action for compiling *.proto files to language specific sources.

    Args:
      actions: (ActionFactory)  Obtained by ctx.actions, used to register the actions.
      proto_info: (ProtoInfo) The ProtoInfo from proto_library to generate the sources for.
      proto_lang_toolchain_info: (ProtoLangToolchainInfo) The proto lang toolchain info.
        Obtained from a `proto_lang_toolchain` target or constructed ad-hoc..
      generated_files: (list[File]) The output files generated by the proto compiler.
        Callee needs to declare files using `ctx.actions.declare_file`.
        See also: `proto_common.declare_generated_files`.
      plugin_output: (File|str) Deprecated: Set `proto_lang_toolchain.output_files`
        and remove the parameter.
        For backwards compatibility, when the proto_lang_toolchain isn't updated
        the value is used.
      additional_args: (Args) Additional arguments to add to the action.
        Accepts a ctx.actions.args() object that is added at the beginning
        of the command line.
      additional_tools: (list[File]) Additional tools to add to the action.
      additional_inputs: (Depset[File]) Additional input files to add to the action.
      resource_set: (func) A callback function that is passed to the created action.
        See `ctx.actions.run`, `resource_set` parameter for full definition of
        the callback.
      experimental_exec_group: (str) Sets `exec_group` on proto compile action.
        Avoid using this parameter.
      experimental_progress_message: Overrides progress_message from the toolchain.
        Don't use this parameter. It's only intended for the transition.
      experimental_output_files: (str) Overwrites output_files from the toolchain.
        Don't use this parameter. It's only intended for the transition.
    """
    if type(generated_files) != type([]):
        fail("generated_files is expected to be a list of Files")
    if not generated_files:
        return  # nothing to do
    if experimental_output_files not in ["single", "multiple", "legacy"]:
        fail('experimental_output_files expected to be one of ["single", "multiple", "legacy"]')

    args = actions.args()
    args.use_param_file(param_file_arg = "@%s")
    args.set_param_file_format("multiline")
    tools = list(additional_tools)

    if experimental_output_files != "legacy":
        output_files = experimental_output_files
    else:
        output_files = getattr(proto_lang_toolchain_info, "output_files", "legacy")
    if output_files != "legacy":
        if proto_lang_toolchain_info.out_replacement_format_flag:
            if output_files == "single":
                if len(generated_files) > 1:
                    fail("generated_files only expected a single file")
                plugin_output = generated_files[0]
            else:
                plugin_output = _output_directory(proto_info, generated_files[0].root)

    if plugin_output:
        args.add(plugin_output, format = proto_lang_toolchain_info.out_replacement_format_flag)
    if proto_lang_toolchain_info.plugin:
        tools.append(proto_lang_toolchain_info.plugin)
        args.add(proto_lang_toolchain_info.plugin.executable, format = proto_lang_toolchain_info.plugin_format_flag)

    # Protoc searches for .protos -I paths in order they are given and then
    # uses the path within the directory as the package.
    # This requires ordering the paths from most specific (longest) to least
    # specific ones, so that no path in the list is a prefix of any of the
    # following paths in the list.
    # For example: 'bazel-out/k8-fastbuild/bin/external/foo' needs to be listed
    # before 'bazel-out/k8-fastbuild/bin'. If not, protoc will discover file under
    # the shorter path and use 'external/foo/...' as its package path.
    args.add_all(proto_info.transitive_proto_path, map_each = _import_virtual_proto_path)
    args.add_all(proto_info.transitive_proto_path, map_each = _import_repo_proto_path)
    args.add_all(proto_info.transitive_proto_path, map_each = _import_main_output_proto_path)
    args.add("-I.")  # Needs to come last

    args.add_all(proto_lang_toolchain_info.protoc_opts)

    args.add_all(proto_info.direct_sources)

    if additional_args:
        additional_args.use_param_file(param_file_arg = "@%s")
        additional_args.set_param_file_format("multiline")

    actions.run(
        mnemonic = proto_lang_toolchain_info.mnemonic,
        progress_message = experimental_progress_message if experimental_progress_message else proto_lang_toolchain_info.progress_message,
        executable = proto_lang_toolchain_info.proto_compiler,
        arguments = [args, additional_args] if additional_args else [args],
        inputs = depset(transitive = [proto_info.transitive_sources, additional_inputs]),
        outputs = generated_files,
        tools = tools,
        use_default_shell_env = True,
        resource_set = resource_set,
        exec_group = experimental_exec_group,
        toolchain = _toolchain_type(proto_lang_toolchain_info),
    )

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
    proto_files = proto_info._direct_proto_sources
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

def _toolchain_type(proto_lang_toolchain_info):
    if toolchains.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION:
        return getattr(proto_lang_toolchain_info, "toolchain_type", None)
    else:
        return None

proto_common = struct(
    compile = _compile,
    declare_generated_files = _declare_generated_files,
    check_collocated = _check_collocated,
    experimental_should_generate_code = _experimental_should_generate_code,
    experimental_filter_sources = _experimental_filter_sources,
    get_import_path = _get_import_path,
    ProtoLangToolchainInfo = ProtoLangToolchainInfo,
    INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION = toolchains.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION,
    INCOMPATIBLE_PASS_TOOLCHAIN_TYPE = (
        getattr(native_proto_common, "INCOMPATIBLE_PASS_TOOLCHAIN_TYPE", False) or
        not hasattr(native_proto_common, "ProtoLangToolchainInfo")
    ),
)
