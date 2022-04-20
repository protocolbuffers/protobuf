# Rules for distributable C++ libraries

load("@rules_cc//cc:action_names.bzl", cc_action_names = "ACTION_NAMES")
load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cc_toolchain")

# Creates an action to build the `output_file` static library (archive)
# using `object_files`.
def _create_archive_action(
        ctx,
        feature_configuration,
        cc_toolchain,
        output_file,
        object_files):
    # Based on Bazel's src/main/starlark/builtins_bzl/common/cc/cc_import.bzl:

    # Build the command line and add args for all of the input files:
    archiver_variables = cc_common.create_link_variables(
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        output_file = output_file.path,
        is_using_linker = False,
    )
    command_line = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name = cc_action_names.cpp_link_static_library,
        variables = archiver_variables,
    )
    args = ctx.actions.args()
    args.add_all(command_line)
    args.add_all(object_files)
    args.use_param_file("@%s", use_always = True)

    archiver_path = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = cc_action_names.cpp_link_static_library,
    )

    env = cc_common.get_environment_variables(
        feature_configuration = feature_configuration,
        action_name = cc_action_names.cpp_link_static_library,
        variables = archiver_variables,
    )

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
        mnemonic = "CppArchiveDist",
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
        user_link_flags = ctx.attr.linkopts,
    )
    library_to_link = link_output.library_to_link

    # Note: library_to_link.dynamic_library and interface_library are often
    # symlinks in the solib directory. For DefaultInfo, prefer reporting
    # the resolved artifact paths.
    if library_to_link.resolved_symlink_dynamic_library != None:
        outputs.append(library_to_link.resolved_symlink_dynamic_library)
    elif library_to_link.dynamic_library != None:
        outputs.append(library_to_link.dynamic_library)

    if library_to_link.resolved_symlink_interface_library != None:
        outputs.append(library_to_link.resolved_symlink_interface_library)
    elif library_to_link.interface_library != None:
        outputs.append(library_to_link.interface_library)

    # We could expose the libraries for use from cc rules:
    #
    # linking_context = cc_common.create_linking_context(
    #     linker_inputs = depset([
    #         cc_common.create_linker_input(
    #             owner = ctx.label,
    #             libraries = depset([library_to_link]),
    #         ),
    #     ]),
    # )
    # cc_info = CcInfo(linking_context = linking_context)  # and return this
    #
    # However, if the goal is to force a protobuf dependency to use the
    # DSO, then `cc_import` is a better-supported way to do so.
    #
    # If we wanted to expose CcInfo from this rule (and make it usable as a
    # C++ dependency), then we would probably want to include the static
    # archive and headers as well. exposing headers would probably require
    # an additional aspect to extract CcInfos with just the deps' headers.

    return [
        DefaultInfo(files = depset(outputs)),
    ]

cc_dist_library = rule(
    implementation = _cc_dist_library_impl,
    doc = """
Create libraries suitable for distribution.

This rule creates static and dynamic libraries from the libraries listed in
'deps'. The resulting libraries are suitable for distributing all of 'deps'
in a single logical library, for example, in an installable binary package.
Only the targets listed in 'deps' are included in the result (i.e., the
output does not include transitive dependencies), allowing precise control
over the library boundary.

The outputs of this rule are a dynamic library and a static library. (If
the build produces both PIC and non-PIC object files, then there is also a
second static library.) The example below illustrates additional details.

This rule is different from Bazel's experimental `shared_cc_library` in
several ways. First, this rule ignores transitive dependencies, which means
that dynamic library dependencies generally need to be specified via
'linkopts'. Second, this rule produces a static archive library in addition
to the dynamic shared library. Third, this rule is not directly usable as a
C++ dependency (although the outputs could be used, e.g., by `cc_import`).

Example:

    cc_library(name = "a", srcs = ["a.cc"], hdrs = ["a.h"])
    cc_library(name = "b", srcs = ["b.cc"], hdrs = ["b.h"], deps = [":a"])
    cc_library(name = "c", srcs = ["c.cc"], hdrs = ["c.h"], deps = [":b"])

    # Creates libdist.so and (typically) libdist.pic.a:
    # (This may also produce libdist.a if the build produces non-PIC objects.)
    cc_dist_library(
        name = "dist",
        linkopts = ["-la"],   # libdist.so dynamically links against liba.so.
        deps = [":b", ":c"],  # Output contains b.o and c.o, but not a.o.
    )
""",
    attrs = {
        "deps": attr.label_list(
            doc = ("The list of libraries to be included in the outputs. " +
                   "Only these targets' compilation outputs will be " +
                   "included (i.e., the transitive dependencies are not " +
                   "included in the output)."),
        ),
        "linkopts": attr.string_list(
            doc = ("Add these flags to the C++ linker command when creating " +
                   "the dynamic library."),
        ),
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
