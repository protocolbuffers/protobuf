load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("//rust/release_crates:substitute_rust_release_version.bzl", "substitute_rust_release_version")

pkg_tar(
    name = "protobuf_crate",
    srcs = [
        ":crate_root_files",
        "//:LICENSE",
        "//rust:rust_protobuf_libupb_src",
        "//rust:rust_protobuf_src_dir",
    ],
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

substitute_rust_release_version(
    src = "README-template.md",
    out = "README.md",
)

filegroup(
    name = "srcs",
    srcs = [
        "Cargo.toml",
        "README.md",
        "build.rs",
    ],
    visibility = ["//rust:__subpackages__"],
)
