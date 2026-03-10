load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files", "strip_prefix")
load("//rust/release_crates:substitute_rust_release_version.bzl", "substitute_rust_release_version")

pkg_filegroup(
    name = "protobuf_crate",
    srcs = [
        ":crate_root_files",
        "//rust:rust_protobuf_libupb_src",
        "//rust:rust_protobuf_src_dir",
        "//rust/release_crates:license",
    ],
    prefix = "protobuf",
    visibility = ["//rust:__subpackages__"],
)

pkg_files(
    name = "crate_root_files",
    srcs = [":srcs"],
    strip_prefix = strip_prefix.from_root("rust/release_crates/protobuf"),
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
        "build.rs",
    ],
    visibility = ["//visibility:private"],
)
