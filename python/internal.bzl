# Internal helpers for building the Python protobuf runtime.

def _internal_copy_files_impl(ctx):
    strip_prefix = ctx.attr.strip_prefix
    if strip_prefix[-1] != "/":
        strip_prefix += "/"

    src_dests = []
    for src in ctx.files.srcs:
        if src.short_path[:len(strip_prefix)] != strip_prefix:
            fail("Source does not start with %s: %s" %
                 (strip_prefix, src.short_path))
        dest = ctx.actions.declare_file(src.short_path[len(strip_prefix):])
        src_dests.append([src, dest])

    if ctx.attr.is_windows:
        bat_file = ctx.actions.declare_file(ctx.label.name + "_copy.bat")
        ctx.actions.write(
            output = bat_file,
            content = "\r\n".join([
                '@copy /Y "{}" "{}" >NUL'.format(
                    src.path.replace("/", "\\"),
                    dest.path.replace("/", "\\"),
                )
                for src, dest in src_dests
            ]) + "\r\n",
        )
        ctx.actions.run(
            inputs = ctx.files.srcs,
            tools = [bat_file],
            outputs = [dest for src, dest in src_dests],
            executable = "cmd.exe",
            arguments = ["/C", bat_file.path.replace("/", "\\")],
            mnemonic = "InternalCopyFile",
            progress_message = "Copying files",
            use_default_shell_env = True,
        )

    else:
        sh_file = ctx.actions.declare_file(ctx.label.name + "_copy.sh")
        ctx.actions.write(
            output = sh_file,
            content = "\n".join([
                'cp -f "{}" "{}"'.format(src.path, dest.path)
                for src, dest in src_dests
            ]),
        )
        ctx.actions.run(
            inputs = ctx.files.srcs,
            tools = [sh_file],
            outputs = [dest for src, dest in src_dests],
            executable = "bash",
            arguments = [sh_file.path],
            mnemonic = "InternalCopyFile",
            progress_message = "Copying files",
            use_default_shell_env = True,
        )

    return [
        DefaultInfo(files = depset([dest for src, dest in src_dests])),
    ]

internal_copy_files_impl = rule(
    implementation = _internal_copy_files_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True, providers = [DefaultInfo]),
        "strip_prefix": attr.string(),
        "is_windows": attr.bool(),
    },
)

def internal_copy_files(name, srcs, strip_prefix):
    internal_copy_files_impl(
        name = name,
        srcs = srcs,
        strip_prefix = strip_prefix,
        is_windows = select({
            "@bazel_tools//src/conditions:host_windows": True,
            "//conditions:default": False,
        }),
    )
