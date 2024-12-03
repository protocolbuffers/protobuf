load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")

pkg_tar(
    name = "protobuf_example_crate",
    srcs = [
        ":protobuf_example_files",
        "//:LICENSE",
    ],
    visibility = ["//rust:__subpackages__"],
)

pkg_files(
    name = "protobuf_example_files",
    srcs = [":srcs"],
    strip_prefix = strip_prefix.from_root("rust/release_crates/protobuf_example"),
)

filegroup(
    name = "srcs",
    srcs = glob(["**/*"]),
    visibility = ["//rust:__subpackages__"],
)
