load(
    ":build_defs.bzl",
    "lua_cclibrary",
    "lua_library",
    "lua_binary",
    "lua_test",
    "generated_file_staleness_test",
    "make_shell_script",
    "upb_amalgamation",
    "upb_proto_library",
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
    copts = [
        "-std=c89",
        "-pedantic",
        "-Wno-long-long",
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
    copts = [
        "-std=c89",
        "-pedantic",
        "-Wno-long-long",
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
    copts = [
        "-std=c89",
        "-pedantic",
        "-Wno-long-long",
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
    copts = [
        "-std=c89",
        "-pedantic",
        "-Wno-long-long",
    ],
    deps = [":upb"],
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
    outs = [
        "upb.c",
        "upb.h",
    ],
    amalgamator = ":amalgamate",
    libs = [
        ":upb",
        ":upb_descriptor",
        ":upb_pb",
        ":upb_json",
    ],
)

cc_library(
    name = "amalgamation",
    srcs = ["upb.c"],
    hdrs = ["upb.h"],
)

# C/C++ tests ##################################################################

cc_library(
    name = "upb_test",
    testonly = 1,
    srcs = [
        "tests/testmain.cc",
    ],
    hdrs = [
        "tests/test_util.h",
        "tests/upb_test.h",
    ],
)

cc_test(
    name = "test_varint",
    srcs = ["tests/pb/test_varint.c"],
    deps = [
        ":upb_pb",
        ":upb_test",
    ],
)

cc_test(
    name = "test_def",
    srcs = ["tests/test_def.c"],
    deps = [
        ":upb_pb",
        ":upb_test",
    ],
)

cc_test(
    name = "test_handlers",
    srcs = ["tests/test_handlers.c"],
    deps = [
        ":upb_pb",
        ":upb_test",
    ],
)

cc_test(
    name = "test_decoder",
    srcs = ["tests/pb/test_decoder.cc"],
    deps = [
        ":upb_pb",
        ":upb_test",
    ],
)

cc_test(
    name = "test_encoder",
    srcs = ["tests/pb/test_encoder.cc"],
    data = ["upb/descriptor/descriptor.pb"],
    deps = [
        ":upb_cc_bindings",
        ":upb_pb",
        ":upb_test",
    ],
)

cc_test(
    name = "test_cpp",
    srcs = ["tests/test_cpp.cc"],
    deps = [
        ":upb",
        ":upb_descriptor",
        ":upb_pb",
        ":upb_test",
    ],
)

cc_test(
    name = "test_table",
    srcs = ["tests/test_table.cc"],
    deps = [
        ":upb",
        ":upb_test",
    ],
)

cc_test(
    name = "test_json",
    srcs = [
        "tests/json/test.upbdefs.c",
        "tests/json/test.upbdefs.h",
        "tests/json/test_json.cc",
    ],
    deps = [
        ":upb_json",
        ":upb_test",
    ],
)

upb_proto_library(
    name = "conformance_proto_upb",
    upbc = ":protoc-gen-upb",
    deps = [
        "@com_google_protobuf//:conformance_proto",
        "@com_google_protobuf//:test_messages_proto3_proto",
    ],
)

cc_binary(
    name = "conformance_upb",
    srcs = [
        "tests/conformance_upb.c",
    ],
    deps = [
        ":conformance_proto_upb",
        ":upb",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb",
    out = "test_conformance_upb.sh",
    contents = "$(rlocation com_google_protobuf/conformance_test_runner) $(rlocation upb/conformance_upb)",
)

sh_test(
    name = "test_conformance_upb",
    srcs = ["test_conformance_upb.sh"],
    data = [
        "tests/conformance_upb_failures.txt",
        ":conformance_upb",
        "@bazel_tools//tools/bash/runfiles",
        "@com_google_protobuf//:conformance_test_runner",
    ],
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
    srcs = ["upb/bindings/lua/upb.lua"],
    luadeps = ["lua/upb_c"],
    strip_prefix = "upb/bindings/lua",
)

lua_cclibrary(
    name = "lua/upb/table_c",
    srcs = ["upb/bindings/lua/upb/table.c"],
    luadeps = ["lua/upb_c"],
    deps = ["upb"],
)

lua_library(
    name = "lua/upb/table",
    srcs = ["upb/bindings/lua/upb/table.lua"],
    luadeps = [
        "lua/upb",
        "lua/upb/table_c",
    ],
    strip_prefix = "upb/bindings/lua",
)

lua_cclibrary(
    name = "lua/upb/pb_c",
    srcs = ["upb/bindings/lua/upb/pb.c"],
    luadeps = ["lua/upb_c"],
    deps = ["upb_pb"],
)

lua_library(
    name = "lua/upb/pb",
    srcs = ["upb/bindings/lua/upb/pb.lua"],
    luadeps = [
        "lua/upb",
        "lua/upb/pb_c",
    ],
    strip_prefix = "upb/bindings/lua",
)

lua_library(
    name = "lua/upbc_lib",
    srcs = [
        "tools/dump_cinit.lua",
        "tools/make_c_api.lua",
    ],
    luadeps = [
        "lua/upb",
        "lua/upb/table",
    ],
    strip_prefix = "tools",
)

# Lua tests. ###################################################################

lua_test(
    name = "lua/test_upb",
    luadeps = ["lua/upb"],
    luamain = "tests/bindings/lua/test_upb.lua",
)

lua_test(
    name = "lua/test_upb_pb",
    luadeps = ["lua/upb/pb"],
    luamain = "tests/bindings/lua/test_upb.pb.lua",
)

# upb compiler #################################################################

lua_binary(
    name = "lua_upbc",
    luadeps = [
        "lua/upbc_lib",
    ],
    luamain = "tools/upbc.lua",
)

cc_library(
    name = "upbc_generator",
    hdrs = ["upbc/generator.h"],
    srcs = ["upbc/generator.cc", "upbc/message_layout.h", "upbc/message_layout.cc"],
    deps = [
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
        "@absl//absl/strings",
    ],
)

cc_binary(
    name = "protoc-gen-upb",
    srcs = ["upbc/main.cc"],
    deps = [
        ":upbc_generator",
        "@com_google_protobuf//:protoc_lib",
    ],
)

# Test the CMake build #########################################################

make_shell_script(
    name = "gen_run_cmake_build",
    out = "run_cmake_build.sh",
    contents = "mkdir build && cd build && cmake .. && make -j8"
)

sh_test(
    name = "cmake_build",
    srcs = ["run_cmake_build.sh"],
    data = glob([
        "CMakeLists.txt",
        "google/**/*",
        "upbc/**/*",
        "upb/**/*",
        "tests/**/*",
    ]) + [
        "@bazel_tools//tools/bash/runfiles",
    ],
)

# Generated files ##############################################################

exports_files(["tools/staleness_test.py"])

py_library(
    name = "staleness_test_lib",
    testonly = 1,
    srcs = ["tools/staleness_test_lib.py"],
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
        "upb/descriptor/descriptor.proto",
    ],
)

py_binary(
    name = "make_cmakelists",
    srcs = ["tools/make_cmakelists.py"],
)

genrule(
    name = "gen_cmakelists",
    outs = ["generated/CMakeLists.txt"],
    srcs = ["BUILD", "WORKSPACE"],
    tools = [":make_cmakelists"],
    cmd = "$(location :make_cmakelists) $@"
)

genrule(
    name = "copy_upb_descriptor_pb",
    srcs = [":upb_descriptor_proto"],
    outs = ["generated/upb/descriptor/descriptor.pb"],
    cmd = "cp $< $@",
)

genrule(
    name = "generate_old_upbdefs",
    srcs = ["generated/upb/descriptor/descriptor.pb"],
    outs = [
        "generated/upb/descriptor/descriptor.upbdefs.h",
        "generated/upb/descriptor/descriptor.upbdefs.c",
    ],
    cmd = "UPBC=$$PWD/$(location :lua_upbc); INFILE=$$PWD/$<; cd $(GENDIR)/generated && $$UPBC --generate-upbdefs $$INFILE",
    tools = [":lua_upbc"],
)

proto_library(
    name = "google_descriptor_proto",
    srcs = [
        "google/protobuf/descriptor.proto",
    ],
)

genrule(
    name = "copy_google_descriptor_pb",
    srcs = [":google_descriptor_proto"],
    outs = ["generated/google/protobuf/descriptor.pb"],
    cmd = "cp $< $@",
)

genrule(
    name = "generate_descriptor_c",
    srcs = ["google/protobuf/descriptor.proto"],
    outs = [
        "generated/google/protobuf/descriptor.upb.h",
        "generated/google/protobuf/descriptor.upb.c",
    ],
    cmd = "$(location @com_google_protobuf//:protoc) $< --upb_out=$(GENDIR)/generated --plugin=protoc-gen-upb=$(location :protoc-gen-upb)",
    tools = [
        "@com_google_protobuf//:protoc",
        ":protoc-gen-upb"
    ],
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
    outs = [
        "generated/tests/json/test.upbdefs.h",
        "generated/tests/json/test.upbdefs.c",
    ],
    cmd = "UPBC=$$PWD/$(location :lua_upbc); INFILE=$$PWD/$<; cd $(GENDIR)/generated && $$UPBC --generate-upbdefs $$INFILE",
    tools = [":lua_upbc"],
)

genrule(
    name = "generate_json_ragel",
    srcs = ["upb/json/parser.rl"],
    outs = ["generated/upb/json/parser.c"],
    cmd = "$(location @ragel//:ragel) -C -o upb/json/parser.c $< && mv upb/json/parser.c $@",
    tools = ["@ragel"],
)

generated_file_staleness_test(
    name = "test_generated_files",
    outs = [
        "CMakeLists.txt",
        "google/protobuf/descriptor.upb.c",
        "google/protobuf/descriptor.upb.h",
        "tests/json/test.proto.pb",
        "tests/json/test.upbdefs.c",
        "tests/json/test.upbdefs.h",
        "upb/descriptor/descriptor.pb",
        "upb/descriptor/descriptor.upbdefs.c",
        "upb/descriptor/descriptor.upbdefs.h",
        "upb/json/parser.c",
        "upb/pb/compile_decoder_x64.h",
    ],
    generated_pattern = "generated/%s",
)
