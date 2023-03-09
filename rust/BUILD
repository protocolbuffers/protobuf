# Protobuf Rust runtime packages.

load("@rules_rust//rust:defs.bzl", "rust_library")
load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain")

package(default_visibility = ["//src/google/protobuf:__subpackages__"])

rust_library(
    name = "protobuf",
    srcs = [
        "lib.rs",
        "upb.rs",
    ],
    deps = [
        ":upb_api",
    ],
)

cc_library(
    name = "upb_api",
    srcs = ["upb_api.c"],
    deps = [
        "//third_party/upb:message_accessors",
    ],
)

# TODO(b/270125787): Move to the right location once rust_proto_library is no longer experimental.
proto_lang_toolchain(
    name = "proto_lang_toolchain",
    command_line = "--rust_out=experimental-codegen=enabled:$(OUT)",
    progress_message = "Generating Rust proto_library %{label}",
    runtime = ":protobuf",
    visibility = ["//visibility:public"],
)
