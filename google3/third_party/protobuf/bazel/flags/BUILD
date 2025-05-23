package(default_applicable_licenses = ["//:license"])

cc_library(name = "no_cc_lib")

label_flag(
    name = "proto_toolchain_for_javamutable",
    # TODO: What is the correct default?
    build_setting_default = ":no_cc_lib",
    help = "Label of proto_lang_toolchain() which describes how to compile Java mutable protos.",
    visibility = ["//bazel:__subpackages__"],
)

label_flag(
    name = "proto_toolchain_for_javamutable_with_stubby1",
    # TODO: What is the correct default?
    build_setting_default = ":no_cc_lib",
    help = "Label of proto_lang_toolchain() which describes how to compile Java mutable " +
           "protos with stubby1.",
    visibility = ["//bazel:__subpackages__"],
)

label_flag(
    name = "proto_toolchain_for_javamutable_with_stubby3",
    # TODO: What is the correct default?
    build_setting_default = ":no_cc_lib",
    help = "Label of proto_lang_toolchain() which describes how to compile Java mutable " +
           "protos with stubby3.",
    visibility = ["//bazel:__subpackages__"],
)

label_flag(
    name = "proto_toolchain_for_javamutable_with_stubby13",
    # TODO: What is the correct default?
    build_setting_default = ":no_cc_lib",
    help = "Label of proto_lang_toolchain() which describes how to compile Java mutable " +
           "protos with stubby1 and stubby3 compatibility mode.",
    visibility = ["//bazel:__subpackages__"],
)

label_flag(
    name = "protoc_cc_lib",
    build_setting_default = ":no_cc_lib",
    help = "Extra C++ library that protoc generated code depends on.",
    visibility = ["//bazel:__subpackages__"],
)

label_flag(
    name = "protoc_cc_plugin",
    # TODO: What is the correct default?
    build_setting_default = ":no_cc_lib",
    help = "Extra C++ plugin for protoc.",
    visibility = ["//bazel:__subpackages__"],
)
