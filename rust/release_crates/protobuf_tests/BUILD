load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files", "strip_prefix")
load("//release_crates:substitute_rust_release_version.bzl", "substitute_rust_release_version")

pkg_filegroup(
    name = "protobuf_tests_crate",
    srcs = [
        ":crate_root_files",
        "//release_crates:license",
        "//test:google_protobuf_proto_srcs",
        "//test:test_proto_srcs",
        "//test/shared:test_srcs",
    ],
    prefix = "protobuf_tests",
    visibility = ["//:__subpackages__"],
)

pkg_files(
    name = "crate_root_files",
    srcs = [":srcs"],
    strip_prefix = strip_prefix.from_root("release_crates/protobuf_tests"),
)

substitute_rust_release_version(
    src = "Cargo-template.toml",
    out = "Cargo.toml",
)

filegroup(
    name = "srcs",
    srcs = [
        "Cargo.toml",
        "build.rs",
        "tests/protos/mod.rs",
    ],
    visibility = ["//:__subpackages__"],
)
