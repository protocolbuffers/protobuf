load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

proto_lang_toolchain(
    name = "toolchain",
    blacklisted_protos = [":denied"],
    command_line = "--java_out=param1,param2:$(OUT)",
    mnemonic = "MyMnemonic",
    plugin = ":plugin",
    plugin_format_flag = "--plugin=%s",
    progress_message = "Progress Message %{label}",
    runtime = ":runtime",
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

proto_lang_toolchain(
    name = "toolchain_noplugin",
    blacklisted_protos = [":denied"],
    command_line = "--java_out=param1,param2:$(OUT)",
    mnemonic = "MyMnemonic",
    progress_message = "Progress Message %{label}",
    runtime = ":runtime",
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

cc_binary(
    name = "plugin",
    srcs = ["plugin.cc"],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

cc_library(
    name = "runtime",
    srcs = ["runtime.cc"],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

filegroup(
    name = "descriptors",
    srcs = [
        "descriptor.proto",
        "metadata.proto",
    ],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

filegroup(
    name = "any",
    srcs = ["any.proto"],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

filegroup(
    name = "something",
    srcs = ["something.proto"],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

proto_library(
    name = "mixed",
    srcs = [
        ":descriptors",
        ":something",
    ],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

proto_library(
    name = "denied",
    srcs = [
        ":any",
        ":descriptors",
    ],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

cc_binary(
    name = "_tool1",
    srcs = ["tool1.cc"],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)

cc_binary(
    name = "_tool2",
    srcs = ["tool2.cc"],
    tags = [
        "manual",
        "nobuilder",
        "notap",
    ],
)
