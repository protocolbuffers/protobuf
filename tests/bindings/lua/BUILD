load(
    "//upb/bindings/lua:lua_proto_library.bzl",
    "lua_proto_library",
)
load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)

licenses(["notice"])

cc_test(
    name = "test_lua",
    srcs = ["main.c"],
    copts = UPB_DEFAULT_COPTS,
    data = [
        "test_upb.lua",
        ":descriptor_proto_lua",
        ":empty_proto_lua",
        ":test_messages_proto2_proto_lua",
        ":test_messages_proto3_proto_lua",
        ":test_proto_lua",
        "//:third_party/lunit/console.lua",
        "//:third_party/lunit/lunit.lua",
        "//upb/bindings/lua:upb.lua",
        "@com_google_protobuf//:conformance_proto",
        "@com_google_protobuf//:descriptor_proto",
    ],
    linkstatic = 1,
    deps = [
        "//upb/bindings/lua:lupb",
        "@lua//:liblua",
    ],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["test.proto"],
)

lua_proto_library(
    name = "test_proto_lua",
    testonly = 1,
    deps = [":test_proto"],
)

lua_proto_library(
    name = "descriptor_proto_lua",
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

lua_proto_library(
    name = "empty_proto_lua",
    deps = ["@com_google_protobuf//:empty_proto"],
)

lua_proto_library(
    name = "test_messages_proto3_proto_lua",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto3_proto"],
)

lua_proto_library(
    name = "test_messages_proto2_proto_lua",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto2_proto"],
)
