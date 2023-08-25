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
        deps = deps + select({
            "//python:limited_api_3.7": ["@python-3.7.0//:python_headers"],
            "//python:full_api_3.7_win32": ["@nuget_python_i686_3.7.0//:python_full_api"],
            "//python:full_api_3.7_win64": ["@nuget_python_x86-64_3.7.0//:python_full_api"],
            "//python:full_api_3.8_win32": ["@nuget_python_i686_3.8.0//:python_full_api"],
            "//python:full_api_3.8_win64": ["@nuget_python_x86-64_3.8.0//:python_full_api"],
            "//python:full_api_3.9_win32": ["@nuget_python_i686_3.9.0//:python_full_api"],
            "//python:full_api_3.9_win64": ["@nuget_python_x86-64_3.9.0//:python_full_api"],
            "//python:limited_api_3.10_win32": ["@nuget_python_i686_3.10.0//:python_limited_api"],
            "//python:limited_api_3.10_win64": ["@nuget_python_x86-64_3.10.0//:python_limited_api"],
            "//conditions:default": ["@system_python//:python_headers"],
        }),
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
        visibility = ["//python:__subpackages__"],
    )
