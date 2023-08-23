load("@rules_cc//cc:defs.bzl", "cc_proto_library")

exports_files(
    glob(["*.proto"]),
    visibility = [
        "//src/google/protobuf/editions:__pkg__",
    ],
)

proto_library(
    name = "proto2_required_proto",
    srcs = ["proto2_required.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto3_optional_proto",
    srcs = ["proto3_optional.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto3_implicit_proto",
    srcs = ["proto3_implicit.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto2_optional_proto",
    srcs = ["proto2_optional.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto2_enum_proto",
    srcs = ["proto2_enum.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto3_enum_proto",
    srcs = ["proto3_enum.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto2_packed_proto",
    srcs = ["proto2_packed.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto3_packed_proto",
    srcs = ["proto3_packed.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto2_group_proto",
    srcs = ["proto2_group.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto2_unpacked_proto",
    srcs = ["proto2_unpacked.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto3_unpacked_proto",
    srcs = ["proto3_unpacked.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto2_utf8_verify_proto",
    srcs = ["proto2_utf8_verify.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto3_utf8_disabled_proto",
    srcs = ["proto3_utf8_disabled.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto2_import_proto",
    srcs = ["proto2_import.proto"],
    strip_import_prefix = "/src",
    deps = [":proto2_optional_proto"],
)

proto_library(
    name = "proto2_utf8_disabled_proto",
    srcs = ["proto2_utf8_disabled.proto"],
    strip_import_prefix = "/src",
)

cc_proto_library(
    name = "proto2_utf8_disabled_cc_proto",
    deps = [":proto2_utf8_disabled_proto"],
)

proto_library(
    name = "proto2_utf8_lite_proto",
    srcs = ["proto2_utf8_lite.proto"],
    strip_import_prefix = "/src",
)

cc_proto_library(
    name = "proto2_utf8_lite_cc_proto",
    deps = [":proto2_utf8_lite_proto"],
)

proto_library(
    name = "proto2_proto3_enum_proto",
    srcs = ["proto2_proto3_enum.proto"],
    strip_import_prefix = "/src",
    deps = [":proto3_enum_proto"],
)

proto_library(
    name = "proto3_utf8_strict_proto",
    srcs = ["proto3_utf8_strict.proto"],
    strip_import_prefix = "/src",
)

proto_library(
    name = "proto3_import_proto",
    srcs = ["proto3_import.proto"],
    strip_import_prefix = "/src",
    deps = [":proto3_implicit_proto"],
)

cc_proto_library(
    name = "proto3_import_cc_proto",
    deps = [":proto3_import_proto"],
)
