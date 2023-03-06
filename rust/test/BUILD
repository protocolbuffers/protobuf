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
