# Protobuf Rust crate packaging for release.

load("@rules_pkg//pkg:mappings.bzl", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")

pkg_files(
    name = "license",
    srcs = ["//:LICENSE"],
    visibility = ["//rust/release_crates:__subpackages__"],
)

pkg_tar(
    name = "workspace",
    srcs = [
        ":Cargo.toml",
        "//rust/release_crates/protobuf:protobuf_crate",
        "//rust/release_crates/protobuf_codegen:protobuf_codegen_crate",
        "//rust/release_crates/protobuf_example:protobuf_example_crate",
        "//rust/release_crates/protobuf_macros:protobuf_macros_crate",
        "//rust/release_crates/protobuf_well_known_types:protobuf_well_known_types_crate",
    ],
    visibility = ["//rust:__pkg__"],
)

# Run the cargo test with only a bundled linux-x86_64 protoc.
sh_binary(
    name = "cargo_test",
    srcs = ["cargo_test.sh"],
    data = [
        ":workspace",
        "//:protoc",
    ],
    tags = ["manual"],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
