load(
    "//:protobuf.bzl",
    "internal_csharp_proto_library",
    "internal_objc_proto_library",
    "internal_py_proto_library",
)
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:java_lite_proto_library.bzl", "java_lite_proto_library")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")

package(
    default_testonly = True,
    default_visibility = ["//conformance:__subpackages__"],
)

exports_files([
    "test_messages_edition2023.proto",
    "test_messages_edition_unstable.proto",
])

internal_csharp_proto_library(
    name = "test_messages_edition2023_csharp_proto",
    srcs = ["test_messages_edition2023.proto"],
)

internal_csharp_proto_library(
    name = "test_messages_edition_unstable_csharp_proto",
    srcs = ["test_messages_edition_unstable.proto"],
)

# Aliases for older monolithic conformance test protos defined elsewhere.
alias(
    name = "test_messages_proto2_editions_proto",
    actual = "//editions:test_messages_proto2_editions_proto",
    visibility = ["//visibility:public"],
)

alias(
    name = "test_messages_proto3_editions_proto",
    actual = "//editions:test_messages_proto3_editions_proto",
    visibility = ["//visibility:public"],
)

alias(
    name = "test_messages_proto2_proto",
    actual = "//src/google/protobuf:test_messages_proto2_proto",
    visibility = ["//visibility:public"],
)

alias(
    name = "test_messages_proto3_proto",
    actual = "//src/google/protobuf:test_messages_proto3_proto",
    visibility = ["//visibility:public"],
)

proto_library(
    name = "test_messages_edition2023_proto",
    srcs = ["test_messages_edition2023.proto"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "test_messages_edition_unstable_proto",
    srcs = ["test_messages_edition_unstable.proto"],
    visibility = ["//visibility:public"],
)

java_lite_proto_library(
    name = "test_messages_edition2023_java_proto_lite",
    deps = [":test_messages_edition2023_proto"],
)

java_lite_proto_library(
    name = "test_messages_edition_unstable_java_proto_lite",
    deps = [":test_messages_edition_unstable_proto"],
)

java_proto_library(
    name = "test_messages_edition2023_java_proto",
    deps = [":test_messages_edition2023_proto"],
)

java_proto_library(
    name = "test_messages_edition_unstable_java_proto",
    deps = [":test_messages_edition_unstable_proto"],
)

cc_proto_library(
    name = "test_messages_edition2023_cc_proto",
    deps = [":test_messages_edition2023_proto"],
)

cc_proto_library(
    name = "test_messages_edition_unstable_cc_proto",
    deps = [":test_messages_edition_unstable_proto"],
)

internal_py_proto_library(
    name = "test_messages_edition2023_py_pb2",
    srcs = [":test_messages_edition2023.proto"],
    srcs_version = "PY2AND3",
)

internal_py_proto_library(
    name = "test_messages_edition_unstable_py_pb2",
    srcs = [":test_messages_edition_unstable.proto"],
    srcs_version = "PY2AND3",
)

internal_objc_proto_library(
    name = "test_messages_edition2023_objc_proto",
    srcs = [":test_messages_edition2023.proto"],
)

internal_objc_proto_library(
    name = "test_messages_edition_unstable_objc_proto",
    srcs = [":test_messages_edition_unstable.proto"],
)
