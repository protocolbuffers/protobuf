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
    alias_args = {}
    if "visibility" in args:
        alias_args["visibility"] = args.pop("visibility")
    native.alias(
        name = name + "_rust_proto",
        actual = select({
            "//rust:use_upb_kernel": name + "_upb_rust_proto",
            "//conditions:default": name + "_cpp_rust_proto",
        }),
        **alias_args
    )

    rust_upb_proto_library(
        name = name + "_upb_rust_proto",
        deps = deps,
        visibility = ["//rust/test:__subpackages__"],
        **args
    )

    rust_cc_proto_library(
        name = name + "_cpp_rust_proto",
        deps = deps,
        visibility = ["//rust/test:__subpackages__"],
        **args
    )
