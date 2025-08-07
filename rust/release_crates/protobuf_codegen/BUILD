load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files", "strip_prefix")
load("//rust/release_crates:substitute_rust_release_version.bzl", "substitute_rust_release_version")

pkg_filegroup(
    name = "protobuf_codegen_crate",
    srcs = [
        ":protobuf_codegen_files",
        "//rust/release_crates:license",
    ],
    prefix = "protobuf_codegen",
    tags = ["manual"],
    visibility = ["//rust:__subpackages__"],
)

pkg_files(
    name = "protobuf_codegen_files",
    srcs = [":srcs"],
    strip_prefix = strip_prefix.from_root("rust/release_crates/protobuf_codegen"),
)

substitute_rust_release_version(
    src = "Cargo-template.toml",
    out = "Cargo.toml",
)

filegroup(
    name = "srcs",
    srcs = [
        "Cargo.toml",
        "README.md",
    ] + glob(["src/**/*"]),
    visibility = ["//rust:__subpackages__"],
)
