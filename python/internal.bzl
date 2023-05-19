"""
Internal helpers for building the Python protobuf runtime.
"""

def _remove_cross_repo_path(path):
    components = path.split("/")
    if components[0] == "..":
        return "/".join(components[2:])
    return path

def _internal_copy_files_impl(ctx):
    strip_prefix = ctx.attr.strip_prefix
    if strip_prefix[-1] != "/":
        strip_prefix += "/"

    src_dests = []
    for src in ctx.files.srcs:
        short_path = _remove_cross_repo_path(src.short_path)
        if short_path[:len(strip_prefix)] != strip_prefix:
            fail("Source does not start with %s: %s" %
                 (strip_prefix, short_path))
        dest = ctx.actions.declare_file(short_path[len(strip_prefix):])
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
    doc = """
Implementation for internal_copy_files macro.

This rule implements file copying, including a compatibility mode for Windows.
""",
    implementation = _internal_copy_files_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True, providers = [DefaultInfo]),
        "strip_prefix": attr.string(),
        "is_windows": attr.bool(),
    },
)

def internal_copy_files(name, srcs, strip_prefix, **kwargs):
    """Copies common proto files to the python tree.

    In order for Python imports to work, generated proto interfaces under
    the google.protobuf package need to be in the same directory as other
    source files. This rule copies the .proto files themselves, e.g. with
    strip_prefix = 'src', 'src/google/protobuf/blah.proto' could be copied
    to '<package>/google/protobuf/blah.proto'.

    (An alternative might be to implement a separate rule to generate
    Python code in a different location for the sources. However, this
    would be strange behavior that doesn't match any other language's proto
    library generation.)

    Args:
      name: the name for the rule.
      srcs: the sources.
      strip_prefix: the prefix to remove from each of the paths in 'srcs'. The
          remainder will be used to construct the output path.
      **kwargs: common rule arguments.

    """
    internal_copy_files_impl(
        name = name,
        srcs = srcs,
        strip_prefix = strip_prefix,
        is_windows = select({
            "@bazel_tools//src/conditions:host_windows": True,
            "//conditions:default": False,
        }),
        **kwargs
    )

def internal_py_test(deps = [], **kwargs):
    """Internal wrapper for shared test configuration

    Args:
      deps: any additional dependencies of the test.
      **kwargs: arguments forwarded to py_test.
    """
    native.py_test(
        imports = ["."],
        deps = deps + ["//python:python_test_lib"],
        target_compatible_with = select({
            "@system_python//:supported": [],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
        **kwargs
    )
