load(
    ":build_defs.bzl",
    "lua_cclibrary",
    "lua_library",
    "lua_binary",
    "lua_test",
    "generated_file_staleness_test",
    "upb_amalgamation",
)

# C/C++ rules ##################################################################

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
    copts = ["-std=c89", "-pedantic", "-Wno-long-long"],
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
    copts = ["-std=c89", "-pedantic", "-Wno-long-long"],
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
    copts = ["-std=c89", "-pedantic", "-Wno-long-long"],
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
    copts = ["-std=c89", "-pedantic", "-Wno-long-long"],
)

cc_library(
    name = "upb_cc_bindings",
    hdrs = [
        "upb/bindings/stdc++/string.h",
    ],
    deps = [":upb"],
)

# Amalgamation #################################################################

py_binary(
    name = "amalgamate",
    srcs = ["tools/amalgamate.py"],
)

upb_amalgamation(
    name = "gen_amalgamation",
    amalgamator = ":amalgamate",
    libs = [
        ":upb",
        ":upb_descriptor",
        ":upb_pb",
        ":upb_json",
    ],
    outs = [
        "upb.h",
        "upb.c",
    ],
)

cc_library(
    name = "amalgamation",
    hdrs = ["upb.h"],
    srcs = ["upb.c"],
)

# C/C++ tests ##################################################################

cc_library(
    testonly = 1,
    name = "upb_test",
    hdrs = [
        "tests/upb_test.h",
        "tests/test_util.h",
    ],
    srcs = [
        "tests/testmain.cc",
    ],
)

cc_test(
    name = "test_varint",
    srcs = ["tests/pb/test_varint.c"],
    deps = [":upb_pb", ":upb_test"],
)

cc_test(
    name = "test_def",
    srcs = ["tests/test_def.c"],
    deps = [":upb_pb", ":upb_test"],
)

cc_test(
    name = "test_handlers",
    srcs = ["tests/test_handlers.c"],
    deps = [":upb_pb", ":upb_test"],
)

cc_test(
    name = "test_decoder",
    srcs = ["tests/pb/test_decoder.cc"],
    deps = [":upb_pb", ":upb_test"],
)

cc_test(
    name = "test_encoder",
    srcs = ["tests/pb/test_encoder.cc"],
    deps = [":upb_pb", ":upb_test", ":upb_cc_bindings"],
    data = ["upb/descriptor/descriptor.pb"],
)

cc_test(
    name = "test_cpp",
    srcs = ["tests/test_cpp.cc"],
    deps = [":upb_descriptor", ":upb", ":upb_pb", ":upb_test"],
)

cc_test(
    name = "test_table",
    srcs = ["tests/test_table.cc"],
    deps = [":upb", ":upb_test"],
)

cc_test(
    name = "test_json",
    srcs = [
        "tests/json/test_json.cc",
        "tests/json/test.upbdefs.h",
        "tests/json/test.upbdefs.c",
    ],
    deps = [":upb_json", ":upb_test"],
)

# Lua libraries. ###############################################################

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
    srcs = [
        "upb/bindings/lua/upb.lua",
    ],
    strip_prefix = "upb/bindings/lua",
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
    srcs = [
        "upb/bindings/lua/upb/table.lua",
    ],
    strip_prefix = "upb/bindings/lua",
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
    srcs = [
        "upb/bindings/lua/upb/pb.lua",
    ],
    strip_prefix = "upb/bindings/lua",
    luadeps = [
        "lua/upb",
        "lua/upb/pb_c",
    ],
)

lua_library(
    name = "lua/upbc_lib",
    srcs = [
        "tools/dump_cinit.lua",
        "tools/make_c_api.lua",
    ],
    strip_prefix = "tools",
    luadeps = [
        "lua/upb",
        "lua/upb/table",
    ],
)

# Lua tests. ###################################################################

lua_test(
    name = "lua/test_upb",
    luamain = "tests/bindings/lua/test_upb.lua",
    luadeps = ["lua/upb"],
)

lua_test(
    name = "lua/test_upb_pb",
    luamain = "tests/bindings/lua/test_upb.pb.lua",
    luadeps = ["lua/upb/pb"],
)

# upb compiler #################################################################

lua_binary(
    name = "upbc",
    luadeps = [
        "lua/upbc_lib",
    ],
    luamain = "tools/upbc.lua",
)

