load(":build_defs.bzl", "lua_cclibrary", "lua_library", "lua_binary")

cc_library(
    name = "upb",
    srcs = [
        "google/protobuf/descriptor.upb.c",
        "upb/decode.c",
        "upb/def.c",
        "upb/encode.c",
        "upb/handlers.c",
        "upb/handlers-inl.h",
        "upb/msg.c",
        "upb/msgfactory.c",
        "upb/port_def.inc",
        "upb/port_undef.inc",
        "upb/refcounted.c",
        "upb/sink.c",
        "upb/structdefs.int.h",
        "upb/structs.int.h",
        "upb/table.c",
        "upb/table.int.h",
        "upb/upb.c",
    ],
    hdrs = [
        "google/protobuf/descriptor.upb.h",
        "upb/decode.h",
        "upb/def.h",
        "upb/encode.h",
        "upb/handlers.h",
        "upb/msg.h",
        "upb/msgfactory.h",
        "upb/refcounted.h",
        "upb/sink.h",
        "upb/upb.h",
    ],
)

cc_library(
    name = "upb_descriptor",
    srcs = [
        "upb/descriptor/descriptor.upbdefs.c",
        "upb/descriptor/reader.c",
    ],
    hdrs = [
        "upb/descriptor/descriptor.upbdefs.h",
        "upb/descriptor/reader.h",
    ],
    deps = [":upb"],
)

cc_library(
    name = "upb_pb",
    srcs = [
        "upb/pb/compile_decoder.c",
        "upb/pb/decoder.c",
        "upb/pb/decoder.int.h",
        "upb/pb/encoder.c",
        "upb/pb/glue.c",
        "upb/pb/textprinter.c",
        "upb/pb/varint.c",
        "upb/pb/varint.int.h",
    ],
    hdrs = [
        "upb/pb/decoder.h",
        "upb/pb/encoder.h",
        "upb/pb/glue.h",
        "upb/pb/textprinter.h",
    ],
    deps = [
        ":upb",
        ":upb_descriptor",
    ],
)

cc_library(
    name = "upb_json",
    srcs = [
        "upb/json/parser.c",
        "upb/json/printer.c",
    ],
    hdrs = [
        "upb/json/parser.h",
        "upb/json/printer.h",
    ],
    deps = [":upb"],
)

lua_cclibrary(
    name = "lua/upb_c",
    srcs = [
        "upb/bindings/lua/def.c",
        "upb/bindings/lua/msg.c",
        "upb/bindings/lua/upb.c",
    ],
    hdrs = [
        "upb/bindings/lua/upb.h",
    ],
    deps = [
        "upb",
        "upb_pb",
    ],
)

lua_library(
    name = "lua/upb",
    base = "upb/bindings/lua",
    srcs = [
        "upb/bindings/lua/upb.lua"
    ],
    luadeps = [
        "lua/upb_c",
    ],
)

lua_cclibrary(
    name = "lua/upb/table_c",
    srcs = [
        "upb/bindings/lua/upb/table.c",
    ],
    luadeps = [
        "lua/upb_c",
    ],
    deps = [
        "upb",
    ],
)

lua_library(
    name = "lua/upb/table",
    base = "upb/bindings/lua",
    srcs = [
        "upb/bindings/lua/upb/table.lua",
    ],
    luadeps = [
        "lua/upb",
        "lua/upb/table_c",
    ],
)

lua_cclibrary(
    name = "lua/upb/pb_c",
    srcs = [
        "upb/bindings/lua/upb/pb.c",
    ],
    luadeps = [
        "lua/upb_c",
    ],
    deps = [
        "upb_pb",
    ],
)

lua_library(
    name = "lua/upb/pb",
    base = "upb/bindings/lua",
    srcs = [
        "upb/bindings/lua/upb/pb.lua",
    ],
    luadeps = [
        "lua/upb",
        "lua/upb/pb_c",
    ],
)

lua_library(
    name = "lua/upbc_lib",
    base = "tools",
    srcs = [
        "tools/dump_cinit.lua",
        "tools/make_c_api.lua",
    ],
    luadeps = [
        "lua/upb",
    ]
)

lua_binary(
    name = "upbc",
    main = "tools/upbc.lua",
    luadeps = [
        "lua/upbc_lib",
    ]
)
