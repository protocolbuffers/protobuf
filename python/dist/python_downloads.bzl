"""Helper methods to download different python versions"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

limited_api_build_file = """
cc_library(
    name = "python_headers",
    hdrs = glob(["**/Include/**/*.h"]),
    strip_include_prefix = "Python-{}/Include",
    visibility = ["//visibility:public"],
)
"""

def python_source_archive(version, sha256):
    """Helper method to create a python_headers target that will work for linux and macos.

    Args:
      name: The name of the target, should be in the form python_{VERSION}
      sha256: The sha256 of the python package for the specified version
    """
    http_archive(
        name = "python-{0}".format(version),
        urls = [
            "https://www.python.org/ftp/python/{0}/Python-{0}.tgz"
                .format(version),
        ],
        sha256 = sha256,
        build_file_content = limited_api_build_file.format(version),
        patch_cmds = [
            "echo '#define SIZEOF_WCHAR_T 4' > Python-{}/Include/pyconfig.h"
                .format(version),
        ],
    )

nuget_build_file = """
cc_import(
    name = "python_full_api",
    hdrs = glob(["**/*.h"]),
    shared_library = "python{0}.dll",
    interface_library = "libs/python{0}.lib",
    visibility = ["@com_google_protobuf//python:__pkg__"],
)

cc_import(
    name = "python_limited_api",
    hdrs = glob(["**/*.h"]),
    shared_library = "python{1}.dll",
    interface_library = "libs/python{1}.lib",
    visibility = ["@com_google_protobuf//python:__pkg__"],
)
"""

def python_nuget_package(cpu, version, sha256):
    """Helper method to create full and limited api dependencies for windows using nuget

    Args:
      name: The name of the target, should be in the form nuget_python_{CPU}_{VERSION}
      sha256: The sha256 of the nuget package for that version
    """
    full_api_lib_number = version.split(".")[0] + version.split(".")[1]
    limited_api_lib_number = version.split(".")[0]

    folder_name_dict = {
        "i686": "pythonx86",
        "x86-64": "python",
    }

    http_archive(
        name = "nuget_python_{0}_{1}".format(cpu, version),
        urls = [
            "https://www.nuget.org/api/v2/package/{}/{}"
                .format(folder_name_dict[cpu], version),
        ],
        sha256 = sha256,
        strip_prefix = "tools",
        build_file_content =
            nuget_build_file.format(full_api_lib_number, limited_api_lib_number),
        type = "zip",
        patch_cmds = ["cp -r include/* ."],
    )

def _python_headers(ctx):
    for mod in ctx.modules:
        for archive in mod.tags.source_archive:
            python_source_archive(
                version = archive.version,
                sha256 = archive.sha256,
            )
        for pkg in mod.tags.nuget_package:
            python_nuget_package(
                version = pkg.version,
                cpu = pkg.cpu,
                sha256 = pkg.sha256,
            )

source_archive = tag_class(attrs = {
    "version": attr.string(doc = "A python source archive"),
    "sha256": attr.string(),
})

nuget_package = tag_class(attrs = {
    "version": attr.string(doc = "A python nuget package"),
    "cpu": attr.string(),
    "sha256": attr.string(),
})

python_headers = module_extension(
    implementation = _python_headers,
    tag_classes = {
        "source_archive": source_archive,
        "nuget_package": nuget_package,
    },
)
