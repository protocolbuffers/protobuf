# An alternate implementation of the proto! macro as a proc macro instead of macro_rules.

# This is only in a separate directory due to the top level package being part of the 'core'
# component, and it not being a resolved decision if we want to add all deps of this proc macro to
# the core component.

load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files", "strip_prefix")
load("@rules_rust//rust:defs.bzl", "rust_proc_macro")

PROC_MACRO_SRCS = ["proto_proc_macro_impl.rs"]

rust_proc_macro(
    name = "proto_proc_macro",
    srcs = PROC_MACRO_SRCS,
    visibility = ["//rust:__subpackages__"],
    deps = [
        "@crate_index//:quote",
        "@crate_index//:syn",
    ],
)

pkg_files(
    name = "rust_protobuf_macros_src",
    srcs = PROC_MACRO_SRCS,
    strip_prefix = strip_prefix.from_root("rust/proto_proc_macro"),
)

pkg_filegroup(
    name = "rust_protobuf_macros_src_dir",
    srcs = [
        ":rust_protobuf_macros_src",
    ],
    prefix = "src",
    visibility = ["//rust/release_crates:__subpackages__"],
)
