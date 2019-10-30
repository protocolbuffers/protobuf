load(
    "//bazel:build_defs.bzl",
    "generated_file_staleness_test",
    "licenses",  # copybara:strip_for_google3
    "lua_binary",
    "lua_cclibrary",
    "lua_library",
    "lua_test",
    "make_shell_script",
    "upb_amalgamation",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

licenses(["notice"])  # BSD (Google-authored w/ possible external contributions)

exports_files([
    "LICENSE",
    "build_defs",
])

CPPOPTS = [
    # copybara:strip_for_google3_begin
    "-Werror",
    "-Wno-long-long",
    # copybara:strip_end
]

COPTS = CPPOPTS + [
    # copybara:strip_for_google3_begin
    "-pedantic",
    "-Wstrict-prototypes",
    # copybara:strip_end
]

config_setting(
    name = "darwin",
    values = {"cpu": "darwin"},
    visibility = ["//visibility:public"],
)

config_setting(
    name = "windows",
    constraint_values = ["@bazel_tools//platforms:windows"],
)

config_setting(
    name = "fuzz",
    values = {"define": "fuzz=true"},
)

# Public C/C++ libraries #######################################################

cc_library(
    name = "upb",
    srcs = [
        "upb/decode.c",
        "upb/encode.c",
        "upb/generated_util.h",
        "upb/msg.c",
        "upb/msg.h",
        "upb/port.c",
        "upb/port_def.inc",
        "upb/port_undef.inc",
        "upb/table.c",
        "upb/table.int.h",
        "upb/upb.c",
    ],
    hdrs = [
        "upb/decode.h",
        "upb/encode.h",
        "upb/upb.h",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
    visibility = ["//visibility:public"],
)

# Common support routines used by generated code.  This library has no
# implementation, but depends on :upb and exposes a few more hdrs.
#
# This is public only because we have no way of visibility-limiting it to
# upb_proto_library() only.  This interface is not stable and by using it you
# give up any backward compatibility guarantees.
cc_library(
    name = "generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = [
        "upb/generated_util.h",
        "upb/msg.h",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
    textual_hdrs = [
        "upb/port_def.inc",
        "upb/port_undef.inc",
    ],
    visibility = ["//visibility:public"],
    deps = [":upb"],
)

upb_proto_library(
    name = "descriptor_upbproto",
    visibility = ["//visibility:public"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

cc_library(
    name = "reflection",
    srcs = [
        "upb/def.c",
        "upb/msgfactory.c",
    ],
    hdrs = [
        "upb/def.h",
        "upb/msgfactory.h",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":descriptor_upbproto",
        ":table",
        ":upb",
    ],
)

# Internal C/C++ libraries #####################################################

cc_library(
    name = "table",
    hdrs = ["upb/table.int.h"],
    deps = [":upb"],
)

# Legacy C/C++ Libraries (not recommended for new code) ########################

cc_library(
    name = "legacy_msg_reflection",
    srcs = [
        "upb/legacy_msg_reflection.c",
    ],
    hdrs = ["upb/legacy_msg_reflection.h"],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
    deps = [
        ":table",
        ":upb",
    ],
)

cc_library(
    name = "handlers",
    srcs = [
        "upb/handlers.c",
        "upb/handlers-inl.h",
        "upb/sink.c",
    ],
    hdrs = [
        "upb/handlers.h",
        "upb/sink.h",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
    deps = [
        ":reflection",
        ":table",
        ":upb",
    ],
)

cc_library(
    name = "upb_pb",
    srcs = [
        "upb/pb/compile_decoder.c",
        "upb/pb/decoder.c",
        "upb/pb/decoder.int.h",
        "upb/pb/encoder.c",
        "upb/pb/textprinter.c",
        "upb/pb/varint.c",
        "upb/pb/varint.int.h",
    ],
    hdrs = [
        "upb/pb/decoder.h",
        "upb/pb/encoder.h",
        "upb/pb/textprinter.h",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
    deps = [
        ":descriptor_upbproto",
        ":handlers",
        ":reflection",
        ":table",
        ":upb",
    ],
)

# copybara:strip_for_google3_begin
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
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
    deps = [
        ":upb",
        ":upb_pb",
    ],
)
# copybara:strip_end

cc_library(
    name = "upb_cc_bindings",
    hdrs = [
        "upb/bindings/stdc++/string.h",
    ],
    deps = [
        ":descriptor_upbproto",
        ":handlers",
        ":upb",
    ],
)

# upb compiler #################################################################

cc_library(
    name = "upbc_generator",
    srcs = [
        "upbc/generator.cc",
        "upbc/message_layout.cc",
        "upbc/message_layout.h",
    ],
    hdrs = ["upbc/generator.h"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }),
    deps = [
        "@absl//absl/base:core_headers",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_binary(
    name = "protoc-gen-upb",
    srcs = ["upbc/main.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":upbc_generator",
        "@com_google_protobuf//:protoc_lib",
    ],
)

# We strip the tests and remaining rules from google3 until the upb_proto_library()
# and upb_proto_reflection_library() rules are fixed.

# C/C++ tests ##################################################################

cc_binary(
    name = "benchmark",
    testonly = 1,
    srcs = ["tests/benchmark.cc"],
    deps = [
        ":descriptor_upbproto",
        ":descriptor_upbreflection",
        "@com_github_google_benchmark//:benchmark_main",
    ],
)

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
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }),
    deps = [
        ":handlers",
        ":upb",
    ],
)

cc_test(
    name = "test_varint",
    srcs = [
        "tests/pb/test_varint.c",
        "upb/pb/varint.int.h",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
    deps = [
        ":upb",
        ":upb_pb",
        ":upb_test",
    ],
)

proto_library(
    name = "test_decoder_proto",
    srcs = [
        "tests/pb/test_decoder.proto",
    ],
)

upb_proto_reflection_library(
    name = "test_decoder_upbproto",
    deps = [":test_decoder_proto"],
)

cc_test(
    name = "test_decoder",
    srcs = [
        "tests/pb/test_decoder.cc",
        "upb/pb/varint.int.h",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }),
    deps = [
        ":handlers",
        ":test_decoder_upbproto",
        ":upb",
        ":upb_pb",
        ":upb_test",
    ],
)

proto_library(
    name = "test_cpp_proto",
    srcs = [
        "tests/test_cpp.proto",
    ],
)

upb_proto_reflection_library(
    name = "test_cpp_upbproto",
    deps = ["test_cpp_proto"],
)

cc_test(
    name = "test_cpp",
    srcs = ["tests/test_cpp.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }),
    deps = [
        ":handlers",
        ":reflection",
        ":test_cpp_upbproto",
        ":upb",
        ":upb_pb",
        ":upb_test",
    ],
)

cc_test(
    name = "test_table",
    srcs = ["tests/test_table.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }),
    deps = [
        ":table",
        ":upb",
        ":upb_test",
    ],
)

# OSS-Fuzz test
cc_binary(
    name = "file_descriptor_parsenew_fuzzer",
    testonly = 1,
    srcs = ["tests/file_descriptor_parsenew_fuzzer.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }) + select({
        "//conditions:default": [],
        ":fuzz": ["-fsanitize=fuzzer,address"],
    }),
    defines = select({
        "//conditions:default": [],
        ":fuzz": ["HAVE_FUZZER"],
    }),
    deps = [
        ":descriptor_upbproto",
        ":upb",
    ],
)

# copybara:strip_for_google3_begin
upb_proto_reflection_library(
    name = "descriptor_upbreflection",
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

cc_test(
    name = "test_encoder",
    srcs = ["tests/pb/test_encoder.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }),
    deps = [
        ":descriptor_upbproto",
        ":descriptor_upbreflection",
        ":upb",
        ":upb_cc_bindings",
        ":upb_pb",
        ":upb_test",
    ],
)

proto_library(
    name = "test_json_enum_from_separate",
    srcs = ["tests/json/enum_from_separate_file.proto"],
    deps = [":test_json_proto"],
)

proto_library(
    name = "test_json_proto",
    srcs = ["tests/json/test.proto"],
)

upb_proto_reflection_library(
    name = "test_json_upbprotoreflection",
    deps = ["test_json_proto"],
)

upb_proto_library(
    name = "test_json_enum_from_separate_upbproto",
    deps = [":test_json_enum_from_separate"],
)

upb_proto_library(
    name = "test_json_upbproto",
    deps = [":test_json_proto"],
)

cc_test(
    name = "test_json",
    srcs = [
        "tests/json/test_json.cc",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS
    }),
    deps = [
        ":test_json_upbproto",
        ":test_json_upbprotoreflection",
        ":upb_json",
        ":upb_test",
    ],
)
# copybara:strip_end

upb_proto_library(
    name = "conformance_proto_upb",
    testonly = 1,
    deps = ["@com_google_protobuf//:conformance_proto"],
)

upb_proto_library(
    name = "test_messages_proto3_proto_upb",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto3_proto"],
)

cc_binary(
    name = "conformance_upb",
    testonly = 1,
    srcs = [
        "tests/conformance_upb.c",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }) + ["-Ibazel-out/k8-fastbuild/bin"],
    deps = [
        ":conformance_proto_upb",
        ":test_messages_proto3_proto_upb",
        ":upb",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb",
    out = "test_conformance_upb.sh",
    contents = "external/com_google_protobuf/conformance_test_runner ./conformance_upb",
)

sh_test(
    name = "test_conformance_upb",
    srcs = ["test_conformance_upb.sh"],
    data = [
        "tests/conformance_upb_failures.txt",
        ":conformance_upb",
        "@com_google_protobuf//:conformance_test_runner",
    ],
)

# copybara:strip_for_google3_begin

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
        ":descriptor_upbproto",
        ":reflection",
        ":handlers",
        ":upb_pb",
        ":upb_json",
    ],
)

cc_library(
    name = "amalgamation",
    srcs = ["upb.c"],
    hdrs = ["upb.h"],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS
    }),
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
        "legacy_msg_reflection",
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

# Test the CMake build #########################################################

filegroup(
    name = "cmake_files",
    srcs = glob([
        "CMakeLists.txt",
        "generated_for_cmake/**/*",
        "google/**/*",
        "upbc/**/*",
        "upb/**/*",
        "tests/**/*",
    ]),
)

make_shell_script(
    name = "gen_run_cmake_build",
    out = "run_cmake_build.sh",
    contents = "find . && mkdir build && cd build && cmake .. && make -j8 && make test",
)

sh_test(
    name = "cmake_build",
    srcs = ["run_cmake_build.sh"],
    data = [":cmake_files"],
)

# Generated files ##############################################################

exports_files(["tools/staleness_test.py"])

py_library(
    name = "staleness_test_lib",
    testonly = 1,
    srcs = ["tools/staleness_test_lib.py"],
)

py_binary(
    name = "make_cmakelists",
    srcs = ["tools/make_cmakelists.py"],
)

genrule(
    name = "gen_cmakelists",
    srcs = [
        "BUILD",
        "WORKSPACE",
        ":cmake_files",
    ],
    outs = ["generated-in/CMakeLists.txt"],
    cmd = "$(location :make_cmakelists) $@",
    tools = [":make_cmakelists"],
)

genrule(
    name = "generate_json_ragel",
    srcs = ["upb/json/parser.rl"],
    outs = ["upb/json/parser.c"],
    cmd = "$(location @ragel//:ragelc) -C -o upb/json/parser.c $< && mv upb/json/parser.c $@",
    tools = ["@ragel//:ragelc"],
)

genrule(
    name = "copy_json_ragel",
    srcs = ["upb/json/parser.c"],
    outs = ["generated-in/generated_for_cmake/upb/json/parser.c"],
    cmd = "cp $< $@",
)

genrule(
    name = "copy_protos",
    srcs = [":descriptor_upbproto"],
    outs = [
        "generated-in/generated_for_cmake/google/protobuf/descriptor.upb.c",
        "generated-in/generated_for_cmake/google/protobuf/descriptor.upb.h",
    ],
    cmd = "cp $(SRCS) $(@D)/generated-in/generated_for_cmake/google/protobuf",
)

generated_file_staleness_test(
    name = "test_generated_files",
    outs = [
        "CMakeLists.txt",
        "generated_for_cmake/google/protobuf/descriptor.upb.c",
        "generated_for_cmake/google/protobuf/descriptor.upb.h",
        "generated_for_cmake/upb/json/parser.c",
    ],
    generated_pattern = "generated-in/%s",
)

# copybara:strip_end
