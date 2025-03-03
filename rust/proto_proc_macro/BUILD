# This directory contains the implementation of the proto! macro as a proc macro.
# It has not yet been made to work with bazel and so no targets are defined here.

load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files", "strip_prefix")

PROC_MACRO_SRCS = ["proto_proc_macro_impl.rs"]

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
