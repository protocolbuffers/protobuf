load(
    "@rules_proto//proto:defs.bzl",
    "proto_library",
)
load(
    "//bazel:build_defs.bzl",
    "generated_file_staleness_test",
    "licenses",  # copybara:strip_for_google3
    "make_shell_script",
    "upb_amalgamation",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)
load(
    "//:upb/bindings/lua/lua_proto_library.bzl",
    "lua_proto_library",
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
    "-Werror=pedantic",
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
    name = "port",
    srcs = [
        "upb/port.c",
    ],
    textual_hdrs = [
        "upb/port_def.inc",
        "upb/port_undef.inc",
    ],
)

cc_library(
    name = "upb",
    srcs = [
        "upb/decode.c",
        "upb/encode.c",
        "upb/msg.c",
        "upb/msg.h",
        "upb/table.c",
        "upb/table.int.h",
        "upb/upb.c",
    ],
    hdrs = [
        "upb/decode.h",
        "upb/encode.h",
        "upb/upb.h",
        "upb/upb.hpp",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS,
    }),
    visibility = ["//visibility:public"],
    deps = [":port"],
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
        "upb/msg.h",
        "upb/port_def.inc",
        "upb/port_undef.inc",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS,
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":table",
        ":upb",
    ],
)

upb_proto_library(
    name = "descriptor_upb_proto",
    visibility = ["//visibility:public"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

upb_proto_reflection_library(
    name = "descriptor_upb_proto_reflection",
    visibility = ["//visibility:public"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

cc_library(
    name = "reflection",
    srcs = [
        "upb/def.c",
        "upb/msg.h",
        "upb/reflection.c",
    ],
    hdrs = [
        "upb/def.h",
        "upb/def.hpp",
        "upb/reflection.h",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS,
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":descriptor_upb_proto",
        ":port",
        ":table",
        ":upb",
    ],
)

cc_library(
    name = "textformat",
    srcs = [
        "upb/text_encode.c",
    ],
    hdrs = [
        "upb/text_encode.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":port",
        ":reflection",
    ],
)

cc_library(
    name = "json",
    srcs = [
        "upb/json_decode.c",
        "upb/json_encode.c",
    ],
    hdrs = [
        "upb/json_decode.h",
        "upb/json_encode.h",
    ],
    deps = [
        ":port",
        ":reflection",
        ":upb",
    ],
)

# Internal C/C++ libraries #####################################################

cc_library(
    name = "table",
    hdrs = ["upb/table.int.h"],
    deps = [
        ":port",
        ":upb",
    ],
)

# Legacy C/C++ Libraries (not recommended for new code) ########################

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
        "//conditions:default": COPTS,
    }),
    deps = [
        ":port",
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
        "//conditions:default": COPTS,
    }),
    deps = [
        ":descriptor_upb_proto",
        ":handlers",
        ":port",
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
        "//conditions:default": COPTS,
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
        ":descriptor_upb_proto",
        ":handlers",
        ":port",
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
        "//conditions:default": CPPOPTS,
    }),
    deps = [
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_binary(
    name = "protoc-gen-upb",
    srcs = ["upbc/main.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS,
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

upb_proto_reflection_library(
    name = "descriptor_upbreflection",
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

cc_binary(
    name = "benchmark",
    testonly = 1,
    srcs = ["tests/benchmark.cc"],
    deps = [
        ":descriptor_upb_proto",
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
        "//conditions:default": CPPOPTS,
    }),
    deps = [
        ":handlers",
        ":port",
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
        "//conditions:default": COPTS,
    }),
    deps = [
        ":port",
        ":upb",
        ":upb_pb",
        ":upb_test",
    ],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["tests/test.proto"],
)

upb_proto_library(
    name = "test_upb_proto",
    testonly = 1,
    deps = [":test_proto"],
)

cc_test(
    name = "test_generated_code",
    srcs = ["tests/test_generated_code.c"],
    deps = [
        ":test_messages_proto3_proto_upb",
        ":empty_upbdefs_proto",
        ":test_upb_proto",
        ":upb_test",
    ],
)

proto_library(
    name = "empty_proto",
    srcs = ["tests/empty.proto"],
)

upb_proto_reflection_library(
    name = "empty_upbdefs_proto",
    testonly = 1,
    deps = [":empty_proto"],
)

upb_proto_library(
    name = "test_messages_proto3_proto_upb",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto3_proto"],
)

proto_library(
    name = "test_decoder_proto",
    srcs = [
        "tests/pb/test_decoder.proto",
    ],
)

upb_proto_reflection_library(
    name = "test_decoder_upb_proto",
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
        "//conditions:default": CPPOPTS,
    }),
    deps = [
        ":handlers",
        ":port",
        ":test_decoder_upb_proto",
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
    name = "test_cpp_upb_proto",
    deps = ["test_cpp_proto"],
)

cc_test(
    name = "test_cpp",
    srcs = ["tests/test_cpp.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS,
    }),
    deps = [
        ":handlers",
        ":port",
        ":reflection",
        ":test_cpp_upb_proto",
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
        "//conditions:default": CPPOPTS,
    }),
    deps = [
        ":port",
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
        "//conditions:default": CPPOPTS,
    }) + select({
        "//conditions:default": [],
        ":fuzz": ["-fsanitize=fuzzer,address"],
    }),
    defines = select({
        "//conditions:default": [],
        ":fuzz": ["HAVE_FUZZER"],
    }),
    deps = [
        ":descriptor_upb_proto",
        ":upb",
    ],
)

