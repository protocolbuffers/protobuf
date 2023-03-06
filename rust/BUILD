# Protobuf Rust runtime packages.

load("@rules_rust//rust:defs.bzl", "rust_library")

package(default_visibility = ["//src/google/protobuf:__subpackages__"])

rust_library(
    name = "protobuf",
    srcs = ["lib.rs"],
)

# TODO(b/270125787): Move to the right location once rust_proto_library is no longer experimental.
proto_lang_toolchain(
    name = "proto_lang_toolchain",
    command_line = "--rust_out=experimental-codegen=enabled:$(OUT)",
    progress_message = "Generating Rust proto_library %{label}",
    runtime = ":protobuf",
    visibility = ["//visibility:public"],
)
