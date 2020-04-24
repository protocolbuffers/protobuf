load("@rules_proto//proto:defs.bzl", "proto_library")
load("@upb//bazel:upb_proto_library.bzl", "upb_proto_library")

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
)

upb_proto_library(
    name = "foo_upbproto",
    deps = [":foo_proto"],
)

cc_binary(
    name = "test_binary",
    srcs = ["test_binary.c"],
    deps = [":foo_upbproto"],
)