# copybara:strip_for_google3_begin
cc_test(
    name = "test_encoder",
    srcs = ["tests/pb/test_encoder.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS,
    }),
    deps = [
        ":descriptor_upb_proto",
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
    name = "test_json_upb_proto_reflection",
    deps = ["test_json_proto"],
)

upb_proto_library(
    name = "test_json_enum_from_separate_upb_proto",
    deps = [":test_json_enum_from_separate"],
)

upb_proto_library(
    name = "test_json_upb_proto",
    deps = [":test_json_proto"],
)

cc_test(
    name = "test_json",
    srcs = [
        "tests/json/test_json.cc",
    ],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS,
    }),
    deps = [
        ":test_json_upb_proto",
        ":test_json_upb_proto_reflection",
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

upb_proto_reflection_library(
    name = "conformance_proto_upbdefs",
    testonly = 1,
    deps = ["@com_google_protobuf//:conformance_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto2_upbdefs",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto2_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto3_upbdefs",
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
        "//conditions:default": COPTS,
    }) + ["-Ibazel-out/k8-fastbuild/bin"],
    deps = [
        ":conformance_proto_upb",
        ":conformance_proto_upbdefs",
        ":json",
        ":reflection",
        ":test_messages_proto2_upbdefs",
        ":test_messages_proto3_upbdefs",
        ":textformat",
        ":upb",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb",
    out = "test_conformance_upb.sh",
    contents = "external/com_google_protobuf/conformance_test_runner --enforce_recommended ./conformance_upb",
)

sh_test(
    name = "test_conformance_upb",
    srcs = ["test_conformance_upb.sh"],
    data = [
        "tests/conformance_upb_failures.txt",
        ":conformance_upb",
        "@com_google_protobuf//:conformance_test_runner",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
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
        ":descriptor_upb_proto",
        ":reflection",
        ":handlers",
        ":port",
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
        "//conditions:default": COPTS,
    }),
)

upb_amalgamation(
    name = "gen_php_amalgamation",
    prefix = "php-",
    outs = [
        "php-upb.c",
        "php-upb.h",
    ],
    amalgamator = ":amalgamate",
    libs = [
        ":upb",
        ":descriptor_upb_proto",
        ":descriptor_upb_proto_reflection",
        ":reflection",
        ":port",
        ":json",
    ],
)

cc_library(
    name = "php_amalgamation",
    srcs = ["php-upb.c"],
    hdrs = ["php-upb.h"],
    copts = select({
        ":windows": [],
        "//conditions:default": COPTS,
    }),
)

# Lua ##########################################################################

cc_library(
    name = "lupb",
    srcs = [
        "upb/bindings/lua/def.c",
        "upb/bindings/lua/msg.c",
        "upb/bindings/lua/upb.c",
    ],
    hdrs = [
        "upb/bindings/lua/upb.h",
    ],
    deps = [
        ":reflection",
        ":textformat",
        ":upb",
        "@lua//:liblua",
    ],
)

cc_test(
    name = "test_lua",
    srcs = ["tests/bindings/lua/main.c"],
    data = [
        "tests/bindings/lua/test_upb.lua",
        "third_party/lunit/console.lua",
        "third_party/lunit/lunit.lua",
        "upb/bindings/lua/upb.lua",
        ":descriptor_proto_lua",
        ":test_messages_proto3_proto_lua",
        ":test_proto_lua",
        "@com_google_protobuf//:conformance_proto",
        "@com_google_protobuf//:descriptor_proto",
    ],
    linkstatic = 1,
    deps = [
        ":lupb",
        "@lua//:liblua",
    ],
)

cc_binary(
    name = "protoc-gen-lua",
    srcs = ["upb/bindings/lua/upbc.cc"],
    copts = select({
        ":windows": [],
        "//conditions:default": CPPOPTS,
    }),
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protoc_lib",
    ],
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
    name = "test_messages_proto3_proto_lua",
    testonly = 1,
    deps = ["@com_google_protobuf//:test_messages_proto3_proto"],
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
    deps = ["@bazel_tools//tools/bash/runfiles"],
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
    srcs = [":descriptor_upb_proto"],
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
