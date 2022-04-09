# Rules for distributable C++ libraries

load("@rules_cc//cc:find_cc_toolchain.bzl", "find_cc_toolchain")

def _create_archive(ctx, cc_toolchain_info, name, objs):
    ar_out = ctx.actions.declare_file(name)

    ar_args = ctx.actions.args()
    ar_args.use_param_file("@%s")
    ar_args.add("cr")
    ar_args.add(ar_out)
    ar_args.add_all(objs)

    ctx.actions.run(
        mnemonic = "CcCreateArchive",
        executable = cc_toolchain_info.ar_executable,
        arguments = [ar_args],
        inputs = objs,
        outputs = [ar_out],
    )

    return ar_out

def _cc_dist_library_impl(ctx):
    cc_toolchain_info = find_cc_toolchain(ctx)
    if cc_toolchain_info.ar_executable == None:
        return []

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
            for lib in link_input.libraries:
                if lib.objects != None:
                    objs.extend(lib.objects)
                if lib.pic_objects != None:
                    pic_objs.extend(lib.pic_objects)

    # For static libraries, use `ar` to construct an archive with all of
    # the immediate deps' objects.
    stemname = "lib" + ctx.label.name
    outputs = []

    if len(objs) > 0:
        if cc_toolchain_info.ar_executable != None:
            outputs.append(_create_archive(
                ctx,
                cc_toolchain_info,
                stemname + ".a",
                objs,
            ))

    if len(pic_objs) > 0:
        if cc_toolchain_info.ar_executable != None:
            outputs.append(_create_archive(
                ctx,
                cc_toolchain_info,
                stemname + ".pic.a",
                pic_objs,
            ))

    # For dynamic libraries, use the `cc_common.link` command to ensure
    # everything gets built correctly according to toolchain definitions.

    compilation_outputs = cc_common.create_compilation_outputs(
        objects = depset(objs),
        pic_objects = depset(pic_objs),
    )

    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain_info,
    )

    link_output = cc_common.link(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain_info,
        compilation_outputs = compilation_outputs,
        name = ctx.label.name,
        output_type = "dynamic_library",
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
