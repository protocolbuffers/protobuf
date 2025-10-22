"""This file implements rust_proto_library (frontend macro)."""

# go/for-macros-only

load(
    ":rules.bzl",
    _ProtoCrateNamesInfo = "ProtoCrateNamesInfo",
    _rust_cc_proto_library = "rust_cc_proto_library",
    _rust_upb_proto_library = "rust_upb_proto_library",
)

ProtoCrateNamesInfo = _ProtoCrateNamesInfo
rust_upb_proto_library = _rust_upb_proto_library
rust_cc_proto_library = _rust_cc_proto_library

def rust_proto_library(name, deps, **args):
    """Declares all the boilerplate needed to use Rust protobufs conveniently.

    Hopefully no user will ever need to read this code.

    Args:
        name: name of the Rust protobuf target.
        deps: proto_library target for which to generate Rust gencode.
        **args: other args passed to the rust_<kernel>_proto_library targets.
    """
    if not name.endswith("_rust_proto"):
        fail(
            "Name rust_proto_library target should end with `_rust_proto`, but was '{}'"
                .format(name),
        )
    name = name.removesuffix("_rust_proto")
    rust_cpp_target_name = "_%s_cpp_rust_proto" % name
    rust_upb_target_name = "_%s_upb_rust_proto" % name
    alias_args = {}
    if "visibility" in args:
        alias_args["visibility"] = args.pop("visibility")
    native.alias(
        name = name + "_rust_proto",
        actual = select({
            "//rust:use_upb_kernel": rust_upb_target_name,
            "//conditions:default": rust_cpp_target_name,
        }),
        **alias_args
    )

    rust_upb_proto_library(
        name = rust_upb_target_name,
        deps = deps,
        visibility = ["//rust/test:__subpackages__"],
        **args
    )

    rust_cc_proto_library(
        name = rust_cpp_target_name,
        deps = deps,
        visibility = ["//rust/test:__subpackages__"],
        **args
    )
