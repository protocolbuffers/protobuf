load("//:protobuf_version.bzl", "PROTOBUF_RUST_VERSION")

genrule(
    name = "gen_cargo_toml",
    srcs = ["Cargo-template.toml"],
    outs = ["Cargo.toml"],
    cmd = "cat $(SRCS) | sed -e 's/{{VERSION}}/{0}-beta/g' > $(OUTS)".format(PROTOBUF_RUST_VERSION),
)

filegroup(
    name = "srcs",
    srcs = [
        ":gen_cargo_toml",
    ] + glob(["src/**/*"]),
    visibility = ["//rust:__subpackages__"],
)
