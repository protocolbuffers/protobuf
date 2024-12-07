load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("//:protobuf_version.bzl", "PROTOBUF_RUST_VERSION")

pkg_tar(
    name = "protobuf_crate",
    srcs = [
        ":crate_root_files",
        ":well_known_types",
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

genrule(
    name = "gen_cargo_toml",
    srcs = ["Cargo-template.toml"],
    outs = ["Cargo.toml"],
    cmd = "cat $(SRCS) | sed -e 's/{{VERSION}}/{0}-beta/g' > $(OUTS)".format(PROTOBUF_RUST_VERSION),
)

filegroup(
    name = "srcs",
    srcs = [
        "README.md",
        "build.rs",
        "crate_mapping",
        ":gen_cargo_toml",
    ],
    visibility = ["//rust:__subpackages__"],
)

pkg_files(
    name = "well_known_types",
    srcs = ["//src/google/protobuf:well_known_type_protos"],
    prefix = "google/protobuf",
)
