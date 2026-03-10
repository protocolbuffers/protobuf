load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")

package(
    default_applicable_licenses = ["//:license"],
)

exports_files(
    glob(["*.proto"]),
    visibility = [
        "//editions:__pkg__",
    ],
)

proto_library(
    name = "proto2_inline_comments_proto",
    srcs = ["proto2_inline_comments.proto"],
)

proto_library(
    name = "proto2_multiline_comments_proto",
    srcs = ["proto2_multiline_comments.proto"],
)

proto_library(
    name = "proto2_required_proto",
    srcs = ["proto2_required.proto"],
)

proto_library(
    name = "proto3_optional_proto",
    srcs = ["proto3_optional.proto"],
)

proto_library(
    name = "proto3_implicit_proto",
    srcs = ["proto3_implicit.proto"],
)

proto_library(
    name = "proto2_optional_proto",
    srcs = ["proto2_optional.proto"],
)

proto_library(
    name = "proto2_enum_proto",
    srcs = ["proto2_enum.proto"],
)

proto_library(
    name = "proto3_enum_proto",
    srcs = ["proto3_enum.proto"],
)

proto_library(
    name = "proto2_packed_proto",
    srcs = ["proto2_packed.proto"],
)

proto_library(
    name = "proto3_packed_proto",
    srcs = ["proto3_packed.proto"],
)

proto_library(
    name = "proto2_group_proto",
    srcs = ["proto2_group.proto"],
)

proto_library(
    name = "proto2_unpacked_proto",
    srcs = ["proto2_unpacked.proto"],
)

proto_library(
    name = "proto3_unpacked_proto",
    srcs = ["proto3_unpacked.proto"],
)

proto_library(
    name = "proto2_utf8_verify_proto",
    srcs = ["proto2_utf8_verify.proto"],
)

proto_library(
    name = "proto2_import_proto",
    srcs = ["proto2_import.proto"],
    deps = [":proto2_optional_proto"],
)

proto_library(
    name = "proto2_utf8_disabled_proto",
    srcs = ["proto2_utf8_disabled.proto"],
)

cc_proto_library(
    name = "proto2_utf8_disabled_cc_proto",
    deps = [":proto2_utf8_disabled_proto"],
)

proto_library(
    name = "proto2_utf8_lite_proto",
    srcs = ["proto2_utf8_lite.proto"],
)

cc_proto_library(
    name = "proto2_utf8_lite_cc_proto",
    deps = [":proto2_utf8_lite_proto"],
)

proto_library(
    name = "proto2_proto3_enum_proto",
    srcs = ["proto2_proto3_enum.proto"],
    deps = [":proto3_enum_proto"],
)

proto_library(
    name = "proto3_utf8_strict_proto",
    srcs = ["proto3_utf8_strict.proto"],
)

proto_library(
    name = "proto3_import_proto",
    srcs = ["proto3_import.proto"],
    deps = [":proto3_implicit_proto"],
)

cc_proto_library(
    name = "proto3_import_cc_proto",
    deps = [":proto3_import_proto"],
)
