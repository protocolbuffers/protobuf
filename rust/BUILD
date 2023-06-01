# Protobuf Rust runtime packages.

load("@rules_rust//rust:defs.bzl", "rust_library")
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")

load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain")

package(
    default_visibility = ["//src/google/protobuf:__subpackages__"],
)

# Represents the current Rust protobuf runtime for the build. Depending on the value of
# `:rust_proto_library_kernel` build setting it forwards to the cpp or upb kernels. This is the
# target that users are expected to depend on.
#
# Rust gencode (via `rust_upb_proto_library` and `rust_cc_proto_library`) doesn't depend on this
# target, instead, it depends on the kernel-specific libraries directly. The reason is that we need
# to be able to use both kernels in the same build. That allows us to have one tap config for both
# kernels, and to run tests using a single Blaze invocation.
#
# WARNING: It's critical that users do not end up using Rust Protobufs with multiple kernels in
# their binaries. Currently this is achieved by using bzl visibility on kernel specific rules.
# TODO(b/149440343): Hide the kernel-specific targets once we can restrict a target visibility to a
# rule.
rust_library(
    name = "protobuf",
    srcs = ["protobuf.rs"],
    rustc_flags = select({
        ":use_upb_kernel": ["--cfg=upb_kernel"],
        "//conditions:default": ["--cfg=cpp_kernel"],
    }),
    deps = select({
        ":use_upb_kernel": ["//rust/upb_kernel:upb"],
        "//conditions:default": ["//rust/cpp_kernel:cpp"],
    }),
)

rust_library(
    name = "common",
    srcs = ["common.rs"],
    visibility = ["//rust:__subpackages__"],
)

proto_lang_toolchain(
    name = "proto_rust_upb_toolchain",
    command_line = "--rust_out=experimental-codegen=enabled,kernel=upb:$(OUT)",
    progress_message = "Generating Rust proto_library %{label}",
    runtime = "//rust/upb_kernel:upb",
    visibility = ["//visibility:public"],
)

proto_lang_toolchain(
    name = "proto_rust_cpp_toolchain",
    command_line = "--rust_out=experimental-codegen=enabled,kernel=cpp:$(OUT)",
    progress_message = "Generating Rust proto_library %{label}",
    runtime = "//rust/cpp_kernel:cpp",
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
