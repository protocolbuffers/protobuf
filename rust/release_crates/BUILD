# Protobuf Rust crate packaging for release.

load("@rules_pkg//pkg:mappings.bzl", "pkg_files")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")

pkg_files(
    name = "license",
    srcs = ["@com_google_protobuf//:LICENSE"],
    visibility = ["//release_crates:__subpackages__"],
)

pkg_tar(
    name = "workspace",
    srcs = [
        ":Cargo.toml",
        "//release_crates/protobuf:protobuf_crate",
        "//release_crates/protobuf_codegen:protobuf_codegen_crate",
        "//release_crates/protobuf_example:protobuf_example_crate",
        "//release_crates/protobuf_macros:protobuf_macros_crate",
        "//release_crates/protobuf_tests:protobuf_tests_crate",
        "//release_crates/protobuf_well_known_types:protobuf_well_known_types_crate",
    ],
    visibility = ["//:__pkg__"],
)

# Run the cargo test with only a bundled linux-x86_64 protoc.
sh_binary(
    name = "cargo_test",
    srcs = ["cargo_test.sh"],
    data = [
        ":workspace",
        "@com_google_protobuf//:protoc",
    ],
    tags = ["manual"],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
