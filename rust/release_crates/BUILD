# Protobuf Rust crate packaging for release.

load("@rules_shell//shell:sh_binary.bzl", "sh_binary")

filegroup(
    name = "crates",
    srcs = [
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
        ":crates",
        "//:protoc",
    ],
    tags = ["manual"],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
