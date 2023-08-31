load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)

# begin:google_only
# load("//bazel:build_defs.bzl", "upb_kt_native_interop_hint")
#
# upb_kt_native_interop_hint(name = "mini_descriptor_aspect_hint")
# end:google_only

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
        "//:base",
        "//:mem",
        "//:mini_table",
        "//:mini_table_internal",
        "//:port",
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
        "//:base",
        "//:base_internal",
        "//:port",
    ],
)

cc_test(
    name = "encode_test",
    srcs = ["internal/encode_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":internal",
        ":mini_descriptor",
        "//:base",
        "//:mem",
        "//:message_accessors_internal",
        "//:mini_table",
        "//:wire",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
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
        "//cmake:__pkg__",
        "//python/dist:__pkg__",
    ]
)
# end:github_only
