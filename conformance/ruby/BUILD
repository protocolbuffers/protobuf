# These conformance tests are in a dedicated subdirectory to avoid including
# the dev-only dependency on `rules_ruby` in the main conformance directory,
# which is meant to be externally accessible.

load("@rules_ruby//ruby:defs.bzl", "rb_binary")
load("//ruby:defs.bzl", "internal_ruby_proto_library")

internal_ruby_proto_library(
    name = "conformance_ruby_proto",
    srcs = ["conformance.proto"],
)

rb_binary(
    name = "conformance_ruby",
    testonly = True,
    srcs = ["conformance_ruby.rb"],
    main = "conformance_ruby.rb",
    visibility = ["//ruby:__subpackages__"],
    deps = [
        ":conformance_ruby_proto",
        "//conformance/test_protos:test_messages_edition2023_ruby_proto",
        "//ruby:conformance_editions_test_ruby_proto",
        "//ruby:conformance_test_ruby_proto",
        "//ruby:protobuf",
    ],
)

