"""Macro to support py_extension """

load("@bazel_skylib//lib:selects.bzl", "selects")

def py_extension(name, srcs, copts, deps = [], **kwargs):
    """Creates a C++ library to extend python

    Args:
      name: Name of the target
      srcs: List of source files to create the target
      copts: List of C++ compile options to use
      deps: Libraries that the target depends on
    """

    native.cc_binary(
        name = name + "_binary",
        srcs = srcs,
        copts = copts + ["-fvisibility=hidden"],
        linkopts = selects.with_or({
            (
                "//python/dist:osx_x86_64",
                "//python/dist:osx_aarch64",
            ): ["-undefined", "dynamic_lookup"],
            "//python/dist:windows_x86_32": ["-static-libgcc"],
            "//conditions:default": [],
        }),
        linkshared = True,
        linkstatic = True,
        deps = deps + ["@rules_python//python/cc:current_py_cc_headers"],
        **kwargs
    )

    EXT_SUFFIX = ".abi3.so"
    output_file = "google/_upb/" + name + EXT_SUFFIX

    native.genrule(
        name = "copy" + name,
        srcs = [":" + name + "_binary"],
        outs = [output_file],
        cmd = "cp $< $@",
        visibility = ["//python:__subpackages__"],
    )

    native.py_library(
        name = name,
        data = [output_file],
        imports = ["."],
        visibility = [
            "//python:__subpackages__",
            "//conformance:__pkg__",
        ],
    )
