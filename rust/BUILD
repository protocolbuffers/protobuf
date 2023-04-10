# Protobuf Rust runtime packages.

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")

load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain")

package(
    default_visibility = ["//src/google/protobuf:__subpackages__"],
)

rust_library(
    name = "protobuf",
    srcs = ["protobuf.rs"],
    rustc_flags = select({
        ":use_upb_kernel": ["--cfg=upb_kernel"],
        "//conditions:default": ["--cfg=cpp_kernel"],
    }),
    deps = select({
        ":use_upb_kernel": [":protobuf_upb"],
        "//conditions:default": [":protobuf_cpp"],
    }),
)

rust_library(
    name = "protobuf_upb",
    srcs = ["shared.rs"],
    rustc_flags = ["--cfg=upb_kernel"],
    deps = ["//rust/upb_kernel:upb"],
)

rust_test(
    name = "protobuf_upb_test",
    crate = ":protobuf_upb",
    rustc_flags = ["--cfg=upb_kernel"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
        # TODO(b/243126140): Enable tsan once we support sanitizers with Rust.
        "notsan",
        # TODO(b/243126140): Enable msan once we support sanitizers with Rust.
        "nomsan",
    ],
)

rust_library(
    name = "protobuf_cpp",
    srcs = ["shared.rs"],
    rustc_flags = ["--cfg=cpp_kernel"],
    deps = ["//rust/cpp_kernel:cpp"],
)

rust_test(
    name = "protobuf_cpp_test",
    crate = ":protobuf_cpp",
    rustc_flags = ["--cfg=cpp_kernel"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
        # TODO(b/243126140): Enable tsan once we support sanitizers with Rust.
        "notsan",
        # TODO(b/243126140): Enable msan once we support sanitizers with Rust.
        "nomsan",
    ],
)

# TODO(b/270125787): Move to the right location once rust_proto_library is no longer experimental.
proto_lang_toolchain(
    name = "proto_rust_upb_toolchain",
    command_line = "--rust_out=experimental-codegen=enabled,kernel=upb:$(OUT)",
    progress_message = "Generating Rust proto_library %{label}",
    runtime = ":protobuf_upb",
    visibility = ["//visibility:public"],
)

proto_lang_toolchain(
    name = "proto_rust_cpp_toolchain",
    command_line = "--rust_out=experimental-codegen=enabled,kernel=cpp:$(OUT)",
    progress_message = "Generating Rust proto_library %{label}",
    runtime = ":protobuf_cpp",
    visibility = ["//visibility:public"],
)

# This flag controls what kernel all Rust Protobufs are using in the current build.
string_flag(
    name = "rust_proto_library_kernel",
    build_setting_default = "cpp",
    values = [
        "upb",
        "cpp",
    ],
)

config_setting(
    name = "use_upb_kernel",
    flag_values = {
        ":rust_proto_library_kernel": "upb",
    },
)
