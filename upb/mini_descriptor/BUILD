load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)

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
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:mini_table_internal",
        "//upb:port",
    ],
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
        "//upb:base",
        "//upb:base_internal",
        "//upb:port",
    ],
)

cc_test(
    name = "encode_test",
    srcs = ["internal/encode_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":internal",
        ":mini_descriptor",
        "//:protobuf",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:mem",
        "//upb:message_accessors_internal",
        "//upb:mini_table",
        "//upb:wire",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)

# begin:github_only
filegroup(
    name = "source_files",
    srcs = glob(
        [
            "**/*.c",
            "**/*.h",
        ],
    ),
    visibility = [
        "//upb/cmake:__pkg__",
        "//python/dist:__pkg__",
    ]
)
# end:github_only
