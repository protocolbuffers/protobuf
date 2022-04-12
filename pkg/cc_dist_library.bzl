# Rules for distributable C++ libraries

load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cc_toolchain")

CPP_LINK_STATIC_LIBRARY_ACTION_NAME = "c++-link-static-library"

# Creates an action to build the `output_file` static library (archive)
# using `object_files`.
def _create_archive_action(
        ctx,
        feature_configuration,
        cc_toolchain,
        output_file,
        object_files):
    # Based on Bazel's src/main/starlark/builtins_bzl/common/cc/cc_import.bzl:
    archiver_path = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
    )
    archiver_variables = cc_common.create_link_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        output_file = output_file.path,
        is_using_linker = False,
    )
    command_line = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
        variables = archiver_variables,
    )
    args = ctx.actions.args()
    args.add_all(command_line)
    args.add_all(object_files)
    args.use_param_file("@%s", use_always = True)

    env = cc_common.get_environment_variables(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
        variables = archiver_variables,
    )

    # TODO(bazel-team): PWD=/proc/self/cwd env var is missing, but it is present when an analogous archiving
    # action is created by cc_library
    ctx.actions.run(
        executable = archiver_path,
        arguments = [args],
        env = env,
        inputs = depset(
            direct = object_files,
            transitive = [
                cc_toolchain.all_files,
            ],
        ),
        use_default_shell_env = True,
        outputs = [output_file],
        mnemonic = "CppArchive",
    )

# Implementation for cc_dist_library rule.
def _cc_dist_library_impl(ctx):
    cc_toolchain_info = find_cc_toolchain(ctx)
    if cc_toolchain_info.ar_executable == None:
        return []

    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain_info,
    )

    # Collect the set of object files from the immediate deps.

    objs = []
    pic_objs = []
    for dep in ctx.attr.deps:
        if CcInfo not in dep:
            continue

        link_ctx = dep[CcInfo].linking_context
        if link_ctx == None:
            continue

        linker_inputs = link_ctx.linker_inputs.to_list()
        for link_input in linker_inputs:
            if link_input.owner != dep.label:
                # This is a transitive dep: skip it.
                continue

            for lib in link_input.libraries:
                objs.extend(lib.objects or [])
                pic_objs.extend(lib.pic_objects or [])

    # For static libraries, build separately with and without pic.

    stemname = "lib" + ctx.label.name
    outputs = []

    if len(objs) > 0:
        archive_out = ctx.actions.declare_file(stemname + ".a")
        _create_archive_action(
            ctx,
            feature_configuration,
            cc_toolchain_info,
            archive_out,
            objs,
        )
        outputs.append(archive_out)

    if len(pic_objs) > 0:
        pic_archive_out = ctx.actions.declare_file(stemname + ".pic.a")
        _create_archive_action(
            ctx,
            feature_configuration,
            cc_toolchain_info,
            pic_archive_out,
            pic_objs,
        )
        outputs.append(pic_archive_out)

    # For dynamic libraries, use the `cc_common.link` command to ensure
    # everything gets built correctly according to toolchain definitions.

    compilation_outputs = cc_common.create_compilation_outputs(
        objects = depset(objs),
        pic_objects = depset(pic_objs),
    )

    link_output = cc_common.link(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain_info,
        compilation_outputs = compilation_outputs,
        name = ctx.label.name,
        output_type = "dynamic_library",
        # TODO(dlj): need to handle -lz -lpthread
        # user_link_flags = [],
    )

    library_to_link = link_output.library_to_link

    linking_context = cc_common.create_linking_context(
        linker_inputs = depset([
            cc_common.create_linker_input(
                owner = ctx.label,
                libraries = depset([library_to_link]),
            ),
        ]),
    )

    if library_to_link.dynamic_library != None:
        outputs.append(library_to_link.dynamic_library)

    if library_to_link.interface_library != None:
        outputs.append(library_to_link.interface_library)

    return [
        DefaultInfo(files = depset(outputs)),
        CcInfo(linking_context = linking_context),
    ]

cc_dist_library = rule(
    implementation = _cc_dist_library_impl,
    attrs = {
        "deps": attr.label_list(),
        # C++ toolchain before https://github.com/bazelbuild/bazel/issues/7260:
        "_cc_toolchain": attr.label(
            default = Label("@rules_cc//cc:current_cc_toolchain"),
        ),
    },
    toolchains = [
        # C++ toolchain after https://github.com/bazelbuild/bazel/issues/7260:
        "@bazel_tools//tools/cpp:toolchain_type",
    ],
    fragments = ["cpp"],
)
