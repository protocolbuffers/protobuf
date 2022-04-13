"""Macro to support py_extension
"""

def py_extension(name, srcs, copts, deps = []):
    """Creates a C++ library to extend python

    Args:
      name: Name of the target
      srcs: List of source files to create the target
      copts: List of C++ compile options to use
      deps: Libraries that the target depends on
    """

    version_script = name + "_version_script.lds"
    symbol = "PyInit_" + name
    native.genrule(
        name = "gen_" + version_script,
        outs = [version_script],
        cmd = "echo 'message { global: " + symbol + "; local: *; };' > $@",
    )

    native.cc_binary(
        name = name + "_binary",
        srcs = srcs,
        copts = copts,
        linkopts = select({
            "//python/dist:osx-x86_64_cpu": ["-undefined", "dynamic_lookup"],
            "//conditions:default": [],
        }),
        linkshared = True,
        linkstatic = True,
        deps = deps + [
            ":" + version_script,
        ] + select({
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
    )

    EXT_SUFFIX = ".abi3.so"

    module_name_map = {
        "_message": "pyext",
        "_api_implementation": "internal",
    }

    output_file = "google/protobuf/" + module_name_map[name] + "/" + name + EXT_SUFFIX

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
