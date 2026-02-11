load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")

package(
    default_applicable_licenses = ["//:license"],
    default_testonly = 1,
    default_visibility = ["//src/google/protobuf:__subpackages__"],
)

proto_library(
    name = "test_protos",
    srcs = glob(["*.proto"]),
    strip_import_prefix = "/src",
)

cc_proto_library(
    name = "test_cc_protos",
    deps = [":test_protos"],
)

filegroup(
    name = "test_proto_srcs",
    srcs = glob(["*.proto"]),
    visibility = ["//:__subpackages__"],
)
