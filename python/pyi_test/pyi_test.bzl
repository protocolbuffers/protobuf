"""Helpers for generated Python-stub golden and type-checker tests."""

load("@rules_python//python:py_info.bzl", "PyInfo")
load("//bazel/common:proto_common.bzl", "proto_common")

def _pyi_files_impl(ctx):
    files = ctx.attr.dep[PyInfo].transitive_pyi_files

    # Place well-known type stubs beside the modules in the runtime's regular
    # google.protobuf package. Separate virtual import roots cannot extend it.
    symlinks = {}
    for file in files.to_list():
        path = proto_common.get_import_path(file)
        if path.startswith("google/protobuf/"):
            symlinks["python/" + path] = file
    return [DefaultInfo(files = files, runfiles = ctx.runfiles(transitive_files = files, symlinks = symlinks))]

pyi_files = rule(
    implementation = _pyi_files_impl,
    attrs = {"dep": attr.label(mandatory = True, providers = [PyInfo])},
)
