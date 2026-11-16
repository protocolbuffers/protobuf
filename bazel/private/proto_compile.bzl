# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Definition of compile logic exposed to proto_common or proto_library rule."""

load("//bazel/private:toolchain_helpers.bzl", "toolchains")

visibility([
    "//bazel/common",
    "//bazel/private",
    "//bazel/tests",
])

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

def _proto_path(file):
    """Returns the proto root directory for a source or generated file, or "." if at the repo root."""
    if type(file) == "string":
        return file
    path = file.path
    import_path = _get_import_path(file)
    if path == import_path:
        return "."
    return path.removesuffix("/" + import_path)

def _import_virtual_proto_path(proto_file):
    """Imports all paths for virtual imports.

      They're of the form:
      'bazel-out/k8-fastbuild/bin/external/foo/e/_virtual_imports/e' or
      'bazel-out/foo/k8-fastbuild/bin/e/_virtual_imports/e'"""
    path = _proto_path(proto_file)
    if path.count("/") > 4:
        return "-I%s" % path
    return None

def _import_repo_proto_path(proto_file):
    """Imports all paths for generated files in external repositories.

      They are of the form:
      'bazel-out/k8-fastbuild/bin/external/foo' or
      'bazel-out/foo/k8-fastbuild/bin'"""
    path = _proto_path(proto_file)
    path_count = path.count("/")
    if path_count > 2 and path_count <= 4:
        return "-I%s" % path
    return None

def _import_main_output_proto_path(proto_file):
    """Imports all paths for generated files or source files in external repositories.

      They're of the form:
      'bazel-out/k8-fastbuild/bin'
      'external/foo'
      '../foo'
    """
    path = _proto_path(proto_file)
    if path.count("/") <= 2 and path != ".":
        return "-I%s" % path
    return None

def _output_directory(proto_info, root):
    """Returns the physical output directory path for generated proto files.

    This computes the correct directory path, correctly routing outputs into the virtual imports
    directory if the `proto_library` uses `import_prefix` or `strip_import_prefix`.

    Args:
      proto_info: (ProtoInfo) The ProtoInfo provider from the proto_library.
      root: (File|root) The root directory (e.g., `ctx.bin_dir` or `ctx.genfiles_dir`) under which
        the generated files should be placed.

    Returns:
      (str) The physical output directory path.
    """
    proto_source_root = proto_info.proto_source_root
    if proto_source_root.startswith(root.path):
        #TODO: remove this branch when bin_dir is removed from proto_source_root
        proto_source_root = proto_source_root.removeprefix(root.path).removeprefix("/")

    if proto_source_root == "" or proto_source_root == ".":
        return root.path

    return root.path + "/" + proto_source_root

def _toolchain_type(proto_lang_toolchain_info):
    if toolchains.INCOMPATIBLE_ENABLE_PROTO_TOOLCHAIN_RESOLUTION:
        return getattr(proto_lang_toolchain_info, "toolchain_type", None)
    else:
        return None

_COMPILE_TYPE = struct(
    TRANSITIVE_SRCS = "TRANSITIVE_SRCS",
    DESCRIPTOR_ONLY = "DESCRIPTOR_ONLY",
)

