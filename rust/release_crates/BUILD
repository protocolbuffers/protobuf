# Protobuf Rust crate packaging for release.

# Run the cargo test with only a bundled linux-x86_64 protoc.
sh_binary(
    name = "cargo_test",
    srcs = ["cargo_test.sh"],
    data = [
        "//:protoc",
        "//rust/release_crates/protobuf:protobuf_crate",
        "//rust/release_crates/protobuf_codegen:protobuf_codegen_crate",
        "//rust/release_crates/protobuf_example:protobuf_example_crate",
        "//rust/release_crates/protobuf_well_known_types:crate",
        "//upb_generator/minitable:protoc-gen-upb_minitable",
    ],
    tags = ["manual"],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)

sh_binary(
    name = "release",
    srcs = ["release.sh"],
    data = [
        "//:protoc",
        "//rust/release_crates/protobuf:protobuf_crate",
        "//rust/release_crates/protobuf_codegen:protobuf_codegen_crate",
        "//rust/release_crates/protobuf_example:protobuf_example_crate",
        "//upb_generator/minitable:protoc-gen-upb_minitable",
    ],
    tags = ["manual"],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
