# Protobuf Rust runtime packages.

load("@bazel_skylib//rules:common_settings.bzl", "string_flag")
load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files", "strip_prefix")
load("@rules_pkg//pkg:tar.bzl", "pkg_tar")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load("//rust:dist.bzl", "pkg_cross_compiled_binaries")

licenses(["notice"])

package_group(
    name = "protobuf_internal",
    packages = [
        "//rust/...",
        "//src/google/protobuf/...",
    ],
)

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
    visibility = ["//visibility:public"],
    deps = select({
        ":use_upb_kernel": [":protobuf_upb"],
        "//conditions:default": [":protobuf_cpp"],
    }),
)

# This list contains kernel-agnostic logic and simple kernel-specific logic controlled by
# `#[cfg(...)]` attributes. That forces us to compile these files twice, once for each kernel. As a
# result these files are included in both `:protobuf_upb` and `:protobuf_cpp`.
# This is in principle identical to how we compile regular Rust source files twice
# (once for production, and once for unit testing).
#
# shared.rs is the root of the crate and has public items re-exported in protobuf.rs for user use.
PROTOBUF_SHARED = [
    # go/keep-sorted start
    "codegen_traits.rs",
    "cord.rs",
    "enum.rs",
    "internal.rs",
    "map.rs",
    "optional.rs",
    "prelude.rs",
    "primitive.rs",
    "proto_macro.rs",
    "proxied.rs",
    "repeated.rs",
    "shared.rs",
    "string.rs",
    # go/keep-sorted end
]

# All Rust source files; this is used for packaging up a crate.
ALL_RUST_SRCS = PROTOBUF_SHARED + [
    # go/keep-sorted start
    "cpp.rs",
    "gtest_matchers.rs",
    "gtest_matchers_impl.rs",
    "protobuf.rs",
    "upb.rs",
    "utf8.rs",
    # go/keep-sorted end
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
    rustc_flags = [
        "--cfg=upb_kernel",
        "--cfg=bzl",
    ],
    visibility = [":protobuf_internal"],
    deps = [
        ":utf8",
        "//rust/upb",
    ],
)

rust_test(
    name = "protobuf_upb_test",
    crate = ":protobuf_upb",
    rustc_flags = [
        "--cfg=upb_kernel",
        "--cfg=bzl",
    ],
    deps = [
        "@crate_index//:googletest",
    ],
)

# This provides an identical set of re-exports as `:protobuf` with `:use_upb_kernel` active.
# This is only used for tests shared between runtimes.
rust_library(
    name = "protobuf_upb_export",
    testonly = True,
    srcs = ["protobuf.rs"],
    rustc_flags = ["--cfg=upb_kernel"],
    visibility = [":protobuf_internal"],
    deps = [":protobuf_upb"],
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
    rustc_flags = [
        "--cfg=cpp_kernel",
        "--cfg=bzl",
    ],
    visibility = [":protobuf_internal"],
    deps = [
        ":utf8",
        "//rust/cpp_kernel:cpp_api",
    ],
)

rust_test(
    name = "protobuf_cpp_test",
    crate = ":protobuf_cpp",
    rustc_flags = [
        "--cfg=cpp_kernel",
        "--cfg=bzl",
    ],
    deps = [
        "@crate_index//:googletest",
    ],
)

# This provides an identical set of re-exports as `:protobuf` with `:use_upb_kernel` inactive.
# This is only used for tests shared between runtimes.
rust_library(
    name = "protobuf_cpp_export",
    testonly = True,
    srcs = ["protobuf.rs"],
    rustc_flags = ["--cfg=cpp_kernel"],
    visibility = [":protobuf_internal"],
    deps = [":protobuf_cpp"],
)

rust_library(
    name = "protobuf_gtest_matchers",
    testonly = True,
    srcs = ["gtest_matchers.rs"],
    aliases = select({
        ":use_upb_kernel": {"//rust:protobuf_gtest_matchers_upb": "protobuf_gtest_matchers_impl"},
        "//conditions:default": {"//rust:protobuf_gtest_matchers_cpp": "protobuf_gtest_matchers_impl"},
    }),
    visibility = ["//visibility:public"],
    deps = select({
        ":use_upb_kernel": [":protobuf_gtest_matchers_upb"],
        "//conditions:default": [":protobuf_gtest_matchers_cpp"],
    }),
)

rust_library(
    name = "protobuf_gtest_matchers_cpp",
    testonly = True,
    srcs = ["gtest_matchers_impl.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
    },
    visibility = [":protobuf_internal"],
    deps = [
        ":protobuf_cpp",
        "@crate_index//:googletest",
    ],
)

rust_library(
    name = "protobuf_gtest_matchers_upb",
    testonly = True,
    srcs = ["gtest_matchers_impl.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
    },
    visibility = [":protobuf_internal"],
    deps = [
        ":protobuf_upb",
        "@crate_index//:googletest",
    ],
)

