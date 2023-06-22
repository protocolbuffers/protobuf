load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)

# begin:github_only
load("@rules_pkg//:mappings.bzl", "pkg_files")
# end:github_only

cc_library(
    name = "mini_table",
    hdrs = [
        "decode.h",
        "extension_registry.h",
        "types.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":mini_table_internal",
        "//:base",
        "//:mem",
        "//:port",
    ],
)

cc_library(
    name = "mini_table_internal",
    srcs = [
        "common.c",
        "decode.c",
        "encode.c",
        "extension_registry.c",
        "message_internal.c",
    ],
    hdrs = [
        "common.h",
        "common_internal.h",
        "decode.h",
        "encode_internal.h",
        "encode_internal.hpp",
        "enum_internal.h",
        "extension_internal.h",
        "extension_registry.h",
        "field_internal.h",
        "file_internal.h",
        "message_internal.h",
        "sub_internal.h",
        "types.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//:base",
        "//:hash",
        "//:mem",
        "//:port",
    ],
)

cc_test(
    name = "encode_test",
    srcs = ["encode_test.cc"],
    deps = [
        ":mini_table_internal",
        "//:collections_internal",
        "//:hash",
        "//:message_internal",
        "//:port",
        "//:upb",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

# begin:github_only
filegroup(
    name = "cmake_files",
    srcs = glob(
        [
            "**/*.c",
            "**/*.h",
        ],
        exclude = ["encode_test.cc"],
    ),
    visibility = ["//cmake:__pkg__"],
)

pkg_files(
    name = "source_files",
    srcs = glob(
        [
            "**/*.c",
            "**/*.h",
        ],
        exclude = ["encode_test.cc"],
    ),
    prefix = "upb/mini_table",
    visibility = ["//python/dist:__pkg__"],
)
# end:github_only
