"""This file implements an experimental, do-not-use-kind of rust_proto_library.

Disclaimer: This project is experimental, under heavy development, and should not
be used yet."""

load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load(
    "//rust:aspects.bzl",
    "RustProtoInfo",
    "rust_cc_proto_library_aspect",
    "rust_upb_proto_library_aspect",
)

visibility(["//rust/..."])

def rust_proto_library(name, deps, visibility = [], **args):
    """Declares all the boilerplate needed to use Rust protobufs conveniently. 

    Hopefully no user will ever need to read this code.


    Args:
        name: name of the Rust protobuf target.
        deps: proto_library target for which to generate Rust gencode.
        **args: other args passed to the rust_<kernel>_proto_library targets.
    """
    if not name.endswith("_rust_proto"):
        fail("Name of each rust_proto_library target should end with `_rust_proto`")
    native.alias(
        name = name,
        actual = select({
            "//rust:use_upb_kernel": name + "_upb_kernel",
            "//conditions:default": name + "_cpp_kernel",
        }),
        visibility = visibility,
    )

    rust_upb_proto_library(
        name = name + "_upb_kernel",
        deps = deps,
        visibility = ["//visibility:private"],
        **args
    )

    # TODO(b/275701445): Stop declaring cc_proto_library once we can use cc_proto_aspect directly.
    _cc_proto_name = name.removesuffix("_rust_proto") + "_cc_proto"
    if not native.existing_rule(_cc_proto_name):
        cc_proto_library(
            name = _cc_proto_name,
            deps = deps,
            visibility = ["//visibility:private"],
            **args
        )
    rust_cc_proto_library(
        name = name + "_cpp_kernel",
        deps = [_cc_proto_name],
        visibility = ["//visibility:private"],
        **args
    )

def _rust_proto_library_impl(ctx):
    deps = ctx.attr.deps
    if not deps:
        fail("Exactly 1 dependency in `deps` attribute expected, none were provided.")
    if len(deps) > 1:
        fail("Exactly 1 dependency in `deps` attribute expected, too many were provided.")

    dep = deps[0]
    rust_proto_info = dep[RustProtoInfo]
    dep_variant_info = rust_proto_info.dep_variant_info
    return [dep_variant_info.crate_info, dep_variant_info.dep_info, dep_variant_info.cc_info]

def _make_rust_proto_library(is_upb):
    return rule(
        implementation = _rust_proto_library_impl,
        attrs = {
            "deps": attr.label_list(
                mandatory = True,
                providers = [ProtoInfo] if is_upb else [CcInfo],
                aspects = [rust_upb_proto_library_aspect if is_upb else rust_cc_proto_library_aspect],
            ),
        },
    )

rust_upb_proto_library = _make_rust_proto_library(is_upb = True)
rust_cc_proto_library = _make_rust_proto_library(is_upb = False)
