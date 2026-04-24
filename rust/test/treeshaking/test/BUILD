load("@rules_rust//rust:defs.bzl", "rust_binary")
load("//bazel:proto_library.bzl", "proto_library")
load("//rust:defs.bzl", "rust_proto_library")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//rust/test/treeshaking:__pkg__"],
)

proto_library(
    name = "treeshaking_proto",
    testonly = True,
    srcs = ["treeshaking.proto"],
)

rust_proto_library(
    name = "treeshaking_rust_proto",
    testonly = True,
    deps = [":treeshaking_proto"],
)

rust_binary(
    name = "treeshaking_app",
    testonly = True,
    srcs = ["main.rs"],
    deps = [":treeshaking_rust_proto"],
)
