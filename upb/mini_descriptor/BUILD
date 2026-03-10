load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load(
    "//upb/bazel:copts.bzl",
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "mini_descriptor",
    srcs = [
        "build_enum.c",
        "decode.c",
        "link.c",
    ],
    hdrs = [
        "build_enum.h",
        "decode.h",
        "link.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb/base",
        "//upb/base:internal",
        "//upb/mem",
        "//upb/message:types",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
    ] + select({
        "//upb:fasttable_enabled_setting": [
            "//upb/wire/decode_fast:function_array",
            "//upb/wire/decode_fast:select",
        ],
        "//conditions:default": [],
    }),
)

cc_library(
    name = "internal",
    srcs = [
        "internal/base92.c",
        "internal/encode.c",
    ],
    hdrs = [
        "internal/base92.h",
        "internal/decoder.h",
        "internal/encode.h",
        "internal/encode.hpp",
        "internal/modifiers.h",
        "internal/wire_constants.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb/base",
        "//upb/base:internal",
        "//upb/port",
    ],
)

cc_test(
    name = "encode_test",
    srcs = ["internal/encode_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":internal",
        ":mini_descriptor",
        "//src/google/protobuf",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/message:internal",
        "//upb/mini_table",
        "//upb/port",
        "//upb/wire",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

filegroup(
    name = "source_files",
    srcs = glob(
        [
            "**/*.c",
            "**/*.h",
        ],
    ),
    visibility = [
        "//python/dist:__pkg__",
        "//upb/cmake:__pkg__",
    ],
)

filegroup(
    name = "test_srcs",
    srcs = glob(
        [
            "**/*test.cc",
        ],
    ),
    visibility = ["//upb:__pkg__"],
)
