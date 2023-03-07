load("//rust:defs.bzl", "rust_proto_library")
load("@rules_rust//rust:defs.bzl", "rust_test")

rust_proto_library(
    name = "hello_world_rs_proto",
    testonly = True,
    deps = ["//third_party/protobuf:unittest_proto"],
)

rust_test(
    name = "hello_world_test",
    srcs = ["hello_world_test.rs"],
    # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
    tags = ["not_build:arm"],
    deps = [":hello_world_rs_proto"],
)

proto_library(
    name = "parent_proto",
    srcs = ["parent.proto"],
)

proto_library(
    name = "child_proto",
    srcs = ["child.proto"],
    exports = [":parent_proto"],
    deps = [":parent_proto"],
)

rust_proto_library(
    name = "parent_rs_proto",
    deps = [":parent_proto"],
)

rust_proto_library(
    name = "child_rs_proto",
    deps = [":child_proto"],
)

rust_test(
    name = "child_parent_test",
    srcs = ["child_parent_test.rs"],
    # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
    tags = ["not_build:arm"],
    deps = [
        ":child_rs_proto",
        ":parent_rs_proto",
    ],
)