# Generated files ##############################################################

exports_files(["staleness_test.py"])

py_library(
    name = "staleness_test_lib",
    testonly = 1,
    srcs = ["staleness_test_lib.py"],
)

genrule(
    name = "make_dynasm_decoder",
    srcs = [
        "third_party/dynasm/dynasm.lua",
        "third_party/dynasm/dasm_x64.lua",
        "third_party/dynasm/dasm_x86.lua",
        "upb/pb/compile_decoder_x64.dasc",
    ],
    outs = ["generated/upb/pb/compile_decoder_x64.h"],
    cmd = "LUA_PATH=third_party/dynasm/?.lua $(location @lua//:lua) third_party/dynasm/dynasm.lua -c upb/pb/compile_decoder_x64.dasc > $@",
    tools = ["@lua"],
)

proto_library(
    name = "upb_descriptor_proto",
    srcs = [
        "upb/descriptor/descriptor.proto"
    ],
)

genrule(
    name = "copy_upb_descriptor_pb",
    outs = ["generated/upb/descriptor/descriptor.pb"],
    srcs = [":upb_descriptor_proto"],
    cmd = "cp $< $@",
)

genrule(
    name = "generate_old_upbdefs",
    srcs = ["generated/upb/descriptor/descriptor.pb"],
    tools = [":upbc"],
    outs = [
        "generated/upb/descriptor/descriptor.upbdefs.h",
        "generated/upb/descriptor/descriptor.upbdefs.c",
    ],
    cmd = "UPBC=$$PWD/$(location :upbc); INFILE=$$PWD/$<; cd $(GENDIR)/generated && $$UPBC --generate-upbdefs $$INFILE",
)

proto_library(
    name = "google_descriptor_proto",
    srcs = [
        "google/protobuf/descriptor.proto"
    ],
)

genrule(
    name = "copy_google_descriptor_pb",
    outs = ["generated/google/protobuf/descriptor.pb"],
    srcs = [":google_descriptor_proto"],
    cmd = "cp $< $@",
)

genrule(
    name = "generate_descriptor_c",
    srcs = ["generated/google/protobuf/descriptor.pb"],
    tools = [":upbc"],
    outs = [
        "generated/google/protobuf/descriptor.upb.h",
        "generated/google/protobuf/descriptor.upb.c",
    ],
    cmd = "UPBC=$$PWD/$(location :upbc); INFILE=$$PWD/$<; cd $(GENDIR)/generated && $$UPBC $$INFILE",
)

proto_library(
    name = "json_test_proto",
    srcs = ["tests/json/test.proto"],
)

genrule(
    name = "copy_json_test_proto",
    srcs = [":json_test_proto"],
    outs = ["generated/tests/json/test.proto.pb"],
    cmd = "cp $< $@",
)

genrule(
    name = "generated_json_test_proto_upbdefs",
    srcs = ["generated/tests/json/test.proto.pb"],
    tools = [":upbc"],
    outs = [
        "generated/tests/json/test.upbdefs.h",
        "generated/tests/json/test.upbdefs.c",
    ],
    cmd = "UPBC=$$PWD/$(location :upbc); INFILE=$$PWD/$<; cd $(GENDIR)/generated && $$UPBC --generate-upbdefs $$INFILE",
)

genrule(
    name = "generate_json_ragel",
    srcs = ["upb/json/parser.rl"],
    outs = ["generated/upb/json/parser.c"],
    tools = ["@ragel//:ragel"],
    cmd = "$(location @ragel//:ragel) -C -o upb/json/parser.c $< && mv upb/json/parser.c $@",
)

generated_file_staleness_test(
    name = "test_generated_files",
    outs = [
        "google/protobuf/descriptor.upb.h",
        "google/protobuf/descriptor.upb.c",
        "upb/pb/compile_decoder_x64.h",
        "upb/descriptor/descriptor.upbdefs.c",
        "upb/descriptor/descriptor.pb",
        "upb/descriptor/descriptor.upbdefs.h",
        "upb/json/parser.c",
        "tests/json/test.upbdefs.c",
        "tests/json/test.upbdefs.h",
        "tests/json/test.proto.pb",
    ],
    generated_pattern = "generated/%s",
)
