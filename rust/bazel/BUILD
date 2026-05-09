load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

licenses(["notice"])

exports_files(
    ["aspects.bzl"],
    visibility = ["//third_party/crubit:__subpackages__"],
)

bzl_library(
    name = "aspects_bzl",
    srcs = [
        "aspects.bzl",
        "encode_raw_string_as_crate_name.bzl",
    ],
    parse_tests = False,
    visibility = [
        "//rust:__pkg__",
        "//third_party/crubit:__subpackages__",
    ],
    deps = [
        "//bazel/private:cc_proto_aspect_bzl",
        "//upb/bazel:upb_proto_library_bzl",
    ],
)