rust_library(
    name = "utf8",
    srcs = ["utf8.rs"],
)

proto_lang_toolchain(
    name = "proto_rust_upb_toolchain",
    command_line = "--rust_out=$(OUT)",
    output_files = "multiple",
    progress_message = "Generating Rust proto_library %{label}",
    runtime = ":protobuf_upb",
    visibility = ["//visibility:public"],
)

proto_lang_toolchain(
    name = "proto_rust_cpp_toolchain",
    command_line = "--rust_out=$(OUT)",
    output_files = "multiple",
    progress_message = "Generating Rust proto_library %{label}",
    runtime = ":protobuf_cpp",
    visibility = ["//visibility:public"],
)

package_group(
    name = "rust_proto_library_allowed_in_different_package",
    packages = ["//rust/test"],  # for unittest proto_libraries
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

# Note: strip_prefix.from_pkg() should work for this below, but has surprising behavior in bazel
# with filegroups and so we only use strip_prefix.from_root() and this constant instead.
SRC_ROOT = "rust"

pkg_files(
    name = "rust_protobuf_src",
    srcs = ALL_RUST_SRCS,
    strip_prefix = strip_prefix.from_root(SRC_ROOT),
)

pkg_files(
    name = "crate_root_files",
    srcs = ["//rust/cargo:srcs"],
    strip_prefix = strip_prefix.from_root(SRC_ROOT + "/cargo"),
)

pkg_filegroup(
    name = "rust_protobuf_src_dir",
    srcs = [
        ":rust_protobuf_src",
        "//rust/upb:rust_protobuf_upb_src",
    ],
    prefix = "src",
)

pkg_files(
    name = "amalgamated_upb",
    srcs = ["//upb:gen_amalgamation"],
    strip_prefix = strip_prefix.from_root(""),
)

pkg_filegroup(
    name = "rust_protobuf_libupb_src",
    srcs = [
        ":amalgamated_upb",
        "//upb/cmake:upb_cmake_dist",
    ],
    prefix = "libupb",
)

pkg_tar(
    name = "protobuf_crate_dist",
    srcs = [
        ":crate_root_files",
        ":rust_protobuf_libupb_src",
        ":rust_protobuf_src_dir",
        "//:LICENSE",
    ],
)

pkg_files(
    name = "protobuf_codegen_files",
    srcs = ["//rust/protobuf_codegen:srcs"],
    strip_prefix = strip_prefix.from_root(SRC_ROOT + "/protobuf_codegen"),
)

pkg_tar(
    name = "codegen_crate_dist",
    srcs = [
        ":protobuf_codegen_files",
        ":vendored_protocs_dist",
        "//:LICENSE",
    ],
    tags = ["manual"],
)

pkg_tar(
    name = "codegen_crate_test",
    srcs = [
        ":protobuf_codegen_files",
        ":vendored_protocs_test",
        "//:LICENSE",
    ],
    tags = ["manual"],
)

pkg_files(
    name = "codegen_example_files",
    srcs = ["//rust/protobuf_codegen/example:srcs"],
    strip_prefix = strip_prefix.from_root(SRC_ROOT + "/protobuf_codegen/example"),
)

pkg_tar(
    name = "codegen_example_test",
    srcs = [
        ":codegen_example_files",
        "//:LICENSE",
    ],
)

# Bundle all protoc binaries for all platforms. Requires the toolchains to be installed.
pkg_cross_compiled_binaries(
    name = "vendored_protocs_dist",
    cpus = [
        # TODO: Re-enable these platforms once the toolchains are available.
        # "osx-x86_64",
        # "osx-aarch_64",
        "linux-aarch_64",
        "linux-ppcle_64",
        # "linux-s390_64",
        "linux-x86_32",
        "linux-x86_64",
        "win32",
        "win64",
    ],
    prefix = "bin",
    tags = ["manual"],
    targets = [
        "//upb_generator/minitable:protoc-gen-upb_minitable",
        "//:protoc",
    ],
)

# Bundle only the linux-x86_64 protoc for testing.
pkg_cross_compiled_binaries(
    name = "vendored_protocs_test",
    cpus = [
        "linux-x86_64",
    ],
    prefix = "bin",
    tags = ["manual"],
    targets = [
        "//upb_generator/minitable:protoc-gen-upb_minitable",
        "//:protoc",
    ],
)

# Run the cargo test with only a bundled linux-x86_64 protoc.
sh_binary(
    name = "cargo_test",
    srcs = ["cargo_test.sh"],
    data = [
        ":codegen_crate_test",
        ":codegen_example_test",
        ":protobuf_crate_dist",
    ],
    tags = ["manual"],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)

# Run the cargo test with all bundled protocs.
sh_binary(
    name = "cargo_release_test",
    srcs = ["cargo_test.sh"],
    data = [
        ":codegen_crate_dist",
        ":codegen_example_test",
        ":protobuf_crate_dist",
    ],
    tags = ["manual"],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
