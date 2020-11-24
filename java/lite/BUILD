load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain")

alias(
    name = "lite",
    actual = "//java/core:lite",
    visibility = ["//visibility:public"],
)

proto_lang_toolchain(
    name = "toolchain",
    command_line = "--java_out=lite:$(OUT)",
    runtime = ":lite",
    visibility = ["//visibility:public"],
)
