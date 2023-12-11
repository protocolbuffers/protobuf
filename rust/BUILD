# Protobuf Rust runtime packages.

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")

load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain")

package(
    default_visibility = [
        "//src/google/protobuf:__subpackages__",
    ],
)

licenses(["notice"])

# The current Rust Protobuf runtime for the build. Depending on the value of
# `:rust_proto_library_kernel` build setting it forwards to the cpp or upb kernels. This is the
# target that users are expected to depend on.
#
# Rust gencode (via `rust_upb_proto_library` and `rust_cc_proto_library`) doesn't depend on this
# target, instead, it depends on the kernel-specific libraries directly. The reason is that we need
# to be able to use both kernels in the same build. That allows us to have one TAP config for both
# kernels, and to run tests using a single Blaze invocation.
#
# WARNING: It's critical that users do not end up using Rust Protobufs with multiple kernels in
# their binaries. Currently this is achieved by using bzl visibility on kernel-specific rules.
# TODO: Hide the kernel-specific targets once we can restrict a target visibility to a
# rule.
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

# This list contains kernel-agnostic logic and simple kernel-specific logic controlled by
# `#[cfg(...)]` attributes. That forces us to compile these files twice, once for each kernel. As a
# result these files are included in both `:protobuf_upb` and `:protobuf_cpp`. This is in principle
# identical to how we compile regular Rust source files twice (once for production, and once for
# unittesting).
#
# shared.rs is the root of the crate and has public items re-exported in protobuf.rs for user use.
PROTOBUF_SHARED = [
    "internal.rs",
    "macros.rs",
    "optional.rs",
    "primitive.rs",
    "proxied.rs",
    "repeated.rs",
    "shared.rs",
    "string.rs",
    "vtable.rs",
    "map.rs",
]

# The Rust Protobuf runtime using the upb kernel.
#
# `rust_upb_proto_library` implicitly depends on this target. This target cannot depend on
# `:rust_proto_library_kernel` build setting; it has to be fully functional under any value of that
# setting.
rust_library(
    name = "protobuf_upb",
    srcs = PROTOBUF_SHARED + ["upb.rs"],
    crate_root = "shared.rs",
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    rustc_flags = ["--cfg=upb_kernel"],
    visibility = [
        "//src/google/protobuf:__subpackages__",
        "//rust:__subpackages__",
    ],
    deps = [
        ":utf8",
        "//rust/upb_kernel:upb_c_api",
    ],
)

rust_test(
    name = "protobuf_upb_test",
    crate = ":protobuf_upb",
    rustc_flags = ["--cfg=upb_kernel"],
    deps = [
        "@crate_index//:googletest",
    ],
)

# The Rust Protobuf runtime using the cpp kernel.
#
# `rust_cpp_proto_library` implicitly depends on this target. This target cannot depend on
# `:rust_proto_library_kernel` build setting; it has to be fully functional under any value of that
# setting.
rust_library(
    name = "protobuf_cpp",
    srcs = PROTOBUF_SHARED + ["cpp.rs"],
    crate_root = "shared.rs",
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    rustc_flags = ["--cfg=cpp_kernel"],
    deps = [
        ":utf8",
        "//rust/cpp_kernel:cpp_api",
    ],
)

rust_test(
    name = "protobuf_cpp_test",
    crate = ":protobuf_cpp",
    rustc_flags = ["--cfg=cpp_kernel"],
    deps = [
        "@crate_index//:googletest",
    ],
)

rust_library(
    name = "utf8",
    srcs = ["utf8.rs"],
    visibility = ["//visibility:private"],
)

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