def _compile_internal(
        compile_type,
        actions,
        proto_info,
        proto_lang_toolchain_info,
        generated_files,
        plugin_output = None,
        additional_args = None,
        additional_tools = [],
        additional_inputs = depset(),
        resource_set = None,
        experimental_exec_group = None,
        experimental_progress_message = None,
        experimental_output_files = "legacy"):
    """Creates proto compile action for compiling *.proto files to language specific sources.

    Args:
      compile_type: (_COMPILE_TYPE) How to compile the given proto.
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
                args.add_all(
                    generated_files[:1],
                    map_each = _proto_path,
                    format_each = proto_lang_toolchain_info.out_replacement_format_flag,
                )

    if plugin_output:
        args.add(plugin_output, format = proto_lang_toolchain_info.out_replacement_format_flag)
    if proto_lang_toolchain_info.plugin:
        tools.append(proto_lang_toolchain_info.plugin)
        args.add(proto_lang_toolchain_info.plugin.executable, format = proto_lang_toolchain_info.plugin_format_flag)

    if compile_type != _COMPILE_TYPE.DESCRIPTOR_ONLY:
        # Protoc searches for .protos -I paths in order they are given and then
        # uses the path within the directory as the package.
        # This requires ordering the paths from most specific (longest) to least
        # specific ones, so that no path in the list is a prefix of any of the
        # following paths in the list.
        # For example: 'bazel-out/k8-fastbuild/bin/external/foo' needs to be listed
        # before 'bazel-out/k8-fastbuild/bin'. If not, protoc will discover file under
        # the shorter path and use 'external/foo/...' as its package path.
        args.add_all(proto_info.transitive_sources, map_each = _import_virtual_proto_path, uniquify = True)
        args.add_all(proto_info.transitive_sources, map_each = _import_repo_proto_path, uniquify = True)
        args.add_all(proto_info.transitive_sources, map_each = _import_main_output_proto_path, uniquify = True)
        args.add("-I.")  # Needs to come last

    args.add_all(proto_lang_toolchain_info.protoc_opts)

    # Only append non-empty depsets to `transitive_inputs` so that in the common
    # single-depset case (`len(transitive_inputs) == 1`) we can pass
    # `transitive_inputs[0]` directly to `actions.run` without allocating a
    # wrapper `depset`, saving memory across this heavily used rule.
    transitive_inputs = []
    if additional_inputs:
        transitive_inputs.append(additional_inputs)

    # Direct sources are required for TRANSITIVE_SRCS, but not DESCRIPTOR_ONLY.
    # However we always pass in the names of the files we're compiling/generating for.
    if compile_type == _COMPILE_TYPE.DESCRIPTOR_ONLY:
        # _get_import_path has no impact on source files, but strips the blaze-out path for
        # generated proto files, allowing us to directly consume the appropriate descriptor in
        # descriptor-only mode.
        args.add_all(proto_info.direct_sources, map_each = _get_import_path)

        # Descriptor sets containing everything needed by protoc to build a DescriptorPool.
        # Build the combined descriptor set depset here rather than precomputing
        # and storing it on every `ProtoInfo` instance to save memory on
        # `proto_library` targets that do not run descriptor-only compilation.
        internal = proto_info.internal_do_not_use
        source_code_info = internal.source_code_info
        dependencies_descriptor_sets = depset(
            direct = [source_code_info] if source_code_info else None,
            transitive = [
                proto_info.transitive_descriptor_sets,
                internal.option_deps_descriptor_sets,
            ],
        )
        if dependencies_descriptor_sets:
            transitive_inputs.append(dependencies_descriptor_sets)

        args.add_joined(
            "--descriptor_set_in",
            dependencies_descriptor_sets,
            join_with = getattr(proto_lang_toolchain_info, "host_path_separator", None) or ":",
        )
    else:
        args.add_all(proto_info.direct_sources)
        transitive_inputs.append(proto_info.transitive_sources)

    if additional_args:
        additional_args.use_param_file(param_file_arg = "@%s")
        additional_args.set_param_file_format("multiline")

    # If any of our deps provide a extension_declaration we need to include
    # them in our inputs since we'll be consuming them for validation.
    # Default to `None` rather than an empty `depset()` to avoid allocating
    # an unused depset or forcing a wrapper depset in `actions.run`.
    transitive_extension_declarations = getattr(proto_info, "transitive_extension_declarations", None)
    if transitive_extension_declarations:
        transitive_inputs.append(transitive_extension_declarations)

    actions.run(
        mnemonic = proto_lang_toolchain_info.mnemonic,
        progress_message = experimental_progress_message if experimental_progress_message else proto_lang_toolchain_info.progress_message,
        executable = proto_lang_toolchain_info.proto_compiler,
        arguments = [args, additional_args] if additional_args else [args],
        # Unwrap single-element `transitive_inputs` to avoid allocating an extra
        # wrapper `depset` per compile action on this heavily used rule.
        inputs = transitive_inputs[0] if len(transitive_inputs) == 1 else depset(transitive = transitive_inputs),
        outputs = generated_files,
        tools = tools,
        use_default_shell_env = True,
        resource_set = resource_set,
        exec_group = experimental_exec_group,
        toolchain = _toolchain_type(proto_lang_toolchain_info),
    )

def _compile_from_transitive_sources(*args, **kwargs):
    """Creates proto compile action for compiling *.proto files to language specific sources.

    Uses sources for everything, (legacy behavior)

    Args:
      *args: See _compile_internal
      **kwargs: See _compile_internal
    """
    _compile_internal(_COMPILE_TYPE.TRANSITIVE_SRCS, *args, **kwargs)

def _compile_from_descriptors(*args, **kwargs):
    """Creates proto compile action for 'compiling' files to language specific sources.

    This version is for triggering plugins from already parsed FileDescriptorSets, typically via aspect.

    Args:
      *args: See _compile_internal
      **kwargs: See _compile_internal
    """
    _compile_internal(_COMPILE_TYPE.DESCRIPTOR_ONLY, *args, **kwargs)

def _create_proto_descriptors(
        actions,
        srcs,
        descriptor_set,
        source_code_info,
        transitive_proto_path,
        dependencies_descriptor_sets,
        transitive_extension_declarations,
        proto_compiler,
        protoc_opts,
        target_label,
        direct_dependencies,
        direct_dependencies_flag_template,
        option_dependencies,
        option_dependencies_flag_template,
        allowed_public_imports,
        include_source_info_in_primary,
        host_path_separator,
        toolchain = None,
        # TODO: Kythe-specific; remove `transitive_sources` parameter
        # and `transitive_inputs.append(transitive_sources)` below once Kythe moves
        # to use the new `source_code_info` FileDescriptorSet outputs.
        transitive_sources = None):
    """Creates an action to generate descriptor sets from direct proto sources.

    Args:
      actions: (ActionFactory) Obtained by ctx.actions, used to register the action.
      srcs: (list[File]) The direct `.proto` source files.
      descriptor_set: (File) Output primary descriptor set file.
      source_code_info: (File|None) Split descriptor set file containing SourceCodeInfo.
      transitive_proto_path: (depset[str]) Transitive search paths for imports.
      dependencies_descriptor_sets: (depset[File]) Transitive descriptor sets of dependencies.
      transitive_extension_declarations: (depset[File]) Extension declarations for MessageSet.
      proto_compiler: (File|FilesToRunProvider) The protoc executable.
      protoc_opts: (list[str]) Additional protoc command-line options.
      target_label: (Label) Label of the proto_library target, formatted lazily into violation messages.
      direct_dependencies: (depset[File]) Direct dependencies for strict deps checking.
      direct_dependencies_flag_template: (str) Single-`%s` format template for direct deps violation flag.
      option_dependencies: (depset[File]) Option dependencies for strict deps checking.
      option_dependencies_flag_template: (str) Single-`%s` format template for option deps violation flag.
      allowed_public_imports: (depset[File]) Allowed public imports.
      include_source_info_in_primary: (bool) Whether to include source info in primary descriptor.
      host_path_separator: (str) Separator for PATH-like environment variables on the host.
      toolchain: Toolchain type if toolchain resolution is enabled.
      transitive_sources: (depset[File]) TODO: Kythe-specific; remove once Kythe uses `source_code_info` outputs.
    """
    if not srcs:
        actions.write(descriptor_set, "")
        if source_code_info:
            actions.write(source_code_info, "")
        return

    args = actions.args()
    args.use_param_file(param_file_arg = "@%s")
    args.set_param_file_format("multiline")

    outputs = [descriptor_set]
    args.add(descriptor_set, format = "--descriptor_set_out=%s")
    if include_source_info_in_primary:
        if "--include_source_info" not in protoc_opts:
            args.add("--include_source_info")

        # When --include_source_info is true we have to write out any
        # source_code_info file as a dummy.
        if source_code_info:
            actions.write(source_code_info, "")

    elif source_code_info:
        args.add(source_code_info, format = "--source_code_info_out=%s")
        outputs.append(source_code_info)

    args.add("--retain_options")

    args.add_all(transitive_proto_path, map_each = _import_virtual_proto_path)
    args.add_all(transitive_proto_path, map_each = _import_repo_proto_path)
    args.add_all(transitive_proto_path, map_each = _import_main_output_proto_path)
    args.add("-I.")

    args.add_all(protoc_opts)

    args.add_all(srcs)

    if dependencies_descriptor_sets:
        args.add_joined(
            "--descriptor_set_in",
            dependencies_descriptor_sets,
            join_with = host_path_separator,
        )

    if direct_dependencies != None:
        if direct_dependencies:
            args.add_joined(
                "--direct_dependencies",
                direct_dependencies,
                map_each = _get_import_path,
                join_with = ":",
            )
        else:
            args.add("--direct_dependencies=")

        # Pass `target_label` with `format` instead of formatting the violation
        # string in Starlark so string expansion is deferred to execution time,
        # saving per-target string allocations on `proto_library` during analysis.
        if direct_dependencies_flag_template:
            args.add(target_label, format = direct_dependencies_flag_template)

    if option_dependencies != None:
        if option_dependencies:
            args.add_joined(
                "--option_dependencies",
                option_dependencies,
                map_each = _get_import_path,
                join_with = ":",
            )
        else:
            args.add("--option_dependencies=")

        # Deferred `target_label` formatting to save analysis-phase memory (see above).
        if option_dependencies_flag_template:
            args.add(target_label, format = option_dependencies_flag_template)

    if allowed_public_imports != None:
        if allowed_public_imports:
            args.add_joined(
                "--allowed_public_imports",
                allowed_public_imports,
                map_each = _get_import_path,
                join_with = ":",
            )
        else:
            args.add("--allowed_public_imports=")

    transitive_inputs = []
    if dependencies_descriptor_sets:
        transitive_inputs.append(dependencies_descriptor_sets)
    if transitive_extension_declarations:
        transitive_inputs.append(transitive_extension_declarations)

    # TODO: Kythe-specific; remove this `if transitive_sources:`
    # block and the `transitive_sources` parameter on `_create_proto_descriptors`
    # once Kythe moves to use the new `source_code_info` FileDescriptorSet outputs.
    if transitive_sources:
        transitive_inputs.append(transitive_sources)

    actions.run(
        mnemonic = "GenProtoDescriptorSet",
        progress_message = "Generating Descriptor Set proto_library %{label}",
        executable = proto_compiler,
        arguments = [args],
        inputs = depset(
            direct = srcs,
            transitive = transitive_inputs,
        ),
        outputs = outputs,
        use_default_shell_env = True,
        toolchain = toolchain,
    )

# Functions exported to proto_common or proto_library
proto_compile = struct(
    compile_from_transitive_sources = _compile_from_transitive_sources,
    compile_from_descriptors = _compile_from_descriptors,
    create_proto_descriptors = _create_proto_descriptors,
    output_directory = _output_directory,
)
