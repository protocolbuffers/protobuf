"""Helpers for generated Python-stub golden and type-checker tests."""

load("@rules_python//python:py_info.bzl", "PyInfo")

def _pyi_files_impl(ctx):
    files = ctx.attr.dep[PyInfo].transitive_pyi_files
    return [DefaultInfo(files = files, runfiles = ctx.runfiles(transitive_files = files))]

pyi_files = rule(
    implementation = _pyi_files_impl,
    attrs = {"dep": attr.label(mandatory = True, providers = [PyInfo])},
)

def _node_tool_impl(ctx):
    node = ctx.toolchains["@rules_nodejs//nodejs:toolchain_type"].nodeinfo.node
    return [DefaultInfo(
        files = depset([node]),
        runfiles = ctx.runfiles(files = [node]),
    )]

node_tool = rule(
    implementation = _node_tool_impl,
    toolchains = ["@rules_nodejs//nodejs:toolchain_type"],
)
